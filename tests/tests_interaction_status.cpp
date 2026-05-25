#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "../src/domain/interaction_status.hpp"

using InteractionType = insura::domain::Interaction::InteractionType;
using ContractStatus = insura::domain::Contract::ContractStatus;

TEST_CASE("interactionTypeToString", "[domain][interaction_status]") {
  auto [type, expected] = GENERATE(table<InteractionType, std::string_view>({
      {InteractionType::APPOINTMENT, "appointment"},
      {InteractionType::CONTRACT, "contract"},
  }));
  REQUIRE(insura::domain::interactionTypeToString(type) == expected);
}

TEST_CASE("interactionTypeFromString", "[domain][interaction_status]") {
  auto [input, expected] = GENERATE(table<std::string_view, InteractionType>({
      {"appointment", InteractionType::APPOINTMENT},
      {"contract", InteractionType::CONTRACT},
  }));
  REQUIRE(insura::domain::interactionTypeFromString(input) == expected);
}

TEST_CASE("interactionTypeFromString throws on unknown input",
          "[domain][interaction_status]") {
  REQUIRE_THROWS_AS(insura::domain::interactionTypeFromString("unknown"),
                    std::invalid_argument);
}

TEST_CASE("contractStatusToString", "[domain][interaction_status]") {
  auto [status, expected] = GENERATE(table<ContractStatus, std::string_view>({
      {ContractStatus::DRAFT, "draft"},
      {ContractStatus::SIGNED, "signed"},
      {ContractStatus::ACTIVE, "active"},
      {ContractStatus::TERMINATED, "terminated"},
  }));
  REQUIRE(insura::domain::contractStatusToString(status) == expected);
}

TEST_CASE("contractStatusFromString", "[domain][interaction_status]") {
  auto [input, expected] = GENERATE(table<std::string_view, ContractStatus>({
      {"draft", ContractStatus::DRAFT},
      {"signed", ContractStatus::SIGNED},
      {"active", ContractStatus::ACTIVE},
      {"terminated", ContractStatus::TERMINATED},
  }));
  REQUIRE(insura::domain::contractStatusFromString(input) == expected);
}

TEST_CASE("contractStatusFromString throws on unknown input",
          "[domain][interaction_status]") {
  REQUIRE_THROWS_AS(insura::domain::contractStatusFromString("unknown"),
                    std::invalid_argument);
}
