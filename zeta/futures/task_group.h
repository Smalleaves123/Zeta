#ifndef ZETA_FUTURES_TASK_GROUP_H
#define ZETA_FUTURES_TASK_GROUP_H

/// @file   futures/task_group.h
/// @brief  Structured task lifetime, cancellation, and joining.

#include "zeta/futures/cancellation.h"
#include "zeta/futures/executor.h"
#include "zeta/status/status.h"

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace zeta {

class TaskGroup {
public:
    using Task = std::function<void(CancellationToken)>;

    explicit TaskGroup(Executor& executor)
        : state_(std::make_shared<State>(&executor)) {}

    TaskGroup(const TaskGroup&) = delete;
    TaskGroup& operator=(const TaskGroup&) = delete;
    TaskGroup(TaskGroup&&) = delete;
    TaskGroup& operator=(TaskGroup&&) = delete;

    ~TaskGroup() {
        Cancel();
        (void)Wait();
    }

    [[nodiscard]] Status Spawn(Task task) {
        if (!task) return InvalidArgumentError("task must not be empty");

        auto state = state_;
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (!state->accepting) {
                return FailedPreconditionError("task group is not accepting tasks");
            }
            ++state->active_tasks;
        }

        try {
            state->executor->Add([state, task = std::move(task)]() mutable {
                try {
                    task(state->cancellation.GetToken());
                } catch (...) {
                    RecordFailure(state);
                }
                Finish(state);
            });
        } catch (...) {
            RecordFailure(state);
            Finish(state);
            return InternalError("task scheduling failed");
        }
        return OkStatus();
    }

    [[nodiscard]] Status Spawn(std::function<void()> task) {
        if (!task) return InvalidArgumentError("task must not be empty");
        return Spawn([task = std::move(task)](CancellationToken) mutable {
            task();
        });
    }

    void Cancel() noexcept {
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            state_->accepting = false;
        }
        (void)state_->cancellation.RequestCancellation();
    }

    [[nodiscard]] Status Wait() {
        std::unique_lock<std::mutex> lock(state_->mutex);
        state_->accepting = false;
        state_->condition.wait(lock, [this] {
            return state_->active_tasks == 0;
        });
        return state_->failure;
    }

    [[nodiscard]] bool IsCancelled() const noexcept {
        return state_->cancellation.GetToken().IsCancellationRequested();
    }

    [[nodiscard]] CancellationToken Token() const noexcept {
        return state_->cancellation.GetToken();
    }

    [[nodiscard]] std::size_t PendingTasks() const noexcept {
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->active_tasks;
    }

private:
    struct State {
        explicit State(Executor* executor) : executor(executor) {}

        Executor* executor;
        CancellationSource cancellation;
        std::mutex mutex;
        std::condition_variable condition;
        std::size_t active_tasks = 0;
        bool accepting = true;
        Status failure = OkStatus();
    };

    static void RecordFailure(const std::shared_ptr<State>& state) noexcept {
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (state->failure.ok()) {
                state->failure = InternalError("task failed with an exception");
            }
            state->accepting = false;
        }
        (void)state->cancellation.RequestCancellation();
    }

    static void Finish(const std::shared_ptr<State>& state) noexcept {
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            --state->active_tasks;
        }
        state->condition.notify_all();
    }

    std::shared_ptr<State> state_;
};

} // namespace zeta

#endif // ZETA_FUTURES_TASK_GROUP_H
