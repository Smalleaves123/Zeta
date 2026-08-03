#include "zeta/synchronization/lock_free_queue.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>

TEST_CASE("LockFreeQueue: provides bounded non-blocking operations",
          "[sync][lock_free_queue]") {
    zeta::LockFreeQueue<int> minimum_queue(1);
    REQUIRE(minimum_queue.Capacity() == 2);
    REQUIRE(minimum_queue.TryPush(1));
    REQUIRE(minimum_queue.TryPush(2));
    REQUIRE_FALSE(minimum_queue.TryPush(3));

    zeta::LockFreeQueue<int> queue(4);

    REQUIRE(queue.Capacity() == 4);
    REQUIRE(queue.TryPush(1));
    REQUIRE(queue.TryPush(2));
    REQUIRE(queue.TryPop().value() == 1);
    REQUIRE(queue.TryPop().value() == 2);
    REQUIRE_FALSE(queue.TryPop().has_value());

    REQUIRE(queue.TryPush(3));
    REQUIRE(queue.TryPush(4));
    REQUIRE(queue.TryPush(5));
    REQUIRE(queue.TryPush(6));
    REQUIRE_FALSE(queue.TryPush(7));
}

TEST_CASE("LockFreeQueue: transfers values between producer and consumer",
          "[sync][lock_free_queue]") {
    zeta::LockFreeQueue<int> queue(64);
    constexpr int kValueCount = 2000;
    std::atomic<int> consumed{0};
    std::atomic<long long> sum{0};

    std::thread producer([&] {
        for (int value = 1; value <= kValueCount; ++value) {
            while (!queue.TryPush(value)) std::this_thread::yield();
        }
    });
    std::thread consumer([&] {
        while (consumed.load(std::memory_order_relaxed) < kValueCount) {
            if (auto value = queue.TryPop()) {
                sum.fetch_add(*value, std::memory_order_relaxed);
                consumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();
    REQUIRE(consumed == kValueCount);
    REQUIRE(sum == static_cast<long long>(kValueCount) *
                         (kValueCount + 1) / 2);
    REQUIRE(queue.Empty());
}
