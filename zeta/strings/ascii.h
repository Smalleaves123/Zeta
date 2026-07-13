#ifndef ZETA_STRINGS_ASCII_H
#define ZETA_STRINGS_ASCII_H

/// @file   strings/ascii.h
/// @brief  ASCII character predicates and case conversion helpers.

#include <string>
#include <string_view>

namespace zeta {

[[nodiscard]] constexpr bool AsciiIsAlpha(char c) noexcept {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

[[nodiscard]] constexpr bool AsciiIsDigit(char c) noexcept {
    return c >= '0' && c <= '9';
}

[[nodiscard]] constexpr bool AsciiIsAlnum(char c) noexcept {
    return AsciiIsAlpha(c) || AsciiIsDigit(c);
}

[[nodiscard]] constexpr bool AsciiIsSpace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
           c == '\v' || c == '\f';
}

[[nodiscard]] constexpr char AsciiToLower(char c) noexcept {
    return c >= 'A' && c <= 'Z'
               ? static_cast<char>(c + ('a' - 'A'))
               : c;
}

[[nodiscard]] constexpr char AsciiToUpper(char c) noexcept {
    return c >= 'a' && c <= 'z'
               ? static_cast<char>(c - ('a' - 'A'))
               : c;
}

[[nodiscard]] inline std::string AsciiStrToLower(std::string_view text) {
    std::string result(text);
    for (char& c : result) c = AsciiToLower(c);
    return result;
}

[[nodiscard]] inline std::string AsciiStrToUpper(std::string_view text) {
    std::string result(text);
    for (char& c : result) c = AsciiToUpper(c);
    return result;
}

} // namespace zeta

#endif // ZETA_STRINGS_ASCII_H
