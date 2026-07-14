#ifndef ZETA_TIME_CIVIL_TIME_H
#define ZETA_TIME_CIVIL_TIME_H

/// @file   time/civil_time.h
/// @brief  Calendar dates, civil timestamps, parsing, and conversion.

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <optional>
#include <string>
#include <string_view>

#include "zeta/time/clock.h"
#include "zeta/time/time_zone.h"

namespace zeta {
namespace time_internal {

[[nodiscard]] constexpr bool IsLeapYear(int year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

[[nodiscard]] constexpr int DaysInMonth(int year, int month) noexcept {
    if (month < 1 || month > 12) return 0;
    constexpr int kDays[] = {0, 31, 28, 31, 30, 31, 30,
                             31, 31, 30, 31, 30, 31};
    return kDays[month] + (month == 2 && IsLeapYear(year) ? 1 : 0);
}

// Proleptic Gregorian date -> days since 1970-01-01.
[[nodiscard]] constexpr std::int64_t DaysFromCivil(int year, unsigned month,
                                                   unsigned day) noexcept {
    std::int64_t y = year;
    y -= month <= 2;
    const std::int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned year_of_era = static_cast<unsigned>(y - era * 400);
    const unsigned adjusted_month = month > 2 ? month - 3 : month + 9;
    const unsigned day_of_year =
        (153 * adjusted_month + 2) / 5 + day - 1;
    const unsigned day_of_era = year_of_era * 365 + year_of_era / 4 -
                                year_of_era / 100 + day_of_year;
    return era * 146097 + static_cast<std::int64_t>(day_of_era) - 719468;
}

struct CivilParts {
    int year;
    unsigned month;
    unsigned day;
};

[[nodiscard]] constexpr CivilParts CivilFromDays(std::int64_t days) noexcept {
    days += 719468;
    const std::int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    const unsigned day_of_era =
        static_cast<unsigned>(days - era * 146097);
    const unsigned year_of_era =
        (day_of_era - day_of_era / 1460 + day_of_era / 36524 -
         day_of_era / 146096) /
        365;
    const std::int64_t y = era * 400 + year_of_era;
    const unsigned day_of_year =
        day_of_era - (365 * year_of_era + year_of_era / 4 -
                      year_of_era / 100);
    const unsigned month_part = (5 * day_of_year + 2) / 153;
    const unsigned day = day_of_year - (153 * month_part + 2) / 5 + 1;
    const unsigned month = month_part + (month_part < 10 ? 3 : -9);
    return {static_cast<int>(y + (month <= 2)), month, day};
}

[[nodiscard]] inline bool ParseDigits(std::string_view text, std::size_t pos,
                                      std::size_t count, int* value) noexcept {
    int parsed = 0;
    if (pos + count > text.size() || value == nullptr) return false;
    for (std::size_t i = 0; i < count; ++i) {
        const char ch = text[pos + i];
        if (ch < '0' || ch > '9') return false;
        parsed = parsed * 10 + (ch - '0');
    }
    *value = parsed;
    return true;
}

[[nodiscard]] inline bool BreakDownTime(std::time_t seconds, TimeZoneSpec zone,
                                        std::tm* out) noexcept {
    if (zone.kind == TimeZoneSpec::Kind::kFixedOffset) {
        seconds += static_cast<std::time_t>(zone.offset_minutes) * 60;
        zone = TimeZoneSpec::Utc();
    }
#if defined(_WIN32)
    return zone.kind == TimeZoneSpec::Kind::kUtc
               ? gmtime_s(out, &seconds) == 0
               : localtime_s(out, &seconds) == 0;
#else
    return zone.kind == TimeZoneSpec::Kind::kUtc
               ? gmtime_r(&seconds, out) != nullptr
               : localtime_r(&seconds, out) != nullptr;
#endif
}

} // namespace time_internal

/// A proleptic Gregorian calendar date without a time zone.
class CivilDate {
public:
    constexpr CivilDate() noexcept = default;
    constexpr CivilDate(int year, int month, int day) noexcept
        : year_(year), month_(month), day_(day) {}

    [[nodiscard]] constexpr int year() const noexcept { return year_; }
    [[nodiscard]] constexpr int month() const noexcept { return month_; }
    [[nodiscard]] constexpr int day() const noexcept { return day_; }

    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return month_ >= 1 && month_ <= 12 && day_ >= 1 &&
               day_ <= time_internal::DaysInMonth(year_, month_);
    }

