#ifndef ZETA_BASE_IN_PLACE_H
#define ZETA_BASE_IN_PLACE_H

/// @file   base/in_place.h
/// @brief  `zeta::in_place` — tag for in-place construction.

namespace zeta {

struct in_place_t {
    explicit in_place_t() = default;
};

inline constexpr in_place_t in_place{};

} // namespace zeta

#endif // ZETA_BASE_IN_PLACE_H
