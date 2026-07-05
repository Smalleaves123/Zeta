#ifndef ZETA_FUTURES_FUTURE_H
#define ZETA_FUTURES_FUTURE_H

/// @file   futures/future.h
/// @brief  Single-shot Future / Promise contract with Folly-style chaining.
///
/// `zeta::Promise<T>` and `zeta::Future<T>` provide a lightweight shared-state
/// contract for one producer and one consumer chain.  The implementation is
/// intentionally small and header-only:
///   - thread-safe value / error fulfillment
///   - blocking `Wait()` / timed `WaitFor()`
///   - `Get()` for value retrieval
///   - `Then()` for simple continuation chaining
///
/// The contract is single-consumer by design.  `Future<T>` is move-only, which
/// keeps move-only payloads practical without forcing copy semantics.

#include "zeta/status/status.h"
#include "zeta/status/statusor.h"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

namespace zeta {

template <typename T>
class Future;

template <typename T>
class SemiFuture;

template <typename T>
class Promise;

class Executor {
public:
    virtual ~Executor() = default;
    virtual void Add(std::function<void()> task) = 0;
};

class InlineExecutor final : public Executor {
public:
    void Add(std::function<void()> task) override { task(); }
};

template <typename T>
std::pair<Promise<T>, Future<T>> makePromiseContract();

namespace detail {

template <typename T>
struct IsStatusOr : std::false_type {};

template <typename U>
struct IsStatusOr<StatusOr<U>> : std::true_type {};

template <typename T>
inline constexpr bool IsStatusOrV = IsStatusOr<std::remove_cvref_t<T>>::value;

template <typename T>
struct UnwrapStatusOr {
    using type = T;
};

template <typename U>
struct UnwrapStatusOr<StatusOr<U>> {
    using type = U;
};

template <typename T>
using UnwrapStatusOrT = typename UnwrapStatusOr<std::remove_cvref_t<T>>::type;

template <typename T>
struct FutureState {
    mutable std::mutex mutex;
    std::condition_variable cv;
    bool ready = false;
    bool consumed = false;
    std::optional<StatusOr<T>> result;

    struct ContinuationBase {
        virtual ~ContinuationBase() = default;
        virtual Executor* executor() const noexcept = 0;
        virtual void Run(StatusOr<T> source) = 0;
    };

    std::unique_ptr<ContinuationBase> continuation;
};

template <typename T>
inline void DispatchContinuation(
    std::unique_ptr<typename FutureState<T>::ContinuationBase> continuation,
    StatusOr<T> source) {
    if (continuation == nullptr) {
        return;
    }

    if (Executor* executor = continuation->executor(); executor != nullptr) {
        auto shared_continuation =
            std::shared_ptr<typename FutureState<T>::ContinuationBase>(std::move(continuation));
        auto shared_source = std::make_shared<StatusOr<T>>(std::move(source));
        executor->Add([shared_continuation, shared_source]() mutable {
            shared_continuation->Run(std::move(*shared_source));
        });
        return;
    }

    continuation->Run(std::move(source));
}

template <typename T>
inline StatusOr<T> MakeBrokenPromiseResult() {
    return StatusOr<T>(CancelledError("broken promise"));
}

template <>
inline StatusOr<void> MakeBrokenPromiseResult<void>() {
    return StatusOr<void>(CancelledError("broken promise"));
}

template <typename T>
inline StatusOr<T> MakeConsumedFutureResult() {
    return StatusOr<T>(FailedPreconditionError("future already consumed"));
}

template <>
inline StatusOr<void> MakeConsumedFutureResult<void>() {
    return StatusOr<void>(FailedPreconditionError("future already consumed"));
}

template <typename T>
inline StatusOr<T> MakeInvalidFutureResult() {
    return StatusOr<T>(InternalError("future is not valid"));
}

template <>
inline StatusOr<void> MakeInvalidFutureResult<void>() {
    return StatusOr<void>(InternalError("future is not valid"));
}

} // namespace detail

// ═══════════════════════════════════════════════════════════════════════
// Promise
// ═══════════════════════════════════════════════════════════════════════

template <typename T>
class Promise {
public:
    Promise()
        : state_(std::make_shared<detail::FutureState<T>>()) {}

    Promise(const Promise&) = delete;
    Promise& operator=(const Promise&) = delete;

    Promise(Promise&& other) noexcept
        : state_(std::move(other.state_))
        , satisfied_(std::exchange(other.satisfied_, true))
        , future_taken_(std::exchange(other.future_taken_, true)) {}

    Promise& operator=(Promise&& other) noexcept {
        if (this != &other) {
            if (state_ != nullptr && !satisfied_ && !state_->ready) {
                (void)SetError(CancelledError("broken promise"));
            }
            state_ = std::move(other.state_);
            satisfied_ = std::exchange(other.satisfied_, true);
            future_taken_ = std::exchange(other.future_taken_, true);
        }
        return *this;
    }

