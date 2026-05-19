#include "utils.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <mutex>
#include <iomanip>
#include <random>
#include <regex>
#include <sstream>

namespace insura::utils {

/*
 * UUID v4 generator implemented from scratch using std::stringstream.
 *
 * Preferred from scratch implementation in order to learn about uuid4
 * generation and string stream.
 *
 * UUID v4 structure: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
 * where x is a random hex digit, 4 is fixed (version), and y is
 * one of 8, 9, a, b (RFC 4122 variant).
 *
 * std::hex tells the stream to format integers as hexadecimal, so
 * writing the integer 10 produces "a" rather than "10".
 *
 * Static variables are initialised once on the first call and reused
 * on subsequent calls to avoid recreating the generator and
 * distributions on every invocation.
 *
 * TODO: not thread-safe due to shared static state. If UUID generation
 * is needed across multiple threads, add a mutex around this function.
 * I must revisit in Milestone 3 when the auto-save thread is introduced.
 *
 */
std::optional<std::string> stringToOptional(const std::string& s) {
  return s.empty() ? std::nullopt : std::optional<std::string>{s};
}

std::string generateUuid() {
  static std::mutex uuid_mtx;
  static std::random_device rd;
  static std::mt19937 mt(rd());
  static std::uniform_int_distribution<> dist(0, 15);
  static std::uniform_int_distribution<> dist2(8, 11);
  std::lock_guard<std::mutex> lock(uuid_mtx);

  std::stringstream ss;
  int i;
  ss << std::hex;

  for (i = 0; i < 8; i++) ss << dist(mt);
  ss << "-";
  for (i = 0; i < 4; i++) ss << dist(mt);
  ss << "-4";
  for (i = 0; i < 3; i++) ss << dist(mt);
  ss << "-";
  ss << dist2(mt);
  for (i = 0; i < 3; i++) ss << dist(mt);
  ss << "-";
  for (i = 0; i < 12; i++) ss << dist(mt);

  return ss.str();
}

/* TODO: looks for performance improvements (maybe some conversion from
 * std::string to std::...::path happens under the hood, idk*/
bool isValidCsvFile(const std::filesystem::path& path) {
  return path.extension() == ".csv";
}

std::string currentTimestamp() {
  auto now = std::chrono::system_clock::now();
  time_t t = std::chrono::system_clock::to_time_t(now);

  std::tm* tm = std::localtime(&t);
  std::ostringstream ss;

  ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
  return ss.str();
};

bool isValidEmail(std::string_view email) {
  /*
   * Regex created with AI assistance (Claude).
   *
   * Stricter than the previous pattern:
   *  - local part must start and end with an alphanumeric character
   *  - domain labels cannot start or end with a hyphen
   *  - TLD is restricted to alphabetic characters only (2+ chars)
   *
   * Known limitation: consecutive dots in the local part (e.g. a..b@x.com)
   * are not rejected; a lookahead would be needed to catch them.
   *
   * Using iterators because std::regex_match has no overload for
   * std::string_view.
   */
  static const std::regex pattern(
      R"([a-zA-Z0-9]([a-zA-Z0-9.+_-]*[a-zA-Z0-9])?@[a-zA-Z0-9]([a-zA-Z0-9-]*[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9-]*[a-zA-Z0-9])?)*\.[a-zA-Z]{2,})");
  return std::regex_match(email.begin(), email.end(), pattern);
}

bool isDigitsOnly(std::string_view str) {
  return !str.empty() &&
         std::find_if(str.begin(), str.end(), [](unsigned char c) {
           return !std::isdigit(c);
         }) == str.end();
}

bool isValidPhone(std::string_view phone) {
  /* TODO: standardise phone display: add country-code prefix (e.g. +39)
   * and decide whether to keep phone as std::string or introduce a dedicated
   * type/format. Handle at end-of-project cleanup. */
  return isDigitsOnly(phone);
}
namespace date {

std::string calculateEndDate(const std::string& start_date, int duration) {
  static constexpr std::array<int, 12> days_per_month = {
      31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  int year = stoi(start_date.substr(0, 4));
  int month = stoi(start_date.substr(5, 2)) + duration;
  int day = stoi(start_date.substr(8, 2));

  if (month > 12) {
    year += (month - 1) / 12;
    month = (month - 1) % 12 + 1;
  }

  int max_day = days_per_month[month - 1];
  if (month == 2 && isLeapYear(year)) {
    max_day = 29;
  }
  if (day > max_day) {
    day = max_day;
  }

  std::ostringstream ss;
  ss << std::setfill('0') << std::setw(4) << year << "-" << std::setw(2)
     << month << "-" << std::setw(2) << day;

  return ss.str();
}

bool isLeapYear(int year) {
  return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
}

/* This function was specifically designed for the format YYYY-MM-DD
 * if the format is not zero-padded, it doesn't work anymore */
bool isDateAfter(const std::string& start_date, const std::string& end_date) {
  return end_date > start_date;
}

bool isValidDate(const std::string& date) {
  static constexpr std::array<int, 12> days_per_month = {
      31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  static const std::regex pattern(R"(\d{4}-\d{2}-\d{2})");
  if (!std::regex_match(date.begin(), date.end(), pattern)) return false;

  int year = stoi(date.substr(0, 4));
  int month = stoi(date.substr(5, 2));
  int day = stoi(date.substr(8, 2));

  if (year < 2026 || year > 9999) return false;
  if (month < 1 || month > 12) return false;
  if (isLeapYear(year) && month == 2) {
    if (day < 1 || day > 29) return false;
    return true;
  }
  if (day < 1 || day > days_per_month[month - 1]) return false;

  return true;
}

std::string today() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm* tm = std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(tm, "%Y-%m-%d");
  return oss.str();
}

}  // namespace date

}  // namespace insura::utils
