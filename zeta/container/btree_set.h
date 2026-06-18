#ifndef ZETA_CONTAINER_BTREE_SET_H
#define ZETA_CONTAINER_BTREE_SET_H

/// @file   container/btree_set.h
/// @brief  B-Tree based ordered set (like `absl::btree_set`).
///
/// Example:
///   zeta::btree_set<int> s = {3, 1, 2};
///   for (int v : s) { ... }  // ordered: 1, 2, 3

#include "zeta/container/internal/btree_impl.h"

#include <cstddef>
#include <functional>
#include <utility>

namespace zeta {

template <typename K, typename Compare = std::less<K>>
class btree_set {
    struct Params {
        using value_type  = K;
        using key_type    = K;
        using key_compare = Compare;
        using allocator_type = void;

        static const key_type& get_key(const value_type& v) noexcept {
            return v;
        }
    };

    using Tree = container_internal::Btree<Params>;

public:
    using key_type        = K;
    using value_type      = K;
    using size_type       = size_t;
    using key_compare     = Compare;
    using iterator        = typename Tree::iterator;

    // ── Construction ────────────────────────────────────────────────

    btree_set() = default;
    ~btree_set() = default;

    btree_set(const btree_set&) = default;
    btree_set& operator=(const btree_set&) = default;
    btree_set(btree_set&&) noexcept = default;
    btree_set& operator=(btree_set&&) noexcept = default;

    btree_set(std::initializer_list<K> ilist) {
        for (const auto& v : ilist) insert(v);
    }

    // ── Capacity ────────────────────────────────────────────────────

    [[nodiscard]] size_t size()  const noexcept { return tree_.size(); }
    [[nodiscard]] bool   empty() const noexcept { return tree_.empty(); }

    // ── Lookup ──────────────────────────────────────────────────────

    [[nodiscard]] iterator find(const K& key) const { return tree_.find(key); }
    [[nodiscard]] bool contains(const K& key) const { return tree_.contains(key); }

    // ── Insert ──────────────────────────────────────────────────────

    std::pair<iterator, bool> insert(const K& v) { return tree_.insert(v); }
    std::pair<iterator, bool> insert(K&& v)      { return tree_.insert(std::move(v)); }

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

#endif // ZETA_CONTAINER_BTREE_SET_H