    ~Promise() {
        if (state_ != nullptr && !satisfied_ && !state_->ready) {
            (void)SetError(CancelledError("broken promise"));
        }
    }

    [[nodiscard]] Future<T> GetFuture() const& {
        assert(!future_taken_);
        future_taken_ = true;
        return Future<T>(state_);
    }

    [[nodiscard]] Status SetError(Status status) {
        if (status.ok()) {
            return FailedPreconditionError("Promise::SetError requires an error status");
        }
        return SetResult(StatusOr<T>(std::move(status)));
    }

    template <typename U = T>
        requires (!std::is_void_v<U>)
    [[nodiscard]] Status SetValue(U value) {
        return SetResult(StatusOr<T>(std::move(value)));
    }

    [[nodiscard]] Status SetValue()
        requires std::is_void_v<T>
    {
        return SetResult(StatusOr<void>());
    }

    [[nodiscard]] Status SetResult(StatusOr<T> result) {
        if (state_ == nullptr) {
            return FailedPreconditionError("promise is not valid");
        }

        std::unique_ptr<typename detail::FutureState<T>::ContinuationBase> continuation;
        std::optional<StatusOr<T>> ready_result;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (state_->ready) {
                return FailedPreconditionError("promise already satisfied");
            }

            state_->result.emplace(std::move(result));
            state_->ready = true;
            satisfied_ = true;

            if (state_->continuation != nullptr) {
                continuation = std::move(state_->continuation);
                ready_result.emplace(std::move(*state_->result));
                state_->result.reset();
            }
        }

        state_->cv.notify_all();

        if (continuation != nullptr) {
            detail::DispatchContinuation<T>(std::move(continuation), std::move(*ready_result));
        }

        return OkStatus();
    }

private:
    explicit Promise(std::shared_ptr<detail::FutureState<T>> state)
        : state_(std::move(state)) {}

    std::shared_ptr<detail::FutureState<T>> state_;
    bool satisfied_ = false;
    mutable bool future_taken_ = false;

    friend class Future<T>;
    friend std::pair<Promise<T>, Future<T>> makePromiseContract<T>();
};

// ═══════════════════════════════════════════════════════════════════════
// Future
// ═══════════════════════════════════════════════════════════════════════

template <typename T>
class Future {
public:
    Future() = default;

    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;

    Future(Future&&) noexcept = default;
    Future& operator=(Future&&) noexcept = default;

    [[nodiscard]] bool valid() const noexcept {
        return state_ != nullptr;
    }

    [[nodiscard]] bool IsReady() const {
        if (state_ == nullptr) {
            return false;
        }
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->ready;
    }

    void Wait() const {
        if (state_ == nullptr) {
            return;
        }

        std::unique_lock<std::mutex> lock(state_->mutex);
        state_->cv.wait(lock, [this] { return state_->ready; });
    }

    template <typename Rep, typename Period>
    [[nodiscard]] bool WaitFor(std::chrono::duration<Rep, Period> timeout) const {
        if (state_ == nullptr) {
            return false;
        }

        std::unique_lock<std::mutex> lock(state_->mutex);
        return state_->cv.wait_for(lock, timeout, [this] { return state_->ready; });
    }

    [[nodiscard]] StatusOr<T> Get() && {
        if (state_ == nullptr) {
            return detail::MakeInvalidFutureResult<T>();
        }

        std::unique_lock<std::mutex> lock(state_->mutex);
        if (state_->consumed) {
            return detail::MakeConsumedFutureResult<T>();
        }

        state_->cv.wait(lock, [this] { return state_->ready; });
        state_->consumed = true;

        if (!state_->result.has_value()) {
            return detail::MakeInvalidFutureResult<T>();
        }

        StatusOr<T> out = std::move(*state_->result);
        state_->result.reset();
        return out;
    }

