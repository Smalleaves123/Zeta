#ifndef ZETA_CONTAINER_FLAT_HASH_MAP_H
#define ZETA_CONTAINER_FLAT_HASH_MAP_H

/// @file   container/flat_hash_map.h
/// @brief  Swiss Table based flat hash map.
///
/// `zeta::flat_hash_map<K, V>` is a drop-in replacement for
/// `std::unordered_map<K, V>` that is typically 2-3x faster and uses
/// significantly less memory thanks to open addressing and SIMD probing.
///
/// Supports heterogeneous lookup when Hash and KeyEq are transparent.
///
/// Example:
///   zeta::flat_hash_map<std::string, int> m;
///   m["hello"] = 42;
///   auto it = m.find("hello"); // heterogeneous: no string temporary

#include "zeta/container/internal/raw_hash_set.h"

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace zeta {

template <typename K, typename V,
          typename Hash  = std::hash<K>,
          typename KeyEq = std::equal_to<K>>
class flat_hash_map {
    using Impl = container_internal::raw_hash_set<
        container_internal::MapPolicy<K, V>, Hash, KeyEq>;

public:
    using key_type        = K;
    using mapped_type     = V;
    using value_type      = std::pair<const K, V>;
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
    flat_hash_map() = default;
    ~flat_hash_map() = default;

    flat_hash_map(std::initializer_list<value_type> ilist) {
        for (const auto& p : ilist) insert(p);
    }

    flat_hash_map(const flat_hash_map&) = default;
    flat_hash_map& operator=(const flat_hash_map&) = default;
    flat_hash_map(flat_hash_map&&) noexcept = default;
    flat_hash_map& operator=(flat_hash_map&&) noexcept = default;

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

    template <typename LK = K>
    V& at(const LK& key) {
        auto it = find(key);
        if (it == end()) throw std::out_of_range("flat_hash_map::at");
        return it->second;
    }
    template <typename LK = K>
    const V& at(const LK& key) const {
        auto it = find(key);
        if (it == end()) throw std::out_of_range("flat_hash_map::at");
        return it->second;
    }

    V& operator[](const K& key) { return table_[key]; }
    V& operator[](K&& key)      { return table_[std::move(key)]; }

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

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(const K& key, Args&&... args) {
        return table_.try_emplace(key, std::forward<Args>(args)...);
    }
    template <typename... Args>
    std::pair<iterator, bool> try_emplace(K&& key, Args&&... args) {
        return table_.try_emplace(std::move(key),
                                  std::forward<Args>(args)...);
    }

    template <typename M>
    std::pair<iterator, bool> insert_or_assign(const K& key, M&& obj) {
        return table_.insert_or_assign(key, std::forward<M>(obj));
    }
    template <typename M>
    std::pair<iterator, bool> insert_or_assign(K&& key, M&& obj) {
        return table_.insert_or_assign(std::move(key),
                                       std::forward<M>(obj));
    }

    // ── Erase ─────────────────────────────────────────────────────
    iterator erase(const_iterator pos) { return table_.erase(pos); }
    iterator erase(const_iterator first, const_iterator last) {
        return table_.erase(first, last);
    }

    template <typename LK = K,
              typename = std::enable_if_t<
                  !std::is_convertible_v<const LK&, const_iterator>>>
    size_t erase(const LK& key) {
        return table_.erase(key);
    }

    void clear() { table_.clear(); }

    // ── Swap ──────────────────────────────────────────────────────
    void swap(flat_hash_map& other) noexcept { table_.swap(other.table_); }
    friend void swap(flat_hash_map& a, flat_hash_map& b) noexcept { a.swap(b); }

    // ── Equality ──────────────────────────────────────────────────
    bool operator==(const flat_hash_map& other) const {
        return table_ == other.table_;
    }
    bool operator!=(const flat_hash_map& other) const {
        return !(*this == other);
    }

private:
    Impl table_;
};

template <typename K, typename V, typename Hash, typename KeyEq, typename Pred>
size_t erase_if(flat_hash_map<K, V, Hash, KeyEq>& map, Pred pred) {
    size_t removed = 0;
    for (auto it = map.begin(); it != map.end();) {
        if (pred(*it)) {
            it = map.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}

} // namespace zeta

#endif // ZETA_CONTAINER_FLAT_HASH_MAP_H
