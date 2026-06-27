#ifndef ZETA_UTILITY_UTILITY_H
#define ZETA_UTILITY_UTILITY_H

/// @file   utility/utility.h
/// @brief  Compatibility wrapper for foundational utilities.
///
/// Provides helpers that don't warrant their own module:
///   - `in_place` — tag type for in-place construction
///   - `as_const`   — add const to lvalue reference
///   - `ignore`     — discard any value (suppress [[nodiscard]] warnings)
///
/// Example:
///   void sink(zeta::ignore_t) { }
///   sink(zeta::ignore(std::printf("discarded\n")));

#include "zeta/base/as_const.h"
#include "zeta/base/ignore.h"
#include "zeta/base/in_place.h"

#endif // ZETA_UTILITY_UTILITY_H
