#ifndef ZETA_BASE_MACROS_H
#define ZETA_BASE_MACROS_H

/// @file   base/macros.h
/// @brief  Small compiler- and readability-oriented helper macros.

#if defined(__clang__) || defined(__GNUC__)
#define ZETA_LIKELY(x)   (__builtin_expect(!!(x), 1))
#define ZETA_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#define ZETA_LIKELY(x)   (x)
#define ZETA_UNLIKELY(x) (x)
#endif

#define ZETA_UNUSED(x) (void)(x)

#if defined(__GNUC__) || defined(__clang__)
#define ZETA_FORCE_INLINE inline __attribute__((always_inline))
#else
#define ZETA_FORCE_INLINE inline
#endif

#endif // ZETA_BASE_MACROS_H
