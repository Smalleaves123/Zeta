#ifndef ZETA_CONTAINER_BTREE_MAP_H
#define ZETA_CONTAINER_BTREE_MAP_H

/// @file   container/btree_map.h
/// @brief  B-Tree based ordered map (like `absl::btree_map`).
///
/// `zeta::btree_map<K, V>` is a sorted associative container backed by a
/// B-Tree.  It provides:
///   - O(log N) lookup, insertion, deletion
///   - Cache-friendly node layout (~64 elements per node)
///   - Bidirectional iterators in key order
///   - Lower memory overhead than `std::map` (no per-element pointers)
///
/// Example:
///   zeta::btree_map<std::string, int> m;
///   m["hello"] = 42;
///   for (auto& [k, v] : m) { ... }

#include "zeta/container/internal/btree_impl.h"

#include <cstddef>
#include <functional>
#include <utility>

namespace zeta {

template <typename K, typename V,
          typename Compare = std::less<K>>
class btree_map {
    struct Params {
        using value_type  = std::pair<K, V>;  // non-const K for vector storage
        using key_type    = K;
        using key_compare = Compare;
        using allocator_type = void;

        static const key_type& get_key(const value_type& v) noexcept {
            return v.first;
        }
    };

    using Tree = container_internal::Btree<Params>;

public:
    using key_type        = K;
    using mapped_type     = V;
    using value_type      = std::pair<const K, V>;
    using size_type       = size_t;
    using key_compare     = Compare;
    using iterator        = typename Tree::iterator;

    // ── Construction ────────────────────────────────────────────────

    btree_map() = default;
    ~btree_map() = default;

    btree_map(const btree_map&) = default;
    btree_map& operator=(const btree_map&) = default;
    btree_map(btree_map&&) noexcept = default;
    btree_map& operator=(btree_map&&) noexcept = default;

    btree_map(std::initializer_list<value_type> ilist) {
        for (const auto& p : ilist) insert(p);
    }

    // ── Capacity ────────────────────────────────────────────────────

    [[nodiscard]] size_t size()  const noexcept { return tree_.size(); }
    [[nodiscard]] bool   empty() const noexcept { return tree_.empty(); }

    // ── Lookup ──────────────────────────────────────────────────────

    [[nodiscard]] iterator find(const K& key) const { return tree_.find(key); }
    [[nodiscard]] bool contains(const K& key) const { return tree_.contains(key); }

    V& operator[](const K& key) {
        auto it = find(key);
        if (it == end()) {
            auto [new_it, _] = insert({key, V{}});
            return const_cast<V&>(new_it->second);
        }
        return const_cast<V&>(it->second);
    }

    // ── Insert ──────────────────────────────────────────────────────

    std::pair<iterator, bool> insert(const value_type& v) {
        return tree_.insert(v);
    }
    std::pair<iterator, bool> insert(value_type&& v) {
        return tree_.insert(std::move(v));
    }

    // ── Erase ───────────────────────────────────────────────────────

    size_t erase(const K& key) { return tree_.erase(key); }
    void   clear()             { tree_.clear(); }

    // ── Iterators ───────────────────────────────────────────────────

    [[nodiscard]] iterator begin() const noexcept { return tree_.begin(); }
    [[nodiscard]] iterator end()   const noexcept { return tree_.end(); }

private:
    Tree tree_;
};

} // namespace zeta

#endif // ZETA_CONTAINER_BTREE_MAP_H
