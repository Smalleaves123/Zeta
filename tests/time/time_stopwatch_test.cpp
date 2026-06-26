#include "zeta/time/stopwatch.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("stopwatch: elapsed grows over time", "[time][stopwatch]") {
    zeta::Stopwatch sw;
    zeta::Clock::SleepFor(zeta::Duration::Milliseconds(2));
    REQUIRE(sw.Elapsed().ToNanoseconds() > 0);
}

TEST_CASE("stopwatch: restart returns elapsed and resets origin", "[time][stopwatch]") {
    zeta::Stopwatch sw;
    zeta::Clock::SleepFor(zeta::Duration::Milliseconds(2));
    const auto first = sw.Restart();
    REQUIRE(first.ToNanoseconds() > 0);

    zeta::Clock::SleepFor(zeta::Duration::Milliseconds(2));
    REQUIRE(sw.Elapsed().ToNanoseconds() > 0);
}

TEST_CASE("deadline: after expires once timeout passes", "[time][deadline]") {
    auto deadline = zeta::Deadline::After(zeta::Duration::Milliseconds(2));
    REQUIRE(!deadline.Expired());
    deadline.Sleep();
    REQUIRE(deadline.Expired());
    REQUIRE(deadline.Remaining().ToNanoseconds() == 0);
}

TEST_CASE("deadline: remaining is bounded and positive before expiry", "[time][deadline]") {
    auto deadline = zeta::Deadline::After(zeta::Duration::Milliseconds(20));
    auto remaining = deadline.Remaining();
    REQUIRE(remaining.ToNanoseconds() > 0);
    REQUIRE(remaining.ToNanoseconds() <= zeta::Duration::Milliseconds(20).ToNanoseconds());
}

TEST_CASE("deadline: never stays open", "[time][deadline]") {
    auto deadline = zeta::Deadline::Never();
    REQUIRE(!deadline.Expired());
    REQUIRE(deadline.Remaining().ToRaw() == zeta::Duration::Infinite().ToRaw());
}
