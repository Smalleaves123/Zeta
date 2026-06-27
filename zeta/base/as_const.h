#ifndef ZETA_BASE_AS_CONST_H
#define ZETA_BASE_AS_CONST_H

/// @file   base/as_const.h
/// @brief  `zeta::as_const` — add const to lvalue references.

#include <type_traits>

namespace zeta {

template <typename T>
[[nodiscard]] constexpr std::add_const_t<T>& as_const(T& t) noexcept {
    return t;
}

template <typename T>
void as_const(const T&&) = delete;

} // namespace zeta

#endif // ZETA_BASE_AS_CONST_H
