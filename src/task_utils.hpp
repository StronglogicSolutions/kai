#pragma once

#include <string>
#include <regex>
#include <ctime>

static bool is_english(const std::string& content)
{
  static const std::regex english_words_regex("the|and|is|a|an|in|of|on|to|for|by|with|at|from|or|that|of|was|in|this|it|is|not|by|for|on|to|with|as|in|at|be|of|or|a|an");
  std::sregex_iterator it(content.begin(), content.end(), english_words_regex);
  std::sregex_iterator end;
  return it != end;
}

std::string post(const std::string& query, const std::string& url);

// Function to find the recent date in the Google search results
static std::string find_recent_date(const std::string& response)
{
    // Use a regular expression to find the date in the Google search results
    std::regex date_regex("([0-9]{4})-([0-9]{2})-([0-9]{2})");
    std::smatch date_match;

    // Search for the date in the response
    if (std::regex_search(response, date_match, date_regex))
    {
        int year = std::stoi(date_match[1]);
        int month = std::stoi(date_match[2]);
        int day = std::stoi(date_match[3]);

        // Format the date as YYYY-MM-DD
        char date_str[11];
        const auto date = std::tm{ .tm_mday = day, .tm_mon = month - 1, .tm_year = year - 1900 };
        std::strftime(date_str, sizeof(date_str), "%Y-%m-%d", &date);
        return date_str;
    }

    // If no date is found, return an empty string
    return "";
}

static bool is_recent(const std::string& date)
{
  const auto       today      = std::time(nullptr);
  const auto       today_date = std::gmtime(&today);
        struct tm  today_tm   = *today_date;
        std::tm    date_tm;
  std::sscanf(date.c_str(), "%d-%d-%d", &date_tm.tm_year, &date_tm.tm_mon, &date_tm.tm_mday);

  const auto date_year   = date_tm.tm_year + 1900;
  const auto date_month  = date_tm.tm_mon + 1;
  const auto today_year  = today_tm.tm_year + 1900;
  const auto today_month = today_tm.tm_mon + 1;
  const auto time_diff   = std::difftime(std::mktime(&date_tm), mktime(&today_tm));

  return time_diff < 7 * 24 * 3600; // 7 days
}

