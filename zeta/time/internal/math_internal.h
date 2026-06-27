#ifndef ZETA_TIME_INTERNAL_MATH_INTERNAL_H
#define ZETA_TIME_INTERNAL_MATH_INTERNAL_H

#include "zeta/time/duration.h"

#include <cstdint>
#include <limits>

namespace zeta::time_internal {

[[nodiscard]] inline int64_t AddDurationSaturating(
    int64_t base, Duration delta) noexcept {
    constexpr auto kMax = std::numeric_limits<int64_t>::max();
    constexpr auto kMin = std::numeric_limits<int64_t>::min();
    const auto raw = delta.ToRaw();
    if (raw >= 0) {
        if (base > kMax - raw) return kMax;
    } else {
        if (base < kMin - raw) return kMin;
    }
    return base + raw;
}

[[nodiscard]] inline Duration RemainingUntil(
    int64_t target, int64_t now) noexcept {
    if (now >= target) return Duration();
    return Duration::FromRaw(target - now);
}

} // namespace zeta::time_internal

#endif // ZETA_TIME_INTERNAL_MATH_INTERNAL_H
