#include "zeta/time/clock.h"

#include <catch2/catch_test_macros.hpp>

#include <thread>

using namespace zeta;

// ═══════════════════════════════════════════════════════════════════════
// Clock (monotonic)
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("clock: Now returns increasing values", "[clock][monotonic]") {
    auto t1 = zeta::Clock::Now();
    auto t2 = zeta::Clock::Now();
    REQUIRE(t2 >= t1);
}

TEST_CASE("clock: Between computes positive duration", "[clock][monotonic]") {
    auto t1 = zeta::Clock::Now();
    // Busy-wait a tiny bit.
    volatile int x = 0;
    for (int i = 0; i < 100000; ++i) x += i;
    (void)x;
    auto t2 = zeta::Clock::Now();

    Duration d = zeta::Clock::Between(t1, t2);
    REQUIRE(d.ToNanoseconds() >= 0);
}

TEST_CASE("clock: ToDuration is reversible", "[clock][monotonic]") {
    auto now = zeta::Clock::Now();
    Duration d = zeta::Clock::ToDuration(now);
    REQUIRE(d.ToRaw() == now);
}

TEST_CASE("clock: sleep between gives measurable duration", "[clock][monotonic]") {
    auto t1 = zeta::Clock::Now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto t2 = zeta::Clock::Now();

    Duration d = zeta::Clock::Between(t1, t2);
    // At least 5 ms (generous margin for OS timer resolution).
    REQUIRE(d.ToMilliseconds() >= 1);
}

// ═══════════════════════════════════════════════════════════════════════
// RealClock (wall-clock)
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("real_clock: Now is reasonable", "[clock][wall]") {
    auto t = zeta::RealClock::Now();
    // Should be > year 2020 (roughly 50 years * nanoseconds per year)
    int64_t ns_2020 = 50LL * 365 * 24 * 3600 * 1'000'000'000LL;
    REQUIRE(t > ns_2020);
}

TEST_CASE("real_clock: ToDuration round-trip", "[clock][wall]") {
    auto now = zeta::RealClock::Now();
    Duration d = zeta::RealClock::ToDuration(now);
    REQUIRE(d.ToRaw() == now);
}
