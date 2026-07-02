#include "zeta/time/retry.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("retry_policy: counts attempts and advances backoff", "[time][retry]") {
    zeta::RetryPolicy policy(
        zeta::ExponentialBackoff(zeta::Duration::Milliseconds(5),
                                 zeta::Duration::Milliseconds(20)),
        3);

    REQUIRE(policy.CanRetry());
    REQUIRE(policy.NextDelay() == zeta::Duration::Milliseconds(5));

    policy.Advance();
    REQUIRE(policy.attempts() == 1);
    REQUIRE(policy.NextDelay() == zeta::Duration::Milliseconds(10));

    policy.Advance();
    REQUIRE(policy.attempts() == 2);
    REQUIRE(policy.NextDelay() == zeta::Duration::Milliseconds(20));

    policy.Advance();
    REQUIRE(policy.attempts() == 3);
    REQUIRE(!policy.CanRetry());
    REQUIRE(policy.NextDelay() == zeta::Duration());
}

TEST_CASE("retry_policy: deadline caps delay", "[time][retry]") {
    zeta::RetryPolicy policy(
        zeta::ExponentialBackoff(zeta::Duration::Milliseconds(50),
                                 zeta::Duration::Milliseconds(50)),
        zeta::RetryPolicy::kUnlimitedAttempts,
        zeta::Deadline::After(zeta::Duration::Milliseconds(5)));

    auto delay = policy.NextDelay();
    REQUIRE(delay.ToNanoseconds() >= 0);
    REQUIRE(delay.ToNanoseconds() <= zeta::Duration::Milliseconds(5).ToNanoseconds());
}

TEST_CASE("retry_policy: reset restores the first delay", "[time][retry]") {
    zeta::RetryPolicy policy(
        zeta::ExponentialBackoff(zeta::Duration::Milliseconds(4),
                                 zeta::Duration::Milliseconds(16)),
        4);

    policy.Advance();
    policy.Advance();
    REQUIRE(policy.attempts() == 2);
    REQUIRE(policy.NextDelay() == zeta::Duration::Milliseconds(16));

    policy.Reset();
    REQUIRE(policy.attempts() == 0);
    REQUIRE(policy.NextDelay() == zeta::Duration::Milliseconds(4));
}

