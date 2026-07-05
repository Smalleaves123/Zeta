#ifndef ZETA_STRINGS_STR_JOIN_H
#define ZETA_STRINGS_STR_JOIN_H

/// @file   strings/str_join.h
/// @brief  Efficient string joining with pre-computed total length.
///
/// `zeta::StrJoin` joins a range of string-like elements with a delimiter,
/// performing a single allocation for the result.
///
/// Examples:
///   std::vector<int> v = {1, 2, 3};
///   std::string s = zeta::StrJoin(v, ", ");  // "1, 2, 3"

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>

namespace zeta {

namespace strings_internal {

template <typename T>
size_t element_size(const T& val) {
    if constexpr (std::is_convertible_v<T, std::string_view>) {
        return std::string_view(val).size();
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        return val.size();
    } else if constexpr (std::is_same_v<std::decay_t<T>, char>) {
        return 1;
    } else {
        return std::to_string(val).size();
    }
}

template <typename T>
void element_append(std::string& out, const T& val) {
    if constexpr (std::is_convertible_v<T, std::string_view>) {
        out.append(std::string_view(val));
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        out.append(val);
    } else if constexpr (std::is_same_v<std::decay_t<T>, char>) {
        out.push_back(val);
    } else {
        out.append(std::to_string(val));
    }
}

} // namespace strings_internal

// ── iterator-pair overload ─────────────────────────────────────────
template <typename InputIt>
std::string StrJoin(InputIt first, InputIt last, std::string_view delim) {
    if (first == last) return {};
    size_t delim_len = delim.size();

    size_t total = 0, count = 0;
    for (auto it = first; it != last; ++it) {
        if (count > 0) total += delim_len;
        total += strings_internal::element_size(*it);
        ++count;
    }

    std::string result;
    result.reserve(total);
    count = 0;
    for (auto it = first; it != last; ++it) {
        if (count > 0) result.append(delim);
        strings_internal::element_append(result, *it);
        ++count;
    }
    return result;
}

// ── range overload ─────────────────────────────────────────────────
template <typename Range>
std::string StrJoin(const Range& range, std::string_view delim) {
    using std::begin;
    using std::end;
    return StrJoin(begin(range), end(range), delim);
}

// ── initializer_list overload ──────────────────────────────────────
template <typename T>
std::string StrJoin(std::initializer_list<T> list, std::string_view delim) {
    return StrJoin(list.begin(), list.end(), delim);
}

} // namespace zeta

#endif // ZETA_STRINGS_STR_JOIN_H
