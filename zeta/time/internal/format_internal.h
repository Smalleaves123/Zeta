#ifndef ZETA_TIME_INTERNAL_FORMAT_INTERNAL_H
#define ZETA_TIME_INTERNAL_FORMAT_INTERNAL_H

#include "zeta/time/clock.h"
#include "zeta/time/time_zone.h"

#include <chrono>
#include <ctime>

namespace zeta::time_internal {

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

} // namespace zeta::time_internal

#endif // ZETA_TIME_INTERNAL_FORMAT_INTERNAL_H
