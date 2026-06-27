#include "zeta/time/backoff.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("backoff: doubles until max", "[time][backoff]") {
    zeta::ExponentialBackoff backoff(
        zeta::Duration::Milliseconds(5),
        zeta::Duration::Milliseconds(20));

    REQUIRE(backoff.NextDelay() == zeta::Duration::Milliseconds(5));
    REQUIRE(backoff.NextDelay() == zeta::Duration::Milliseconds(10));
    REQUIRE(backoff.NextDelay() == zeta::Duration::Milliseconds(20));
    REQUIRE(backoff.NextDelay() == zeta::Duration::Milliseconds(20));
    REQUIRE(backoff.AtMax());
}

TEST_CASE("backoff: reset clears attempts and delay", "[time][backoff]") {
    zeta::ExponentialBackoff backoff(
        zeta::Duration::Milliseconds(5),
        zeta::Duration::Milliseconds(20));

    (void)backoff.NextDelay();
    (void)backoff.NextDelay();
    REQUIRE(backoff.attempts() == 2);
    backoff.Reset();
    REQUIRE(backoff.attempts() == 0);
    REQUIRE(backoff.Peek() == zeta::Duration::Milliseconds(5));
}

TEST_CASE("backoff: deadline expires after returned delay", "[time][backoff]") {
    zeta::ExponentialBackoff backoff(
        zeta::Duration::Milliseconds(2),
        zeta::Duration::Milliseconds(10));

    auto deadline = backoff.NextDeadline();
    REQUIRE(!deadline.Expired());
    deadline.Sleep();
    REQUIRE(deadline.Expired());
}

TEST_CASE("backoff: multiplier is clamped to one", "[time][backoff]") {
    zeta::ExponentialBackoff backoff(
        zeta::Duration::Milliseconds(3),
        zeta::Duration::Milliseconds(10),
        0);

    REQUIRE(backoff.NextDelay() == zeta::Duration::Milliseconds(3));
    REQUIRE(backoff.NextDelay() == zeta::Duration::Milliseconds(3));
}
