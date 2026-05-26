#pragma once
#include <filesystem>
#include <optional>
#include <string>

namespace insura::utils {
std::string generateUuid();
std::optional<std::string> stringToOptional(const std::string& s);
std::string currentTimestamp();
bool isValidEmail(std::string_view email);
bool isDigitsOnly(std::string_view str);
bool isValidCsvFile(const std::filesystem::path& path);

inline std::tm safe_localtime(const std::time_t& t) {
  std::tm buf;
#if defined(_WIN32)
  localtime_s(&buf, &t);
#else
  localtime_r(&t, &buf);
#endif
  return buf;
}

namespace date {
bool isValidDate(const std::string& date);
bool isDateAfter(const std::string& start_date, const std::string& end_date);
bool isLeapYear(int year);
std::string today();
std::string calculateEndDate(const std::string& start_date, int duration);
}
}  // namespace insura::utils
