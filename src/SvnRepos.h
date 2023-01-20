#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <functional>
#include <stdbool.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// store svn commit infos
struct svn_commit
{
  std::string repos;
  std::string date;
  int id;
  std::string author;
  std::string files;
  std::string desc;
};

// store all svn repositories infos
class SvnRepos final
{
public:
  // constructor/destructor
  SvnRepos(const std::string& filename = "svn-repos.json");
  ~SvnRepos();

  // load/save svn-repository infos into json database file
  bool load();
  bool save();

  // scan directory recursively for svn repositories
  bool scan_directory(const std::wstring& path);

  // get all the logs - with callback for progression
  bool get_logs(const std::function<void(std::size_t, std::size_t)>& cb = {});

  // retrieve all the commits
  const std::set<std::shared_ptr<struct svn_commit>> get_commits() const;

  // retrieve one commit
  const std::shared_ptr<struct svn_commit> get_commit(const std::string& repos, int id);

private:
  // execute command and read output
  std::string exec(const std::string& cmd);

  // parse svn log output
  std::vector<std::shared_ptr<struct svn_commit>> parse(const std::string& repos, const std::string& logs);

private:
  // store repository infos on database
  std::string m_filename;

  // list of repositories and their commits
  std::set<std::shared_ptr<struct svn_commit>> m_commits;
  std::map<std::string, std::map<int, std::shared_ptr<struct svn_commit>>> m_repos;
};
