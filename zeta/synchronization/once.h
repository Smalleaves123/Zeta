#ifndef ZETA_SYNCHRONIZATION_ONCE_H
#define ZETA_SYNCHRONIZATION_ONCE_H

/// @file   synchronization/once.h
/// @brief  One-time initialisation.

#include <atomic>
#include <mutex>

namespace zeta {

// Forward declaration for friend.
template <typename F>
void CallOnce(class OnceFlag& flag, F&& fn) noexcept(noexcept(fn()));

// ═══════════════════════════════════════════════════════════════════════
// OnceFlag
// ═══════════════════════════════════════════════════════════════════════

class OnceFlag {
public:
    OnceFlag() noexcept : state_(0) {}

    OnceFlag(const OnceFlag&) = delete;
    OnceFlag& operator=(const OnceFlag&) = delete;

private:
    template <typename F>
    friend void CallOnce(OnceFlag& flag, F&& fn) noexcept(noexcept(fn()));

    std::atomic<int> state_;
    std::mutex       mu_;
};

// ═══════════════════════════════════════════════════════════════════════
// CallOnce
// ═══════════════════════════════════════════════════════════════════════

template <typename F>
void CallOnce(OnceFlag& flag, F&& fn) noexcept(noexcept(fn())) {
    if (flag.state_.load(std::memory_order_acquire) == 2) return;
    std::lock_guard<std::mutex> lock(flag.mu_);
    if (flag.state_.load(std::memory_order_relaxed) == 0) {
        flag.state_.store(1, std::memory_order_relaxed);
        fn();
        flag.state_.store(2, std::memory_order_release);
    }
}

} // namespace zeta

#endif // ZETA_SYNCHRONIZATION_ONCE_H
