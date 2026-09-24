#ifndef ZETA_FUTURES_COROUTINE_H
#define ZETA_FUTURES_COROUTINE_H

/// @file   futures/coroutine.h
/// @brief  C++20 coroutine adapters for zeta::Future.

#include "zeta/futures/future.h"

#include <coroutine>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace zeta {

template <typename T>
class Coroutine;

namespace detail {

template <typename T>
struct CancellationRaceState {
    explicit CancellationRaceState(Promise<T> promise)
        : promise(std::move(promise)) {}

    Promise<T> promise;
    std::atomic<bool> completed{false};
    std::mutex registration_mutex;
    CancellationRegistration registration;

    void ResetRegistration() noexcept {
        std::lock_guard<std::mutex> lock(registration_mutex);
        registration.Reset();
    }

    void SetRegistration(CancellationRegistration value) noexcept {
        std::lock_guard<std::mutex> lock(registration_mutex);
        if (completed.load(std::memory_order_acquire)) {
            value.Reset();
            return;
        }
        registration = std::move(value);
    }
};

} // namespace detail

/// Returns a Future that completes with `source`, or with cancellation if the
/// token is requested first. The source producer is not interrupted.
template <typename T>
[[nodiscard]] Future<T> WithCancellation(
    Future<T> source,
    CancellationToken token) {
    auto [promise, output] = makePromiseContract<T>();
    auto state = std::make_shared<detail::CancellationRaceState<T>>(
        std::move(promise));

    (void)std::move(source).ThenTry(
        [state](StatusOr<T> result) mutable {
            if (!state->completed.exchange(true, std::memory_order_acq_rel)) {
                (void)state->promise.SetResult(std::move(result));
                state->ResetRegistration();
            }
        });

    auto registration = token.Register([state] {
        if (!state->completed.exchange(true, std::memory_order_acq_rel)) {
            (void)state->promise.Cancel();
        }
        state->ResetRegistration();
    });
    state->SetRegistration(std::move(registration));
    return std::move(output);
}

/// Returns a Future that completes with `source` or DeadlineExceededError
/// after `timeout`. The source producer is not interrupted.
template <typename T, typename Rep, typename Period>
[[nodiscard]] Future<T> WithTimeout(
    Future<T> source,
    ScheduledExecutor& scheduler,
    std::chrono::duration<Rep, Period> timeout) {
    auto cancellation = std::make_shared<CancellationSource>();
    auto timed_out = std::make_shared<std::atomic<bool>>(false);
    auto output = WithCancellation(std::move(source), cancellation->GetToken());
    try {
        const auto delay = std::chrono::duration_cast<
            std::chrono::steady_clock::duration>(timeout);
        scheduler.ScheduleAfter(delay, [cancellation, timed_out] {
            timed_out->store(true, std::memory_order_release);
            (void)cancellation->RequestCancellation();
        });
    } catch (...) {
        timed_out->store(true, std::memory_order_release);
        (void)cancellation->RequestCancellation();
    }

    // Translate cancellation caused by the timer into a deadline status while
    // preserving any source result that won the race.
    return std::move(output).ThenTry(
        [timed_out](StatusOr<T> result) -> StatusOr<T> {
        if (result.ok()) return result;
        if (timed_out->load(std::memory_order_acquire) &&
            result.status().code() == StatusCode::kCancelled) {
            return DeadlineExceededError("future timed out");
        }
        return std::move(result).status();
        });
}

template <typename T, typename Rep, typename Period>
[[nodiscard]] Future<T> WithTimeout(
    SemiFuture<T> source,
    ScheduledExecutor& scheduler,
    std::chrono::duration<Rep, Period> timeout) {
    return WithTimeout(
        detail::ToFuturePreservingExecutor(std::move(source)),
        scheduler,
        timeout);
}

template <typename T>
[[nodiscard]] Future<T> WithCancellation(
    SemiFuture<T> source,
    CancellationToken token) {
    return WithCancellation(
        detail::ToFuturePreservingExecutor(std::move(source)), token);
}

/// Coroutine-friendly wrappers for the standard Future fan-in operations.
template <typename T>
[[nodiscard]] Coroutine<detail::WhenAllResultT<T>> coWhenAll(
    std::vector<Future<T>> futures);

template <typename T>
[[nodiscard]] Coroutine<std::vector<StatusOr<T>>> coCollectAll(
    std::vector<Future<T>> futures);

template <typename T>
[[nodiscard]] Coroutine<detail::IndexedResult<T>> coCollectAny(
    std::vector<Future<T>> futures);

