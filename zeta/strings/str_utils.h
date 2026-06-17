#ifndef ZETA_STRINGS_STR_UTILS_H
#define ZETA_STRINGS_STR_UTILS_H

/// @file   strings/str_utils.h
/// @brief  Common string predicates and transformations.
///
/// All functions operate on `std::string_view` for zero-copy compatibility.
///
/// Examples:
///   zeta::StartsWith("hello world", "hello");       // true
///   zeta::StrContains("hello world", "lo wo");      // true
///   zeta::StripPrefix("--flag", "--");              // "flag"

#include <cstring>
#include <string>
#include <string_view>

namespace zeta {

// ── Predicates ─────────────────────────────────────────────────────

/// True if `text` contains `needle`.
inline bool StrContains(std::string_view text,
                        std::string_view needle) noexcept {
    return text.find(needle) != std::string_view::npos;
}

/// True if `text` starts with `prefix`.
inline bool StartsWith(std::string_view text,
                       std::string_view prefix) noexcept {
    return text.size() >= prefix.size() &&
           text.substr(0, prefix.size()) == prefix;
}

/// True if `text` ends with `suffix`.
inline bool EndsWith(std::string_view text,
                     std::string_view suffix) noexcept {
    return text.size() >= suffix.size() &&
           text.substr(text.size() - suffix.size()) == suffix;
}

/// Case-insensitive ASCII version of StartsWith.
inline bool StartsWithIgnoreCase(std::string_view text,
                                  std::string_view prefix) noexcept {
    if (text.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        char a = text[i], b = prefix[i];
        if (a >= 'A' && a <= 'Z') a += ('a' - 'A');
        if (b >= 'A' && b <= 'Z') b += ('a' - 'A');
        if (a != b) return false;
    }
    return true;
}

/// Case-insensitive ASCII version of EndsWith.
inline bool EndsWithIgnoreCase(std::string_view text,
                                std::string_view suffix) noexcept {
    if (text.size() < suffix.size()) return false;
    auto offset = text.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i) {
        char a = text[offset + i], b = suffix[i];
        if (a >= 'A' && a <= 'Z') a += ('a' - 'A');
        if (b >= 'A' && b <= 'Z') b += ('a' - 'A');
        if (a != b) return false;
    }
    return true;
}

// ── Stripping ──────────────────────────────────────────────────────

/// If `text` starts with `prefix`, removes it and returns the remainder.
/// Otherwise returns `text` unchanged.
inline std::string_view StripPrefix(std::string_view text,
                                     std::string_view prefix) noexcept {
    return StartsWith(text, prefix)
               ? text.substr(prefix.size())
               : text;
}

/// If `text` ends with `suffix`, removes it and returns the remainder.
/// Otherwise returns `text` unchanged.
inline std::string_view StripSuffix(std::string_view text,
                                     std::string_view suffix) noexcept {
    return EndsWith(text, suffix)
               ? text.substr(0, text.size() - suffix.size())
               : text;
}

/// Removes leading ASCII whitespace (' ', '\\t', '\\n', '\\r', '\\v', '\\f').
inline std::string_view StripLeadingAsciiWhitespace(
    std::string_view text) noexcept {
    size_t i = 0;
    while (i < text.size() &&
           (text[i] == ' ' || text[i] == '\t' ||
            text[i] == '\n' || text[i] == '\r' ||
            text[i] == '\v' || text[i] == '\f')) {
        ++i;
    }
    return text.substr(i);
}

/// Removes trailing ASCII whitespace.
inline std::string_view StripTrailingAsciiWhitespace(
    std::string_view text) noexcept {
    size_t end = text.size();
    while (end > 0 &&
           (text[end - 1] == ' ' || text[end - 1] == '\t' ||
            text[end - 1] == '\n' || text[end - 1] == '\r' ||
            text[end - 1] == '\v' || text[end - 1] == '\f')) {
        --end;
    }
    return text.substr(0, end);
}

/// Removes both leading and trailing ASCII whitespace.
inline std::string_view StripAsciiWhitespace(
    std::string_view text) noexcept {
    return StripTrailingAsciiWhitespace(
        StripLeadingAsciiWhitespace(text));
}

// ── Case conversion ────────────────────────────────────────────────

/// Returns a new `std::string` with all ASCII letters lowercased.
inline std::string AsciiStrToLower(std::string_view text) {
    std::string result(text);
    for (char& c : result) {
        if (c >= 'A' && c <= 'Z') c += ('a' - 'A');
    }
    return result;
}

/// Returns a new `std::string` with all ASCII letters uppercased.
inline std::string AsciiStrToUpper(std::string_view text) {
    std::string result(text);
    for (char& c : result) {
        if (c >= 'a' && c <= 'z') c -= ('a' - 'A');
    }
    return result;
}

// ── Replacement ────────────────────────────────────────────────────

/// Replaces the first occurrence of `from` with `to` in `text`.
/// Returns a new string; leaves `text` unchanged.
inline std::string StrReplaceFirst(std::string_view text,
                                     std::string_view from,
                                     std::string_view to) {
    size_t pos = text.find(from);
    if (pos == std::string_view::npos) return std::string(text);
    std::string result;
    result.reserve(text.size() - from.size() + to.size());
    result.append(text.data(), pos);
    result.append(to);
    result.append(text.data() + pos + from.size(),
                  text.size() - pos - from.size());
    return result;
}

/// Replaces all occurrences of `from` with `to` in `text`.
/// Returns a new string.
inline std::string StrReplaceAll(std::string_view text,
                                   std::string_view from,
                                   std::string_view to) {
    if (from.empty()) return std::string(text);
    std::string result;
    size_t start = 0;
    for (;;) {
        size_t pos = text.find(from, start);
        if (pos == std::string_view::npos) {
            result.append(text.data() + start, text.size() - start);
            break;
        }
        result.append(text.data() + start, pos - start);
        result.append(to);
        start = pos + from.size();
    }
    return result;
}

} // namespace zeta

#endif // ZETA_STRINGS_STR_UTILS_H
