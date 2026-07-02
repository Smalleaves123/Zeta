#ifndef ZETA_TIME_RETRY_H
#define ZETA_TIME_RETRY_H

/// @file   time/retry.h
/// @brief  Retry policy helpers built on top of Deadline and Backoff.

#include "zeta/time/backoff.h"

#include <cstddef>
#include <limits>

namespace zeta {

class RetryPolicy {
public:
    static constexpr std::size_t kUnlimitedAttempts =
        std::numeric_limits<std::size_t>::max();

    RetryPolicy(ExponentialBackoff backoff,
                std::size_t max_attempts = kUnlimitedAttempts,
                Deadline deadline = Deadline::Never()) noexcept
        : backoff_(backoff),
          max_attempts_(max_attempts),
          deadline_(deadline) {}

    [[nodiscard]] bool CanRetry() const noexcept {
        return attempts_ < max_attempts_ && !deadline_.Expired();
    }

    [[nodiscard]] Duration NextDelay() const noexcept {
        if (!CanRetry()) return Duration();
        const auto delay = backoff_.Peek();
        const auto remaining = deadline_.Remaining();
        if (remaining == Duration::Infinite()) return delay;
        return Min(delay, remaining);
    }

    [[nodiscard]] Deadline NextDeadline() const noexcept {
        const auto delay = NextDelay();
        return delay == Duration::Infinite() ? Deadline::Never()
                                             : Deadline::After(delay);
    }

    void Advance() noexcept {
        if (!CanRetry()) return;
        (void)backoff_.NextDelay();
        ++attempts_;
    }

    void Sleep() noexcept {
        Clock::SleepFor(NextDelay());
        Advance();
    }

    void Reset() noexcept {
        backoff_.Reset();
        attempts_ = 0;
    }

    [[nodiscard]] std::size_t attempts() const noexcept {
        return attempts_;
    }

    [[nodiscard]] std::size_t max_attempts() const noexcept {
        return max_attempts_;
    }

    [[nodiscard]] Duration remaining() const noexcept {
        return deadline_.Remaining();
    }

    [[nodiscard]] const ExponentialBackoff& backoff() const noexcept {
        return backoff_;
    }

    [[nodiscard]] Deadline deadline() const noexcept {
        return deadline_;
    }

private:
    ExponentialBackoff backoff_;
    std::size_t max_attempts_;
    std::size_t attempts_ = 0;
    Deadline deadline_;
};

} // namespace zeta

#endif // ZETA_TIME_RETRY_H
