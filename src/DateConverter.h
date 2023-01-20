#pragma once
#include <string>
#include <ctime>

// helper class to convert date from/to string <=> std::time_t
class DateConverter final
{
public:
  // convert from std::time_t to string
  static const std::string to_str(const std::time_t& date,
                                  const std::string& fmt = "%Y-%m-%d %H:%M:%S");

  // convert from string to std::time_t
  static const std::time_t to_time(const std::string& str,
                                   const std::string& fmt = "%Y-%m-%d %H:%M:%S");
};