#ifndef ZETA_CONTAINER_FLAT_HASH_SET_H
#define ZETA_CONTAINER_FLAT_HASH_SET_H

/// @file   container/flat_hash_set.h
/// @brief  Swiss Table based flat hash set.
///
/// `zeta::flat_hash_set<K>` is a drop-in replacement for
/// `std::unordered_set<K>` with better performance and lower memory usage.
///
/// Supports heterogeneous lookup when Hash and KeyEq are transparent.
///
/// Example:
///   zeta::flat_hash_set<std::string> s = {"hello", "world"};
///   bool ok = s.contains("hello"); // heterogeneous: no string temporary

#include "zeta/container/internal/raw_hash_set.h"

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace zeta {

template <typename K,
          typename Hash  = std::hash<K>,
          typename KeyEq = std::equal_to<K>>
class flat_hash_set {
    using Impl = container_internal::raw_hash_set<
        container_internal::SetPolicy<K>, Hash, KeyEq>;

public:
    using key_type        = K;
    using value_type      = K;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using hasher          = Hash;
    using key_equal       = KeyEq;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;
    using iterator        = typename Impl::iterator;
    using const_iterator  = typename Impl::const_iterator;

    // ── Construction ──────────────────────────────────────────────
    flat_hash_set() = default;
    ~flat_hash_set() = default;

    flat_hash_set(std::initializer_list<value_type> ilist) {
        for (const auto& v : ilist) insert(v);
    }

    flat_hash_set(const flat_hash_set&) = default;
    flat_hash_set& operator=(const flat_hash_set&) = default;
    flat_hash_set(flat_hash_set&&) noexcept = default;
    flat_hash_set& operator=(flat_hash_set&&) noexcept = default;

    // ── Capacity ──────────────────────────────────────────────────
    size_t size()      const noexcept { return table_.size(); }
    size_t capacity()  const noexcept { return table_.capacity(); }
    bool   empty()     const noexcept { return table_.empty(); }
    size_t max_size()  const noexcept { return table_.max_size(); }

    void reserve(size_t n) { table_.reserve(n); }
    void rehash(size_t n)  { table_.rehash(n); }

    // ── Iterators ─────────────────────────────────────────────────
    iterator       begin()        noexcept { return table_.begin(); }
    const_iterator begin()  const noexcept { return table_.begin(); }
    iterator       end()          noexcept { return table_.end(); }
    const_iterator end()    const noexcept { return table_.end(); }
    const_iterator cbegin() const noexcept { return table_.cbegin(); }
    const_iterator cend()   const noexcept { return table_.cend(); }

    // ── Lookup (heterogeneous when transparent) ───────────────────
    template <typename LK = K>
    iterator find(const LK& key) {
        return table_.find(key);
    }
    template <typename LK = K>
    const_iterator find(const LK& key) const {
        return table_.find(key);
    }

    template <typename LK = K>
    bool contains(const LK& key) const {
        return table_.contains(key);
    }

    template <typename LK = K>
    size_t count(const LK& key) const {
        return table_.count(key);
    }

    // ── Insert / emplace ──────────────────────────────────────────
    std::pair<iterator, bool> insert(const value_type& v) {
        return table_.insert(v);
    }
    std::pair<iterator, bool> insert(value_type&& v) {
        return table_.insert(std::move(v));
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        return table_.emplace(std::forward<Args>(args)...);
    }

    // ── Erase ─────────────────────────────────────────────────────
    iterator erase(const_iterator pos) { return table_.erase(pos); }

    template <typename LK = K,
              typename = std::enable_if_t<
                  !std::is_convertible_v<const LK&, const_iterator>>>
    size_t erase(const LK& key) {
        return table_.erase(key);
    }

    void clear() { table_.clear(); }

    // ── Swap ──────────────────────────────────────────────────────
    void swap(flat_hash_set& other) noexcept { table_.swap(other.table_); }
    friend void swap(flat_hash_set& a, flat_hash_set& b) noexcept { a.swap(b); }

    // ── Equality ──────────────────────────────────────────────────
    bool operator==(const flat_hash_set& other) const {
        return table_ == other.table_;
    }
    bool operator!=(const flat_hash_set& other) const {
        return !(*this == other);
    }

private:
    Impl table_;
};

} // namespace zeta

#endif // ZETA_CONTAINER_FLAT_HASH_SET_H
