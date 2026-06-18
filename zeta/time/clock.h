#ifndef ZETA_TIME_CLOCK_H
#define ZETA_TIME_CLOCK_H

/// @file   time/clock.h
/// @brief  zeta::Clock — monotonic and wall-clock time source.
///
/// `zeta::Clock` wraps the OS high-resolution timer and provides
/// a stable `int64_t` nanosecond timestamp that never goes backward.
///
/// A separate `RealClock` provides wall-clock time (may jump due to NTP
/// or manual adjustments).  Use `Clock` for interval measurement;
/// use `RealClock` for logging and deadlines.
///
/// Example:
///   auto start = zeta::Clock::Now();
///   DoWork();
///   zeta::Duration elapsed = zeta::Clock::Now() - start;
///   LOG(INFO) << "took " << elapsed.ToMilliseconds() << " ms";

#include <chrono>
#include <cstdint>

#include "zeta/time/duration.h"

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// Clock — monotonic, suitable for interval measurement
// ═══════════════════════════════════════════════════════════════════════

/// Monotonic clock.
///
/// The underlying clock source is `std::chrono::steady_clock`.  Unlike
/// `std::chrono::system_clock` (wall clock), this clock is guaranteed
/// to only increase and is not affected by NTP or manual time changes.
///
/// A `Time` value is an opaque `int64_t` — compare or subtract, but
/// don't interpret it as a calendar time.
class Clock {
public:
    using rep        = int64_t;
    using period     = std::nano;
    using duration   = Duration;
    using time_point = rep;  // opaque int64_t nanosecond count

    Clock() = delete;

    /// Current monotonic time in nanoseconds since an arbitrary epoch.
    [[nodiscard]] static time_point Now() noexcept {
        auto tp = std::chrono::steady_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(tp).count();
    }

    /// Convert a time point to an absolute Duration (since epoch).
    [[nodiscard]] static Duration ToDuration(time_point t) noexcept {
        return Duration::FromRaw(t);
    }

    /// Compute the Duration between two time points: `b - a`.
    [[nodiscard]] static Duration Between(time_point a,
                                          time_point b) noexcept {
        return Duration::FromRaw(b - a);
    }
};

// ═══════════════════════════════════════════════════════════════════════
// RealClock — wall-clock, may jump
// ═══════════════════════════════════════════════════════════════════════

/// Wall clock suitable for timestamps and deadlines.
///
/// Uses `std::chrono::system_clock`.  Values may decrease after NTP
/// adjustments or daylight-saving transitions.  Do NOT use for interval
/// measurement — use `Clock` for that.
class RealClock {
public:
    using rep        = int64_t;
    using period     = std::nano;
    using duration   = Duration;
    using time_point = rep;

    RealClock() = delete;

    /// Current wall-clock time in nanoseconds since Unix epoch.
    [[nodiscard]] static time_point Now() noexcept {
        auto tp = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(tp).count();
    }

    /// Convert to Duration (from Unix epoch).
    [[nodiscard]] static Duration ToDuration(time_point t) noexcept {
        return Duration::FromRaw(t);
    }
};

} // namespace zeta

#endif // ZETA_TIME_CLOCK_H
