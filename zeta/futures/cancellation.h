#ifndef ZETA_FUTURES_CANCELLATION_H
#define ZETA_FUTURES_CANCELLATION_H

/// @file   futures/cancellation.h
/// @brief  Cooperative cancellation source and observation token.

#include <atomic>
#include <memory>
#include <utility>

namespace zeta {

namespace detail {

struct CancellationState {
    std::atomic<bool> requested{false};
};

} // namespace detail

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

class CancellationSource {
public:
    CancellationSource()
        : state_(std::make_shared<detail::CancellationState>()) {}

    [[nodiscard]] CancellationToken GetToken() const noexcept {
        return CancellationToken(state_);
    }

    [[nodiscard]] bool RequestCancellation() noexcept {
        bool expected = false;
        return state_->requested.compare_exchange_strong(
            expected, true, std::memory_order_release,
            std::memory_order_relaxed);
    }

private:
    std::shared_ptr<detail::CancellationState> state_;
};

} // namespace zeta

#endif // ZETA_FUTURES_CANCELLATION_H
