#include "zeta/futures/coroutine.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

zeta::Coroutine<int> AddOne(zeta::Future<int> input) {
    zeta::StatusOr<int> result = co_await std::move(input);
    if (!result.ok()) co_return 0;
    co_return result.value() + 1;
}

zeta::Coroutine<int> PreserveError(zeta::Future<int> input) {
    zeta::StatusOr<int> result = co_await std::move(input);
    if (!result.ok()) co_return result.status();
    co_return result.value();
}

zeta::Coroutine<void> Complete() {
    co_return;
}

zeta::Coroutine<void> AwaitVoid(zeta::Future<void> input) {
    auto result = co_await std::move(input);
    if (!result.ok()) co_return;
    co_return;
}

zeta::Coroutine<int> ThrowingCoroutine() {
    throw std::runtime_error("coroutine failure");
    co_return 0;
}

zeta::Coroutine<std::thread::id> ResumeOn(zeta::SemiFuture<int> input) {
    auto result = co_await std::move(input);
    if (!result.ok()) co_return std::thread::id{};
    co_return std::this_thread::get_id();
}

zeta::Coroutine<int> CancelAware(
    zeta::Future<int> input,
    zeta::CancellationToken token) {
    auto result = co_await zeta::WithCancellation(std::move(input), token);
    if (!result.ok()) co_return result.status();
    co_return result.value();
}

zeta::Coroutine<std::vector<int>> Gather(std::vector<zeta::Future<int>> futures) {
    auto result = co_await zeta::whenAll(std::move(futures));
    if (!result.ok()) co_return result.status();
    co_return std::move(result).value();
}

} // namespace

TEST_CASE("Coroutine awaits a future", "[futures][coroutine]") {
    auto [promise, future] = zeta::makePromiseContract<int>();
    auto output = AddOne(std::move(future)).GetFuture();

    REQUIRE(promise.SetValue(41).ok());
    auto result = std::move(output).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == 42);
}

TEST_CASE("Coroutine awaits an already-ready future", "[futures][coroutine]") {
    auto [promise, future] = zeta::makePromiseContract<int>();
    REQUIRE(promise.SetValue(41).ok());

    auto output = AddOne(std::move(future)).GetFuture();
    auto result = std::move(output).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == 42);
}

TEST_CASE("Coroutine propagates future errors", "[futures][coroutine]") {
    auto [promise, future] = zeta::makePromiseContract<int>();
    auto output = PreserveError(std::move(future)).GetFuture();

    REQUIRE(promise.SetError(zeta::NotFoundError("missing")).ok());
    auto result = std::move(output).Get();
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.status().code() == zeta::StatusCode::kNotFound);
}

TEST_CASE("Void coroutine completes", "[futures][coroutine]") {
    auto output = Complete().GetFuture();
    REQUIRE(std::move(output).Get().ok());
}

TEST_CASE("Coroutine awaits a void future", "[futures][coroutine]") {
    auto [promise, future] = zeta::makePromiseContract<void>();
    auto output = AwaitVoid(std::move(future)).GetFuture();

    REQUIRE(promise.SetValue().ok());
    REQUIRE(std::move(output).Get().ok());
}

TEST_CASE("Coroutine exceptions become internal errors", "[futures][coroutine]") {
    auto output = ThrowingCoroutine().GetFuture();
    auto result = std::move(output).Get();
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.status().code() == zeta::StatusCode::kInternal);
}

TEST_CASE("Coroutine preserves SemiFuture executor", "[futures][coroutine]") {
    zeta::ThreadPoolExecutor executor(1);
    auto [promise, future] = zeta::makePromiseContract<int>();
    auto output = ResumeOn(std::move(future).Via(executor)).GetFuture();

    const auto caller = std::this_thread::get_id();
    REQUIRE(promise.SetValue(7).ok());
    auto result = std::move(output).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() != caller);
}

TEST_CASE("WithCancellation completes when source wins", "[futures][coroutine]") {
    zeta::CancellationSource cancellation;
    auto [promise, future] = zeta::makePromiseContract<int>();
    auto output = CancelAware(std::move(future), cancellation.GetToken()).GetFuture();

    REQUIRE(promise.SetValue(9).ok());
    auto result = std::move(output).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == 9);
}

TEST_CASE("WithCancellation returns cancelled when token wins", "[futures][coroutine]") {
    zeta::CancellationSource cancellation;
    auto [promise, future] = zeta::makePromiseContract<int>();
    auto output = CancelAware(std::move(future), cancellation.GetToken()).GetFuture();

    REQUIRE(cancellation.RequestCancellation());
    auto result = std::move(output).Get();
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.status().code() == zeta::StatusCode::kCancelled);
    REQUIRE(promise.SetValue(9).ok());
}

