#include "zeta/time/periodic_timer.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("periodic_timer: poll becomes true after interval", "[time][periodic]") {
    zeta::PeriodicTimer timer(zeta::Duration::Milliseconds(2));
    REQUIRE(!timer.Poll());
    zeta::Clock::SleepFor(zeta::Duration::Milliseconds(3));
    REQUIRE(timer.Poll());
}

TEST_CASE("periodic_timer: remaining drops to zero after wait", "[time][periodic]") {
    zeta::PeriodicTimer timer(zeta::Duration::Milliseconds(2));
    REQUIRE(timer.Remaining().ToNanoseconds() > 0);
    timer.Wait();
    REQUIRE(timer.Remaining().ToNanoseconds() == 0);
}

TEST_CASE("periodic_timer: wait_next advances next tick", "[time][periodic]") {
    zeta::PeriodicTimer timer(zeta::Duration::Milliseconds(2));
    const auto first = timer.next_time();
    timer.WaitNext();
    REQUIRE(timer.next_time() > first);
}

TEST_CASE("periodic_timer: zero interval is always ready", "[time][periodic]") {
    zeta::PeriodicTimer timer{zeta::Duration()};
    REQUIRE(timer.Ready());
    REQUIRE(timer.Poll());
}
