#include "zeta/time/timestamp.h"
#include "zeta/time/civil_time.h"

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

TEST_CASE("civil date: validates leap days and performs calendar arithmetic",
          "[time][civil]") {
    const zeta::CivilDate leap_day(2024, 2, 29);
    REQUIRE(leap_day.IsValid());
    REQUIRE(!zeta::CivilDate(2023, 2, 29).IsValid());
    REQUIRE(leap_day.AddDays(1) == zeta::CivilDate(2024, 3, 1));
    REQUIRE(leap_day.DayOfWeek() == 4); // Thursday.
    REQUIRE(zeta::FormatCivilDate(leap_day) == "2024-02-29");
}

TEST_CASE("civil date: parses ISO calendar dates", "[time][civil][parse]") {
    const auto parsed = zeta::ParseCivilDate("2000-02-29");
    REQUIRE(parsed.has_value());
    REQUIRE(*parsed == zeta::CivilDate(2000, 2, 29));
    REQUIRE(!zeta::ParseCivilDate("2001-02-29").has_value());
    REQUIRE(!zeta::ParseCivilDate("2000/02/29").has_value());
}

TEST_CASE("civil time: parses, formats, and round-trips UTC", "[time][civil]") {
    const auto parsed = zeta::ParseCivilTime("1970-01-01T00:00:01.234");
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->ToString() == "1970-01-01T00:00:01.234000000");

    const auto nanos = parsed->ToUnixNanos(zeta::TimeZoneSpec::Utc());
    REQUIRE(nanos.has_value());
    REQUIRE(*nanos == 1234000000LL);
    REQUIRE(zeta::CivilTime::FromUnixNanos(*nanos, zeta::TimeZoneSpec::Utc()) ==
            parsed);
}

TEST_CASE("civil time: fixed offsets convert deterministically", "[time][timezone]") {
    const zeta::CivilTime local(zeta::CivilDate(1970, 1, 1), 8, 0, 0);
    const auto utc_nanos = local.ToUnixNanos(zeta::TimeZoneSpec::FixedOffset(480));
    REQUIRE(utc_nanos.has_value());
    REQUIRE(*utc_nanos == 0);

    const auto converted = zeta::CivilTime::FromUnixNanos(
        0, zeta::TimeZoneSpec::FixedOffset(480));
    REQUIRE(converted.has_value());
    REQUIRE(*converted == local);
}

TEST_CASE("civil time: negative Unix timestamps use floor day arithmetic",
          "[time][timezone]") {
    const auto converted = zeta::CivilTime::FromUnixNanos(
        -1, zeta::TimeZoneSpec::Utc());
    REQUIRE(converted.has_value());
    REQUIRE(converted->ToString() == "1969-12-31T23:59:59.999999999");
}
