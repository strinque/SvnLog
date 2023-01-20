#include "pch.h"
#include "framework.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <cstdio>
#include <utf8.h>
#include "SvnRepos.h"

// svn commit parser separator definitions
const std::string commit_sep = "------------------------------------------------------------------------\r\n";
const std::string commit_elem_sep = " | ";
const std::string commit_path_sep = "Changed paths:\r\n";

// constructor
SvnRepos::SvnRepos(const std::string& filename) :
  m_filename(filename),
  m_commits(),
  m_repos()
{
}

// destructor
SvnRepos::~SvnRepos()
{
}

// load svn repository database
bool SvnRepos::load()
{
  try
  {
    // open program database
    std::ifstream file(m_filename);
    if (!file)
      return true;

    // parse json database
    const json& svn_db = json::parse(file);

    // parse all repositories
    const auto& repos_obj = svn_db["repos"];
    for (const auto& r : repos_obj)
    {
      // retrieve reposiroty infos
      const std::string path = r["path"].get<std::string>();

      // parse all commits
      std::map<int, std::shared_ptr<struct svn_commit>> repos;
      const auto& commits_obj = r["commits"];
      for (const auto& c : commits_obj)
      {
        // retrieve commit infos
        std::shared_ptr<struct svn_commit> commit = std::make_shared<struct svn_commit>(svn_commit{
          from_utf8(path),
          from_utf8(c["date"].get<std::string>()),
          c["id"].get<int>(),
          from_utf8(c["author"].get<std::string>()),
          from_utf8(c["files"].get<std::string>()),
          from_utf8(c["desc"].get<std::string>())});

        // store commit infos
        m_commits.insert(commit);
        repos[commit->id] = commit;
      }
      m_repos[path] = repos;
    }

    return true;
  }
  catch (...)
  {
    return false;
  }
}

