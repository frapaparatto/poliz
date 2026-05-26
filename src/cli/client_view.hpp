#pragma once

#include <vector>
#include "../domain/client.hpp"
#include "../domain/policy.hpp"

namespace insura::cli {

class ClientView {
 public:
  static void displayAll(const std::vector<domain::Client>& clients);
  static void displayOne(const domain::Client& client);
  static void displayPolicies(const std::vector<domain::Policy>& policies);
};

}  // namespace insura::cli
