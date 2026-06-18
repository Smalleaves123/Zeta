#ifndef ZETA_STRINGS_NUMBERS_H
#define ZETA_STRINGS_NUMBERS_H

/// @file   strings/numbers.h
/// @brief  Fast integer-to-string and string-to-integer conversions.

#include <cassert>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <string_view>
#include <system_error>

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// FastIntToBuffer
// ═══════════════════════════════════════════════════════════════════════

template <typename T>
[[nodiscard]] std::string_view FastIntToBuffer(T n, char* buf) noexcept {
    static_assert(sizeof(T) <= sizeof(unsigned long long),
                  "FastIntToBuffer: type too large");
    auto [ptr, ec] = std::to_chars(buf, buf + 32, n);
    assert(ec == std::errc{});
    *ptr = '\0';
    return std::string_view(buf, static_cast<size_t>(ptr - buf));
}

// ═══════════════════════════════════════════════════════════════════════
// SimpleAtoi — integer (uses from_chars)
// ═══════════════════════════════════════════════════════════════════════

template <typename T>
[[nodiscard]] bool SimpleAtoi(std::string_view sv, T* out) noexcept {
    if (sv.empty()) return false;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), *out);
    return ec == std::errc{} && ptr == sv.data() + sv.size();
}

// ── Integer overloads (forward to template) ────────────────────────

[[nodiscard]] inline bool SimpleAtoi(std::string_view sv, int* out)           noexcept { return SimpleAtoi<int>(sv, out); }
[[nodiscard]] inline bool SimpleAtoi(std::string_view sv, long* out)          noexcept { return SimpleAtoi<long>(sv, out); }
[[nodiscard]] inline bool SimpleAtoi(std::string_view sv, long long* out)     noexcept { return SimpleAtoi<long long>(sv, out); }
[[nodiscard]] inline bool SimpleAtoi(std::string_view sv, unsigned* out)      noexcept { return SimpleAtoi<unsigned>(sv, out); }
[[nodiscard]] inline bool SimpleAtoi(std::string_view sv, unsigned long* out) noexcept { return SimpleAtoi<unsigned long>(sv, out); }
[[nodiscard]] inline bool SimpleAtoi(std::string_view sv, unsigned long long* out) noexcept { return SimpleAtoi<unsigned long long>(sv, out); }

// ── Floating-point overloads (use strtof/strtod for macOS compat) ───

[[nodiscard]] inline bool SimpleAtoi(std::string_view sv, float* out) noexcept {
    if (sv.empty()) return false;
    char tmp[64];
    size_t n = sv.size() < 63 ? sv.size() : 63;
    for (size_t i = 0; i < n; ++i) tmp[i] = sv[i];
    tmp[n] = '\0';
    char* end = nullptr;
    *out = std::strtof(tmp, &end);
    return end == tmp + n;
}

[[nodiscard]] inline bool SimpleAtoi(std::string_view sv, double* out) noexcept {
    if (sv.empty()) return false;
    char tmp[256];
    size_t n = sv.size() < 255 ? sv.size() : 255;
    for (size_t i = 0; i < n; ++i) tmp[i] = sv[i];
    tmp[n] = '\0';
    char* end = nullptr;
    *out = std::strtod(tmp, &end);
    return end == tmp + n;
}

} // namespace zeta

#endif // ZETA_STRINGS_NUMBERS_H
