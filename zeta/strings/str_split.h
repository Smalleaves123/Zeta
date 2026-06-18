#ifndef ZETA_STRINGS_STR_SPLIT_H
#define ZETA_STRINGS_STR_SPLIT_H

/// @file   strings/str_split.h
/// @brief  Zero-copy string splitting yielding `std::string_view` pieces.
///
/// Delimiter types:
///   - ByChar{c}        — split on single character
///   - ByString{s}      — split on multi-character string
///   - ByAnyChar{chars} — split on any character in set
///
/// Options (compile-time template flags):
///   - SkipEmpty — omit empty pieces
///   - MaxSplits — produce at most N pieces (remaining goes into last piece)
///
/// Examples:
///   for (auto p : StrSplit("a,b,c", ',')) { ... }
///   for (auto p : StrSplit("a::b::c", "::")) { ... }
///   for (auto p : StrSplit("x, y, z", ByAnyChar{", "}, SkipEmpty)) { ... }

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <string_view>
#include <type_traits>

namespace zeta {

// ── Delimiter types ────────────────────────────────────────────────
struct ByChar   { char c; };
struct ByString { std::string_view s; };
struct ByAnyChar{ std::string_view chars; };

// ── Option tags ────────────────────────────────────────────────────
struct SkipEmpty_t {};
constexpr SkipEmpty_t SkipEmpty{};
struct MaxSplits_t { size_t n; };

// ═══════════════════════════════════════════════════════════════════
namespace strings_internal {

template <typename Delim, bool SkipEmptyV>
class SplitIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type        = std::string_view;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const value_type*;
    using reference         = const value_type&;

    SplitIterator() noexcept : done_(true) {}

    SplitIterator(std::string_view text, Delim delim,
                  size_t max_splits = SIZE_MAX) noexcept
        : remaining_(text), delim_(delim),
          max_splits_(max_splits), count_(0) {
        advance();
    }

    reference operator*()  const noexcept { return current_; }
    pointer   operator->() const noexcept { return &current_; }

    SplitIterator& operator++() noexcept {
        advance();
        return *this;
    }
    SplitIterator operator++(int) noexcept {
        SplitIterator tmp = *this;
        advance();
        return tmp;
    }

    friend bool operator==(const SplitIterator& a,
                           const SplitIterator& b) noexcept {
        if (a.done_ && b.done_) return true;
        return a.remaining_.data() == b.remaining_.data() &&
               a.remaining_.size() == b.remaining_.size() &&
               a.done_ == b.done_;
    }
    friend bool operator!=(const SplitIterator& a,
                           const SplitIterator& b) noexcept {
        return !(a == b);
    }

private:
    static constexpr size_t npos = std::string_view::npos;

    std::string_view remaining_;
    Delim            delim_;
    std::string_view current_;
    bool             done_          = false;
    bool             need_trailing_ = false;
    size_t           max_splits_    = SIZE_MAX;
    size_t           count_         = 0;

    void advance() {
        if (done_) return;

        // Trailing empty after delimiter-at-end
        if (need_trailing_) {
            need_trailing_ = false;
            if constexpr (SkipEmptyV) {
                // trailing empty counts as empty → skip it
                done_ = true;
                return;
            }
            current_ = std::string_view{};
            ++count_;
            return;
        }

        if (remaining_.empty()) {
            done_ = true;
            return;
        }

        // MaxSplits: if we've produced enough, the rest is one piece.
        // Don't set done_=true — let the next advance() detect empty
        // remaining_ so the current piece is yielded to the caller.
        if (max_splits_ != SIZE_MAX && count_ + 1 >= max_splits_) {
            current_   = remaining_;
            remaining_ = std::string_view{};
            ++count_;
            return;
        }

        size_t pos = find_delim(remaining_);
        if (pos == npos) {
            current_   = remaining_;
            remaining_ = std::string_view{};
        } else {
            current_   = remaining_.substr(0, pos);
            size_t dsz = delim_size();
            remaining_ = remaining_.substr(pos + dsz);
            if (remaining_.empty()) need_trailing_ = true;
        }

        // Skip empty pieces
        if constexpr (SkipEmptyV) {
            if (current_.empty() && !done_) {
                advance();
                return;
            }
        }
        ++count_;
    }

    size_t find_delim(std::string_view s) const noexcept {
        if constexpr (std::is_same_v<Delim, ByChar>) {
            return s.find(delim_.c);
        } else if constexpr (std::is_same_v<Delim, ByString>) {
            return s.find(delim_.s);
        } else {
            return s.find_first_of(delim_.chars);
        }
    }

    size_t delim_size() const noexcept {
        if constexpr (std::is_same_v<Delim, ByString>) {
            return delim_.s.size();
        } else {
            return 1;
        }
    }
};

} // namespace strings_internal

// ═══════════════════════════════════════════════════════════════════
// SplitRange
// ═══════════════════════════════════════════════════════════════════
template <typename Delim, bool SkipEmptyV = false>
class SplitRange {
public:
    using iterator = strings_internal::SplitIterator<Delim, SkipEmptyV>;

    SplitRange(std::string_view text, Delim delim,
               size_t max_splits = SIZE_MAX) noexcept
        : text_(text), delim_(delim), max_splits_(max_splits) {}

    iterator begin() const noexcept {
        return iterator(text_, delim_, max_splits_);
    }
    iterator end()   const noexcept { return iterator(); }

private:
    std::string_view text_;
    Delim            delim_;
    size_t           max_splits_ = SIZE_MAX;
};

// ── Factory functions ──────────────────────────────────────────────

/// Split by a single character.
inline SplitRange<ByChar> StrSplit(std::string_view text,
                                    char delim) noexcept {
    return {text, ByChar{delim}};
}
inline SplitRange<ByChar, true> StrSplit(std::string_view text,
                                          char delim, SkipEmpty_t) noexcept {
    return {text, ByChar{delim}};
}
inline SplitRange<ByChar, true> StrSplit(std::string_view text,
                                          char delim, SkipEmpty_t,
                                          MaxSplits_t ms) noexcept {
    return {text, ByChar{delim}, ms.n};
}
inline SplitRange<ByChar, false> StrSplit(std::string_view text,
                                           char delim, MaxSplits_t ms) noexcept {
    return {text, ByChar{delim}, ms.n};
}

/// Split by a multi-character string.
inline SplitRange<ByString> StrSplit(std::string_view text,
                                      std::string_view delim) noexcept {
    return {text, ByString{delim}};
}
inline SplitRange<ByString, true> StrSplit(std::string_view text,
                                            std::string_view delim,
                                            SkipEmpty_t) noexcept {
    return {text, ByString{delim}};
}

/// Split by any character in a set.
inline SplitRange<ByAnyChar> StrSplit(std::string_view text,
                                       ByAnyChar delim) noexcept {
    return {text, delim};
}
inline SplitRange<ByAnyChar, true> StrSplit(std::string_view text,
                                             ByAnyChar delim,
                                             SkipEmpty_t) noexcept {
    return {text, delim};
}

} // namespace zeta

#endif // ZETA_STRINGS_STR_SPLIT_H
