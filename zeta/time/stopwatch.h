#ifndef ZETA_TIME_STOPWATCH_H
#define ZETA_TIME_STOPWATCH_H

/// @file   time/stopwatch.h
/// @brief  Monotonic stopwatch and deadline helpers for request flow control.

#include "zeta/time/clock.h"

namespace zeta {

class Stopwatch {
public:
    Stopwatch() noexcept : start_(Clock::Now()) {}

    [[nodiscard]] static Stopwatch StartNew() noexcept {
        return Stopwatch();
    }

    void Reset() noexcept {
        start_ = Clock::Now();
    }

    [[nodiscard]] Duration Elapsed() const noexcept {
        return Clock::Between(start_, Clock::Now());
    }

    [[nodiscard]] Duration Restart() noexcept {
        const auto now = Clock::Now();
        const auto elapsed = Clock::Between(start_, now);
        start_ = now;
        return elapsed;
    }

    [[nodiscard]] Clock::time_point start_time() const noexcept {
        return start_;
    }

private:
    Clock::time_point start_;
};

class Deadline {
public:
    Deadline() noexcept : target_(Clock::Now()) {}

    explicit Deadline(Clock::time_point target) noexcept : target_(target) {}

    [[nodiscard]] static Deadline After(Duration timeout) noexcept {
        return Deadline(Clock::Now() + timeout.ToNanoseconds());
    }

    [[nodiscard]] static Deadline Never() noexcept {
        return Deadline(Duration::Infinite().ToRaw());
    }

    [[nodiscard]] bool Expired() const noexcept {
        return Clock::Now() >= target_;
    }

    [[nodiscard]] Duration Remaining() const noexcept {
        if (target_ == Duration::Infinite().ToRaw()) {
            return Duration::Infinite();
        }
        const auto now = Clock::Now();
        if (now >= target_) return Duration();
        return Clock::Between(now, target_);
    }

    void Sleep() const {
        if (target_ == Duration::Infinite().ToRaw()) return;
        Clock::SleepUntil(target_);
    }

    [[nodiscard]] Clock::time_point time_point() const noexcept {
        return target_;
    }

private:
    Clock::time_point target_;
};

} // namespace zeta

#endif // ZETA_TIME_STOPWATCH_H
