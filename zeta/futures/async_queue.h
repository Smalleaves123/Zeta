#ifndef ZETA_FUTURES_ASYNC_QUEUE_H
#define ZETA_FUTURES_ASYNC_QUEUE_H

/// @file   futures/async_queue.h
/// @brief  Future-based queue for asynchronous producers and consumers.

#include "zeta/futures/cancellation.h"
#include "zeta/futures/future.h"

#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace zeta {

template <typename T>
class AsyncQueue {
    struct Waiter {
        explicit Waiter(Promise<T> promise) : promise(std::move(promise)) {}

        Promise<T> promise;
        CancellationRegistration cancellation;
        bool active = true;
    };

    struct State {
        explicit State(std::size_t capacity) : capacity(capacity) {}

        const std::size_t capacity;
        std::mutex mutex;
        std::deque<T> values;
        std::deque<std::shared_ptr<Waiter>> waiters;
        bool closed = false;
        Status close_status = CancelledError("async queue closed");
    };

public:
    explicit AsyncQueue(std::size_t capacity = 0)
        : state_(std::make_shared<State>(capacity)) {}

    AsyncQueue(const AsyncQueue&) = delete;
    AsyncQueue& operator=(const AsyncQueue&) = delete;
    AsyncQueue(AsyncQueue&&) = delete;
    AsyncQueue& operator=(AsyncQueue&&) = delete;

    ~AsyncQueue() {
        Close();
    }

    [[nodiscard]] Future<T> Receive(CancellationToken token = {}) {
        auto [promise, future] = makePromiseContract<T>();
        auto waiter = std::make_shared<Waiter>(std::move(promise));
        std::optional<T> value;
        Status error = OkStatus();
        bool queued = false;

        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (token.IsCancellationRequested()) {
                waiter->active = false;
                error = CancelledError("async receive cancelled");
            } else if (!state_->values.empty()) {
                waiter->active = false;
                value.emplace(std::move(state_->values.front()));
                state_->values.pop_front();
            } else if (state_->closed) {
                waiter->active = false;
                error = state_->close_status;
            } else {
                state_->waiters.push_back(waiter);
                queued = true;
            }
        }

        if (queued) {
            std::weak_ptr<State> weak_state = state_;
            std::weak_ptr<Waiter> weak_waiter = waiter;
            waiter->cancellation = token.Register(
                [weak_state, weak_waiter] {
                    auto state = weak_state.lock();
                    auto waiter = weak_waiter.lock();
                    if (state == nullptr || waiter == nullptr) return;

                    bool cancel = false;
                    {
                        std::lock_guard<std::mutex> lock(state->mutex);
                        if (waiter->active) {
                            waiter->active = false;
                            for (auto it = state->waiters.begin();
                                 it != state->waiters.end(); ++it) {
                                if (it->get() == waiter.get()) {
                                    state->waiters.erase(it);
                                    break;
                                }
                            }
                            cancel = true;
                        }
                    }
                    if (cancel) {
                        (void)waiter->promise.Cancel();
                    }
                });
        } else if (value.has_value()) {
            (void)waiter->promise.SetValue(std::move(*value));
        } else {
            (void)waiter->promise.SetError(std::move(error));
        }
        return std::move(future);
    }

    [[nodiscard]] Status Send(T value) {
        return TrySend(std::move(value));
    }

    [[nodiscard]] Status TrySend(T value) {
        std::shared_ptr<Waiter> waiter;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (state_->closed) {
                return FailedPreconditionError("async queue is closed");
            }

            while (!state_->waiters.empty()) {
                auto candidate = std::move(state_->waiters.front());
                state_->waiters.pop_front();
                if (candidate->active) {
                    candidate->active = false;
                    waiter = std::move(candidate);
                    break;
                }
            }

            if (waiter == nullptr) {
                if (state_->capacity != 0 &&
                    state_->values.size() >= state_->capacity) {
                    return ResourceExhaustedError("async queue is full");
                }
                state_->values.push_back(std::move(value));
                return OkStatus();
            }
        }

        return waiter->promise.SetValue(std::move(value));
    }

    [[nodiscard]] std::optional<T> TryReceive() {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if (state_->values.empty()) return std::nullopt;
        std::optional<T> value(std::in_place, std::move(state_->values.front()));
        state_->values.pop_front();
        return value;
    }

    void Close(Status reason = CancelledError("async queue closed")) {
        if (reason.ok()) reason = CancelledError("async queue closed");

        std::deque<std::shared_ptr<Waiter>> waiters;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (state_->closed) return;
            state_->closed = true;
            state_->close_status = reason;
            waiters.swap(state_->waiters);
            for (const auto& waiter : waiters) waiter->active = false;
        }

        for (auto& waiter : waiters) {
            (void)waiter->promise.SetError(reason);
        }
    }

    [[nodiscard]] bool IsClosed() const noexcept {
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->closed;
    }

    [[nodiscard]] std::size_t Size() const noexcept {
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->values.size();
    }

    [[nodiscard]] std::size_t PendingReceivers() const noexcept {
        std::lock_guard<std::mutex> lock(state_->mutex);
        std::size_t count = 0;
        for (const auto& waiter : state_->waiters) {
            if (waiter->active) ++count;
        }
        return count;
    }

private:
    std::shared_ptr<State> state_;
};

} // namespace zeta

#endif // ZETA_FUTURES_ASYNC_QUEUE_H
