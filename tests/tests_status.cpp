#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "../src/domain/client_status.hpp"
#include "../src/domain/policy_status.hpp"

using ClientStatus = insura::domain::Client::ClientStatus;
using PolicyStatus = insura::domain::Policy::PolicyStatus;
using PolicyType = insura::domain::Policy::PolicyType;

TEST_CASE("statusToString", "[domain][client_status]") {
  auto [status, expected] = GENERATE(table<ClientStatus, std::string_view>({
      {ClientStatus::NEW, "new"},
      {ClientStatus::CONTACTED, "contacted"},
      {ClientStatus::IN_PROGRESS, "in_progress"},
      {ClientStatus::OPEN_DEAL, "open_deal"},
      {ClientStatus::ATTEMPTED_TO_CONTACT, "attempted_to_contact"},
      {ClientStatus::CLOSED_LOST, "closed_lost"},
      {ClientStatus::CLOSED_WON, "closed_won"},
  }));
  REQUIRE(insura::domain::statusToString(status) == expected);
}

TEST_CASE("statusFromString", "[domain][client_status]") {
  auto [input, expected] = GENERATE(table<std::string_view, ClientStatus>({
      {"new", ClientStatus::NEW},
      {"contacted", ClientStatus::CONTACTED},
      {"in_progress", ClientStatus::IN_PROGRESS},
      {"open_deal", ClientStatus::OPEN_DEAL},
      {"attempted_to_contact", ClientStatus::ATTEMPTED_TO_CONTACT},
      {"closed_lost", ClientStatus::CLOSED_LOST},
      {"closed_won", ClientStatus::CLOSED_WON},
  }));
  REQUIRE(insura::domain::statusFromString(input) == expected);
}

TEST_CASE("statusFromString throws on unknown input",
          "[domain][client_status]") {
  REQUIRE_THROWS_AS(insura::domain::statusFromString("unknown"),
                    std::invalid_argument);
}

TEST_CASE("policyStatusToString", "[domain][policy_status]") {
  auto [status, expected] = GENERATE(table<PolicyStatus, std::string_view>({
      {PolicyStatus::ACTIVE, "active"},
      {PolicyStatus::EXPIRED, "expired"},
      {PolicyStatus::CANCELLED, "cancelled"},
      {PolicyStatus::PENDING, "pending"},
  }));
  REQUIRE(insura::domain::policyStatusToString(status) == expected);
}

TEST_CASE("policyStatusFromString", "[domain][policy_status]") {
  auto [input, expected] = GENERATE(table<std::string_view, PolicyStatus>({
      {"active", PolicyStatus::ACTIVE},
      {"expired", PolicyStatus::EXPIRED},
      {"cancelled", PolicyStatus::CANCELLED},
      {"pending", PolicyStatus::PENDING},
  }));
  REQUIRE(insura::domain::policyStatusFromString(input) == expected);
}

TEST_CASE("policyStatusFromString throws on unknown input",
          "[domain][policy_status]") {
  REQUIRE_THROWS_AS(insura::domain::policyStatusFromString("unknown"),
                    std::invalid_argument);
}

TEST_CASE("policyTypeToString", "[domain][policy_status]") {
  auto [type, expected] = GENERATE(table<PolicyType, std::string_view>({
      {PolicyType::AUTO, "auto"},
      {PolicyType::LIFE, "life"},
      {PolicyType::HOME, "home"},
      {PolicyType::HEALTH, "health"},
  }));
  REQUIRE(insura::domain::policyTypeToString(type) == expected);
}

TEST_CASE("policyTypeFromString", "[domain][policy_status]") {
  auto [input, expected] = GENERATE(table<std::string_view, PolicyType>({
      {"auto", PolicyType::AUTO},
      {"life", PolicyType::LIFE},
      {"home", PolicyType::HOME},
      {"health", PolicyType::HEALTH},
  }));
  REQUIRE(insura::domain::policyTypeFromString(input) == expected);
}

TEST_CASE("policyTypeFromString throws on unknown input",
          "[domain][policy_status]") {
  REQUIRE_THROWS_AS(insura::domain::policyTypeFromString("unknown"),
                    std::invalid_argument);
}
