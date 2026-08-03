#include "zeta/synchronization/bounded_queue.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>

TEST_CASE("BoundedQueue: rejects pushes when full", "[sync][queue]") {
    zeta::BoundedQueue<int> queue(2);

    REQUIRE(queue.TryPush(1));
    REQUIRE(queue.TryPush(2));
    REQUIRE(queue.Full());
    REQUIRE_FALSE(queue.TryPush(3));
    REQUIRE(queue.TryPop().value() == 1);
    REQUIRE(queue.TryPush(3));
    REQUIRE(queue.TryPop().value() == 2);
    REQUIRE(queue.TryPop().value() == 3);
    REQUIRE(queue.Empty());
}

TEST_CASE("BoundedQueue: supports move-only values and clearing",
          "[sync][queue]") {
    zeta::BoundedQueue<std::unique_ptr<int>> queue(2);
    REQUIRE(queue.TryPush(std::make_unique<int>(42)));
    REQUIRE(queue.Size() == 1);
    queue.Clear();
    REQUIRE(queue.Empty());
    REQUIRE_FALSE(queue.TryPop().has_value());
}
