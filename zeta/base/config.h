#ifndef ZETA_BASE_CONFIG_H
#define ZETA_BASE_CONFIG_H

/// @file   base/config.h
/// @brief  Compiler, platform, and language feature detection.

#if defined(_MSVC_LANG) && _MSVC_LANG > __cplusplus
#define ZETA_CXX_STANDARD _MSVC_LANG
#else
#define ZETA_CXX_STANDARD __cplusplus
#endif

#if defined(__clang__)
#define ZETA_COMPILER_CLANG 1
#else
#define ZETA_COMPILER_CLANG 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define ZETA_COMPILER_GCC 1
#else
#define ZETA_COMPILER_GCC 0
#endif

#if defined(_MSC_VER)
#define ZETA_COMPILER_MSVC 1
#else
#define ZETA_COMPILER_MSVC 0
#endif

#if defined(__APPLE__)
#define ZETA_OS_MACOS 1
#else
#define ZETA_OS_MACOS 0
#endif

#if defined(__linux__)
#define ZETA_OS_LINUX 1
#else
#define ZETA_OS_LINUX 0
#endif

#if defined(_WIN32)
#define ZETA_OS_WINDOWS 1
#else
#define ZETA_OS_WINDOWS 0
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define ZETA_ARCH_ARM64 1
#else
#define ZETA_ARCH_ARM64 0
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define ZETA_ARCH_X86_64 1
#else
#define ZETA_ARCH_X86_64 0
#endif

#if defined(__SIZEOF_INT128__)
#define ZETA_HAVE_INT128 1
#else
#define ZETA_HAVE_INT128 0
#endif

#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
#define ZETA_HAVE_STD_SPAN 1
#else
#define ZETA_HAVE_STD_SPAN 0
#endif

#endif // ZETA_BASE_CONFIG_H
