
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "../src/domain/utils.hpp"

TEST_CASE("stringToOptional", "[domain][utils]") {
  SECTION("returns nullopt for empty string") {
    REQUIRE(insura::utils::stringToOptional("") == std::nullopt);
  }
  SECTION("wraps non-empty string") {
    auto result = insura::utils::stringToOptional("hello");
    REQUIRE(result.has_value());
    REQUIRE(*result == "hello");
  }
}

TEST_CASE("generateUuid", "[domain][utils]") {
  auto uuid = insura::utils::generateUuid();

  SECTION("conforms to UUID v4 format") {
    REQUIRE_THAT(uuid, Catch::Matchers::Matches(
        "[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}"
    ));
  }
}

TEST_CASE("currentTimestamp", "[domain][utils]") {
  auto ts = insura::utils::currentTimestamp();

  SECTION("conforms to YYYY-MM-DD HH:MM:SS format") {
    REQUIRE_THAT(ts, Catch::Matchers::Matches(
        "[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}"
    ));
  }
}

TEST_CASE("isValidEmail", "[domain][utils]") {
  SECTION("missing @") {
    CHECK_FALSE(insura::utils::isValidEmail("bjarnestroustrup"));
    CHECK_FALSE(insura::utils::isValidEmail("bjarnestroustrup.com"));
  }
  SECTION("empty string") { REQUIRE_FALSE(insura::utils::isValidEmail("")); }
  SECTION("missing local part") {
    REQUIRE_FALSE(insura::utils::isValidEmail("@gmail.com"));
  }
  SECTION("missing domain extension") {
    REQUIRE_FALSE(insura::utils::isValidEmail("user@domain"));
  }
  SECTION("standard address") {
    REQUIRE(insura::utils::isValidEmail("bstroustrup@cpp.com"));
  }
  SECTION("address with plus sign in local part") {
    REQUIRE(insura::utils::isValidEmail("user+tag@example.com"));
  }
  SECTION("address with subdomain") {
    REQUIRE(insura::utils::isValidEmail("user@mail.example.com"));
  }
}

TEST_CASE("isDigitsOnly", "[domain][utils]") {
  SECTION("empty string returns false") {
    REQUIRE_FALSE(insura::utils::isDigitsOnly(""));
  }
  SECTION("all digits returns true") {
    REQUIRE(insura::utils::isDigitsOnly("1234567890"));
  }
  SECTION("letters mixed with digits returns false") {
    REQUIRE_FALSE(insura::utils::isDigitsOnly("123abc"));
  }
  SECTION("spaces return false") {
    REQUIRE_FALSE(insura::utils::isDigitsOnly("123 456"));
  }
  SECTION("leading minus sign returns false") {
    REQUIRE_FALSE(insura::utils::isDigitsOnly("-123"));
  }
}

TEST_CASE("isValidPhone", "[domain][utils]") {
  SECTION("empty string is invalid") {
    REQUIRE_FALSE(insura::utils::isValidPhone(""));
  }
  SECTION("digits only is valid") {
    REQUIRE(insura::utils::isValidPhone("3401234567"));
  }

  /* About the +39 case, I think I should evaluate if
   * make that invalid or allow user to add +prefix number
   * and also number as valid and then standardize in
   * view or saving it in the standardized way*/
  SECTION("non-digit characters make it invalid") {
    CHECK_FALSE(insura::utils::isValidPhone("+39 3401234567"));
    CHECK_FALSE(insura::utils::isValidPhone("340-123-4567"));
  }
}

TEST_CASE("isValidCsvFile", "[domain][utils]") {
  SECTION(".csv extension is valid") {
    REQUIRE(insura::utils::isValidCsvFile("clients.csv"));
  }
  SECTION(".txt extension is invalid") {
    REQUIRE_FALSE(insura::utils::isValidCsvFile("clients.txt"));
  }
  SECTION("no extension is invalid") {
    REQUIRE_FALSE(insura::utils::isValidCsvFile("clients"));
  }
  SECTION(".CSV uppercase is invalid") {
    REQUIRE_FALSE(insura::utils::isValidCsvFile("clients.CSV"));
  }
}