    [[nodiscard]] constexpr std::int64_t DaysSinceEpoch() const noexcept {
        return time_internal::DaysFromCivil(
            year_, static_cast<unsigned>(month_), static_cast<unsigned>(day_));
    }

    [[nodiscard]] constexpr CivilDate AddDays(std::int64_t days) const noexcept {
        const auto parts = time_internal::CivilFromDays(DaysSinceEpoch() + days);
        return CivilDate(parts.year, static_cast<int>(parts.month),
                         static_cast<int>(parts.day));
    }

    /// Sunday = 0, Monday = 1, ..., Saturday = 6.
    [[nodiscard]] constexpr int DayOfWeek() const noexcept {
        const auto days = DaysSinceEpoch();
        return static_cast<int>((days >= -4 ? days + 4 : days - 3) % 7 + 7) % 7;
    }

    [[nodiscard]] std::string ToString() const {
        char out[32];
        std::snprintf(out, sizeof(out), "%04d-%02d-%02d", year_, month_, day_);
        return out;
    }

    friend constexpr bool operator==(CivilDate lhs, CivilDate rhs) noexcept {
        return lhs.year_ == rhs.year_ && lhs.month_ == rhs.month_ &&
               lhs.day_ == rhs.day_;
    }

private:
    int year_ = 1970;
    int month_ = 1;
    int day_ = 1;
};

[[nodiscard]] inline std::optional<CivilDate> ParseCivilDate(
    std::string_view text) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') return std::nullopt;
    int year = 0;
    int month = 0;
    int day = 0;
    if (!time_internal::ParseDigits(text, 0, 4, &year) ||
        !time_internal::ParseDigits(text, 5, 2, &month) ||
        !time_internal::ParseDigits(text, 8, 2, &day)) {
        return std::nullopt;
    }
    CivilDate date(year, month, day);
    return date.IsValid() ? std::optional<CivilDate>(date) : std::nullopt;
}

/// A civil timestamp with nanosecond precision and no implicit time zone.
class CivilTime {
public:
    constexpr CivilTime() noexcept = default;
    constexpr CivilTime(CivilDate date, int hour, int minute, int second,
                        int nanosecond = 0) noexcept
        : date_(date), hour_(hour), minute_(minute), second_(second),
          nanosecond_(nanosecond) {}

