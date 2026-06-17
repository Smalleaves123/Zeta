#ifndef ZETA_STRINGS_INTERNAL_ALPHA_NUM_H
#define ZETA_STRINGS_INTERNAL_ALPHA_NUM_H

/// @file   strings/internal/alpha_num.h
/// @brief  Lightweight "string or number" piece for StrCat / StrAppend.
///
/// Each AlphaNum stores either a `string_view` into existing data or a
/// small inline buffer populated by `std::to_chars`.
///
/// This is an **internal** detail of the strings module.  Prefer
/// `zeta::StrCat(...)` over direct use of AlphaNum.

#include <cstddef>
#include <charconv>
#include <string_view>

namespace zeta {
namespace strings_internal {

class AlphaNum {
public:
    static constexpr size_t kBufSize = 32;

    // -- string-like ------------------------------------------------
    AlphaNum(const char* s) noexcept : piece_(s) {}
    AlphaNum(const std::string& s) noexcept : piece_(s) {}
    AlphaNum(std::string_view s) noexcept : piece_(s) {}

    AlphaNum(char c) noexcept {
        buf_[0] = c;
        piece_ = std::string_view(buf_, 1);
    }

    AlphaNum(bool b) noexcept
        : piece_(b ? std::string_view("true", 4)
                    : std::string_view("false", 5)) {}

    // -- signed integers --------------------------------------------
    AlphaNum(int n) noexcept               { from_int(n); }
    AlphaNum(long n) noexcept              { from_int(n); }
    AlphaNum(long long n) noexcept         { from_int(n); }

    // -- unsigned integers ------------------------------------------
    AlphaNum(unsigned int n) noexcept      { from_uint(n); }
    AlphaNum(unsigned long n) noexcept     { from_uint(n); }
    AlphaNum(unsigned long long n) noexcept{ from_uint(n); }

    // -- floating point ---------------------------------------------
    AlphaNum(float n) noexcept  { from_double(n); }
    AlphaNum(double n) noexcept { from_double(n); }

    // -- accessors --------------------------------------------------
    std::string_view piece() const noexcept { return piece_; }

private:
    char buf_[kBufSize]{};
    std::string_view piece_;

    template <typename T>
    void from_int(T n) {
        auto [ptr, _] = std::to_chars(buf_, buf_ + kBufSize, n);
        piece_ = std::string_view(buf_, ptr - buf_);
    }

    template <typename T>
    void from_uint(T n) {
        auto [ptr, _] = std::to_chars(buf_, buf_ + kBufSize, n);
        piece_ = std::string_view(buf_, ptr - buf_);
    }

    void from_double(double n) {
        auto [ptr, _] = std::to_chars(buf_, buf_ + kBufSize, n);
        piece_ = std::string_view(buf_, ptr - buf_);
    }
};

} // namespace strings_internal
} // namespace zeta

#endif // ZETA_STRINGS_INTERNAL_ALPHA_NUM_H
