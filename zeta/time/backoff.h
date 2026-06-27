#ifndef ZETA_TIME_BACKOFF_H
#define ZETA_TIME_BACKOFF_H

/// @file   time/backoff.h
/// @brief  Exponential backoff helper for retries and reconnect loops.

#include "zeta/time/stopwatch.h"

#include <cstddef>
#include <cstdint>

namespace zeta {

class ExponentialBackoff {
public:
    ExponentialBackoff(Duration initial, Duration max, int64_t multiplier = 2) noexcept
        : initial_(initial <= Duration() ? Duration() : initial),
          max_(max < initial_ ? initial_ : max),
          multiplier_(multiplier < 1 ? 1 : multiplier),
          current_(initial_) {}

    [[nodiscard]] Duration Peek() const noexcept {
        return current_;
    }

    [[nodiscard]] Duration max_delay() const noexcept {
        return max_;
    }

    [[nodiscard]] std::size_t attempts() const noexcept {
        return attempts_;
    }

    [[nodiscard]] bool AtMax() const noexcept {
        return current_ >= max_;
    }

    [[nodiscard]] Duration NextDelay() noexcept {
        const auto out = current_;
        Advance();
        ++attempts_;
        return out;
    }

    [[nodiscard]] Deadline NextDeadline() noexcept {
        const auto delay = NextDelay();
        return delay == Duration::Infinite() ? Deadline::Never()
                                             : Deadline::After(delay);
    }

    void Sleep() noexcept {
        Clock::SleepFor(NextDelay());
    }

    void Reset() noexcept {
        current_ = initial_;
        attempts_ = 0;
    }

private:
    void Advance() noexcept {
        if (current_ >= max_) {
            current_ = max_;
            return;
        }
        current_ = Min(max_, current_ * multiplier_);
    }

    Duration initial_;
    Duration max_;
    int64_t multiplier_;
    Duration current_;
    std::size_t attempts_ = 0;
};

} // namespace zeta

#endif // ZETA_TIME_BACKOFF_H
