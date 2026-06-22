#include "zeta/strings/str_cat.h"

#include <catch2/catch_test_macros.hpp>
#include <cfloat>
#include <climits>
#include <string>

// ═══════════════════════════════════════════════════════════════════
// StrCat
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrCat: empty call", "[str_cat]") {
    REQUIRE(zeta::StrCat() == "");
}

TEST_CASE("StrCat: single string", "[str_cat]") {
    REQUIRE(zeta::StrCat("hello") == "hello");
}

TEST_CASE("StrCat: two strings", "[str_cat]") {
    REQUIRE(zeta::StrCat("hello", " world") == "hello world");
}

TEST_CASE("StrCat: mixed ints and strings", "[str_cat]") {
    REQUIRE(zeta::StrCat("x=", 10, ", y=", 20) == "x=10, y=20");
}

TEST_CASE("StrCat: negative numbers", "[str_cat]") {
    REQUIRE(zeta::StrCat(-42) == "-42");
    REQUIRE(zeta::StrCat(0) == "0");
}

TEST_CASE("StrCat: large number", "[str_cat]") {
    REQUIRE(zeta::StrCat(123456789012345LL) == "123456789012345");
}

TEST_CASE("StrCat: double values", "[str_cat]") {
    auto result = zeta::StrCat("pi=", 3.14);
    REQUIRE(result.find("pi=3.14") == 0);
}

TEST_CASE("StrCat: boolean", "[str_cat]") {
    REQUIRE(zeta::StrCat(true) == "true");
    REQUIRE(zeta::StrCat(false) == "false");
    REQUIRE(zeta::StrCat("is_ok=", true) == "is_ok=true");
}

TEST_CASE("StrCat: char", "[str_cat]") {
    REQUIRE(zeta::StrCat('A', 'B', 'C') == "ABC");
}

TEST_CASE("StrCat: std::string and string_view", "[str_cat]") {
    std::string      s  = "hello";
    std::string_view sv = " world";
    REQUIRE(zeta::StrCat(s, sv) == "hello world");
}

TEST_CASE("StrCat: ten arguments", "[str_cat]") {
    REQUIRE(zeta::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
            == "12345678910");
}

// ── Edge cases: numeric extremes ────────────────────────────────────
TEST_CASE("StrCat: INT_MIN / LONG_MIN", "[str_cat][edge]") {
    REQUIRE(zeta::StrCat(INT_MIN) == "-2147483648");
    if constexpr (sizeof(long) == 8) {
        REQUIRE(zeta::StrCat(LONG_MIN) == "-9223372036854775808");
    } else {
        REQUIRE(zeta::StrCat(LONG_MIN) == "-2147483648");
    }
}

TEST_CASE("StrCat: unsigned types", "[str_cat][edge]") {
    unsigned int u = 42;
    REQUIRE(zeta::StrCat(u) == "42");
    REQUIRE(zeta::StrCat(UINT64_MAX) == "18446744073709551615");
    REQUIRE(zeta::StrCat(ULLONG_MAX) == "18446744073709551615");
}

TEST_CASE("StrCat: float", "[str_cat][edge]") {
    auto r = zeta::StrCat(3.14f);
    REQUIRE(r.find("3.14") == 0);
}

// ── Edge cases: double special values ──────────────────────────────
TEST_CASE("StrCat: negative double", "[str_cat][edge]") {
    auto r = zeta::StrCat(-2.5);
    REQUIRE(r.find("-2.5") == 0);
}

// ── Edge cases: empty and boundary inputs ──────────────────────────
TEST_CASE("StrCat: empty string_view", "[str_cat][edge]") {
    std::string_view empty;
    REQUIRE(zeta::StrCat("a", empty, "b") == "ab");
}

TEST_CASE("StrCat: const char* explicit", "[str_cat][edge]") {
    const char* s = "hello";
    REQUIRE(zeta::StrCat(s) == "hello");
}

// ── Property-based invariants ──────────────────────────────────────
TEST_CASE("StrCat: monoid identity property", "[str_cat][property]") {
    REQUIRE(zeta::StrCat() == "");
    REQUIRE(zeta::StrCat(zeta::StrCat()) == "");
    REQUIRE(zeta::StrCat("a", zeta::StrCat()) == "a");
}

TEST_CASE("StrCat: concatenation associativity", "[str_cat][property]") {
    // StrCat(a, b) should equal StrCat(a) + StrCat(b)
    std::string ab  = zeta::StrCat("hello", " world");
    std::string a_b = zeta::StrCat("hello") + zeta::StrCat(" world");
    REQUIRE(ab == a_b);
}

// ═══════════════════════════════════════════════════════════════════
// StrAppend
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrAppend: basic", "[str_cat]") {
    std::string out = "hello";
    zeta::StrAppend(out, " ", "world", "!", 42);
    REQUIRE(out == "hello world!42");
}

TEST_CASE("StrAppend: onto empty", "[str_cat]") {
    std::string out;
    zeta::StrAppend(out, "a", "b", "c");
    REQUIRE(out == "abc");
}

TEST_CASE("StrAppend: equivalence property", "[str_cat][property]") {
    // StrAppend(out, a, b) should be equivalent to out += StrCat(a, b)
    std::string s1 = "prefix";
    zeta::StrAppend(s1, "hello", 42);

    std::string s2 = "prefix";
    s2 += zeta::StrCat("hello", 42);

    REQUIRE(s1 == s2);
}

TEST_CASE("StrAppend: empty args no-op", "[str_cat][edge]") {
    std::string out = "hello";
    zeta::StrAppend(out);
    REQUIRE(out == "hello");
}
