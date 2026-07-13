#include "zeta/futures/future.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

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

TEST_CASE("Future: Promise cancel completes the future", "[futures][cancel]") {
    auto [promise, future] = zeta::makePromiseContract<int>();

    REQUIRE(promise.Cancel().ok());
    auto result = std::move(future).Get();

    REQUIRE(!result.ok());
    REQUIRE(result.status().code() == zeta::StatusCode::kCancelled);
}

TEST_CASE("Future: GetFor returns deadline exceeded", "[futures][timeout]") {
    auto [promise, future] = zeta::makePromiseContract<int>();

    auto result = std::move(future).GetFor(std::chrono::milliseconds(5));

    REQUIRE(!result.ok());
    REQUIRE(result.status().code() == zeta::StatusCode::kDeadlineExceeded);
}

TEST_CASE("Future: GetFor observes cooperative cancellation", "[futures][cancel]") {
    auto [promise, future] = zeta::makePromiseContract<int>();
    zeta::CancellationSource source;
    auto token = source.GetToken();

    std::thread canceller([&source] {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        REQUIRE(source.RequestCancellation());
        REQUIRE(!source.RequestCancellation());
    });

    auto result = std::move(future).GetFor(std::chrono::seconds(1), token);
    canceller.join();

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

TEST_CASE("Future: ThenTry observes errors", "[futures][then_try]") {
    auto [promise, future] = zeta::makePromiseContract<int>();
    std::atomic<bool> observed{false};

    auto next = std::move(future).ThenTry([&](zeta::StatusOr<int> result) {
        if (!result.ok()) {
            observed.store(true, std::memory_order_relaxed);
            return zeta::StatusOr<std::string>(std::move(result).status());
        }
        return zeta::StatusOr<std::string>(std::to_string(result.value()));
    });

    REQUIRE(promise.SetError(zeta::UnavailableError("downstream")).ok());

    auto result = std::move(next).Get();
    REQUIRE(!result.ok());
    REQUIRE(observed.load(std::memory_order_relaxed));
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

TEST_CASE("Future: collectAll preserves order and errors", "[futures][collect_all]") {
    auto [p1, f1] = zeta::makePromiseContract<int>();
    auto [p2, f2] = zeta::makePromiseContract<int>();
    auto [p3, f3] = zeta::makePromiseContract<int>();

    std::vector<zeta::Future<int>> futures;
    futures.push_back(std::move(f1));
    futures.push_back(std::move(f2));
    futures.push_back(std::move(f3));
    auto grouped = zeta::collectAll<int>(std::move(futures));

    REQUIRE(p2.SetError(zeta::UnavailableError("down")).ok());
    REQUIRE(p1.SetValue(1).ok());
    REQUIRE(p3.SetValue(3).ok());

    auto result = std::move(grouped).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value().size() == 3);
    REQUIRE(result.value()[0].ok());
    REQUIRE(result.value()[0].value() == 1);
    REQUIRE(!result.value()[1].ok());
    REQUIRE(result.value()[1].status().code() == zeta::StatusCode::kUnavailable);
    REQUIRE(result.value()[2].ok());
    REQUIRE(result.value()[2].value() == 3);
}

TEST_CASE("Future: whenAll aggregates successful values", "[futures][when_all]") {
    auto [p1, f1] = zeta::makePromiseContract<int>();
    auto [p2, f2] = zeta::makePromiseContract<int>();

    std::vector<zeta::Future<int>> futures;
    futures.push_back(std::move(f1));
    futures.push_back(std::move(f2));
    auto grouped = zeta::whenAll<int>(std::move(futures));

    REQUIRE(p1.SetValue(20).ok());
    REQUIRE(p2.SetValue(22).ok());

    auto result = std::move(grouped).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value().size() == 2);
    REQUIRE(result.value()[0] == 20);
    REQUIRE(result.value()[1] == 22);
}

TEST_CASE("Future: whenAll returns first error status", "[futures][when_all]") {
    auto [p1, f1] = zeta::makePromiseContract<int>();
    auto [p2, f2] = zeta::makePromiseContract<int>();

    std::vector<zeta::Future<int>> futures;
    futures.push_back(std::move(f1));
    futures.push_back(std::move(f2));
    auto grouped = zeta::whenAll<int>(std::move(futures));

    REQUIRE(p1.SetValue(20).ok());
    REQUIRE(p2.SetError(zeta::InvalidArgumentError("bad")).ok());

    auto result = std::move(grouped).Get();
    REQUIRE(!result.ok());
    REQUIRE(result.status().code() == zeta::StatusCode::kInvalidArgument);
}

TEST_CASE("Future: whenAll supports void futures", "[futures][when_all][void]") {
    auto [p1, f1] = zeta::makePromiseContract<void>();
    auto [p2, f2] = zeta::makePromiseContract<void>();

    std::vector<zeta::Future<void>> futures;
    futures.push_back(std::move(f1));
    futures.push_back(std::move(f2));
    auto grouped = zeta::whenAll<void>(std::move(futures));

    REQUIRE(p1.SetValue().ok());
    REQUIRE(p2.SetValue().ok());

    auto result = std::move(grouped).Get();
    REQUIRE(result.ok());
}

TEST_CASE("Future: collectAny returns first completed future with index", "[futures][collect_any]") {
    auto [p1, f1] = zeta::makePromiseContract<int>();
    auto [p2, f2] = zeta::makePromiseContract<int>();
    auto [p3, f3] = zeta::makePromiseContract<int>();

    std::vector<zeta::Future<int>> futures;
    futures.push_back(std::move(f1));
    futures.push_back(std::move(f2));
    futures.push_back(std::move(f3));
    auto grouped = zeta::collectAny<int>(std::move(futures));

    std::thread t1([p = std::move(p1)]() mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        REQUIRE(p.SetValue(1).ok());
    });
    std::thread t2([p = std::move(p2)]() mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        REQUIRE(p.SetError(zeta::UnavailableError("fast")).ok());
    });
    std::thread t3([p = std::move(p3)]() mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        REQUIRE(p.SetValue(3).ok());
    });

    auto result = std::move(grouped).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value().first == 1);
    REQUIRE(!result.value().second.ok());
    REQUIRE(result.value().second.status().code() == zeta::StatusCode::kUnavailable);

    t1.join();
    t2.join();
    t3.join();
}

TEST_CASE("Future: collectN returns first N completed futures in completion order", "[futures][collect_n]") {
    auto [p1, f1] = zeta::makePromiseContract<int>();
    auto [p2, f2] = zeta::makePromiseContract<int>();
    auto [p3, f3] = zeta::makePromiseContract<int>();

    std::vector<zeta::Future<int>> futures;
    futures.push_back(std::move(f1));
    futures.push_back(std::move(f2));
    futures.push_back(std::move(f3));
    auto grouped = zeta::collectN<int>(std::move(futures), 2);

    std::thread t1([p = std::move(p1)]() mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        REQUIRE(p.SetValue(10).ok());
    });
    std::thread t2([p = std::move(p2)]() mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        REQUIRE(p.SetValue(20).ok());
    });
    std::thread t3([p = std::move(p3)]() mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        REQUIRE(p.SetValue(30).ok());
    });

    auto result = std::move(grouped).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value().size() == 2);
    REQUIRE(result.value()[0].first == 1);
    REQUIRE(result.value()[0].second.ok());
    REQUIRE(result.value()[0].second.value() == 20);
    REQUIRE(result.value()[1].first == 2);
    REQUIRE(result.value()[1].second.ok());
    REQUIRE(result.value()[1].second.value() == 30);

    t1.join();
    t2.join();
    t3.join();
}
