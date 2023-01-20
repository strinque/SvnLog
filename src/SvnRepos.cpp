#include "pch.h"
#include "framework.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <vector>
#include <string>
#include <regex>
#include <set>
#include <fmt/core.h>
#include <winpp/win.hpp>
#include <winpp/files.hpp>
#include <winpp/utf8.hpp>
#include "SvnRepos.h"
#include "DateConverter.h"

// constructor
SvnRepos::SvnRepos(const std::string& filename) :
  m_filename(filename),
  m_root_directory(),
  m_running(true),
  m_mutex(),
  m_queue(),
  m_processed_repos(),
  m_cb(),
  m_repos()
{
}

// destructor
SvnRepos::~SvnRepos()
{
}

// load svn repository database
bool SvnRepos::load(std::string& root_directory)
{
  try
  {
    // open program database
    std::ifstream file(m_filename);
    if (!file)
      return true;

    // parse json database
    const json& svn_db = json::parse(file);

    // retrieve root directory
    root_directory = utf8::from_utf8(svn_db["root_directory"].get<std::string>());

    // parse all repositories
    for (const auto& r : svn_db["repos"])
    {
      // retrieve data from one repository
      const std::string& path = utf8::from_utf8(r["path"].get<std::string>());
      auto& repos = m_repos[path];
      repos.url = utf8::from_utf8(r["url"].get<std::string>());

      // parse all commits
      const auto& commits_obj = r["commits"];
      for (const auto& c : commits_obj)
      {
        // retrieve commit infos
        const std::shared_ptr<struct svn_commit>& commit = std::make_shared<struct svn_commit>(
          svn_commit{
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
bool SvnRepos::save(const std::string& root_directory)
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
bool SvnRepos::scan_directory(const std::string& path)
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

// get all the logs - threaded
bool SvnRepos::get_logs(const std::function<void(std::size_t, std::size_t)>& cb)
{
  // create queue of logs to retrieve
  m_processed_repos = 0;
  m_queue = std::queue<std::pair<std::string, std::size_t>>();
  for (const auto& r : m_repos)
    m_queue.push({ r.first, r.second.commits.empty() ? 1 : r.second.commits.rbegin()->first });

  // initialize progress-bar
  m_cb = cb;
  m_cb(0, m_repos.size());

  // create and start all threads
  const std::size_t nb_threads = std::min(m_repos.size(), static_cast<std::size_t>(std::thread::hardware_concurrency()) * 2);
  std::vector<std::thread> threads(nb_threads);
  for (auto& t : threads)
    t = std::thread(&SvnRepos::run, this);

  // wait for threads termination
  for (auto& t : threads)
    if (t.joinable())
      t.join();

  return m_processed_repos == m_repos.size();
}

// retrieve one commit
const std::shared_ptr<struct svn_commit> SvnRepos::get_commit(const std::string& repos, int id) const
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
const std::map<std::string, struct svn_repos>& SvnRepos::get_commits() const
{
  return m_repos;
}

// display the show log dialog
void SvnRepos::show_logs(const std::string& project, const int revision) const
{
  const std::string& cmd = fmt::format("TortoiseProc.exe {} {} {} {}",
    "/command:log ",
    fmt::format("/path:\"{}\" ", project),
    fmt::format("/startrev:{} ", revision),
    fmt::format("/endrev:{}", revision));
  static_cast<void>(win::execute(cmd));
}

// thread task that try to read svn logs
void SvnRepos::run()
{
  // process items from queue
  while(m_running)
  {
    // retrieve item from queue
    std::string repos;
    std::size_t revision;
    {
      std::lock_guard<std::mutex> lck(m_mutex);
      if (m_queue.empty())
        break;
      repos = m_queue.front().first;
      revision = m_queue.front().second;
      m_queue.pop();
    }

    // retrieve logs
    const bool res = get_log(repos, revision);

    // update progress-bar
    {
      std::lock_guard<std::mutex> lck(m_mutex);
      if(res)
        m_processed_repos++;
      m_cb(m_processed_repos, m_repos.size());
    }
  }
}

// retrieve the log of one repository
bool SvnRepos::get_log(const std::string& repos, const std::size_t revision)
{
  try
  {
    // launch the svn get log command
    // url: https://stackoverflow.com/questions/4881129/how-do-you-see-recent-svn-log-entries
    std::string cmd = fmt::format("svn.exe log -r {}:HEAD \"{}\"", revision, repos);
    std::string logs;
    if (win::execute(cmd, logs) != 0)
      return false;

    // parse the svn commit output
    std::vector<std::shared_ptr<struct svn_commit>> commits;
    {
      std::string commit_separator(72, '-');
      std::regex r(R"(^r(\d+) \| (.*?) \| (\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}) .*\r\n\r\n([\s\S]*))");
      std::cmatch cm;
      std::istringstream ss(logs);
      std::string commit_lines;
      std::string line;
      while (std::getline(ss, line))
      {
        if (line.find(commit_separator) != std::string::npos)
        {
          // parse commit using regex
          if (std::regex_search(commit_lines.c_str(), cm, r))
          {
            const std::time_t date = DateConverter::to_time(cm.str(3));
            const int id = std::stoi(cm.str(1));
            const std::string& author = cm.str(2);
            const std::string& desc = cm.str(4);
            commits.push_back(std::make_shared<struct svn_commit>(svn_commit{ date, id, author, desc }));
          }
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
    std::string url;
    {
      // extract the url
      std::regex r(R"(^URL: (.*))");
      std::cmatch cm;
      if (!std::regex_search(logs.c_str(), cm, r))
        return false;

      // convert escape sequence of URL
      CHAR buf[1024];
      DWORD buf_size;
      std::string encoded_url = cm.str(1);
      buf_size = (sizeof(buf) / sizeof(char)) - 1;
      if (UrlUnescapeA(encoded_url.data(), buf, &buf_size, URL_UNESCAPE) == S_OK)
        url = utf8::from_utf8(buf);
    }

    // update database - protected by mutex
    std::lock_guard<std::mutex> lck(m_mutex);
    const auto& it = m_repos.find(repos);
    if (it != m_repos.end())
    {
      it->second.url = url;
      for (const auto& c : commits)
        if (it->second.commits.find(c->id) == it->second.commits.end())
          it->second.commits[c->id] = c;
    }

    return true;
  }
  catch (...)
  {
    return false;
  }
}