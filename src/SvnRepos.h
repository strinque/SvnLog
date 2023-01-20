#pragma once
#include <vector>
#include <string>
#include <stdbool.h>

class SvnRepos
{
public:
  // constructor/destructor
  SvnRepos();
  ~SvnRepos();

  // scan directory recursively for svn repositories
  bool scan_directory(const std::wstring& path);

  // get the number of svn repositories found
  std::size_t get_nb_repos() const;

private:
  // list of repositories
  std::vector<std::string> m_repos;
};