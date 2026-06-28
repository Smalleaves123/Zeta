#ifndef ZETA_TIME_PERIODIC_TIMER_H
#define ZETA_TIME_PERIODIC_TIMER_H

/// @file   time/periodic_timer.h
/// @brief  Fixed-interval timer for polling loops and background work.

#include "zeta/time/clock.h"
#include "zeta/time/internal/math_internal.h"

#include <cstdint>

namespace zeta {

class PeriodicTimer {
public:
    explicit PeriodicTimer(Duration interval) noexcept
        : interval_(interval <= Duration() ? Duration() : interval),
          next_(time_internal::AddDurationSaturating(Clock::Now(), interval_)) {}

    [[nodiscard]] static PeriodicTimer Every(Duration interval) noexcept {
        return PeriodicTimer(interval);
    }

    void Reset() noexcept {
        next_ = time_internal::AddDurationSaturating(Clock::Now(), interval_);
    }

    [[nodiscard]] bool Ready() const noexcept {
        if (interval_.IsZero()) return true;
        return Clock::Now() >= next_;
    }

    [[nodiscard]] Duration Remaining() const noexcept {
        if (interval_.IsZero()) return Duration();
        return time_internal::RemainingUntil(next_, Clock::Now());
    }

    [[nodiscard]] Duration interval() const noexcept {
        return interval_;
    }

    [[nodiscard]] Clock::time_point next_time() const noexcept {
        return next_;
    }

    [[nodiscard]] bool Poll() noexcept {
        if (!Ready()) return false;
        Advance();
        return true;
    }

    void Wait() const {
        if (interval_.IsZero()) return;
        while (!Ready()) {
            Clock::SleepUntil(next_);
        }
    }

    void WaitNext() {
        Wait();
        Advance();
    }

private:
    void Advance() noexcept {
        if (interval_.IsZero()) {
            next_ = Clock::Now();
            return;
        }

        const auto step = interval_.ToNanoseconds();
        const auto now = Clock::Now();
        if (now < next_) {
            next_ = time_internal::AddDurationSaturating(next_, interval_);
            return;
        }

        const auto overdue = now - next_;
        const int64_t ticks = overdue / step + 1;
        next_ = time_internal::AddDurationSaturating(next_, interval_ * ticks);
    }

    Duration interval_;
    Clock::time_point next_;
};

} // namespace zeta

#endif // ZETA_TIME_PERIODIC_TIMER_H
