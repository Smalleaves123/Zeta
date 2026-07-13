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
///   - cooperative cancellation and bounded `GetFor()` retrieval
///
/// The contract is single-consumer by design.  `Future<T>` is move-only, which
/// keeps move-only payloads practical without forcing copy semantics.

#include "zeta/status/status.h"
#include "zeta/status/statusor.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace zeta {

namespace detail {

struct CancellationState {
    std::atomic<bool> requested{false};
};

} // namespace detail

/// A read-only handle for observing a cooperative cancellation request.
class CancellationToken {
public:
    CancellationToken() = default;

    [[nodiscard]] bool IsCancellationRequested() const noexcept {
        return state_ != nullptr &&
               state_->requested.load(std::memory_order_acquire);
    }

private:
    explicit CancellationToken(std::shared_ptr<detail::CancellationState> state)
        : state_(std::move(state)) {}

    std::shared_ptr<detail::CancellationState> state_;

    friend class CancellationSource;
};

/// Owns a cancellation request and produces observation tokens.
class CancellationSource {
public:
    CancellationSource()
        : state_(std::make_shared<detail::CancellationState>()) {}

    [[nodiscard]] CancellationToken GetToken() const noexcept {
        return CancellationToken(state_);
    }

    /// Requests cancellation. Returns false if it was already requested.
    [[nodiscard]] bool RequestCancellation() noexcept {
        bool expected = false;
        return state_->requested.compare_exchange_strong(
            expected, true, std::memory_order_release,
            std::memory_order_relaxed);
    }

private:
    std::shared_ptr<detail::CancellationState> state_;
};

template <typename T>
class Future;

template <typename T>
class SemiFuture;

template <typename T>
class Promise;

class Executor {
public:
    virtual ~Executor() = default;