/// A single-shot coroutine whose result is exposed as a Future.
///
/// The coroutine starts immediately and completes its returned Future when it
/// reaches co_return or throws. Coroutine functions should return Coroutine<T>.
template <typename T>
class Coroutine {
public:
    struct promise_type {
        Promise<T> promise;

        Coroutine get_return_object() {
            return Coroutine(promise.GetFuture());
        }

        std::suspend_never initial_suspend() const noexcept { return {}; }
        std::suspend_never final_suspend() const noexcept { return {}; }

        void unhandled_exception() noexcept {
            (void)promise.SetError(InternalError("coroutine failed with an exception"));
        }

        template <typename U>
        void return_value(U&& value) {
            (void)promise.SetValue(std::forward<U>(value));
        }
    };

    Coroutine(Coroutine&&) noexcept = default;
    Coroutine& operator=(Coroutine&&) noexcept = default;
    Coroutine(const Coroutine&) = delete;
    Coroutine& operator=(const Coroutine&) = delete;

    [[nodiscard]] Future<T> GetFuture() && { return std::move(future_); }

private:
    explicit Coroutine(Future<T> future) : future_(std::move(future)) {}

    Future<T> future_;
};

template <>
class Coroutine<void> {
public:
    struct promise_type {
        Promise<void> promise;

        Coroutine get_return_object() {
            return Coroutine(promise.GetFuture());
        }

        std::suspend_never initial_suspend() const noexcept { return {}; }
        std::suspend_never final_suspend() const noexcept { return {}; }

        void unhandled_exception() noexcept {
            (void)promise.SetError(InternalError("coroutine failed with an exception"));
        }

        void return_void() { (void)promise.SetValue(); }
    };

    Coroutine(Coroutine&&) noexcept = default;
    Coroutine& operator=(Coroutine&&) noexcept = default;
    Coroutine(const Coroutine&) = delete;
    Coroutine& operator=(const Coroutine&) = delete;

    [[nodiscard]] Future<void> GetFuture() && { return std::move(future_); }

private:
    explicit Coroutine(Future<void> future) : future_(std::move(future)) {}

    Future<void> future_;
};

template <typename T>
Coroutine<detail::WhenAllResultT<T>> coWhenAll(
    std::vector<Future<T>> futures) {
    auto result = co_await whenAll(std::move(futures));
    if (!result.ok()) co_return result.status();
    if constexpr (std::is_void_v<T>) {
        co_return;
    } else {
        co_return std::move(result).value();
    }
}

template <typename T>
Coroutine<std::vector<StatusOr<T>>> coCollectAll(
    std::vector<Future<T>> futures) {
    auto result = co_await collectAll(std::move(futures));
    if (!result.ok()) co_return result.status();
    co_return std::move(result).value();
}

template <typename T>
Coroutine<detail::IndexedResult<T>> coCollectAny(
    std::vector<Future<T>> futures) {
    auto result = co_await collectAny(std::move(futures));
    if (!result.ok()) co_return result.status();
    co_return std::move(result).value();
}

namespace detail {

template <typename T>
struct FutureAwaitState {
    std::optional<StatusOr<T>> result;
};

template <typename T>
class FutureAwaiter {
public:
    explicit FutureAwaiter(Future<T> future) : future_(std::move(future)) {}

    bool await_ready() const { return future_.IsReady(); }

    void await_suspend(std::coroutine_handle<> handle) {
        state_ = std::make_shared<FutureAwaitState<T>>();
        auto state = state_;
        (void)std::move(future_).ThenTry(
            [state, handle](StatusOr<T> result) mutable {
                state->result.emplace(std::move(result));
                handle.resume();
            });
    }

    StatusOr<T> await_resume() {
        if (state_ != nullptr) return std::move(*state_->result);
        return std::move(future_).Get();
    }

private:
    Future<T> future_;
    std::shared_ptr<FutureAwaitState<T>> state_;
};

} // namespace detail

template <typename T>
[[nodiscard]] detail::FutureAwaiter<T> operator co_await(Future<T>&& future) {
    return detail::FutureAwaiter<T>(std::move(future));
}

/// Awaits a SemiFuture while preserving the executor selected by Via().
///
/// This makes the following pattern resume the coroutine on the borrowed
/// executor rather than on the thread that fulfills the source promise:
/// `co_await std::move(std::move(future).Via(executor))`.
template <typename T>
[[nodiscard]] detail::FutureAwaiter<T> operator co_await(SemiFuture<T>&& future) {
    return detail::FutureAwaiter<T>(
        detail::ToFuturePreservingExecutor(std::move(future)));
}

} // namespace zeta

#endif // ZETA_FUTURES_COROUTINE_H
