#include "pch.h"
#include "framework.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <regex>
#include <set>
#include <queue>
#include <utility>
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <winpp/win.hpp>
#include <winpp/files.hpp>
#include <winpp/utf8.hpp>
#include "SvnRepos.h"
#include "DateConverter.h"
using json = nlohmann::json;

// implementation of the svn repos api
class SvnReposImpl final
{
public:
  // constructor/destructor
  SvnReposImpl(const std::string& filename) :
    m_filename(filename),
    m_root_directory(),
    m_repos()
  {
  }
  ~SvnReposImpl() = default;

  // load svn repository database
  bool load(std::string& root_directory)
  {
    try
    {
      // open program database
      std::ifstream file(m_filename);
      if (!file)
        return true;

      // parse json database
      const json& svn_db = json::parse(file);
      if ((!svn_db.contains("repos")          || !svn_db["repos"].is_array()) ||
          (!svn_db.contains("root-directory") || !svn_db["root-directory"].is_string()))
        return false;

      // retrieve root directory
      root_directory = utf8::from_utf8(svn_db["root-directory"].get<std::string>());

      // parse all repositories
      for (const auto& r : svn_db["repos"])
      {
        // check repos validity
        if ((!r.contains("commits") || !r["commits"].is_array()) ||
            (!r.contains("path")    || !r["path"].is_string()) ||
            (!r.contains("url")     || !r["url"].is_string()))
          continue;

        // retrieve data from one repository
        const std::string& path = utf8::from_utf8(r["path"].get<std::string>());
        auto& repos = m_repos[path];
        repos.url = utf8::from_utf8(r["url"].get<std::string>());

        // parse all commits
        const auto& commits_obj = r["commits"];
        for (const auto& c : commits_obj)
        {
          // check commit validity
          if ((!c.contains("branch") || !c["branch"].is_string()) ||
              (!c.contains("date")   || !c["date"].is_string()) ||
              (!c.contains("id")     || !c["id"].is_number()) ||
              (!c.contains("author") || !c["author"].is_string()) ||
              (!c.contains("desc")   || !c["desc"].is_string()))
            continue;

          // retrieve commit infos
          const std::shared_ptr<struct svn_commit>& commit = std::make_shared<struct svn_commit>(
            svn_commit{
              utf8::from_utf8(c["branch"].get<std::string>()),
              DateConverter::to_time(c["date"].get<std::string>()),
              c["id"].get<int>(),
              utf8::from_utf8(c["author"].get<std::string>()),
              utf8::from_utf8(c["desc"].get<std::string>())
            });
          
          // store commit infos
          repos.commits[commit->id] = commit;
        }
      }
      return true;
    }
    catch (...)
    {
      return false;
    }
  }

  // save svn repository database
  bool save(const std::string& root_directory)
  {
    try
    {
      // create json repositories format
      json svn_db = {
        {"root_directory", utf8::from_utf8(root_directory)},
        {"repos", json::array()}
      };

      // loop through all repositories
      for (const auto& r : m_repos)
      {
        // create repository object
        json repos = {
          {"path", utf8::to_utf8(r.first)},
          {"url", utf8::to_utf8(r.second.url)},
          {"commits", json::array()}
        };

        // loop through all commits
        for (const auto& c : r.second.commits)
        {
          const json& commit = {
            {"branch", utf8::to_utf8(c.second->branch)},
            {"date", DateConverter::to_str(c.second->date)},
            {"id", c.second->id},
            {"author", utf8::to_utf8(c.second->author)},
            {"desc", utf8::to_utf8(c.second->desc)}
          };
          repos["commits"].push_back(commit);
        }

        // add repository to repos array
        svn_db["repos"].push_back(repos);
      }

      // save json to file
      std::ofstream file(m_filename);
      if (!file)
        return false;
      file << std::setw(2) << svn_db << std::endl;

      return true;
    }
    catch (...)
    {
      return false;
    }
  }

