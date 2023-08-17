#include "pch.h"
#include "framework.h"
#include "DateConverter.h"
#include "FilteredCommits.h"
#include <fmt/core.h>
#include <regex>

// constructor
FilteredCommits::FilteredCommits() :
  m_branch(),
  m_date_from(),
  m_date_to(),
  m_project(),
  m_author(),
  m_branch_enabled(false),
  m_date_from_enabled(false),
  m_date_to_enabled(false),
  m_project_enabled(false),
  m_author_enabled(false),
  m_sorted(),
  m_filtered(),
  m_branches(),
  m_projects(),
  m_authors()
{
}

// destructor
FilteredCommits::~FilteredCommits()
{
}

// register commits
void FilteredCommits::set_commits(const std::map<std::string, struct svn_repos>& repos)
{
  // construct list of sorted commits
  m_sorted.clear();
  m_filtered.clear();
  m_branches.clear();
  m_projects.clear();
  m_authors.clear();
  for (const auto& r : repos)
  {
    // add project name
    m_projects.insert(r.first);

    // sort all commits
    for (const auto& c : r.second.commits)
    {
      // add author name
      m_authors.insert(c.second->author);

      // add branch
      m_branches.insert(c.second->branch);

      // add commit information
      m_sorted.insert(std::make_shared<struct commit>(
        commit{ 
          r.first,
          c.second->branch,
          r.second.url,
          c.second->date,
          DateConverter::to_str(c.second->date),
          c.second->id,
          std::to_string(c.second->id),
          c.second->author
        }));
    }
  }

  // filters the commits
  filter_commits();
}

// retrieve the sorted and filtered commit list
std::vector<std::shared_ptr<struct commit>> FilteredCommits::get_commits() const
{
  return m_filtered;
}

std::shared_ptr<struct commit> FilteredCommits::get_commit(const int idx) const
{
  return m_filtered.at(idx);
}

std::set<std::string> FilteredCommits::get_branches() const
{
  return m_branches;
}

std::set<std::string> FilteredCommits::get_projects() const
{
  return m_projects;
}

std::set<std::string> FilteredCommits::get_authors() const
{
  return m_authors;
}

void FilteredCommits::enable_filter_branch(const std::string& str)
{
  m_branch = str;
  m_branch_enabled = true;
}

void FilteredCommits::disable_filter_branch()
{
  m_branch_enabled = false;
}

void FilteredCommits::enable_filter_from(const SYSTEMTIME& st)
{
  m_date_from = to_time(st);
  m_date_from_enabled = true;
}

void FilteredCommits::disable_filter_from()
{
  m_date_from_enabled = false;
}

void FilteredCommits::enable_filter_to(const SYSTEMTIME& st)
{
  m_date_to = to_time(st);
  m_date_to_enabled = true;
}

void FilteredCommits::disable_filter_to()
{
  m_date_to_enabled = false;
}

void FilteredCommits::enable_filter_project(const std::string& str)
{
  m_project = str;
  m_project_enabled = true;
}

void FilteredCommits::disable_filter_project()
{
  m_project_enabled = false;
}

void FilteredCommits::enable_filter_author(const std::string& str)
{
  m_author = str;
  m_author_enabled = true;
}

void FilteredCommits::disable_filter_author()
{
  m_author_enabled = false;
}

void FilteredCommits::filter_commits()
{
  // construct list of sorted and filtered commits
  m_filtered.clear();
  for (const auto& c : m_sorted)
  {
    // apply filter: date_from
    if (m_date_from_enabled &&
      (m_date_from > c->date))
      break;

    // apply filter: date_to
    if (m_date_to_enabled &&
      (m_date_to < c->date))
      continue;

    // apply filter: branch
    if (m_branch_enabled &&
      (m_branch != c->branch))
      continue;

    // apply filter: project
    if (m_project_enabled &&
      (m_project != c->repos))
      continue;

    // apply filter: author
    if (m_author_enabled &&
      (m_author != c->author))
      continue;

    // add the filtered commit
    m_filtered.push_back(c);
  }
}

// convert from SYSTEMTIME to std::time_t
std::time_t FilteredCommits::to_time(const SYSTEMTIME& st)
{
  return DateConverter::to_time(
    fmt::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
      st.wYear,
      st.wMonth,
      st.wDay,
      st.wHour,
      st.wMinute,
      st.wSecond));
}