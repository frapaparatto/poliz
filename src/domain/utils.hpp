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
bool isValidPhone(std::string_view phone);
bool isValidCsvFile(const std::filesystem::path& path);

/* I have to create helper like isValidTime for appointments*/

namespace date {
bool isValidDate(const std::string& date);
bool isDateAfter(const std::string& start_date, const std::string& end_date);
bool isLeapYear(int year);
std::string today();
std::string calculateEndDate(const std::string& start_date, int duration);
}
}  // namespace insura::utils
