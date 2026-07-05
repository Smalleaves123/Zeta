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