TEST_CASE("isLeapYear", "[domain][utils][date]") {
  SECTION("divisible by 4 but not 100 is a leap year") {
    CHECK(insura::utils::date::isLeapYear(2024));
    CHECK(insura::utils::date::isLeapYear(2028));
  }
  SECTION("divisible by 400 is a leap year") {
    REQUIRE(insura::utils::date::isLeapYear(2000));
  }
  SECTION("divisible by 100 but not 400 is not a leap year") {
    CHECK_FALSE(insura::utils::date::isLeapYear(2100));
    CHECK_FALSE(insura::utils::date::isLeapYear(1900));
  }
  SECTION("not divisible by 4 is not a leap year") {
    CHECK_FALSE(insura::utils::date::isLeapYear(2026));
    CHECK_FALSE(insura::utils::date::isLeapYear(2027));
  }
}

TEST_CASE("isValidDate", "[domain][utils][date]") {
  SECTION("year before 2026") {
    CHECK_FALSE(insura::utils::date::isValidDate("2025-12-31"));
    CHECK_FALSE(insura::utils::date::isValidDate("1999-01-01"));
  }
  SECTION("invalid month") {
    CHECK_FALSE(insura::utils::date::isValidDate("2026-00-01"));
    CHECK_FALSE(insura::utils::date::isValidDate("2026-13-01"));
  }
  SECTION("invalid day") {
    CHECK_FALSE(insura::utils::date::isValidDate("2026-01-00"));
    CHECK_FALSE(insura::utils::date::isValidDate("2026-01-32"));
  }
  SECTION("day exceeds month length") {
    CHECK_FALSE(insura::utils::date::isValidDate("2026-04-31"));
    CHECK_FALSE(insura::utils::date::isValidDate("2026-11-31"));
  }
  SECTION("February 29 on a non-leap year") {
    REQUIRE_FALSE(insura::utils::date::isValidDate("2026-02-29"));
  }
  SECTION("first valid year boundary") {
    REQUIRE(insura::utils::date::isValidDate("2026-01-01"));
  }
  SECTION("last day of a 31-day month") {
    REQUIRE(insura::utils::date::isValidDate("2026-01-31"));
  }
  SECTION("last day of a 30-day month") {
    REQUIRE(insura::utils::date::isValidDate("2026-04-30"));
  }
  SECTION("February 28 on a non-leap year") {
    REQUIRE(insura::utils::date::isValidDate("2026-02-28"));
  }
  SECTION("February 29 on a leap year") {
    REQUIRE(insura::utils::date::isValidDate("2028-02-29"));
  }
  SECTION("no separators") {
    REQUIRE_FALSE(insura::utils::date::isValidDate("20260101"));
  }
  SECTION("slash separators") {
    REQUIRE_FALSE(insura::utils::date::isValidDate("2026/01/01"));
  }
  SECTION("empty string") {
    REQUIRE_FALSE(insura::utils::date::isValidDate(""));
  }
}

TEST_CASE("isDateAfter", "[domain][utils][date]") {
  SECTION("end date after start date returns true") {
    CHECK(insura::utils::date::isDateAfter("2026-01-01", "2026-01-02"));
    CHECK(insura::utils::date::isDateAfter("2026-01-01", "2027-01-01"));
  }
  SECTION("end date before start date returns false") {
    REQUIRE_FALSE(insura::utils::date::isDateAfter("2026-06-01", "2026-01-01"));
  }
  SECTION("equal dates return false") {
    REQUIRE_FALSE(insura::utils::date::isDateAfter("2026-01-01", "2026-01-01"));
  }
}

TEST_CASE("calculateEndDate", "[domain][utils][date]") {
  SECTION("advance by one month within same year") {
    REQUIRE(insura::utils::date::calculateEndDate("2026-01-01", 1) ==
            "2026-02-01");
  }
  SECTION("advance by 12 months crosses year boundary") {
    REQUIRE(insura::utils::date::calculateEndDate("2026-01-01", 12) ==
            "2027-01-01");
  }
  SECTION("advance from December rolls over to next year") {
    REQUIRE(insura::utils::date::calculateEndDate("2026-12-01", 1) ==
            "2027-01-01");
  }
  SECTION("day is clamped to last day of February on a non-leap year") {
    REQUIRE(insura::utils::date::calculateEndDate("2026-01-31", 1) ==
            "2026-02-28");
  }
  SECTION("day is clamped to last day of a 30-day month") {
    REQUIRE(insura::utils::date::calculateEndDate("2026-03-31", 1) ==
            "2026-04-30");
  }
}
