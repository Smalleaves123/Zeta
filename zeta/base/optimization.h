#ifndef ZETA_BASE_OPTIMIZATION_H
#define ZETA_BASE_OPTIMIZATION_H

/// @file   base/optimization.h
/// @brief  Portable compiler optimization hints.

#if defined(__clang__) || defined(__GNUC__)
#define ZETA_ASSUME(condition) \
    do {                        \
        if (!(condition)) __builtin_unreachable(); \
    } while (false)
#define ZETA_PREFETCH(address) __builtin_prefetch(address)
#define ZETA_RESTRICT __restrict__
#elif defined(_MSC_VER)
#define ZETA_ASSUME(condition) __assume(condition)
#define ZETA_PREFETCH(address) ((void)0)
#define ZETA_RESTRICT __restrict
#else
#define ZETA_ASSUME(condition) ((void)0)
#define ZETA_PREFETCH(address) ((void)0)
#define ZETA_RESTRICT
#endif

#define ZETA_CACHELINE_SIZE 64

#endif // ZETA_BASE_OPTIMIZATION_H
