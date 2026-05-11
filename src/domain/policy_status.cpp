#include "policy_status.hpp"

#include <stdexcept>
#include <string>

namespace insura::domain {

std::string policyStatusToString(Policy::PolicyStatus status) {
  switch (status) {
    case Policy::PolicyStatus::ACTIVE:
      return "active";
    case Policy::PolicyStatus::EXPIRED:
      return "expired";
    case Policy::PolicyStatus::CANCELLED:
      return "cancelled";
    case Policy::PolicyStatus::PENDING:
      return "pending";
    default:
      throw std::invalid_argument("unhandled policy status value");
  }
}

Policy::PolicyStatus policyStatusFromString(std::string_view str) {
  if (str == "active") return Policy::PolicyStatus::ACTIVE;
  if (str == "expired") return Policy::PolicyStatus::EXPIRED;
  if (str == "cancelled") return Policy::PolicyStatus::CANCELLED;
  if (str == "pending") return Policy::PolicyStatus::PENDING;

  throw std::invalid_argument("unknown policy status: " + std::string(str));
}

std::string policyTypeToString(Policy::PolicyType type) {
  switch (type) {
    case Policy::PolicyType::AUTO:
      return "auto";
    case Policy::PolicyType::LIFE:
      return "life";
    case Policy::PolicyType::HEALTH:
      return "health";
    case Policy::PolicyType::HOME:
      return "home";
    default:
      throw std::invalid_argument("unhandled policy type value");
  }
}

Policy::PolicyType policyTypeFromString(std::string_view str) {
  if (str == "auto") return Policy::PolicyType::AUTO;
  if (str == "life") return Policy::PolicyType::LIFE;
  if (str == "health") return Policy::PolicyType::HEALTH;
  if (str == "home") return Policy::PolicyType::HOME;

  throw std::invalid_argument("unknown policy type: " + std::string(str));
}

}  // namespace insura::domain