    template <typename F>
    [[nodiscard]] auto Then(F&& f) && {
        using StoredF = std::decay_t<F>;
        using RawResult = std::remove_cvref_t<
            decltype(InvokeCallable(std::declval<StoredF&>(), std::declval<StatusOr<T>&&>()))>;
        using Out = detail::UnwrapStatusOrT<RawResult>;

        auto next = makePromiseContract<Out>();
        Promise<Out> promise = std::move(next.first);
        Future<Out> future = std::move(next.second);

        if (state_ == nullptr) {
            (void)promise.SetError(InternalError("future is not valid"));
            return future;
        }

        struct Continuation final : detail::FutureState<T>::ContinuationBase {
            Continuation(StoredF fn, Promise<Out> next_promise)
                : fn_(std::move(fn))
                , next_promise_(std::move(next_promise)) {}

            Executor* executor() const noexcept override { return nullptr; }

            void Run(StatusOr<T> source) override {
                if (!source.ok()) {
                    (void)next_promise_.SetError(std::move(source).status());
                    return;
                }

                if constexpr (std::is_void_v<T>) {
                    if constexpr (detail::IsStatusOrV<RawResult>) {
                        (void)next_promise_.SetResult(InvokeCallable(fn_, std::move(source)));
                    } else if constexpr (std::is_void_v<RawResult>) {
                        InvokeCallable(fn_, std::move(source));
                        (void)next_promise_.SetValue();
                    } else {
                        (void)next_promise_.SetValue(InvokeCallable(fn_, std::move(source)));
                    }
                } else {
                    if constexpr (detail::IsStatusOrV<RawResult>) {
                        (void)next_promise_.SetResult(InvokeCallable(fn_, std::move(source)));
                    } else if constexpr (std::is_void_v<RawResult>) {
                        InvokeCallable(fn_, std::move(source));
                        (void)next_promise_.SetValue();
                    } else {
                        (void)next_promise_.SetValue(InvokeCallable(fn_, std::move(source)));
                    }
                }
            }

            static decltype(auto) InvokeCallable(StoredF& fn, StatusOr<T>&& source) {
                if constexpr (std::is_void_v<T>) {
                    return std::invoke(fn);
                } else {
                    return std::invoke(fn, std::move(source).value());
                }
            }

            StoredF fn_;
            Promise<Out> next_promise_;
        };

        auto continuation = std::make_unique<Continuation>(std::forward<F>(f), std::move(promise));
        std::optional<StatusOr<T>> ready_result;

        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (state_->consumed) {
                auto failed = makePromiseContract<Out>();
                (void)failed.first.SetError(FailedPreconditionError("future already consumed"));
                return std::move(failed.second);
            }

            state_->consumed = true;

            if (state_->ready) {
                ready_result.emplace(std::move(*state_->result));
                state_->result.reset();
            } else {
                state_->continuation = std::move(continuation);
                return future;
            }
        }

        continuation->Run(std::move(*ready_result));
        return future;
    }

    [[nodiscard]] SemiFuture<T> Via(Executor& executor) &&;

private:
    explicit Future(std::shared_ptr<detail::FutureState<T>> state)
        : state_(std::move(state)) {}

    template <typename Callable>
    static decltype(auto) InvokeCallable(Callable& fn, StatusOr<T>&& source) {
        if constexpr (std::is_void_v<T>) {
            return std::invoke(fn);
        } else {
            return std::invoke(fn, std::move(source).value());
        }
    }

    std::shared_ptr<detail::FutureState<T>> state_;

    friend class Promise<T>;
    friend class SemiFuture<T>;
    friend std::pair<Promise<T>, Future<T>> makePromiseContract<T>();
};

// ═══════════════════════════════════════════════════════════════════════
// SemiFuture
// ═══════════════════════════════════════════════════════════════════════

template <typename T>
class SemiFuture {
public:
    SemiFuture() = default;

    SemiFuture(const SemiFuture&) = delete;
    SemiFuture& operator=(const SemiFuture&) = delete;

    SemiFuture(SemiFuture&&) noexcept = default;
    SemiFuture& operator=(SemiFuture&&) noexcept = default;

    [[nodiscard]] bool valid() const noexcept {
        return state_ != nullptr;
    }

    [[nodiscard]] bool IsReady() const {
        if (state_ == nullptr) {
            return false;
        }
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->ready;
    }

    void Wait() const {
        if (state_ == nullptr) {
            return;
        }

        std::unique_lock<std::mutex> lock(state_->mutex);
        state_->cv.wait(lock, [this] { return state_->ready; });
    }

    template <typename Rep, typename Period>
    [[nodiscard]] bool WaitFor(std::chrono::duration<Rep, Period> timeout) const {
        if (state_ == nullptr) {
            return false;
        }

        std::unique_lock<std::mutex> lock(state_->mutex);
        return state_->cv.wait_for(lock, timeout, [this] { return state_->ready; });
    }

    [[nodiscard]] StatusOr<T> Get() && {
        if (state_ == nullptr) {
            return detail::MakeInvalidFutureResult<T>();
        }

        std::unique_lock<std::mutex> lock(state_->mutex);
        if (state_->consumed) {
            return detail::MakeConsumedFutureResult<T>();
        }

        state_->cv.wait(lock, [this] { return state_->ready; });
        state_->consumed = true;

        if (!state_->result.has_value()) {
            return detail::MakeInvalidFutureResult<T>();
        }

        StatusOr<T> out = std::move(*state_->result);
        state_->result.reset();
        return out;
    }

