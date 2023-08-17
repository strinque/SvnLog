#pragma once
#include <string>
#include <memory>
#include <map>
#include <functional>
#include <stdbool.h>

// store svn commit infos
struct svn_commit
{
  std::string branch;
  std::time_t date;
  int id = 0;
  std::string author;
  std::string desc;
};

// store one svn repository info
struct svn_repos
{
  std::string url;
  std::map<int, std::shared_ptr<struct svn_commit>> commits;
};

// store all svn repositories infos
class SvnReposImpl;
class SvnRepos final
{
  // delete copy/assignement operators
  SvnRepos(const SvnRepos&) = delete;
  SvnRepos& operator=(const SvnRepos&) = delete;
  SvnRepos(SvnRepos&&) = delete;
  SvnRepos& operator=(SvnRepos&&) = delete;

public:
  // constructor/destructor
  SvnRepos(const std::string& filename = "svn-repos.json");
  ~SvnRepos();

  // load/save svn-repository infos into json database file
  bool load(std::string& root_directory);
  bool save(const std::string& root_directory);

  // scan directory recursively for svn repositories
  bool scan_directory(const std::string& path);

  // get all the logs - with callback for progression
  bool get_logs(const std::function<void(std::size_t, std::size_t)>& cb = {});

  // retrieve one specific commit
  std::shared_ptr<struct svn_commit> get_commit(const std::string& repos, int id) const;

  // retrieve all commits
  std::map<std::string, struct svn_repos> get_commits() const;

  // display the show log dialog
  void show_logs(const std::string& project, const int revision) const;

private:
  // pointer to internal implementation
  std::unique_ptr<SvnReposImpl> m_pimpl;
};