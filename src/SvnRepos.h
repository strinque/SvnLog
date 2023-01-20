#pragma once
#include <map>
#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <utility>
#include <mutex>
#include <stdbool.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// store svn commit infos
struct svn_commit
{
  std::time_t date;
  int id;
  std::string author;
  std::string desc;
};

// store one svn repository info
struct svn_repos
{
  // default constructor
  svn_repos() : url(), commits() {}

  std::string url;
  std::map<int, std::shared_ptr<struct svn_commit>> commits;
};

// store all svn repositories infos
class SvnRepos final
{
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
  const std::shared_ptr<struct svn_commit> get_commit(const std::string& repos, int id) const;

  // retrieve all commits
  const std::map<std::string, struct svn_repos>& get_commits() const;

  // display the show log dialog
  void show_logs(const std::string& project, const int revision) const;

private:
  // thread which
  void run();

  // retrieve the log of one repository
  bool get_log(const std::string& repos, const std::size_t revision);

private:
  // store repository infos on database
  std::string m_filename;
  std::string m_root_directory;

  // protecting data used in threads
  std::atomic<bool> m_running;
  std::mutex m_mutex;
  std::queue<std::pair<std::string, std::size_t>> m_queue;  // (string: svn repos path, size_t: last revision checked)
  std::size_t m_processed_repos;                            // number of processed repos
  std::function<void(std::size_t, std::size_t)> m_cb;       // used for updating progress-bar

  // list of repositories and their commits
  std::map<std::string, struct svn_repos> m_repos;
};