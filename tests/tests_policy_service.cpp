#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <stdexcept>

#include "../src/data/csv_policy_repository.hpp"
#include "../src/domain/policy.hpp"
#include "../src/service/policy_service.hpp"

using Type = insura::domain::Policy::PolicyType;

TEST_CASE("PolicyService::calculateAmount returns correct amount for all type/duration combinations", "[service][policy]") {
  insura::data::CsvPolicyRepository repo("dummy_policy.csv");
  insura::service::PolicyService service(repo);

  auto [type, duration, expected] =
      GENERATE(table<Type, int, double>({
          {Type::AUTO,   6,    350.00},
          {Type::AUTO,   12,   650.00},
          {Type::AUTO,   24,  1200.00},
          {Type::AUTO,   36,  1700.00},
          {Type::LIFE,   6,    150.00},
          {Type::LIFE,   12,   280.00},
          {Type::LIFE,   24,   520.00},
          {Type::LIFE,   36,   740.00},
          {Type::HOME,   6,    120.00},
          {Type::HOME,   12,   220.00},
          {Type::HOME,   24,   400.00},
          {Type::HOME,   36,   570.00},
          {Type::HEALTH, 6,    200.00},
          {Type::HEALTH, 12,   380.00},
          {Type::HEALTH, 24,   700.00},
          {Type::HEALTH, 36,  1000.00},
      }));

  REQUIRE(service.calculateAmount(type, duration) == Catch::Approx(expected));
}

TEST_CASE("PolicyService::calculateAmount throws on invalid duration", "[service][policy]") {
  insura::data::CsvPolicyRepository repo("dummy_policy.csv");
  insura::service::PolicyService service(repo);

  REQUIRE_THROWS_AS(service.calculateAmount(Type::AUTO, 13),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(service.calculateAmount(Type::HOME, 0),
                    std::invalid_argument);
}
