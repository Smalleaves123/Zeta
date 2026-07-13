#ifndef ZETA_STRINGS_MATCH_H
#define ZETA_STRINGS_MATCH_H

/// @file   strings/match.h
/// @brief  Allocation-free wildcard string matching.
///
/// Patterns support `*` for any sequence, including empty, and `?` for one
/// character. All other characters match literally.

#include "zeta/strings/ascii.h"

#include <cstddef>
#include <string_view>

namespace zeta {

namespace strings_internal {

template <typename Compare>
[[nodiscard]] bool MatchWildcard(std::string_view text,
                                 std::string_view pattern,
                                 Compare compare) noexcept {
    std::size_t text_pos = 0;
    std::size_t pattern_pos = 0;
    std::size_t star_pos = std::string_view::npos;
    std::size_t star_match = 0;

    while (text_pos < text.size()) {
        if (pattern_pos < pattern.size() && pattern[pattern_pos] != '*' &&
            (pattern[pattern_pos] == '?' ||
             compare(text[text_pos], pattern[pattern_pos]))) {
            ++text_pos;
            ++pattern_pos;
        } else if (pattern_pos < pattern.size() && pattern[pattern_pos] == '*') {
            star_pos = pattern_pos++;
            star_match = text_pos;
        } else if (star_pos != std::string_view::npos) {
            pattern_pos = star_pos + 1;
            text_pos = ++star_match;
        } else {
            return false;
        }
    }

    while (pattern_pos < pattern.size() && pattern[pattern_pos] == '*') {
        ++pattern_pos;
    }
    return pattern_pos == pattern.size();
}

} // namespace strings_internal

[[nodiscard]] inline bool StrMatch(std::string_view text,
                                   std::string_view pattern) noexcept {
    return strings_internal::MatchWildcard(
        text, pattern, [](char actual, char expected) {
            return actual == expected;
        });
}

[[nodiscard]] inline bool StrMatchIgnoreCase(
    std::string_view text, std::string_view pattern) noexcept {
    return strings_internal::MatchWildcard(
        text, pattern, [](char actual, char expected) {
            return AsciiToLower(actual) == AsciiToLower(expected);
        });
}

} // namespace zeta

#endif // ZETA_STRINGS_MATCH_H