  // scan directory recursively for svn repositories
  bool scan_directory(const std::string& path)
  {
    try
    {
      // retrieve all svn repositories without nospy file
      const auto& dir_filter = [](const std::filesystem::path& p) {
        if(std::filesystem::is_directory(p) &&
           std::filesystem::is_directory(std::filesystem::path(p.string() + "\\.svn")) &&
           !std::filesystem::exists(std::filesystem::path(p.string() + "\\nospy")))
          return true;
        else
          return false;
      };
      const auto& all_svn_dirs = files::get_dirs(path, files::infinite_depth, dir_filter);
      std::set<std::filesystem::path> svn_dirs(all_svn_dirs.begin(), all_svn_dirs.end());

      // remove repos stored in database but not present in directories
      std::vector<std::string> to_remove;
      for (const auto& r : m_repos)
      {
        if (svn_dirs.find(std::filesystem::path(r.first)) == svn_dirs.end())
          to_remove.push_back(r.first);
      }
      for (const auto& r : to_remove)
        m_repos.erase(r);

      // add repos found in directories but not present on the database
      for (const auto& r : svn_dirs)
      {
        if (m_repos.find(r.string()) == m_repos.end())
          m_repos[r.string()];
      }

      return !m_repos.empty();
    }
    catch (...)
    {
      return false;
    }
  }

  // get all the logs - multithread
  bool get_logs(const std::function<void(std::size_t, std::size_t)>& pbar)
  {
    // create queue of logs to retrieve
    std::queue<std::pair<std::string, std::size_t>> queue;  // (string: svn repos path, size_t: last revision checked)
    for (const auto& r : m_repos)
      queue.push({ r.first, r.second.commits.empty() ? 1 : r.second.commits.rbegin()->first });

    // initialize progress-bar
    const std::size_t total_repos = m_repos.size();
    pbar(0, total_repos);

    // protecting data used in threads
    std::mutex mutex;
    std::size_t processed_repos = 0;

    // create and start all threads
    const std::size_t nb_cpu = static_cast<std::size_t>(std::thread::hardware_concurrency()) * 2;
    const std::size_t nb_threads = std::min(queue.size(), nb_cpu);
    std::vector<std::thread> threads(nb_threads);
    for (auto& t : threads)
      t = std::thread(&SvnReposImpl::run,
                      this,
                      std::ref(mutex),
                      std::ref(queue),
                      std::ref(processed_repos),
                      std::ref(pbar));

    // wait for threads termination
    for (auto& t : threads)
      if (t.joinable())
        t.join();

    pbar(total_repos, total_repos);
    return processed_repos == total_repos;
  }

  // retrieve one commit
  std::shared_ptr<struct svn_commit> get_commit(const std::string& repos, int id) const
  {
    // find repos
    const auto& r = m_repos.find(repos);
    if (r == m_repos.end())
      throw std::runtime_error("can't find the repos");

    // find commit
    const auto& c = r->second.commits.find(id);
    if (c == r->second.commits.end())
      throw std::runtime_error("can't find the commit");
    return c->second;
  }

  // retrieve all commits
  std::map<std::string, struct svn_repos> get_commits() const
  {
    return m_repos;
  }

  // display the show log dialog
  void show_logs(const std::string& project, const int revision) const
  {
    const std::string& cmd = fmt::format("TortoiseProc.exe {} {} {} {}",
      "/command:log ",
      fmt::format("/path:\"{}\" ", project),
      fmt::format("/startrev:{} ", revision),
      fmt::format("/endrev:{}", revision));
    static_cast<void>(win::execute(cmd));
  }

private:
  // thread task that try to read svn logs
  void run(std::mutex& mutex, 
           std::queue<std::pair<std::string, std::size_t>>& queue,
           std::size_t& processed_repos,
           const std::function<void(std::size_t, std::size_t)>& pbar)
  {
    while(true)
    {
      // retrieve one item from queue - protected by mutex
      std::string repos;
      std::size_t revision;
      {
        std::lock_guard<std::mutex> lck(mutex);
        if (queue.empty())
          break;
        repos = queue.front().first;
        revision = queue.front().second;
        queue.pop();
      }

      // retrieve and parse logs
      std::string url;
      std::vector<std::shared_ptr<struct svn_commit>> commits;
      if (get_log(repos, revision, url, commits))
      {
        // update database and progress-bar - protected by mutex
        std::lock_guard<std::mutex> lck(mutex);
        const auto& it = m_repos.find(repos);
        if (it != m_repos.end())
        {
          it->second.url = url;
          for (const auto& c : commits)
            if (it->second.commits.find(c->id) == it->second.commits.end())
              it->second.commits[c->id] = c;
        }
        pbar(++processed_repos, m_repos.size());
      }
    }
  }

