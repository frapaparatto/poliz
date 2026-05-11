#include <catch2/catch_test_macros.hpp>

#include "../src/domain/strops.hpp"

TEST_CASE("trim", "[domain][strops]") {
    SECTION("removes leading whitespace") {
        REQUIRE(insura::domain::strops::trim("   Elon") == "Elon");
    }
    SECTION("removes trailing whitespace") {
        REQUIRE(insura::domain::strops::trim("Axel ") == "Axel");
    }
    SECTION("removes both leading and trailing whitespace") {
        REQUIRE(insura::domain::strops::trim("  hello  ") == "hello");
    }
    SECTION("preserves spaces in the middle") {
        REQUIRE(insura::domain::strops::trim("  hello world  ") == "hello world");
    }
    SECTION("returns empty string when input is all spaces") {
        REQUIRE(insura::domain::strops::trim("     ") == "");
    }
    SECTION("returns empty string when input is empty") {
        REQUIRE(insura::domain::strops::trim("") == "");
    }
    SECTION("does not modify string with no whitespace") {
        REQUIRE(insura::domain::strops::trim("hello") == "hello");
    }
}

TEST_CASE("lower", "[domain][strops]") {
    SECTION("converts all uppercase to lowercase") {
        REQUIRE(insura::domain::strops::lower("AAAA") == "aaaa");
    }
    SECTION("converts mixed case to lowercase") {
        REQUIRE(insura::domain::strops::lower("AbAb") == "abab");
    }
    SECTION("does not modify already lowercase string") {
        REQUIRE(insura::domain::strops::lower("hello") == "hello");
    }
    SECTION("returns empty string when input is empty") {
        REQUIRE(insura::domain::strops::lower("") == "");
    }
    SECTION("preserves digits and special characters") {
        REQUIRE(insura::domain::strops::lower("Hello123!") == "hello123!");
    }
}

TEST_CASE("capitalize", "[domain][strops]") {
    SECTION("capitalizes first letter of a single word") {
        REQUIRE(insura::domain::strops::capitalize("hello") == "Hello");
    }
    SECTION("capitalizes first letter of each word") {
        REQUIRE(insura::domain::strops::capitalize("hello world") == "Hello World");
    }
    SECTION("preserves already correct capitalization") {
        REQUIRE(insura::domain::strops::capitalize("Hello") == "Hello");
    }
    SECTION("handles single character") {
        REQUIRE(insura::domain::strops::capitalize("a") == "A");
    }
    SECTION("returns empty string when input is empty") {
        REQUIRE(insura::domain::strops::capitalize("") == "");
    }
    SECTION("handles string starting with space") {
        REQUIRE(insura::domain::strops::capitalize(" hello") == " Hello");
    }
    SECTION("handles consecutive spaces between words") {
        REQUIRE(insura::domain::strops::capitalize("hello  world") == "Hello  World");
    }
}

TEST_CASE("contains", "[domain][strops]") {
    SECTION("finds substring at the beginning") {
        REQUIRE(insura::domain::strops::contains("hello world", "hel"));
    }
    SECTION("finds substring in the middle") {
        REQUIRE(insura::domain::strops::contains("hello world", "lo wo"));
    }
    SECTION("finds substring at the end") {
        REQUIRE(insura::domain::strops::contains("hello world", "orld"));
    }
    SECTION("returns false when substring not found") {
        REQUIRE_FALSE(insura::domain::strops::contains("world", "el"));
    }
    SECTION("case insensitive match") {
        REQUIRE(insura::domain::strops::contains("Software Engineer", "softw"));
    }
    SECTION("exact match") {
        REQUIRE(insura::domain::strops::contains("hello", "hello"));
    }
    SECTION("returns false when needle is longer than haystack") {
        REQUIRE_FALSE(insura::domain::strops::contains("hi", "hello"));
    }
    SECTION("returns true when needle is empty") {
        REQUIRE(insura::domain::strops::contains("hello", ""));
    }
    SECTION("returns false when haystack is empty") {
        REQUIRE_FALSE(insura::domain::strops::contains("", "hello"));
    }
}
