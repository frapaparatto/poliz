#include "strops.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace insura::domain::strops {

std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return ::tolower(c); });
  return s;
}

std::string trim(std::string s) {
  /* trim starting from the left */
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {
            return !std::isspace(c);
          }));

  /* trim starting from right */
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char c) { return !std::isspace(c); })
              .base(),
          s.end());

  return s;
}

std::string capitalize(std::string s) {

  bool next = true;
  for (auto& c : s) {
    if (std::isspace(static_cast<unsigned char>(c))) {
      next = true;
    } else if (next) {
      c = ::toupper(static_cast<unsigned char>(c));
      next = false;
    }
  }

  return s;
}

bool contains(std::string_view text, std::string_view query) {
  auto it = std::search(text.begin(), text.end(), query.begin(), query.end(),
                        [](unsigned char a, unsigned char b) {
                          return ::tolower(a) == ::tolower(b);
                        });

  return it != text.end();
}

}  // namespace insura::domain::strops