  // retrieve the log of one repository
  bool get_log(const std::string& repos, 
               const std::size_t revision,
               std::string& url,
               std::vector<std::shared_ptr<struct svn_commit>>& commits)
  {
    try
    {
      // launch the svn get log command
      std::string cmd = fmt::format("svn.exe log --use-merge-history --verbose -r {}:HEAD \"{}\"", revision, repos);
      std::string logs;
      if (win::execute(cmd, logs) != 0)
        return false;

      // lambda to parse one svn commit
      auto parse_commit = [=](const std::string& str) -> std::shared_ptr<struct svn_commit> {
        // parse date, id, author
        struct svn_commit commit;
        {
          std::regex r(R"(^r(\d+) [|] (.*?) [|] (\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}))");
          std::smatch sm;
          if (!std::regex_search(str, sm, r))
            return {};
          commit.date = DateConverter::to_time(sm.str(3));
          commit.id = std::stoi(sm.str(1));
          commit.author = sm.str(2);
        }
        // parse branch and desc
        {
          const std::size_t begin = str.find("Changed paths:");
          if (begin == std::string::npos)
            return {};
          const std::size_t end = str.find("\r\n\r\n", begin);
          if (end == std::string::npos)
            return {};
          std::regex r_trunk(R"(^[ ]{3}. [\/]trunk[\/])");
          std::regex r_branch(R"(^[ ]{3}. [\/]branches[\/]([^\/\r]+))");
          std::smatch sm;
          const std::string& branches = str.substr(begin, end - begin);
          if (std::regex_search(branches, sm, r_trunk))
            commit.branch = "trunk";
          else if (std::regex_search(branches, sm, r_branch))
          {
            const std::string& pattern = " (from ";
            const std::size_t pos = sm.str(1).find(pattern);
            if (pos != std::string::npos)
              commit.branch = sm.str(1).substr(0, pos);
            else
              commit.branch = sm.str(1);
          }
          else
            return {};
          commit.desc = str.substr(end + strlen("\r\n\r\n"));
        }
        return std::make_shared<struct svn_commit>(commit);
      };

      // parse the svn commits output
      {
        std::istringstream ss(logs);
        std::string line;
        std::string commit_separator(72, '-');
        std::string commit_lines;
        while (std::getline(ss, line))
        {
          if (line.find(commit_separator) != std::string::npos)
          {
            const std::shared_ptr<struct svn_commit>& commit = parse_commit(commit_lines);
            if (commit.get())
              commits.push_back(commit);
            commit_lines.clear();
          }
          else
            commit_lines += line + "\n";
        }
      }

      // retrieve repository url
      cmd = fmt::format("svn.exe info \"{}\"", repos);
      if (win::execute(cmd, logs) != 0)
        return false;

      // parse the svn info output
      {
        // extract the url
        std::regex r(R"(^URL: (.*))");
        std::smatch sm;
        if (!std::regex_search(logs, sm, r))
          return false;

        // convert escape sequence of URL
        CHAR buf[1024];
        DWORD buf_size;
        std::string encoded_url = sm.str(1);
        buf_size = (sizeof(buf) / sizeof(char)) - 1;
        if (UrlUnescapeA(encoded_url.data(), buf, &buf_size, URL_UNESCAPE) == S_OK)
          url = utf8::from_utf8(buf);
      }
      return true;
    }
    catch (...)
    {
      return false;
    }
  }

private:
  // store repository infos on database
  std::string m_filename;
  std::string m_root_directory;

  // list of repositories and their commits
  std::map<std::string, struct svn_repos> m_repos;
};

// separate interface from implementation
SvnRepos::SvnRepos(const std::string& filename) : m_pimpl(std::make_unique<SvnReposImpl>(filename)) {}
SvnRepos::~SvnRepos() = default;
bool SvnRepos::load(std::string& root_directory) { if (m_pimpl) return m_pimpl->load(root_directory); else return false; }
bool SvnRepos::save(const std::string& root_directory) { if (m_pimpl) return m_pimpl->save(root_directory); else return false; }
bool SvnRepos::scan_directory(const std::string& path) { if (m_pimpl) return m_pimpl->scan_directory(path); else return false; }
bool SvnRepos::get_logs(const std::function<void(std::size_t, std::size_t)>& cb) { if (m_pimpl) return m_pimpl->get_logs(cb); else return false; }
std::shared_ptr<struct svn_commit> SvnRepos::get_commit(const std::string& repos, int id) const { if (m_pimpl) return m_pimpl->get_commit(repos, id); else return {}; }
std::map<std::string, struct svn_repos> SvnRepos::get_commits() const { if (m_pimpl) return m_pimpl->get_commits(); else return {}; }
void SvnRepos::show_logs(const std::string& project, const int revision) const { if (m_pimpl) return m_pimpl->show_logs(project, revision); }