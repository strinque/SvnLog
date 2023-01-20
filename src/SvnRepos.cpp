#include "pch.h"
#include "framework.h"
#include <filesystem>
#include <array>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include "SvnRepos.h"

// svn commit parser separator definitions
const std::string commit_sep = "------------------------------------------------------------------------\r\n";
const std::string commit_elem_sep = " | ";
const std::string commit_path_sep = "Changed paths:\r\n";

// constructor
SvnCommits::SvnCommits() : 
  m_commits()
{
}

// destructor
SvnCommits::~SvnCommits()
{
}

// parse svn log output
void SvnCommits::parse(const std::string& logs)
{
  // clear previous commits
  m_commits.clear();

  // find the start of a commit
  std::size_t pos = 0;
  std::size_t pos_e = 0;
  while ((pos = logs.find(commit_sep, pos)) != std::string::npos)
  {
    // create empty commit
    struct svn_commit commit {};

    // parse id
    pos += commit_sep.size();
    pos_e = logs.find(commit_elem_sep, pos);
    if (pos_e == std::string::npos)
      break;
    commit.id = logs.substr(pos, pos_e - pos);
    pos = pos_e + commit_elem_sep.size();

    // parse author
    pos_e = logs.find(commit_elem_sep, pos);
    if (pos_e == std::string::npos)
      break;
    commit.author = logs.substr(pos, pos_e - pos);
    pos = pos_e + commit_elem_sep.size();

    // parse files
    pos_e = logs.find(commit_path_sep, pos);
    if (pos_e == std::string::npos)
      break;
    pos = pos_e + commit_path_sep.size();
    pos_e = logs.find("\r\n\r\n", pos);
    if (pos_e == std::string::npos)
      break;
    commit.files = logs.substr(pos, pos_e - pos);
    pos = pos_e + 4;

    // parse commit description
    pos_e = logs.find(commit_sep, pos);
    if (pos_e == std::string::npos)
      break;
    commit.description = logs.substr(pos, pos_e - pos);

    // add to commit list
    m_commits.push_back(commit);
  }
}

// constructor
SvnRepos::SvnRepos() :
  m_repos()
{
}

// destructor
SvnRepos::~SvnRepos()
{
}

// scan directory recursively for svn repositories
bool SvnRepos::scan_directory(const std::wstring& path)
{
  try
  {
    // loop through all subdirectories
    m_repos.clear();
    for (const auto& p : std::filesystem::recursive_directory_iterator(path))
    {
      if (std::filesystem::is_directory(p) &&
        (p.path().filename().u8string() == ".svn"))
      {
        const std::string& repos_path = p.path().u8string();
        const std::size_t pos = repos_path.rfind("\\.svn");
        m_repos[repos_path.substr(0, pos)];
      }
    }
    return !m_repos.empty();
  }
  catch (...)
  {
    return false;
  }
}

// get the number of svn repositories found
std::size_t SvnRepos::get_nb_repos() const
{
  return m_repos.size();
}

// get all the logs
void SvnRepos::get_logs()
{
  for (auto& r : m_repos)
    r.second.parse(get_log(r.first));
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

// retrieve log of a specific svn repository
const std::string SvnRepos::get_log(const std::string& repos)
{
  // launch the svn get log command
  // https://stackoverflow.com/questions/4881129/how-do-you-see-recent-svn-log-entries
  std::string cmd = "svn.exe log ";
  cmd += "-r HEAD:1 "; // sort logs
  cmd += "-v "; // verbose mode
  cmd += '"' + repos + '"';
  return exec(cmd);
}
