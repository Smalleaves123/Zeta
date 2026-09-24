#include "zeta/futures/async_queue.h"
#include "zeta/futures/executor.h"
#include "zeta/futures/task_group.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <thread>

TEST_CASE("CancellationToken: registrations run once and can be reset",
          "[futures][cancellation]") {
    zeta::CancellationSource source;
    auto token = source.GetToken();
    std::atomic<int> calls{0};
    auto registration = token.Register([&] {
        calls.fetch_add(1, std::memory_order_relaxed);
    });

    registration.Reset();
    REQUIRE(source.RequestCancellation());
    REQUIRE_FALSE(source.RequestCancellation());
    REQUIRE(calls.load(std::memory_order_relaxed) == 0);

    (void)token.Register([&] {
        calls.fetch_add(1, std::memory_order_relaxed);
    });
    REQUIRE(calls.load(std::memory_order_relaxed) == 1);
}

TEST_CASE("CancellationToken: immediate callback exceptions are contained",
          "[futures][cancellation][exception]") {
    zeta::CancellationSource source;
    REQUIRE(source.RequestCancellation());

    REQUIRE_NOTHROW((void)source.GetToken().Register([] {
        throw std::runtime_error("callback failed");
    }));
}

TEST_CASE("CancellationRegistration: reset races safely with cancellation",
          "[futures][cancellation][race]") {
    for (int iteration = 0; iteration != 200; ++iteration) {
        zeta::CancellationSource source;
        std::atomic<int> callbacks{0};
        auto registration = source.GetToken().Register([&callbacks] {
            callbacks.fetch_add(1, std::memory_order_relaxed);
        });

        std::thread requester([&source] {
            (void)source.RequestCancellation();
        });
        registration.Reset();
        requester.join();

        REQUIRE(callbacks.load(std::memory_order_relaxed) <= 1);
    }
}

TEST_CASE("ThreadPoolExecutor: runs submitted tasks", "[futures][executor]") {
    zeta::ThreadPoolExecutor executor(2);
    std::atomic<int> completed{0};

    for (int index = 0; index < 8; ++index) {
        executor.Add([&completed] {
            completed.fetch_add(1, std::memory_order_relaxed);
        });
    }
    executor.Shutdown();

    REQUIRE(completed.load(std::memory_order_relaxed) == 8);
    REQUIRE(executor.IsShutdown());
    REQUIRE_THROWS(executor.Add([] {}));
}

TEST_CASE("ThreadPoolExecutor: reports task exceptions to handler",
          "[futures][executor][exception]") {
    std::atomic<int> errors{0};
    std::atomic<bool> saw_runtime_error{false};
    zeta::ThreadPoolExecutor executor(
        1, [&errors, &saw_runtime_error](std::exception_ptr error) {
            if (error == nullptr) return;
            try {
                std::rethrow_exception(error);
            } catch (const std::runtime_error&) {
                saw_runtime_error.store(true, std::memory_order_relaxed);
                errors.fetch_add(1, std::memory_order_relaxed);
            }
        });

    executor.Add([] { throw std::runtime_error("task failed"); });
    executor.Shutdown();
    REQUIRE(saw_runtime_error.load(std::memory_order_relaxed));
    REQUIRE(errors.load(std::memory_order_relaxed) == 1);
}

TEST_CASE("Future: continuation falls back when executor rejects work",
          "[futures][executor]") {
    zeta::ThreadPoolExecutor executor(1);
    executor.Shutdown();
    auto [promise, future] = zeta::makePromiseContract<int>();
    auto next = std::move(future).Via(executor).Then([](int value) {
        return value + 1;
    });

    REQUIRE(promise.SetValue(41).ok());
    auto result = std::move(next).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == 42);
}

TEST_CASE("Future: continuation exceptions become downstream errors",
          "[futures][executor][exception]") {
    zeta::ThreadPoolExecutor executor(1);
    auto [promise, future] = zeta::makePromiseContract<int>();
    auto next = std::move(future).Via(executor).Then([](int) -> int {
        throw std::runtime_error("continuation failed");
    });

    REQUIRE(promise.SetValue(1).ok());
    auto result = std::move(next).Get();
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.status().code() == zeta::StatusCode::kInternal);
}

TEST_CASE("TaskGroup: cancels and joins cooperative tasks",
          "[futures][task_group]") {
    zeta::ThreadPoolExecutor executor(2);
    zeta::TaskGroup group(executor);
    std::atomic<bool> started{false};

    REQUIRE(group.Spawn([&](zeta::CancellationToken token) {
        started.store(true, std::memory_order_release);
        while (!token.IsCancellationRequested()) {
            std::this_thread::yield();
        }
    }).ok());

    while (!started.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    group.Cancel();
    REQUIRE(group.Wait().ok());
    REQUIRE(group.IsCancelled());
    REQUIRE(group.PendingTasks() == 0);
}

TEST_CASE("TaskGroup: reports task exceptions", "[futures][task_group]") {
    zeta::InlineExecutor executor;
    zeta::TaskGroup group(executor);

    REQUIRE(group.Spawn([] { throw 1; }).ok());
    REQUIRE_FALSE(group.Wait().ok());
    REQUIRE_FALSE(group.Spawn([] {}).ok());
}

TEST_CASE("AsyncQueue: delivers values and enforces capacity",
          "[futures][async_queue]") {
    zeta::AsyncQueue<int> queue(1);
    REQUIRE(queue.Send(42).ok());
    REQUIRE(queue.TrySend(43).code() == zeta::StatusCode::kResourceExhausted);

    auto result = std::move(queue.Receive()).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == 42);
    queue.Close();
    REQUIRE_FALSE(std::move(queue.Receive()).Get().ok());
}

TEST_CASE("AsyncQueue: pending receive observes cancellation",
          "[futures][async_queue][cancel]") {
    zeta::AsyncQueue<int> queue;
    zeta::CancellationSource source;
    auto pending = queue.Receive(source.GetToken());
    REQUIRE(queue.PendingReceivers() == 1);

    std::thread canceller([&source] {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        (void)source.RequestCancellation();
    });

    auto result = std::move(pending).GetFor(
        std::chrono::seconds(1), source.GetToken());
    canceller.join();
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.status().code() == zeta::StatusCode::kCancelled);
    REQUIRE(queue.PendingReceivers() == 0);
}

TEST_CASE("AsyncQueue: send and cancellation race safely",
          "[futures][async_queue][race]") {
    for (int iteration = 0; iteration != 200; ++iteration) {
        zeta::AsyncQueue<int> queue;
        zeta::CancellationSource source;
        auto pending = queue.Receive(source.GetToken());
        std::atomic<bool> start{false};

        std::thread sender([&queue, &start, iteration] {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            (void)queue.Send(iteration);
        });
        std::thread canceller([&source, &start] {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            (void)source.RequestCancellation();
        });
        start.store(true, std::memory_order_release);
        sender.join();
        canceller.join();

        auto result = std::move(pending).Get();
        REQUIRE((result.ok() ||
                 result.status().code() == zeta::StatusCode::kCancelled));
        queue.Close();
    }
}
