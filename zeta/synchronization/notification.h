#ifndef ZETA_SYNCHRONIZATION_NOTIFICATION_H
#define ZETA_SYNCHRONIZATION_NOTIFICATION_H

/// @file   synchronization/notification.h
/// @brief  One-shot notification — lighter than `std::condition_variable`.
///
/// `zeta::Notification` allows one or more threads to wait for a single
/// event.  Once `Notify()` is called, all past and future waiters return
/// immediately.  It cannot be reset.
///
/// Example:
///   zeta::Notification done;
///   std::thread worker([&] { DoWork(); done.Notify(); });
///   done.WaitForNotification();  // blocks until Notify() called
///   worker.join();

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace zeta {

class Notification {
public:
    Notification() : notified_(false) {}

    Notification(const Notification&) = delete;
    Notification& operator=(const Notification&) = delete;

    /// True if `Notify()` has been called.
    [[nodiscard]] bool HasBeenNotified() const noexcept {
        return notified_.load(std::memory_order_acquire);
    }

    /// Block until `Notify()` is called.  Returns immediately if already
    /// notified.
    void WaitForNotification() noexcept {
        if (HasBeenNotified()) return;
        std::unique_lock<std::mutex> lock(mu_);
        cv_.wait(lock, [this] { return notified_.load(std::memory_order_acquire); });
    }

    /// Block with a timeout.  Returns `true` if notified, `false` on timeout.
    template <typename Rep, typename Period>
    bool WaitForNotificationWithTimeout(
        std::chrono::duration<Rep, Period> timeout) noexcept {
        if (HasBeenNotified()) return true;
        std::unique_lock<std::mutex> lock(mu_);
        return cv_.wait_for(lock, timeout,
                            [this] { return notified_.load(std::memory_order_acquire); });
    }

    /// Signal the event.  All current and future waiters will unblock.
    /// Safe to call multiple times — subsequent calls are no-ops.
    void Notify() noexcept {
        if (!notified_.exchange(true, std::memory_order_acq_rel)) {
            std::lock_guard<std::mutex> lock(mu_);
            cv_.notify_all();
        }
    }

private:
    std::atomic<bool>        notified_;
    std::mutex               mu_;
    std::condition_variable  cv_;
};

} // namespace zeta

#endif // ZETA_SYNCHRONIZATION_NOTIFICATION_H
