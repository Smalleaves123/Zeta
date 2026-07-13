#ifndef ZETA_BASE_ATTRIBUTES_H
#define ZETA_BASE_ATTRIBUTES_H

/// @file   base/attributes.h
/// @brief  Portable declaration attributes used by Zeta headers.

#define ZETA_DEPRECATED(message) [[deprecated(message)]]
#define ZETA_MUST_USE_RESULT [[nodiscard]]
#define ZETA_NO_UNIQUE_ADDRESS [[no_unique_address]]

#if defined(__clang__) || defined(__GNUC__)
#define ZETA_ATTRIBUTE(x) __attribute__((x))
#define ZETA_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#define ZETA_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#define ZETA_ATTRIBUTE(x)
#define ZETA_NONNULL(...)
#define ZETA_RETURNS_NONNULL
#endif

#endif // ZETA_BASE_ATTRIBUTES_H
