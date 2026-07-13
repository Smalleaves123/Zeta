#ifndef ZETA_STRINGS_SUBSTITUTE_H
#define ZETA_STRINGS_SUBSTITUTE_H

/// @file   strings/substitute.h
/// @brief  `$0` ... `$9` positional string substitution.

#include "zeta/strings/internal/alpha_num.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace zeta {

inline std::string StrSubstitute(std::string_view format) {
    return std::string(format);
}

template <typename... Args>
[[nodiscard]] std::string StrSubstitute(std::string_view format,
                                        const Args&... args) {
    const strings_internal::AlphaNum pieces[] = {
        strings_internal::AlphaNum(args)...};
    std::string result;

    for (std::size_t i = 0; i < format.size(); ++i) {
        if (format[i] != '$' || i + 1 >= format.size()) {
            result.push_back(format[i]);
            continue;
        }

        const char next = format[i + 1];
        if (next == '$') {
            result.push_back('$');
            ++i;
        } else if (next >= '0' && next <= '9') {
            const std::size_t index = static_cast<std::size_t>(next - '0');
            if (index < sizeof...(Args)) {
                result.append(pieces[index].piece());
                ++i;
            } else {
                result.push_back('$');
            }
        } else {
            result.push_back('$');
        }
    }
    return result;
}

} // namespace zeta

#endif // ZETA_STRINGS_SUBSTITUTE_H