    /// Schedules one task. Executor is non-owning in Future/SemiFuture:
    /// callers must keep it alive until all attached continuations finish.
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
struct WhenAllResult {
    using type = std::vector<T>;
};

template <>
struct WhenAllResult<void> {
    using type = void;
};

template <typename T>
using WhenAllResultT = typename WhenAllResult<T>::type;

template <typename T>
using IndexedResult = std::pair<std::size_t, StatusOr<T>>;

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

template <typename T>
inline StatusOr<T> ConsumeFutureResult(
    std::unique_lock<std::mutex>& lock,
    const std::shared_ptr<FutureState<T>>& state) {
    if (state->consumed) {
        return MakeConsumedFutureResult<T>();
    }

    state->consumed = true;
    if (!state->result.has_value()) {
        return MakeInvalidFutureResult<T>();
    }

    StatusOr<T> out = std::move(*state->result);
    state->result.reset();
    return out;
}

template <typename T>
struct CollectAllSharedState {
    std::mutex mutex;
    std::size_t remaining = 0;
    std::vector<std::optional<StatusOr<T>>> results;
    std::shared_ptr<Promise<std::vector<StatusOr<T>>>> promise;
};

template <typename T>
inline Future<std::vector<StatusOr<T>>> StartCollectAll(
    std::vector<Future<T>> futures) {
    auto [promise, future] = makePromiseContract<std::vector<StatusOr<T>>>();

    if (futures.empty()) {
        (void)promise.SetValue({});
        return std::move(future);
    }

    auto shared = std::make_shared<CollectAllSharedState<T>>();
    shared->remaining = futures.size();
    shared->results.resize(futures.size());
    shared->promise =
        std::make_shared<Promise<std::vector<StatusOr<T>>>>(std::move(promise));

    for (std::size_t index = 0; index < futures.size(); ++index) {
        (void)std::move(futures[index]).ThenTry(
            [shared, index](StatusOr<T> result) {
                std::optional<std::vector<StatusOr<T>>> ready_results;
                {
                    std::lock_guard<std::mutex> lock(shared->mutex);
                    shared->results[index].emplace(std::move(result));
                    --shared->remaining;
                    if (shared->remaining == 0) {
                        std::vector<StatusOr<T>> values;
                        values.reserve(shared->results.size());
                        for (auto& item : shared->results) {
                            values.push_back(std::move(*item));
                        }
                        ready_results.emplace(std::move(values));
                    }
                }

                if (ready_results.has_value()) {
                    (void)shared->promise->SetValue(std::move(*ready_results));
                }
            });
    }

    return std::move(future);
}

template <typename T>
inline Future<WhenAllResultT<T>> StartWhenAll(
    std::vector<Future<T>> futures) {
    auto grouped = StartCollectAll<T>(std::move(futures));
    return std::move(grouped).ThenTry([](StatusOr<std::vector<StatusOr<T>>> result) {
        if (!result.ok()) {
            return StatusOr<WhenAllResultT<T>>(std::move(result).status());
        }

        auto values = std::move(result).value();
        if constexpr (std::is_void_v<T>) {
            for (auto& item : values) {
                if (!item.ok()) {
                    return StatusOr<void>(std::move(item).status());
                }
            }
            return StatusOr<void>();
        } else {
            std::vector<T> output;
            output.reserve(values.size());
            for (auto& item : values) {
                if (!item.ok()) {
                    return StatusOr<std::vector<T>>(std::move(item).status());
                }
                output.push_back(std::move(item).value());
            }
            return StatusOr<std::vector<T>>(std::move(output));
        }
    });
}

template <typename T>
struct CollectSomeSharedState {
    std::mutex mutex;
    std::size_t target = 0;
    bool done = false;
    std::vector<IndexedResult<T>> results;
    std::shared_ptr<Promise<std::vector<IndexedResult<T>>>> promise;
};

template <typename T>
inline Future<std::vector<IndexedResult<T>>> StartCollectN(
    std::vector<Future<T>> futures,
    std::size_t count) {
    auto [promise, future] = makePromiseContract<std::vector<IndexedResult<T>>>();

    if (count == 0) {
        (void)promise.SetValue({});
        return std::move(future);
    }

    if (futures.empty()) {
        (void)promise.SetError(InvalidArgumentError("collectN requires at least one future"));
        return std::move(future);
    }

    auto shared = std::make_shared<CollectSomeSharedState<T>>();
    shared->target = std::min(count, futures.size());
    shared->results.reserve(shared->target);
    shared->promise =
        std::make_shared<Promise<std::vector<IndexedResult<T>>>>(std::move(promise));

    for (std::size_t index = 0; index < futures.size(); ++index) {
        (void)std::move(futures[index]).ThenTry(
            [shared, index](StatusOr<T> result) {
                IndexedResult<T> completion(index, std::move(result));
                std::optional<std::vector<IndexedResult<T>>> ready_results;

                {
                    std::lock_guard<std::mutex> lock(shared->mutex);
                    if (shared->done) {
                        return;
                    }

                    shared->results.push_back(std::move(completion));
                    if (shared->results.size() == shared->target) {
                        shared->done = true;
                        ready_results.emplace(std::move(shared->results));
                    }
                }

                if (ready_results.has_value()) {
                    (void)shared->promise->SetValue(std::move(*ready_results));
                }
            });
    }

    return std::move(future);
}

template <typename T>
inline Future<IndexedResult<T>> StartCollectAny(
    std::vector<Future<T>> futures) {
    auto [promise, future] = makePromiseContract<IndexedResult<T>>();

    if (futures.empty()) {
        (void)promise.SetError(InvalidArgumentError("collectAny requires at least one future"));
        return std::move(future);
    }

    auto grouped = StartCollectN<T>(std::move(futures), 1);
    (void)std::move(grouped).ThenTry(
        [promise = std::move(promise)](StatusOr<std::vector<IndexedResult<T>>> results) mutable {
            if (!results.ok()) {
                (void)promise.SetError(std::move(results).status());
                return;
            }

            if (results.value().empty()) {
                (void)promise.SetError(InternalError("collectAny produced no results"));
                return;
            }

            (void)promise.SetValue(std::move(results.value().front()));
        });

    return std::move(future);
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

    /// Completes the contract with a cooperative cancellation status.
    [[nodiscard]] Status Cancel() {
        return SetError(CancelledError("future cancelled"));
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
        state_->cv.wait(lock, [this] { return state_->ready; });
        return detail::ConsumeFutureResult(lock, state_);
    }

    /// Waits for a value, timeout, or cooperative cancellation request.
    /// Cancellation does not terminate a running producer; it only stops this
    /// wait and returns a cancelled status.
    template <typename Rep, typename Period>
    [[nodiscard]] StatusOr<T> GetFor(
        std::chrono::duration<Rep, Period> timeout,
        CancellationToken token = {}) && {
        if (state_ == nullptr) {
            return detail::MakeInvalidFutureResult<T>();
        }

        std::unique_lock<std::mutex> lock(state_->mutex);
        const auto timeout_duration =
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout);
        const auto poll_interval = std::chrono::duration_cast<
            std::chrono::steady_clock::duration>(std::chrono::milliseconds(1));
        const auto deadline = std::chrono::steady_clock::now() + timeout_duration;

        while (!state_->ready) {
            if (token.IsCancellationRequested()) {
                return CancelledError("future wait cancelled");
            }
            if (timeout_duration <= std::chrono::steady_clock::duration::zero() ||
                std::chrono::steady_clock::now() >= deadline) {
                return DeadlineExceededError("future wait timed out");
            }

            const auto remaining = deadline - std::chrono::steady_clock::now();
            state_->cv.wait_for(lock, std::min(remaining, poll_interval));
        }

        return detail::ConsumeFutureResult(lock, state_);
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

    /// Attach a continuation that receives the complete success/error result.
    /// Unlike Then(), the callback is also invoked when the source contains an
    /// error. This is useful for fan-in combinators that must preserve errors.
    template <typename F>
    [[nodiscard]] auto ThenTry(F&& f) && {
        using StoredF = std::decay_t<F>;
        using RawResult = std::remove_cvref_t<
            std::invoke_result_t<StoredF&, StatusOr<T>>>;
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
                : fn_(std::move(fn)), next_promise_(std::move(next_promise)) {}

            Executor* executor() const noexcept override { return nullptr; }

            void Run(StatusOr<T> source) override {
                if constexpr (detail::IsStatusOrV<RawResult>) {
                    (void)next_promise_.SetResult(
                        std::invoke(fn_, std::move(source)));
                } else if constexpr (std::is_void_v<RawResult>) {
                    std::invoke(fn_, std::move(source));
                    (void)next_promise_.SetValue();
                } else {
                    (void)next_promise_.SetValue(
                        std::invoke(fn_, std::move(source)));
                }
            }

            StoredF fn_;
            Promise<Out> next_promise_;
        };

        auto continuation = std::make_unique<Continuation>(
            std::forward<F>(f), std::move(promise));
        std::optional<StatusOr<T>> ready_result;

        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (state_->consumed) {
                auto failed = makePromiseContract<Out>();
                (void)failed.first.SetError(
                    FailedPreconditionError("future already consumed"));
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

    /// Moves this future to a non-owning executor view. The executor must
    /// outlive the returned SemiFuture and every continuation attached to it.
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
        state_->cv.wait(lock, [this] { return state_->ready; });
        return detail::ConsumeFutureResult(lock, state_);
    }

    /// Waits for a value, timeout, or cooperative cancellation request.
    template <typename Rep, typename Period>
    [[nodiscard]] StatusOr<T> GetFor(
        std::chrono::duration<Rep, Period> timeout,
        CancellationToken token = {}) && {
        if (state_ == nullptr) {
            return detail::MakeInvalidFutureResult<T>();
        }

        std::unique_lock<std::mutex> lock(state_->mutex);
        const auto timeout_duration =
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout);
        const auto poll_interval = std::chrono::duration_cast<
            std::chrono::steady_clock::duration>(std::chrono::milliseconds(1));
        const auto deadline = std::chrono::steady_clock::now() + timeout_duration;

        while (!state_->ready) {
            if (token.IsCancellationRequested()) {
                return CancelledError("future wait cancelled");
            }
            if (timeout_duration <= std::chrono::steady_clock::duration::zero() ||
                std::chrono::steady_clock::now() >= deadline) {
                return DeadlineExceededError("future wait timed out");
            }

            const auto remaining = deadline - std::chrono::steady_clock::now();
            state_->cv.wait_for(lock, std::min(remaining, poll_interval));
        }

        return detail::ConsumeFutureResult(lock, state_);
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

    /// Attach a continuation that receives the complete success/error result.
    template <typename F>
    [[nodiscard]] auto ThenTry(F&& f) && {
        using StoredF = std::decay_t<F>;
        using RawResult = std::remove_cvref_t<
            std::invoke_result_t<StoredF&, StatusOr<T>>>;
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
                if constexpr (detail::IsStatusOrV<RawResult>) {
                    (void)next_promise_.SetResult(
                        std::invoke(fn_, std::move(source)));
                } else if constexpr (std::is_void_v<RawResult>) {
                    std::invoke(fn_, std::move(source));
                    (void)next_promise_.SetValue();
                } else {
                    (void)next_promise_.SetValue(
                        std::invoke(fn_, std::move(source)));
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
                (void)failed.first.SetError(
                    FailedPreconditionError("future already consumed"));
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

    /// The executor is borrowed, not owned; keep it alive until this chain is
    /// fully consumed and all scheduled continuations have finished.
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

template <typename T>
[[nodiscard]] Future<std::vector<StatusOr<T>>> collectAll(
    std::vector<Future<T>> futures) {
    return detail::StartCollectAll<T>(std::move(futures));
}

template <typename T>
[[nodiscard]] Future<std::vector<StatusOr<T>>> collectAll(
    std::vector<SemiFuture<T>> futures) {
    std::vector<Future<T>> converted;
    converted.reserve(futures.size());
    for (auto& future : futures) {
        converted.push_back(std::move(future).ToFuture());
    }
    return detail::StartCollectAll<T>(std::move(converted));
}

template <typename T>
[[nodiscard]] Future<detail::WhenAllResultT<T>> whenAll(
    std::vector<Future<T>> futures) {
    return detail::StartWhenAll<T>(std::move(futures));
}

template <typename T>
[[nodiscard]] Future<detail::WhenAllResultT<T>> whenAll(
    std::vector<SemiFuture<T>> futures) {
    std::vector<Future<T>> converted;
    converted.reserve(futures.size());
    for (auto& future : futures) {
        converted.push_back(std::move(future).ToFuture());
    }
    return detail::StartWhenAll<T>(std::move(converted));
}

template <typename T>
[[nodiscard]] Future<detail::IndexedResult<T>> collectAny(
    std::vector<Future<T>> futures) {
    return detail::StartCollectAny<T>(std::move(futures));
}

template <typename T>
[[nodiscard]] Future<detail::IndexedResult<T>> collectAny(
    std::vector<SemiFuture<T>> futures) {
    std::vector<Future<T>> converted;
    converted.reserve(futures.size());
    for (auto& future : futures) {
        converted.push_back(std::move(future).ToFuture());
    }
    return detail::StartCollectAny<T>(std::move(converted));
}

template <typename T>
[[nodiscard]] Future<std::vector<detail::IndexedResult<T>>> collectN(
    std::vector<Future<T>> futures,
    std::size_t count) {
    return detail::StartCollectN<T>(std::move(futures), count);
}

template <typename T>
[[nodiscard]] Future<std::vector<detail::IndexedResult<T>>> collectN(
    std::vector<SemiFuture<T>> futures,
    std::size_t count) {
    std::vector<Future<T>> converted;
    converted.reserve(futures.size());
    for (auto& future : futures) {
        converted.push_back(std::move(future).ToFuture());
    }
    return detail::StartCollectN<T>(std::move(converted), count);
}

} // namespace zeta

#endif // ZETA_FUTURES_FUTURE_H
