#include "zeta/synchronization/notification.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

TEST_CASE("notification: single waiter", "[sync][notify]") {
    zeta::Notification n;
    REQUIRE(!n.HasBeenNotified());

    std::thread t([&] { n.Notify(); });
    n.WaitForNotification();
    t.join();

    REQUIRE(n.HasBeenNotified());
}

TEST_CASE("notification: already notified returns immediately", "[sync][notify]") {
    zeta::Notification n;
    n.Notify();
    REQUIRE(n.HasBeenNotified());
    // Should return immediately.
    n.WaitForNotification();
}

TEST_CASE("notification: timeout works", "[sync][notify]") {
    zeta::Notification n;
    bool notified = n.WaitForNotificationWithTimeout(std::chrono::milliseconds(10));
    REQUIRE(!notified);  // timeout, not notified
    REQUIRE(!n.HasBeenNotified());
}

TEST_CASE("notification: multiple notify calls are safe", "[sync][notify]") {
    zeta::Notification n;
    n.Notify();
    n.Notify();
    n.Notify();
    REQUIRE(n.HasBeenNotified());
}
