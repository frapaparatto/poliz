#include "interaction_status.hpp"

#include <stdexcept>
#include <string>

namespace insura::domain {

std::string interactionTypeToString(Interaction::InteractionType type) {
  switch (type) {
    case Interaction::InteractionType::APPOINTMENT:
      return "appointment";
    case Interaction::InteractionType::CONTRACT:
      return "contract";
    default:
      throw std::invalid_argument("unhandled interaction type value");
  }
}

Interaction::InteractionType interactionTypeFromString(std::string_view str) {
  if (str == "appointment") return Interaction::InteractionType::APPOINTMENT;
  if (str == "contract") return Interaction::InteractionType::CONTRACT;

  throw std::invalid_argument("unknown interaction type: " + std::string(str));
}

std::string contractStatusToString(Contract::ContractStatus status) {
  switch (status) {
    case Contract::ContractStatus::DRAFT:
      return "draft";
    case Contract::ContractStatus::SIGNED:
      return "signed";
    case Contract::ContractStatus::ACTIVE:
      return "active";
    case Contract::ContractStatus::TERMINATED:
      return "terminated";
    default:
      throw std::invalid_argument("unhandled contract status value");
  }
}

Contract::ContractStatus contractStatusFromString(std::string_view str) {
  if (str == "draft") return Contract::ContractStatus::DRAFT;
  if (str == "signed") return Contract::ContractStatus::SIGNED;
  if (str == "active") return Contract::ContractStatus::ACTIVE;
  if (str == "terminated") return Contract::ContractStatus::TERMINATED;

  throw std::invalid_argument("unknown contract status: " + std::string(str));
}

}  // namespace insura::domain
