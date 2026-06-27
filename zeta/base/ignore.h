#ifndef ZETA_BASE_IGNORE_H
#define ZETA_BASE_IGNORE_H

/// @file   base/ignore.h
/// @brief  `zeta::ignore` — discard values and silence [[nodiscard]] warnings.

namespace zeta {

struct ignore_t {
    template <typename T>
    constexpr void operator=(T&&) const noexcept {}
};

inline constexpr ignore_t ignore{};

} // namespace zeta

#endif // ZETA_BASE_IGNORE_H
