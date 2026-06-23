#ifndef ZETA_TIME_TIMESTAMP_H
#define ZETA_TIME_TIMESTAMP_H

/// @file   time/timestamp.h
/// @brief  Wall-clock timestamp formatting helpers.
///
/// Provides small, dependency-free helpers for turning `RealClock`
/// nanosecond time points into human-readable strings.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>

#include "zeta/time/clock.h"

namespace zeta {

enum class TimeZone {
    kLocal,
    kUtc,
};

namespace time_internal {

[[nodiscard]] inline bool BreakDownTime(std::time_t seconds, TimeZone zone,
                                        std::tm* out) noexcept {
#if defined(_WIN32)
    return zone == TimeZone::kUtc ? gmtime_s(out, &seconds) == 0
                                  : localtime_s(out, &seconds) == 0;
#else
    return zone == TimeZone::kUtc ? gmtime_r(&seconds, out) != nullptr
                                  : localtime_r(&seconds, out) != nullptr;
#endif
}

[[nodiscard]] inline std::chrono::system_clock::time_point ToSystemTimePoint(
    RealClock::time_point t) noexcept {
    auto d = std::chrono::duration_cast<std::chrono::system_clock::duration>(
        std::chrono::nanoseconds(t));
    return std::chrono::system_clock::time_point(d);
}

} // namespace time_internal

/// Format a wall-clock time point as `YYYY-MM-DD HH:MM:SS.mmm`.
[[nodiscard]] inline std::string FormatTimestamp(
    RealClock::time_point t,
    TimeZone zone = TimeZone::kLocal) {
    auto tp = time_internal::ToSystemTimePoint(t);
    auto secs_tp = std::chrono::time_point_cast<std::chrono::seconds>(tp);
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(tp - secs_tp);
    if (millis.count() < 0) {
        secs_tp -= std::chrono::seconds(1);
        millis += std::chrono::milliseconds(1000);
    }

    std::time_t seconds = std::chrono::system_clock::to_time_t(secs_tp);
    std::tm tm{};
    if (!time_internal::BreakDownTime(seconds, zone, &tm)) return {};

    char buf[32];
    if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm) == 0) return {};

    char out[40];
    std::snprintf(out, sizeof(out), "%s.%03lld", buf,
                  static_cast<long long>(millis.count()));
    return out;
}

/// Format a wall-clock time point as `YYYY-MM-DDTHH:MM:SS.mmmZ`.
[[nodiscard]] inline std::string FormatTimestampUtc(
    RealClock::time_point t) {
    auto tp = time_internal::ToSystemTimePoint(t);
    auto secs_tp = std::chrono::time_point_cast<std::chrono::seconds>(tp);
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(tp - secs_tp);
    if (millis.count() < 0) {
        secs_tp -= std::chrono::seconds(1);
        millis += std::chrono::milliseconds(1000);
    }

    std::time_t seconds = std::chrono::system_clock::to_time_t(secs_tp);
    std::tm tm{};
    if (!time_internal::BreakDownTime(seconds, TimeZone::kUtc, &tm)) return {};

    char buf[32];
    if (std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm) == 0) return {};

    char out[40];
    std::snprintf(out, sizeof(out), "%s.%03lldZ", buf,
                  static_cast<long long>(millis.count()));
    return out;
}

/// Format the current wall-clock time in local time.
[[nodiscard]] inline std::string FormatNow() {
    return FormatTimestamp(RealClock::Now(), TimeZone::kLocal);
}

/// Format the current wall-clock time in UTC.
[[nodiscard]] inline std::string FormatNowUtc() {
    return FormatTimestampUtc(RealClock::Now());
}

} // namespace zeta

#endif // ZETA_TIME_TIMESTAMP_H
