#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "../domain/client.hpp"
#include "../domain/policy.hpp"

namespace insura::cli {

class PolicyView {
 public:
  static void displayAll(
      const std::vector<domain::Policy>& policies,
      const std::unordered_map<std::string, std::string>& client_names);
  static bool confirmClient(const domain::Client& client);
  static void displayOne(const domain::Policy& policy,
                         std::string_view client_name);
};

}  // namespace insura::cli
