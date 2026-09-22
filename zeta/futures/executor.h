#ifndef ZETA_FUTURES_EXECUTOR_H
#define ZETA_FUTURES_EXECUTOR_H

/// @file   futures/executor.h
/// @brief  General task scheduling interfaces and a basic thread-pool executor.

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace zeta {

class Executor {
public:
    virtual ~Executor() = default;

    virtual void Add(std::function<void()> task) = 0;
};

class InlineExecutor final : public Executor {
public:
    void Add(std::function<void()> task) override {
        if (task) task();
    }
};

class ThreadPoolExecutor final : public Executor {
public:
    /// Receives exceptions escaping a task. If unset, the default behavior is
    /// to terminate the process, preserving the historical fail-fast policy.
    using TaskErrorHandler = std::function<void(std::exception_ptr)>;

    explicit ThreadPoolExecutor(
        std::size_t worker_count = std::thread::hardware_concurrency(),
        TaskErrorHandler error_handler = {})
        : error_handler_(std::move(error_handler)) {
        if (worker_count == 0) worker_count = 1;
        workers_.reserve(worker_count);
        try {
            for (std::size_t index = 0; index < worker_count; ++index) {
                workers_.emplace_back([this] { WorkerLoop(); });
            }
        } catch (...) {
            Shutdown();
            throw;
        }
    }

    ThreadPoolExecutor(const ThreadPoolExecutor&) = delete;
    ThreadPoolExecutor& operator=(const ThreadPoolExecutor&) = delete;
    ThreadPoolExecutor(ThreadPoolExecutor&&) = delete;
    ThreadPoolExecutor& operator=(ThreadPoolExecutor&&) = delete;

    ~ThreadPoolExecutor() override {
        Shutdown();
    }

    void Add(std::function<void()> task) override {
        if (!task) return;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) {
                throw std::runtime_error("executor is shut down");
            }
            tasks_.push_back(std::move(task));
        }
        condition_.notify_one();
    }

    void Shutdown() noexcept {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) return;
            stopping_ = true;
        }
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            workers_.clear();
        }
    }

    [[nodiscard]] bool IsShutdown() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return stopping_;
    }

    [[nodiscard]] std::size_t PendingTasks() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.size();
    }

    [[nodiscard]] std::size_t WorkerCount() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return workers_.size();
    }

private:
    void WorkerLoop() noexcept {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] {
                    return stopping_ || !tasks_.empty();
                });
                if (tasks_.empty()) {
                    return;
                }
                task = std::move(tasks_.front());
                tasks_.pop_front();
            }

            try {
                task();
            } catch (...) {
                if (!error_handler_) {
                    std::terminate();
                }
                try {
                    error_handler_(std::current_exception());
                } catch (...) {
                    std::terminate();
                }
            }
        }
    }

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
    TaskErrorHandler error_handler_;
    bool stopping_ = false;
};

} // namespace zeta

#endif // ZETA_FUTURES_EXECUTOR_H