    template <typename F>
    [[nodiscard]] auto Then(F&& f) && {
        using StoredF = std::decay_t<F>;
        using RawResult = std::remove_cvref_t<
            decltype(InvokeCallable(std::declval<StoredF&>(), std::declval<StatusOr<T>&&>()))>;
        using Out = detail::UnwrapStatusOrT<RawResult>;

        auto next = makePromiseContract<Out>();
        Promise<Out> promise = std::move(next.first);
        SemiFuture<Out> future(std::move(next.second), executor_);

        if (state_ == nullptr) {
            (void)promise.SetError(InternalError("future is not valid"));
            return future;
        }

        struct Continuation final : detail::FutureState<T>::ContinuationBase {
            Continuation(StoredF fn, Promise<Out> next_promise, Executor* executor)
                : fn_(std::move(fn))
                , next_promise_(std::move(next_promise))
                , executor_(executor) {}

            Executor* executor() const noexcept override { return executor_; }

            void Run(StatusOr<T> source) override {
                if (!source.ok()) {
                    (void)next_promise_.SetError(std::move(source).status());
                    return;
                }

                if constexpr (std::is_void_v<T>) {
                    if constexpr (detail::IsStatusOrV<RawResult>) {
                        (void)next_promise_.SetResult(InvokeCallable(fn_, std::move(source)));
                    } else if constexpr (std::is_void_v<RawResult>) {
                        InvokeCallable(fn_, std::move(source));
                        (void)next_promise_.SetValue();
                    } else {
                        (void)next_promise_.SetValue(InvokeCallable(fn_, std::move(source)));
                    }
                } else {
                    if constexpr (detail::IsStatusOrV<RawResult>) {
                        (void)next_promise_.SetResult(InvokeCallable(fn_, std::move(source)));
                    } else if constexpr (std::is_void_v<RawResult>) {
                        InvokeCallable(fn_, std::move(source));
                        (void)next_promise_.SetValue();
                    } else {
                        (void)next_promise_.SetValue(InvokeCallable(fn_, std::move(source)));
                    }
                }
            }

            static decltype(auto) InvokeCallable(StoredF& fn, StatusOr<T>&& source) {
                if constexpr (std::is_void_v<T>) {
                    return std::invoke(fn);
                } else {
                    return std::invoke(fn, std::move(source).value());
                }
            }

            StoredF fn_;
            Promise<Out> next_promise_;
            Executor* executor_ = nullptr;
        };

        auto continuation = std::make_unique<Continuation>(
            std::forward<F>(f), std::move(promise), executor_);
        std::optional<StatusOr<T>> ready_result;

        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (state_->consumed) {
                auto failed = makePromiseContract<Out>();
                (void)failed.first.SetError(FailedPreconditionError("future already consumed"));
                return SemiFuture<Out>(std::move(failed.second), executor_);
            }

            state_->consumed = true;

            if (state_->ready) {
                ready_result.emplace(std::move(*state_->result));
                state_->result.reset();
            } else {
                state_->continuation = std::move(continuation);
                return future;
            }
        }

        detail::DispatchContinuation<T>(std::move(continuation), std::move(*ready_result));
        return future;
    }

    [[nodiscard]] Future<T> ToFuture() && {
        return Future<T>(std::move(state_));
    }

    [[nodiscard]] SemiFuture<T> Via(Executor& executor) && {
        executor_ = &executor;
        return std::move(*this);
    }

private:
    explicit SemiFuture(std::shared_ptr<detail::FutureState<T>> state, Executor* executor)
        : state_(std::move(state))
        , executor_(executor) {}

    explicit SemiFuture(Future<T>&& future, Executor* executor)
        : state_(std::move(future.state_))
        , executor_(executor) {}

    template <typename Callable>
    static decltype(auto) InvokeCallable(Callable& fn, StatusOr<T>&& source) {
        if constexpr (std::is_void_v<T>) {
            return std::invoke(fn);
        } else {
            return std::invoke(fn, std::move(source).value());
        }
    }

    std::shared_ptr<detail::FutureState<T>> state_;
    Executor* executor_ = nullptr;

    friend class Future<T>;
    friend std::pair<Promise<T>, Future<T>> makePromiseContract<T>();
};

template <typename T>
[[nodiscard]] SemiFuture<T> Future<T>::Via(Executor& executor) && {
    return SemiFuture<T>(std::move(*this), &executor);
}

// ═══════════════════════════════════════════════════════════════════════
// Contract factory
// ═══════════════════════════════════════════════════════════════════════

template <typename T>
std::pair<Promise<T>, Future<T>> makePromiseContract() {
    auto state = std::make_shared<detail::FutureState<T>>();
    Promise<T> promise(state);
    promise.future_taken_ = true;
    return {std::move(promise), Future<T>(std::move(state))};
}

} // namespace zeta

#endif // ZETA_FUTURES_FUTURE_H
