#ifndef ZETA_UTILITY_UTILITY_H
#define ZETA_UTILITY_UTILITY_H

/// @file   utility/utility.h
/// @brief  Small general-purpose utilities.
///
/// Provides helpers that don't warrant their own module:
///   - `in_place` — tag type for in-place construction
///   - `as_const`   — add const to lvalue reference
///   - `ignore`     — discard any value (suppress [[nodiscard]] warnings)
///
/// Example:
///   void sink(zeta::ignore_t) { }
///   sink(zeta::ignore(std::printf("discarded\n")));

#include <type_traits>
#include <utility>

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// in_place — tag for in-place construction
// ═══════════════════════════════════════════════════════════════════════

struct in_place_t {
    explicit in_place_t() = default;
};
inline constexpr in_place_t in_place{};

// ═══════════════════════════════════════════════════════════════════════
// as_const — like std::as_const (C++17), as a zeta helper
// ═══════════════════════════════════════════════════════════════════════

template <typename T>
[[nodiscard]] constexpr std::add_const_t<T>& as_const(T& t) noexcept {
    return t;
}
template <typename T>
void as_const(const T&&) = delete;

// ═══════════════════════════════════════════════════════════════════════
// ignore — suppress [[nodiscard]] and unused-variable warnings
// ═══════════════════════════════════════════════════════════════════════

struct ignore_t {
    template <typename T>
    constexpr void operator=(T&&) const noexcept {}
};
inline constexpr ignore_t ignore{};

} // namespace zeta

#endif // ZETA_UTILITY_UTILITY_H