    [[nodiscard]] constexpr const CivilDate& date() const noexcept { return date_; }
    [[nodiscard]] constexpr int hour() const noexcept { return hour_; }
    [[nodiscard]] constexpr int minute() const noexcept { return minute_; }
    [[nodiscard]] constexpr int second() const noexcept { return second_; }
    [[nodiscard]] constexpr int nanosecond() const noexcept { return nanosecond_; }

    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return date_.IsValid() && hour_ >= 0 && hour_ < 24 && minute_ >= 0 &&
               minute_ < 60 && second_ >= 0 && second_ < 60 &&
               nanosecond_ >= 0 && nanosecond_ < 1000000000;
    }

    [[nodiscard]] std::string ToString() const {
        char out[48];
        if (nanosecond_ == 0) {
            std::snprintf(out, sizeof(out), "%sT%02d:%02d:%02d",
                          date_.ToString().c_str(), hour_, minute_, second_);
        } else {
            std::snprintf(out, sizeof(out), "%sT%02d:%02d:%02d.%09d",
                          date_.ToString().c_str(), hour_, minute_, second_,
                          nanosecond_);
        }
        return out;
    }

    /// Converts this civil value to Unix nanoseconds in the selected zone.
    [[nodiscard]] std::optional<RealClock::time_point> ToUnixNanos(
        TimeZoneSpec zone = TimeZoneSpec::Local()) const noexcept {
        if (!IsValid()) return std::nullopt;
        if (zone.kind == TimeZoneSpec::Kind::kLocal) {
            std::tm local{};
            local.tm_year = date_.year() - 1900;
            local.tm_mon = date_.month() - 1;
            local.tm_mday = date_.day();
            local.tm_hour = hour_;
            local.tm_min = minute_;
            local.tm_sec = second_;
            local.tm_isdst = -1;
            const std::time_t seconds = std::mktime(&local);
            if (seconds == static_cast<std::time_t>(-1)) return std::nullopt;
            return static_cast<RealClock::time_point>(seconds) * 1000000000LL +
                   nanosecond_;
        }

        std::int64_t seconds = date_.DaysSinceEpoch() * 86400 +
                               hour_ * 3600 + minute_ * 60 + second_;
        if (zone.kind == TimeZoneSpec::Kind::kFixedOffset) {
            seconds -= static_cast<std::int64_t>(zone.offset_minutes) * 60;
        }
        return seconds * 1000000000LL + nanosecond_;
    }

    [[nodiscard]] std::optional<RealClock::time_point> ToUnixNanos(
        TimeZone zone) const noexcept {
        return ToUnixNanos(TimeZoneSpec::From(zone));
    }

    /// Converts Unix nanoseconds to civil time in the selected zone.
    [[nodiscard]] static std::optional<CivilTime> FromUnixNanos(
        RealClock::time_point nanos,
        TimeZoneSpec zone = TimeZoneSpec::Local()) noexcept {
        std::int64_t seconds = nanos / 1000000000LL;
        std::int64_t remainder = nanos % 1000000000LL;
        if (remainder < 0) {
            --seconds;
            remainder += 1000000000LL;
        }

        std::tm broken_down{};
        if (zone.kind == TimeZoneSpec::Kind::kLocal ||
            zone.kind == TimeZoneSpec::Kind::kUtc) {
            if (!time_internal::BreakDownTime(
                    static_cast<std::time_t>(seconds), zone, &broken_down)) {
                return std::nullopt;
            }
            return CivilTime(
                CivilDate(broken_down.tm_year + 1900, broken_down.tm_mon + 1,
                          broken_down.tm_mday),
                broken_down.tm_hour, broken_down.tm_min, broken_down.tm_sec,
                static_cast<int>(remainder));
        }

        seconds += static_cast<std::int64_t>(zone.offset_minutes) * 60;
        std::int64_t days = seconds / 86400;
        std::int64_t day_seconds = seconds % 86400;
        if (day_seconds < 0) {
            --days;
            day_seconds += 86400;
        }
        const auto parts = time_internal::CivilFromDays(days);
        return CivilTime(
            CivilDate(parts.year, static_cast<int>(parts.month),
                      static_cast<int>(parts.day)),
            static_cast<int>(day_seconds / 3600),
            static_cast<int>((day_seconds / 60) % 60),
            static_cast<int>(day_seconds % 60), static_cast<int>(remainder));
    }

    [[nodiscard]] static std::optional<CivilTime> FromUnixNanos(
        RealClock::time_point nanos, TimeZone zone) noexcept {
        return FromUnixNanos(nanos, TimeZoneSpec::From(zone));
    }

private:
    CivilDate date_;
    int hour_ = 0;
    int minute_ = 0;
    int second_ = 0;
    int nanosecond_ = 0;

    friend constexpr bool operator==(const CivilTime& lhs,
                                     const CivilTime& rhs) noexcept {
        return lhs.date_ == rhs.date_ && lhs.hour_ == rhs.hour_ &&
               lhs.minute_ == rhs.minute_ && lhs.second_ == rhs.second_ &&
               lhs.nanosecond_ == rhs.nanosecond_;
    }
};

[[nodiscard]] inline std::optional<CivilTime> ParseCivilTime(
    std::string_view text) {
    if (text.size() < 19 || text[10] != 'T' || text[13] != ':' ||
        text[16] != ':') {
        return std::nullopt;
    }
    const auto date = ParseCivilDate(text.substr(0, 10));
    if (!date.has_value()) return std::nullopt;

    int hour = 0;
    int minute = 0;
    int second = 0;
    if (!time_internal::ParseDigits(text, 11, 2, &hour) ||
        !time_internal::ParseDigits(text, 14, 2, &minute) ||
        !time_internal::ParseDigits(text, 17, 2, &second)) {
        return std::nullopt;
    }

    int nanosecond = 0;
    if (text.size() > 19) {
        if (text[19] != '.' || text.size() > 29 || text.size() == 20) {
            return std::nullopt;
        }
        const std::size_t digits = text.size() - 20;
        int fraction = 0;
        if (!time_internal::ParseDigits(text, 20, digits, &fraction)) {
            return std::nullopt;
        }
        for (std::size_t i = digits; i < 9; ++i) fraction *= 10;
        nanosecond = fraction;
    }
    CivilTime result(*date, hour, minute, second, nanosecond);
    return result.IsValid() ? std::optional<CivilTime>(result) : std::nullopt;
}

[[nodiscard]] inline std::string FormatCivilDate(const CivilDate& date) {
    return date.ToString();
}

[[nodiscard]] inline std::string FormatCivilTime(const CivilTime& time) {
    return time.ToString();
}

} // namespace zeta

#endif // ZETA_TIME_CIVIL_TIME_H
