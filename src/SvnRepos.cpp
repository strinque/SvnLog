#include "pch.h"
#include "framework.h"
#include <filesystem>
#include "SvnRepos.h"

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
  // loop through all subdirectories
  m_repos.clear();
  for (const auto& p : std::filesystem::recursive_directory_iterator(path))
  {
    if (std::filesystem::is_directory(p) && 
       (p.path().filename().u8string() == ".svn"))
    {
      const std::string& repos_path = p.path().u8string();
      const std::size_t pos = repos_path.rfind("\\.svn");
      m_repos.push_back(repos_path.substr(0, pos));
    }
  }
  return !m_repos.empty();
}

// get the number of svn repositories found
std::size_t SvnRepos::get_nb_repos() const
{
  return m_repos.size();
}
