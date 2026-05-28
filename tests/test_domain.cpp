#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "../src/domain/client.hpp"
#include "../src/domain/interaction.hpp"
#include "../src/domain/policy.hpp"

TEST_CASE("Client constructor rejects invalid input", "[domain][client]") {
  using S = insura::domain::Client::ClientStatus;

  SECTION("empty first name throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Client("", "Rossi", "mario@rossi.it", std::nullopt,
                               std::nullopt, std::nullopt, std::nullopt,
                               std::nullopt, std::nullopt, S::NEW,
                               std::nullopt),
        std::invalid_argument);
  }

  SECTION("empty last name throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Client("Mario", "", "mario@rossi.it", std::nullopt,
                               std::nullopt, std::nullopt, std::nullopt,
                               std::nullopt, std::nullopt, S::NEW,
                               std::nullopt),
        std::invalid_argument);
  }

  SECTION("empty email throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Client("Mario", "Rossi", "", std::nullopt, std::nullopt,
                               std::nullopt, std::nullopt, std::nullopt,
                               std::nullopt, S::NEW, std::nullopt),
        std::invalid_argument);
  }

  SECTION("malformed email throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Client("Mario", "Rossi", "not-an-email", std::nullopt,
                               std::nullopt, std::nullopt, std::nullopt,
                               std::nullopt, std::nullopt, S::NEW,
                               std::nullopt),
        std::invalid_argument);
  }
}

TEST_CASE("Client constructor succeeds with valid input", "[domain][client]") {
  using S = insura::domain::Client::ClientStatus;

  REQUIRE_NOTHROW(
      insura::domain::Client("Mario", "Rossi", "mario@rossi.it", std::nullopt,
                             std::nullopt, std::nullopt, std::nullopt,
                             std::nullopt, std::nullopt, S::NEW, std::nullopt));
}

TEST_CASE("Policy constructor rejects invalid input", "[domain][policy]") {
  using T = insura::domain::Policy::PolicyType;
  using S = insura::domain::Policy::PolicyStatus;
  const std::string uuid = "client-uuid";

  SECTION("empty start date throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Policy(uuid, T::AUTO, "", std::nullopt, 350.00,
                               S::PENDING, std::nullopt),
        std::invalid_argument);
  }

  SECTION("wrongly formatted start date throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Policy(uuid, T::AUTO, "01-01-2026", std::nullopt,
                               350.00, S::PENDING, std::nullopt),
        std::invalid_argument);
  }

  SECTION("amount of zero throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Policy(uuid, T::AUTO, "2026-01-01", std::nullopt, 0.0,
                               S::PENDING, std::nullopt),
        std::invalid_argument);
  }

  SECTION("negative amount throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Policy(uuid, T::AUTO, "2026-01-01", std::nullopt, -1.0,
                               S::PENDING, std::nullopt),
        std::invalid_argument);
  }
}

TEST_CASE("Policy constructor succeeds with valid input", "[domain][policy]") {
  using T = insura::domain::Policy::PolicyType;
  using S = insura::domain::Policy::PolicyStatus;

  REQUIRE_NOTHROW(insura::domain::Policy("client-uuid", T::AUTO, "2026-01-01",
                                         std::nullopt, 350.00, S::PENDING,
                                         std::nullopt));
}


TEST_CASE("Appointment constructor rejects invalid input",
          "[domain][interaction]") {
  const std::string uuid = "client-uuid";

  SECTION("empty time throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Appointment(uuid, "2026-01-01", "", 60, std::nullopt,
                                    std::nullopt),
        std::invalid_argument);
  }

  SECTION("zero duration throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Appointment(uuid, "2026-01-01", "10:00", 0, std::nullopt,
                                    std::nullopt),
        std::invalid_argument);
  }

  SECTION("negative duration throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Appointment(uuid, "2026-01-01", "10:00", -30,
                                    std::nullopt, std::nullopt),
        std::invalid_argument);
  }
}

TEST_CASE("Appointment constructor succeeds with valid input",
          "[domain][interaction]") {
  REQUIRE_NOTHROW(insura::domain::Appointment("client-uuid", "2026-01-01",
                                               "10:00", 60, std::nullopt,
                                               std::nullopt));
}

TEST_CASE("Contract constructor rejects invalid input",
          "[domain][interaction]") {
  using CS = insura::domain::Contract::ContractStatus;
  const std::string uuid = "client-uuid";

  SECTION("empty signed date throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Contract(uuid, "2026-01-01", 1000.00, "Home Plan A",
                                 "", std::nullopt, CS::DRAFT, std::nullopt),
        std::invalid_argument);
  }

  SECTION("wrongly formatted signed date throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Contract(uuid, "2026-01-01", 1000.00, "Home Plan A",
                                 "01-01-2026", std::nullopt, CS::DRAFT,
                                 std::nullopt),
        std::invalid_argument);
  }

  SECTION("empty product name throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Contract(uuid, "2026-01-01", 1000.00, "",
                                 "2026-01-01", std::nullopt, CS::DRAFT,
                                 std::nullopt),
        std::invalid_argument);
  }

  SECTION("value of zero throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Contract(uuid, "2026-01-01", 0.0, "Home Plan A",
                                 "2026-01-01", std::nullopt, CS::DRAFT,
                                 std::nullopt),
        std::invalid_argument);
  }

  SECTION("negative value throws") {
    REQUIRE_THROWS_AS(
        insura::domain::Contract(uuid, "2026-01-01", -500.0, "Home Plan A",
                                 "2026-01-01", std::nullopt, CS::DRAFT,
                                 std::nullopt),
        std::invalid_argument);
  }
}

TEST_CASE("Contract constructor succeeds with valid input",
          "[domain][interaction]") {
  using CS = insura::domain::Contract::ContractStatus;

  REQUIRE_NOTHROW(
      insura::domain::Contract("client-uuid", "2026-01-01", 1000.00,
                               "Home Plan A", "2026-01-01", std::nullopt,
                               CS::DRAFT, std::nullopt));
}