TEST_CASE("WithCancellation handles completion and cancellation races",
          "[futures][coroutine][race]") {
    for (int iteration = 0; iteration != 200; ++iteration) {
        zeta::CancellationSource cancellation;
        auto [promise, future] = zeta::makePromiseContract<int>();
        auto output = zeta::WithCancellation(
            std::move(future), cancellation.GetToken());
        std::atomic<bool> start{false};

        std::thread producer([p = std::move(promise), &start, iteration]() mutable {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            (void)p.SetValue(iteration);
        });
        std::thread canceller([&cancellation, &start] {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            (void)cancellation.RequestCancellation();
        });
        start.store(true, std::memory_order_release);
        producer.join();
        canceller.join();

        auto result = std::move(output).Get();
        REQUIRE((result.ok() ||
                 result.status().code() == zeta::StatusCode::kCancelled));
    }
}

TEST_CASE("Coroutine fan-in awaits whenAll", "[futures][coroutine]") {
    auto [first_promise, first] = zeta::makePromiseContract<int>();
    auto [second_promise, second] = zeta::makePromiseContract<int>();
    std::vector<zeta::Future<int>> futures;
    futures.push_back(std::move(first));
    futures.push_back(std::move(second));
    auto output = Gather(std::move(futures)).GetFuture();

    REQUIRE(first_promise.SetValue(1).ok());
    REQUIRE(second_promise.SetValue(2).ok());
    auto result = std::move(output).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == std::vector<int>{1, 2});
}

TEST_CASE("coWhenAll returns a coroutine future", "[futures][coroutine]") {
    auto [first_promise, first] = zeta::makePromiseContract<int>();
    auto [second_promise, second] = zeta::makePromiseContract<int>();
    std::vector<zeta::Future<int>> futures;
    futures.push_back(std::move(first));
    futures.push_back(std::move(second));
    auto output = zeta::coWhenAll(std::move(futures)).GetFuture();

    REQUIRE(first_promise.SetValue(3).ok());
    REQUIRE(second_promise.SetValue(4).ok());
    auto result = std::move(output).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == std::vector<int>{3, 4});
}

TEST_CASE("ScheduledExecutor runs delayed tasks", "[futures][executor]") {
    zeta::ScheduledExecutor scheduler;
    std::atomic<bool> ran{false};
    scheduler.ScheduleAfter(std::chrono::milliseconds(5), [&ran] {
        ran.store(true, std::memory_order_release);
    });

    for (int attempt = 0; attempt != 100 &&
         !ran.load(std::memory_order_acquire); ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    REQUIRE(ran.load(std::memory_order_acquire));
}

TEST_CASE("WithTimeout returns deadline status", "[futures][coroutine]") {
    zeta::ScheduledExecutor scheduler;
    auto [promise, source] = zeta::makePromiseContract<int>();
    auto output = zeta::WithTimeout(
        std::move(source), scheduler, std::chrono::milliseconds(10));

    auto result = std::move(output).Get();
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.status().code() == zeta::StatusCode::kDeadlineExceeded);
    REQUIRE(promise.SetValue(11).ok());
}

TEST_CASE("ScheduledExecutor drains delayed tasks during shutdown",
          "[futures][executor]") {
    zeta::ScheduledExecutor scheduler;
    std::atomic<bool> ran{false};
    scheduler.ScheduleAfter(std::chrono::hours(1), [&ran] {
        ran.store(true, std::memory_order_release);
    });

    scheduler.Shutdown();
    REQUIRE(ran.load(std::memory_order_acquire));
    REQUIRE(scheduler.PendingTasks() == 0);
    REQUIRE_THROWS(scheduler.Add([] {}));
}

TEST_CASE("ScheduledExecutor reports task exceptions",
          "[futures][executor][exception]") {
    std::atomic<int> errors{0};
    std::atomic<bool> saw_runtime_error{false};
    zeta::ScheduledExecutor scheduler(
        [&errors, &saw_runtime_error](std::exception_ptr error) {
            if (error == nullptr) return;
            try {
                std::rethrow_exception(error);
            } catch (const std::runtime_error&) {
                saw_runtime_error.store(true, std::memory_order_release);
                errors.fetch_add(1, std::memory_order_relaxed);
            }
        });

    scheduler.Add([] { throw std::runtime_error("scheduled task failed"); });
    scheduler.Shutdown();
    REQUIRE(saw_runtime_error.load(std::memory_order_acquire));
    REQUIRE(errors.load(std::memory_order_relaxed) == 1);
}
