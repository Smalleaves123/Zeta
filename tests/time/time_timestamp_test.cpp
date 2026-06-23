#include "zeta/time/timestamp.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("timestamp: UTC epoch formatting", "[time][timestamp]") {
    REQUIRE(zeta::FormatTimestampUtc(0) == "1970-01-01T00:00:00.000Z");
}

TEST_CASE("timestamp: UTC formatting preserves milliseconds", "[time][timestamp]") {
    auto t = zeta::Duration::Seconds(1).ToNanoseconds() +
             zeta::Duration::Milliseconds(234).ToNanoseconds();
    REQUIRE(zeta::FormatTimestampUtc(t) == "1970-01-01T00:00:01.234Z");
}

TEST_CASE("timestamp: local formatting has expected shape", "[time][timestamp]") {
    auto s = zeta::FormatNow();
    REQUIRE(s.size() == 23);
    REQUIRE(s[4] == '-');
    REQUIRE(s[7] == '-');
    REQUIRE(s[10] == ' ');
    REQUIRE(s[13] == ':');
    REQUIRE(s[16] == ':');
    REQUIRE(s[19] == '.');
}

TEST_CASE("timestamp: UTC now formatting has expected shape", "[time][timestamp]") {
    auto s = zeta::FormatNowUtc();
    REQUIRE(s.size() == 24);
    REQUIRE(s[4] == '-');
    REQUIRE(s[7] == '-');
    REQUIRE(s[10] == 'T');
    REQUIRE(s[13] == ':');
    REQUIRE(s[16] == ':');
    REQUIRE(s[19] == '.');
    REQUIRE(s[23] == 'Z');
}
