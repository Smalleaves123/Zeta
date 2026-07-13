#ifndef ZETA_STRINGS_ESCAPING_H
#define ZETA_STRINGS_ESCAPING_H

/// @file   strings/escaping.h
/// @brief  C-style string escaping and unescaping.

#include "zeta/status/statusor.h"

#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>

namespace zeta {

[[nodiscard]] inline std::string CEscape(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    constexpr char kHex[] = "0123456789ABCDEF";

    for (unsigned char c : text) {
        switch (c) {
        case '\\': result += "\\\\"; break;
        case '"':  result += "\\\""; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\v': result += "\\v"; break;
        default:
            if (c >= 0x20U && c <= 0x7EU) {
                result.push_back(static_cast<char>(c));
            } else {
                result += "\\x";
                result.push_back(kHex[c >> 4U]);
                result.push_back(kHex[c & 0x0FU]);
            }
            break;
        }
    }
    return result;
}

namespace strings_internal {

[[nodiscard]] inline int HexValue(char c) noexcept {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

} // namespace strings_internal

/// Decodes C-style escapes. Invalid or truncated escapes return an error.
[[nodiscard]] inline StatusOr<std::string> CUnescape(std::string_view text) {
    std::string result;
    result.reserve(text.size());

    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '\\') {
            result.push_back(text[i]);
            continue;
        }
        if (++i == text.size()) {
            return InvalidArgumentError("trailing escape character");
        }

        switch (text[i]) {
        case '\\': result.push_back('\\'); break;
        case '"':  result.push_back('"'); break;
        case '\'': result.push_back('\''); break;
        case 'a': result.push_back('\a'); break;
        case 'b': result.push_back('\b'); break;
        case 'f': result.push_back('\f'); break;
        case 'n': result.push_back('\n'); break;
        case 'r': result.push_back('\r'); break;
        case 't': result.push_back('\t'); break;
        case 'v': result.push_back('\v'); break;
        case 'x': {
            if (i + 2 >= text.size()) {
                return InvalidArgumentError("truncated hexadecimal escape");
            }
            const int high = strings_internal::HexValue(text[i + 1]);
            const int low = strings_internal::HexValue(text[i + 2]);
            if (high < 0 || low < 0) {
                return InvalidArgumentError("invalid hexadecimal escape");
            }
            result.push_back(static_cast<char>((high << 4) | low));
            i += 2;
            break;
        }
        default:
            if (text[i] >= '0' && text[i] <= '7') {
                unsigned value = static_cast<unsigned>(text[i] - '0');
                int digits = 1;
                while (digits < 3 && i + 1 < text.size() &&
                       text[i + 1] >= '0' && text[i + 1] <= '7') {
                    value = value * 8U + static_cast<unsigned>(text[++i] - '0');
                    ++digits;
                }
                result.push_back(static_cast<char>(value));
            } else {
                return InvalidArgumentError("unknown escape sequence");
            }
            break;
        }
    }
    return result;
}

} // namespace zeta

#endif // ZETA_STRINGS_ESCAPING_H
