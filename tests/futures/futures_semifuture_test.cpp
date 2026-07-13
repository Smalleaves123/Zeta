#include "zeta/futures/future.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace {

class BackgroundExecutor final : public zeta::Executor {
public:
    BackgroundExecutor()
        : worker_([this] { Run(); }) {}

    ~BackgroundExecutor() override {
        {
            std::lock_guard<std::mutex> lock(mu_);
            stopping_ = true;
        }
        cv_.notify_all();
        worker_.join();
    }

    void Add(std::function<void()> task) override {
        {
            std::lock_guard<std::mutex> lock(mu_);
            tasks_.push(std::move(task));
        }
        cv_.notify_one();
    }

private:
    void Run() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mu_);
                cv_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
                if (stopping_ && tasks_.empty()) {
                    return;
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

    std::mutex mu_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> tasks_;
    bool stopping_ = false;
    std::thread worker_;
};

} // namespace

TEST_CASE("SemiFuture: Via schedules continuations on executor", "[futures][semifuture]") {
    BackgroundExecutor executor;
    auto [promise, future] = zeta::makePromiseContract<int>();
    auto main_thread = std::this_thread::get_id();

    auto semi = std::move(future).Via(executor);
    std::atomic<bool> ran_on_background{false};
    auto next = std::move(semi).Then([&](int value) {
        ran_on_background.store(std::this_thread::get_id() != main_thread,
                                 std::memory_order_relaxed);
        return value + 1;
    });

    REQUIRE(promise.SetValue(41).ok());

    auto result = std::move(next).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == 42);
    REQUIRE(ran_on_background.load(std::memory_order_relaxed));
}

TEST_CASE("SemiFuture: executor is borrowed and kept alive by the caller",
          "[futures][semifuture][lifetime]") {
    std::atomic<bool> completed{false};
    {
        BackgroundExecutor executor;
        auto [promise, future] = zeta::makePromiseContract<int>();
        auto semi = std::move(future).Via(executor);
        auto next = std::move(semi).Then([&](int value) {
            completed.store(true, std::memory_order_relaxed);
            return value + 1;
        });

        REQUIRE(promise.SetValue(41).ok());
        auto result = std::move(next).Get();
        REQUIRE(result.ok());
        REQUIRE(result.value() == 42);
    }

    REQUIRE(completed.load(std::memory_order_relaxed));
}

TEST_CASE("SemiFuture: ToFuture keeps the same shared state", "[futures][semifuture]") {
    BackgroundExecutor executor;
    auto [promise, future] = zeta::makePromiseContract<int>();

    auto semi = std::move(future).Via(executor);
    auto back = std::move(semi).ToFuture();

    REQUIRE(promise.SetValue(7).ok());

    auto result = std::move(back).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value() == 7);
}

TEST_CASE("SemiFuture: collectAll accepts semi futures", "[futures][semifuture][collect_all]") {
    BackgroundExecutor executor;
    auto [p1, f1] = zeta::makePromiseContract<int>();
    auto [p2, f2] = zeta::makePromiseContract<int>();

    std::vector<zeta::SemiFuture<int>> futures;
    futures.push_back(std::move(f1).Via(executor));
    futures.push_back(std::move(f2).Via(executor));
    auto grouped = zeta::collectAll<int>(std::move(futures));

    REQUIRE(p1.SetValue(10).ok());
    REQUIRE(p2.SetValue(32).ok());

    auto result = std::move(grouped).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value().size() == 2);
    REQUIRE(result.value()[0].value() == 10);
    REQUIRE(result.value()[1].value() == 32);
}

TEST_CASE("SemiFuture: collectN accepts semi futures", "[futures][semifuture][collect_n]") {
    BackgroundExecutor executor;
    auto [p1, f1] = zeta::makePromiseContract<int>();
    auto [p2, f2] = zeta::makePromiseContract<int>();
    auto [p3, f3] = zeta::makePromiseContract<int>();

    std::vector<zeta::SemiFuture<int>> futures;
    futures.push_back(std::move(f1).Via(executor));
    futures.push_back(std::move(f2).Via(executor));
    futures.push_back(std::move(f3).Via(executor));
    auto grouped = zeta::collectN<int>(std::move(futures), 2);

    REQUIRE(p2.SetValue(20).ok());
    REQUIRE(p3.SetError(zeta::InvalidArgumentError("bad")).ok());
    REQUIRE(p1.SetValue(10).ok());

    auto result = std::move(grouped).Get();
    REQUIRE(result.ok());
    REQUIRE(result.value().size() == 2);
    REQUIRE(result.value()[0].first == 1);
    REQUIRE(result.value()[0].second.ok());
    REQUIRE(result.value()[0].second.value() == 20);
    REQUIRE(result.value()[1].first == 2);
    REQUIRE(!result.value()[1].second.ok());
    REQUIRE(result.value()[1].second.status().code() == zeta::StatusCode::kInvalidArgument);
}
