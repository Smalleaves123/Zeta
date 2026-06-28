#include "zeta/strings/numbers.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("numbers: FastIntToBuffer", "[strings][numbers]") {
    char buf[32];
    REQUIRE(zeta::FastIntToBuffer(0, buf) == "0");
    REQUIRE(zeta::FastIntToBuffer(42, buf) == "42");
    REQUIRE(zeta::FastIntToBuffer(-1, buf) == "-1");
    REQUIRE(zeta::FastIntToBuffer(1234567890LL, buf) == "1234567890");
}

TEST_CASE("numbers: SimpleAtoi int", "[strings][numbers]") {
    int v = 0;
    REQUIRE(zeta::SimpleAtoi("42", &v));
    REQUIRE(v == 42);

    REQUIRE(zeta::SimpleAtoi("-100", &v));
    REQUIRE(v == -100);

    REQUIRE(zeta::SimpleAtoi("0", &v));
    REQUIRE(v == 0);
}

TEST_CASE("numbers: SimpleAtoi rejects bad input", "[strings][numbers]") {
    int v = 0;
    REQUIRE(!zeta::SimpleAtoi("", &v));
    REQUIRE(!zeta::SimpleAtoi("abc", &v));
    REQUIRE(!zeta::SimpleAtoi("42abc", &v));     // trailing chars
    REQUIRE(!zeta::SimpleAtoi("  42", &v));      // leading space
}

TEST_CASE("numbers: SimpleAtoi unsigned", "[strings][numbers]") {
    unsigned v = 0;
    REQUIRE(zeta::SimpleAtoi("42", &v));
    REQUIRE(v == 42u);
    REQUIRE(!zeta::SimpleAtoi("-1", &v));        // negative doesn't parse as unsigned
}

TEST_CASE("numbers: SimpleAtoi double", "[strings][numbers]") {
    double v = 0.0;
    REQUIRE(zeta::SimpleAtoi("3.14", &v));
    REQUIRE(v == 3.14);
    REQUIRE(zeta::SimpleAtoi("1e5", &v));
    REQUIRE(v == 100000.0);
    REQUIRE(!zeta::SimpleAtoi("1e400", &v));
    REQUIRE(!zeta::SimpleAtoi("inf", &v));
}
