#ifndef ZETA_TIME_TIME_ZONE_H
#define ZETA_TIME_TIME_ZONE_H

/// @file   time/time_zone.h
/// @brief  Portable time-zone selectors for civil-time conversion.

namespace zeta {

/// Compatibility selector used by the existing timestamp helpers.
enum class TimeZone {
    kLocal,
    kUtc,
};

/// A portable time-zone description.
///
/// Named IANA zones require a platform time-zone database and are intentionally
/// outside this header-only core. Fixed offsets cover deterministic protocols;
/// `kLocal` delegates daylight-saving behavior to the operating system.
struct TimeZoneSpec {
    enum class Kind {
        kLocal,
        kUtc,
        kFixedOffset,
    };

    Kind kind = Kind::kLocal;
    int offset_minutes = 0;

    [[nodiscard]] static constexpr TimeZoneSpec Local() noexcept {
        return {Kind::kLocal, 0};
    }

    [[nodiscard]] static constexpr TimeZoneSpec Utc() noexcept {
        return {Kind::kUtc, 0};
    }

    [[nodiscard]] static constexpr TimeZoneSpec FixedOffset(
        int minutes) noexcept {
        return {Kind::kFixedOffset, minutes};
    }

    [[nodiscard]] static constexpr TimeZoneSpec From(TimeZone zone) noexcept {
        return zone == TimeZone::kUtc ? Utc() : Local();
    }
};

} // namespace zeta

#endif // ZETA_TIME_TIME_ZONE_H
