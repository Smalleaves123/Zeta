#ifndef ZETA_STRINGS_FORMAT_H
#define ZETA_STRINGS_FORMAT_H

/// @file   strings/format.h
/// @brief  Lightweight string formatting — `zeta::Format("hello {}", name)`.
///
/// Unlike `std::format` (C++20) or `fmtlib`, this implementation is a
/// minimal, header-only formatter that supports `{}` placeholders and the
/// common positional specifier `{0}`, `{1}`, ...
///
/// Arguments are converted via their `AlphaNum` wrapper (see
/// `strings/internal/alpha_num.h`), so all types supported by `StrCat`
/// are automatically supported by `Format`.
///
/// Example:
///   std::string s = zeta::Format("x={}, y={}", 10, 3.14);
///   // s == "x=10, y=3.14"

#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "zeta/strings/internal/alpha_num.h"

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// Format
// ═══════════════════════════════════════════════════════════════════════

namespace strings_internal {

// Parse a single `{N}` or `{}` placeholder and return the position after
// the closing `}`.  `pos` is the index of the placed argument (0-based,
// incrementing for each `{}`).
[[nodiscard]] inline const char* ParsePlaceholder(
    const char* fmt, const char* end, int& arg_index) noexcept {
    const char* p = fmt;
    assert(p != end && *p == '{');
    ++p; // skip '{'

    if (p != end && *p == '}') {
        // '{}' — auto-increment.
        ++p;
        return p;
    }

    // '{' — literal brace.
    if (p != end && *p == '{') { ++p; return p; }

    // '{N}' — parse index.
    int idx = 0;
    while (p != end && *p >= '0' && *p <= '9') {
        idx = idx * 10 + static_cast<int>(*p - '0');
        ++p;
    }
    if (p != end && *p == '}') {
        arg_index = idx;
        ++p;
        return p;
    }

    // Malformed — treat as literal text.
    arg_index = -1;
    return fmt + 1;
}

} // namespace strings_internal

/// Format a string with `{}` / `{N}` placeholders.
///
/// @param fmt   Format string: `"hello {} world {1}"`
/// @param args  Zero or more arguments convertible to AlphaNum.
///
/// All arguments are converted to string pieces and spliced into the
/// result.  `{{` produces a literal `{`, `}}` a literal `}`.
template <typename... Args>
[[nodiscard]] std::string Format(const char* fmt, const Args&... args) {
    using strings_internal::AlphaNum;
    constexpr size_t kNumArgs = sizeof...(Args);

    std::string result;
    result.reserve(128);

    if constexpr (kNumArgs == 0) {
        // Fast path: just copy the format string literally (no placeholders).
        result = fmt;
        return result;
    } else {
        const AlphaNum pieces[] = {AlphaNum(args)...};
        bool used[kNumArgs] = {};
        (void)used;

        const char* end = fmt;
        while (*end) ++end;

        int auto_index = 0;
        const char* p = fmt;
        while (p != end) {
            if (*p == '{') {
                if (p + 1 != end && *(p + 1) == '{') {
                    result += '{';
                    p += 2;
                } else if (p + 1 != end && *(p + 1) == '}') {
                    if (auto_index < static_cast<int>(kNumArgs)) {
                        result += pieces[auto_index].piece();
                    }
                    ++auto_index;
                    p += 2;
                } else {
                    int idx = auto_index;
                    p = strings_internal::ParsePlaceholder(p, end, idx);
                    if (idx >= 0 && static_cast<size_t>(idx) < kNumArgs) {
                        result += pieces[idx].piece();
                        used[idx] = true;
                    } else {
                        result += '{';
                    }
                }
            } else if (*p == '}') {
                if (p + 1 != end && *(p + 1) == '}') {
                    result += '}';
                    p += 2;
                } else {
                    result += '}';  // lone '}' — copy literally
                    ++p;
                }
            } else {
                result += *p;
                ++p;
            }
        }
    }
    return result;
}

/// Overload for `std::string` format strings.
template <typename... Args>
[[nodiscard]] std::string Format(const std::string& fmt, const Args&... args) {
    return Format(fmt.c_str(), args...);
}

/// Overload for `std::string_view` format strings.
template <typename... Args>
[[nodiscard]] std::string Format(std::string_view fmt, const Args&... args) {
    // Create null-terminated copy.
    return Format(std::string(fmt).c_str(), args...);
}

} // namespace zeta

#endif // ZETA_STRINGS_FORMAT_H
