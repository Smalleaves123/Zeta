#ifndef ZETA_DEBUGGING_ASSERT_H
#define ZETA_DEBUGGING_ASSERT_H

/// @file   debugging/assert.h
/// @brief  Debug-only assertions and always-on checks.

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "zeta/base/macros.h"

namespace zeta {

using AssertionHandler = void (*)(const char* expression,
                                  const char* file,
                                  int line,
                                  const char* message) noexcept;

inline AssertionHandler& AssertionHandlerStorage() noexcept {
    static AssertionHandler handler = nullptr;
    return handler;
}

/// Installs a process-local handler for failed `ZETA_CHECK` expressions.
inline void SetAssertionHandler(AssertionHandler handler) noexcept {
    AssertionHandlerStorage() = handler;
}

[[noreturn]] inline void AssertionFailure(const char* expression,
                                           const char* file,
                                           int line,
                                           const char* message = nullptr) noexcept {
    if (auto handler = AssertionHandlerStorage(); handler != nullptr) {
        handler(expression, file, line, message);
    }

    std::fprintf(stderr, "Zeta check failed: %s (%s:%d)%s%s\n",
                 expression, file, line,
                 message != nullptr ? ": " : "",
                 message != nullptr ? message : "");
    std::abort();
}

} // namespace zeta

/// Debug-only assertion. The expression is not evaluated with `NDEBUG`.
#define ZETA_ASSERT(condition) assert(condition)

/// Always-on assertion for validating production invariants.
#define ZETA_CHECK(condition)                                               \
    do {                                                                    \
        if (ZETA_UNLIKELY(!(condition))) {                                  \
            ::zeta::AssertionFailure(#condition, __FILE__, __LINE__);       \
        }                                                                   \
    } while (false)

/// Always-on assertion with a short diagnostic message.
#define ZETA_CHECK_MSG(condition, message)                                  \
    do {                                                                    \
        if (ZETA_UNLIKELY(!(condition))) {                                  \
            ::zeta::AssertionFailure(#condition, __FILE__, __LINE__,        \
                                     (message));                             \
        }                                                                   \
    } while (false)

#ifdef NDEBUG
#define ZETA_DCHECK(condition) ((void)0)
#else
#define ZETA_DCHECK(condition) ZETA_CHECK(condition)
#endif

#endif // ZETA_DEBUGGING_ASSERT_H
