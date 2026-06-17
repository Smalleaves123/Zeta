#ifndef ZETA_STRINGS_STR_CAT_H
#define ZETA_STRINGS_STR_CAT_H

/// @file   strings/str_cat.h
/// @brief  Efficient variadic string concatenation.
///
/// `zeta::StrCat(args...)` pre-computes the total output size and performs
/// a single heap allocation — unlike chained `operator+` which creates
/// a temporary string for every intermediate result.
///
/// Example:
///   std::string s = zeta::StrCat("pi = ", 3.14, ", e = ", 2.718);

#include <string>
#include <string_view>

#include "zeta/strings/internal/alpha_num.h"

namespace zeta {

// ── StrCat (empty overload) ────────────────────────────────────────
inline std::string StrCat() { return {}; }

// ── StrCat ─────────────────────────────────────────────────────────
template <typename... Args>
std::string StrCat(const Args&... args) {
    const strings_internal::AlphaNum pieces[] = {
        strings_internal::AlphaNum(args)...};
    size_t total = 0;
    for (const auto& p : pieces) {
        total += p.piece().size();
    }
    std::string result;
    result.reserve(total);
    for (const auto& p : pieces) {
        const auto sv = p.piece();
        result.append(sv.data(), sv.size());
    }
    return result;
}

// ── StrAppend (empty overload) ─────────────────────────────────────
inline void StrAppend(std::string& /*out*/) {}

// ── StrAppend ──────────────────────────────────────────────────────
template <typename... Args>
void StrAppend(std::string& out, const Args&... args) {
    const strings_internal::AlphaNum pieces[] = {
        strings_internal::AlphaNum(args)...};
    size_t extra = 0;
    for (const auto& p : pieces) extra += p.piece().size();
    out.reserve(out.size() + extra);
    for (const auto& p : pieces) {
        const auto sv = p.piece();
        out.append(sv.data(), sv.size());
    }
}

} // namespace zeta

#endif // ZETA_STRINGS_STR_CAT_H
