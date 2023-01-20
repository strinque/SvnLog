#pragma once
#include <map>
#include <vector>
#include <string>
#include <stdbool.h>

// store on svn commit infos
struct svn_commit
{
public:
  std::string id;
  std::string author;
  std::string files;
  std::string description;
  // struct tm time
};

// store svn commits infos for one repository
class SvnCommits final
{
public:
  // constructor/destructor
  SvnCommits();
  ~SvnCommits();

  // parse svn log output
  void parse(const std::string& logs);

private:
  std::vector<struct svn_commit> m_commits;
};

// store all svn repositories infos
class SvnRepos final
{
public:
  // constructor/destructor
  SvnRepos();
  ~SvnRepos();

  // scan directory recursively for svn repositories
  bool scan_directory(const std::wstring& path);

  // get the number of svn repositories found
  std::size_t get_nb_repos() const;

  // get all the logs
  void get_logs();

private:
  // execute command and read output
  std::string exec(const std::string& cmd);

  // retrieve log of a specific svn repository
  const std::string get_log(const std::string& repos);

private:
  // list of repositories and their commits
  std::map<std::string, SvnCommits> m_repos;
};
