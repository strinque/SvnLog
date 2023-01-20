#include "pch.h"
#include "framework.h"
#include <iomanip>
#include <sstream>
#include "DateConverter.h"

// convert from std::time_t to string
const std::string DateConverter::to_str(const std::time_t& date, const std::string& fmt)
{
#pragma warning( push )
#pragma warning( disable: 4996 )
  const struct std::tm tm = *std::localtime(&date);
#pragma warning( pop )
  std::ostringstream ss;
  ss << std::put_time(&tm, fmt.c_str());
  return ss.str();
}

// convert from string to std::time_t
const std::time_t DateConverter::to_time(const std::string& str, const std::string& fmt)
{
  struct std::tm tm {};
  std::istringstream ss(str);
  ss >> std::get_time(&tm, fmt.c_str());
  return std::mktime(&tm);
}