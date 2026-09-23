#ifndef ZETA_FUTURES_CANCELLATION_H
#define ZETA_FUTURES_CANCELLATION_H

/// @file   futures/cancellation.h
/// @brief  Cooperative cancellation source and observation token.

#include <atomic>
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace zeta {

class CancellationRegistration;

namespace detail {

struct CancellationCallback {
    std::size_t id;
    std::function<void()> callback;
};

struct CancellationState {
    std::mutex mutex;
    bool requested = false;
    std::size_t next_callback_id = 1;
    std::vector<CancellationCallback> callbacks;
};

} // namespace detail

class CancellationRegistration {
public:
    CancellationRegistration() = default;
    CancellationRegistration(const CancellationRegistration&) = delete;
    CancellationRegistration& operator=(const CancellationRegistration&) = delete;

    CancellationRegistration(CancellationRegistration&& other) noexcept
        : state_(std::move(other.state_))
        , id_(std::exchange(other.id_, 0)) {}

    CancellationRegistration& operator=(CancellationRegistration&& other) noexcept {
        if (this != &other) {
            Reset();
            state_ = std::move(other.state_);
            id_ = std::exchange(other.id_, 0);
        }
        return *this;
    }

    ~CancellationRegistration() {
        Reset();
    }

    void Reset() noexcept {
        if (state_ == nullptr || id_ == 0) return;
        std::lock_guard<std::mutex> lock(state_->mutex);
        auto& callbacks = state_->callbacks;
        callbacks.erase(
            std::remove_if(callbacks.begin(), callbacks.end(),
                           [this](const auto& callback) {
                               return callback.id == id_;
                           }),
            callbacks.end());
        state_.reset();
        id_ = 0;
    }

private:
    CancellationRegistration(
        std::shared_ptr<detail::CancellationState> state,
        std::size_t id)
        : state_(std::move(state)), id_(id) {}

    std::shared_ptr<detail::CancellationState> state_;
    std::size_t id_ = 0;

    friend class CancellationToken;
};

class CancellationToken {
public:
    CancellationToken() = default;

    [[nodiscard]] bool IsCancellationRequested() const noexcept {
        if (state_ == nullptr) return false;
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->requested;
    }

    /// Registers a callback that runs once when cancellation is requested.
    /// If cancellation already happened, the callback runs synchronously.
    /// Callback exceptions are contained so registration has the same
    /// non-throwing behavior as CancellationSource::RequestCancellation().
    [[nodiscard]] CancellationRegistration Register(
        std::function<void()> callback) const {
        if (state_ == nullptr || !callback) return {};

        std::size_t id = 0;
        bool invoke_now = false;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (state_->requested) {
                invoke_now = true;
            } else {
                id = state_->next_callback_id++;
                state_->callbacks.push_back(
                    detail::CancellationCallback{id, std::move(callback)});
            }
        }

        if (invoke_now) {
            try {
                callback();
            } catch (...) {
            }
        }
        return CancellationRegistration(
            invoke_now ? nullptr : state_, invoke_now ? 0 : id);
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
        std::vector<detail::CancellationCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (state_->requested) return false;
            state_->requested = true;
            callbacks = std::move(state_->callbacks);
        }

        for (auto& callback : callbacks) {
            try {
                callback.callback();
            } catch (...) {
            }
        }
        return true;
    }

private:
    std::shared_ptr<detail::CancellationState> state_;
};

} // namespace zeta

#endif // ZETA_FUTURES_CANCELLATION_H
