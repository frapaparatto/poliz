#pragma once

#include <string>

#include "./policy.hpp"

namespace insura::domain {

struct PolicyData {
  std::string client_uuid;
  Policy::PolicyType policy_type_;
  std::string start_date;
  int duration_months;
  std::optional<Policy::PolicyStatus> policy_status;
  std::optional<std::string> notes;
};

struct PolicyFilter {
  std::optional<std::string> client_uuid;
  std::optional<Policy::PolicyStatus> status;
  std::optional<Policy::PolicyType> type;
};

}  // namespace insura::domain