// save svn repository database
bool SvnRepos::save()
{
  try
  {
    // loop through all repositories
    json svn_db;
    auto& repos_array_obj = svn_db["repos"] = json::array();
    for (const auto& r : m_repos)
    {
      // create repository object
      json repos_obj;
      repos_obj["path"] = to_utf8(r.first);
      repos_obj["last_commit"] = r.second.empty() ? "" : std::to_string(r.second.begin()->first);

      // loop through all commits
      auto& commits_obj = repos_obj["commits"] = json::array();
      for (const auto& c : r.second)
      {
        json commit_obj = {
          {"date", c.second->date},
          {"id", c.second->id},
          {"author", to_utf8(c.second->author)},
          {"files", to_utf8(c.second->files)},
          {"desc", to_utf8(c.second->desc)}
        };
        commits_obj.push_back(commit_obj);
      }

      // add repository to repos array
      repos_array_obj.push_back(repos_obj);
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
bool SvnRepos::scan_directory(const std::wstring& path)
{
  try
  {
    // loop through all subdirectories
    std::set<std::string> svn_dirs;
    for (const auto& p : std::filesystem::recursive_directory_iterator(path))
    {
      if (std::filesystem::is_directory(p) &&
        (p.path().filename().u8string() == ".svn"))
      {
        const std::string& repos_path = p.path().u8string();
        const std::size_t pos = repos_path.rfind("\\.svn");
        svn_dirs.insert(repos_path.substr(0, pos));
      }
    }

    // remove repos stored in database but not present in directories
    std::vector<std::string> to_remove;
    for (const auto& r : m_repos)
    {
      if (svn_dirs.find(r.first) == svn_dirs.end())
        to_remove.push_back(r.first);
    }
    for (const auto& r : to_remove)
      m_repos.erase(r);

    // add repos found in directories but not present on the database
    for (const auto& r : svn_dirs)
    {
      if (m_repos.find(r) == m_repos.end())
        m_repos[r];
    }

    return !m_repos.empty();
  }
  catch (...)
  {
    return false;
  }
}

// get all the logs
bool SvnRepos::get_logs(const std::function<void(std::size_t, std::size_t)>& cb)
{
  try
  {
    // get the logs of each repository
    const std::size_t total = m_repos.size();
    std::size_t idx = 0;
    for (auto& r : m_repos)
    {
      // launch the svn get log command
      // https://stackoverflow.com/questions/4881129/how-do-you-see-recent-svn-log-entries
      std::string cmd = "svn.exe log ";
      cmd += "-r HEAD:1 "; // sort logs
      cmd += "-v "; // verbose mode
      cmd += '"' + r.first + '"';
      const std::string& logs = exec(cmd);

      // parse logs
      const std::vector<std::shared_ptr<struct svn_commit>>& commits = parse(r.first, logs);
      for (const auto& c : commits)
      {
        if (r.second.find(c->id) == r.second.end())
        {
          r.second[c->id] = c;
          m_commits.insert(c);
        }
      }

      // call the update callback
      if (cb)
        cb(++idx, total);
    }
    return true;
  }
  catch (...)
  {
    return false;
  }
}

// retrieve all the commits
const std::set<std::shared_ptr<struct svn_commit>> SvnRepos::get_commits() const
{
  return m_commits;
}

// retrieve one commit
const std::shared_ptr<struct svn_commit> SvnRepos::get_commit(const std::string& repos, int id)
{
  // find repos
  const auto& r = m_repos.find(repos);
  if (r == m_repos.end())
    throw std::exception("can't find the repos");

  // find commit
  const auto& c = r->second.find(id);
  if (c == r->second.end())
    throw std::exception("can't find the commit");
  return c->second;
}

// execute command and read output
std::string SvnRepos::exec(const std::string& cmd)
{
  // convert from string to wide_string
  wchar_t cmd_wide[MAX_PATH];
  wcsncpy_s(cmd_wide, MAX_PATH, CString(CStringA(cmd.c_str())).GetString(), MAX_PATH);

  // configure handles for stdout
  HANDLE hStdOutPipeRead = NULL;
  HANDLE hStdOutPipeWrite = NULL;
  SECURITY_ATTRIBUTES sa{ sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
  if (!CreatePipe(&hStdOutPipeRead, &hStdOutPipeWrite, &sa, 0))
    throw std::exception("can't create pipes");

  // create and execute the process
  STARTUPINFO si{};
  GetStartupInfo(&si);
  si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
  si.wShowWindow = SW_HIDE;
  si.hStdOutput = hStdOutPipeWrite;
  PROCESS_INFORMATION pi{};
  if (!CreateProcess(NULL, cmd_wide, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
  {
    CloseHandle(hStdOutPipeRead);
    throw std::exception("can't start program");
  }
  CloseHandle(hStdOutPipeWrite);

  // read program stdout
  std::string output;
  char buf[4096];
  DWORD bytes = 0;
  while (ReadFile(hStdOutPipeRead, buf, sizeof(buf) - 1, &bytes, NULL) && bytes)
  {
    buf[bytes] = 0;
    output += buf;
  }

  // read exit code
  DWORD exit_code = 0;
  GetExitCodeProcess(pi.hProcess, &exit_code);

  // cleanup
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  CloseHandle(hStdOutPipeRead);

  // check exit-code
  if (exit_code != 0)
    throw std::exception("program execution failed");

  return output;
}

// parse svn log output
std::vector<std::shared_ptr<struct svn_commit>> SvnRepos::parse(const std::string& repos, const std::string& logs)
{
  // find the start of a commit
  std::size_t pos = 0;
  std::size_t pos_e = 0;
  std::vector<std::shared_ptr<struct svn_commit>> commits;
  while ((pos = logs.find(commit_sep, pos)) != std::string::npos)
  {
    // parse id
    pos += commit_sep.size();
    pos_e = logs.find(commit_elem_sep, pos);
    if (pos_e == std::string::npos)
      break;
    const int id = std::stoi(logs.substr(pos+1, pos_e - pos - 1));
    pos = pos_e + commit_elem_sep.size();

    // parse author
    pos_e = logs.find(commit_elem_sep, pos);
    if (pos_e == std::string::npos)
      break;
    const std::string& author = logs.substr(pos, pos_e - pos);
    pos = pos_e + commit_elem_sep.size();

    // parse date
    pos_e = logs.find(commit_elem_sep, pos);
    if (pos_e == std::string::npos)
      break;
    const std::string& date = logs.substr(pos, strlen("xxxx-xx-xx xx:xx:xx"));
    pos = pos_e + commit_elem_sep.size();

    // parse files
    pos_e = logs.find(commit_path_sep, pos);
    if (pos_e == std::string::npos)
      break;
    pos = pos_e + commit_path_sep.size();
    pos_e = logs.find("\r\n\r\n", pos);
    if (pos_e == std::string::npos)
      break;
    const std::string& files = logs.substr(pos, pos_e - pos);
    pos = pos_e + 4;

    // parse commit description
    pos_e = logs.find(commit_sep, pos);
    if (pos_e == std::string::npos)
      break;
    const std::string& desc = logs.substr(pos, pos_e - pos);

    // add to commit list
    commits.push_back(std::make_shared<struct svn_commit>(svn_commit{ repos,
                                                                      date,
                                                                      id, 
                                                                      author,
                                                                      files,
                                                                      desc }));
  }
  return commits;
}
