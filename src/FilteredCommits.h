#pragma once
#include <ctime>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include "SvnRepos.h"

// store all values of a specific commit
struct commit
{
  std::string repos;
  std::string url;
  std::time_t date;
  std::string date_str;
  int id;
  std::string id_str;
  std::string author;
};

// special sort function for commits (date/id/pointer)
struct commit_sort
{
  bool operator()(const std::shared_ptr<struct commit>& lhs, 
                  const std::shared_ptr<struct commit>& rhs) const
  {
    // sort by date
    if (lhs->date < rhs->date)
      return false;
    else if (lhs->date > rhs->date)
      return true;

    // sort by id
    if (lhs->id < rhs->id)
      return false;
    else if (lhs->id > rhs->id)
      return true;

    // sort by pointers
    return (lhs.get() < rhs.get());
  }
};

// sort commits and filter them
class FilteredCommits final
{
public:
  // constructor/destructor
  FilteredCommits();
  ~FilteredCommits();

  // register commits
  void set_commits(const std::map<std::string, struct svn_repos>& repos);

  // retrieve the sorted and filtered commit list
  std::vector<std::shared_ptr<struct commit>> get_commits() const;
  std::shared_ptr<struct commit> get_commit(const int idx) const;
  std::set<std::string> get_projects() const;
  std::set<std::string> get_authors() const;

  // set filters properties
  void enable_filter_from(const SYSTEMTIME& st);
  void disable_filter_from();
  void enable_filter_to(const SYSTEMTIME& st);
  void disable_filter_to();
  void enable_filter_project(const std::string& str);
  void disable_filter_project();
  void enable_filter_author(const std::string& str);
  void disable_filter_author();

  // filter commits
  void filter_commits();

private:
  // convert from SYSTEMTIME to std::time_t
  std::time_t to_time(const SYSTEMTIME& st);

private:
  // filters value
  std::time_t m_date_from;
  std::time_t m_date_to;
  std::string m_project;
  std::string m_author;

  // filters activation
  bool m_date_from_enabled;
  bool m_date_to_enabled;
  bool m_project_enabled;
  bool m_author_enabled;

  // sort commits
  std::set<std::shared_ptr<struct commit>, struct commit_sort> m_sorted;
  std::vector<std::shared_ptr<struct commit>> m_filtered;
  std::set<std::string> m_projects;
  std::set<std::string> m_authors;
};