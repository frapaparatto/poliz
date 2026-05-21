#pragma once
#include <string>
#include <string_view>

#include "interaction.hpp"

namespace insura::domain {

std::string interactionTypeToString(Interaction::InteractionType type);
Interaction::InteractionType interactionTypeFromString(std::string_view str);

std::string contractStatusToString(Contract::ContractStatus status);
Contract::ContractStatus contractStatusFromString(std::string_view str);

}  // namespace insura::domain
