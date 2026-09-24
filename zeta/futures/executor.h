#ifndef ZETA_FUTURES_EXECUTOR_H
#define ZETA_FUTURES_EXECUTOR_H

/// @file   futures/executor.h
/// @brief  General task scheduling interfaces and a basic thread-pool executor.

#include <condition_variable>
#include <chrono>
#include <cstddef>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <queue>
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

/// A single-worker executor for delayed and immediate tasks.
///
/// Scheduled tasks are owned by the executor and are drained during shutdown.
/// The executor is intentionally small; use ThreadPoolExecutor when parallel
/// task execution is required.
class ScheduledExecutor final : public Executor {
public:
    using TaskErrorHandler = std::function<void(std::exception_ptr)>;

    explicit ScheduledExecutor(TaskErrorHandler error_handler = {})
        : error_handler_(std::move(error_handler))
        , worker_([this] { WorkerLoop(); }) {}

    ScheduledExecutor(const ScheduledExecutor&) = delete;
    ScheduledExecutor& operator=(const ScheduledExecutor&) = delete;
    ScheduledExecutor(ScheduledExecutor&&) = delete;
    ScheduledExecutor& operator=(ScheduledExecutor&&) = delete;

    ~ScheduledExecutor() override { Shutdown(); }

    void Add(std::function<void()> task) override {
        ScheduleAfter(std::chrono::steady_clock::duration::zero(),
                      std::move(task));
    }

    void ScheduleAfter(
        std::chrono::steady_clock::duration delay,
        std::function<void()> task) {
        if (!task) return;
        if (delay < std::chrono::steady_clock::duration::zero()) {
            delay = std::chrono::steady_clock::duration::zero();
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) {
                throw std::runtime_error("scheduled executor is shut down");
            }
            tasks_.push(TaskEntry{
                std::chrono::steady_clock::now() + delay,
                next_sequence_++,
                std::move(task)});
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
        if (worker_.joinable()) worker_.join();
    }

    [[nodiscard]] bool IsShutdown() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return stopping_;
    }

    [[nodiscard]] std::size_t PendingTasks() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.size();
    }

private:
    struct TaskEntry {
        std::chrono::steady_clock::time_point deadline;
        std::size_t sequence;
        std::function<void()> task;
    };

    struct Earlier {
        bool operator()(const TaskEntry& lhs, const TaskEntry& rhs) const {
            if (lhs.deadline != rhs.deadline) {
                return lhs.deadline > rhs.deadline;
            }
            return lhs.sequence > rhs.sequence;
        }
    };

    void WorkerLoop() noexcept {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                for (;;) {
                    if (tasks_.empty()) {
                        if (stopping_) return;
                        condition_.wait(lock);
                        continue;
                    }

                    const auto now = std::chrono::steady_clock::now();
                    if (!stopping_ && tasks_.top().deadline > now) {
                        condition_.wait_until(lock, tasks_.top().deadline);
                        continue;
                    }

                    task = std::move(const_cast<TaskEntry&>(tasks_.top()).task);
                    tasks_.pop();
                    break;
                }
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
    std::priority_queue<TaskEntry, std::vector<TaskEntry>, Earlier> tasks_;
    TaskErrorHandler error_handler_;
    std::size_t next_sequence_ = 0;
    bool stopping_ = false;
    std::thread worker_;
};

} // namespace zeta

#endif // ZETA_FUTURES_EXECUTOR_H
