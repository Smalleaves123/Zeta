#include "zeta/futures/future.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <utility>

namespace {

struct MoveOnly {
    explicit MoveOnly(std::string v) : value(std::move(v)) {}
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) noexcept = default;
    MoveOnly& operator=(MoveOnly&&) noexcept = default;

    std::string value;
};

} // namespace

TEST_CASE("Future: fulfill and get value", "[futures]") {
    auto [promise, future] = zeta::makePromiseContract<int>();

    REQUIRE(promise.SetValue(42).ok());
    auto result = std::move(future).Get();

    REQUIRE(result.ok());
    REQUIRE(result.value() == 42);
}

TEST_CASE("Future: wait until value is ready", "[futures][thread]") {
    auto [promise, future] = zeta::makePromiseContract<std::string>();

    std::thread worker([p = std::move(promise)]() mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        REQUIRE(p.SetValue("ready").ok());
    });

    REQUIRE(future.WaitFor(std::chrono::seconds(1)));
    auto result = std::move(future).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == "ready");

    worker.join();
}

TEST_CASE("Future: void contract works", "[futures]") {
    auto [promise, future] = zeta::makePromiseContract<void>();
    REQUIRE(promise.SetValue().ok());

    auto result = std::move(future).Get();
    REQUIRE(result.ok());
}

TEST_CASE("Future: broken promise becomes cancelled status", "[futures]") {
    zeta::Future<int> future;
    {
        auto [promise, raw_future] = zeta::makePromiseContract<int>();
        future = std::move(raw_future);
    }

    auto result = std::move(future).Get();
    REQUIRE(!result.ok());
    REQUIRE(result.status().code() == zeta::StatusCode::kCancelled);
}

TEST_CASE("Future: double set is rejected", "[futures]") {
    auto [promise, future] = zeta::makePromiseContract<int>();
    REQUIRE(promise.SetValue(7).ok());

    auto status = promise.SetValue(8);
    REQUIRE(!status.ok());
    REQUIRE(status.code() == zeta::StatusCode::kFailedPrecondition);

    auto result = std::move(future).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == 7);
}

TEST_CASE("Future: then can transform a value", "[futures][then]") {
    auto [promise, future] = zeta::makePromiseContract<int>();

    auto next = std::move(future).Then([](int value) {
        return value + 1;
    });

    REQUIRE(promise.SetValue(41).ok());

    auto result = std::move(next).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == 42);
}

TEST_CASE("Future: then propagates errors", "[futures][then]") {
    auto [promise, future] = zeta::makePromiseContract<int>();

    auto next = std::move(future).Then([](int value) {
        return value + 1;
    });

    REQUIRE(promise.SetError(zeta::UnavailableError("downstream")).ok());

    auto result = std::move(next).Get();
    REQUIRE(!result.ok());
    REQUIRE(result.status().code() == zeta::StatusCode::kUnavailable);
}

TEST_CASE("Future: then supports void continuations", "[futures][then]") {
    auto [promise, future] = zeta::makePromiseContract<void>();

    auto next = std::move(future).Then([] {
        return std::string("done");
    });

    REQUIRE(promise.SetValue().ok());

    auto result = std::move(next).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == "done");
}

TEST_CASE("Future: then handles move-only payloads", "[futures][move]") {
    auto [promise, future] = zeta::makePromiseContract<MoveOnly>();

    std::atomic<bool> saw_move_only{false};
    auto next = std::move(future).Then([&](MoveOnly value) {
        saw_move_only.store(true, std::memory_order_relaxed);
        return value.value.size();
    });

    REQUIRE(promise.SetValue(MoveOnly("abc")).ok());

    auto result = std::move(next).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == 3);
    REQUIRE(saw_move_only.load(std::memory_order_relaxed));
}
