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
#include <iterator>
#include <optional>
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
        static V& get_mapped(value_type& v) noexcept { return v.second; }
    };

    using Tree = container_internal::Btree<Params>;

public:
    using key_type        = K;
    using mapped_type     = V;
    using value_type      = std::pair<const K, V>;
    using size_type       = size_t;
    using key_compare     = Compare;
    class iterator {
        using BaseIterator = typename Tree::iterator;

        struct reference_proxy {
            const K& first;
            V& second;
        };

        friend class btree_map;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::pair<const K, V>;
        using difference_type = std::ptrdiff_t;
        using pointer = reference_proxy*;
        using reference = reference_proxy&;

        iterator() = default;
        iterator(const iterator& other) : base_(other.base_) {}
        iterator& operator=(const iterator& other) {
            base_ = other.base_;
            proxy_.reset();
            return *this;
        }
        iterator(iterator&& other) noexcept : base_(std::move(other.base_)) {}
        iterator& operator=(iterator&& other) noexcept {
            base_ = std::move(other.base_);
            proxy_.reset();
            return *this;
        }

        reference operator*() const noexcept {
            proxy_.emplace(reference_proxy{base_->first, base_.mapped_value()});
            return *proxy_;
        }
        pointer operator->() const noexcept { return &operator*(); }

        iterator& operator++() noexcept {
            ++base_;
            proxy_.reset();
            return *this;
        }
        iterator operator++(int) noexcept {
            iterator copy = *this;
            ++(*this);
            return copy;
        }
        iterator& operator--() noexcept {
            --base_;
            proxy_.reset();
            return *this;
        }
        iterator operator--(int) noexcept {
            iterator copy = *this;
            --(*this);
            return copy;
        }

        friend bool operator==(const iterator& left,
                               const iterator& right) noexcept {
            return left.base_ == right.base_;
        }
        friend bool operator!=(const iterator& left,
                               const iterator& right) noexcept {
            return !(left == right);
        }

    private:
        explicit iterator(BaseIterator base) noexcept : base_(std::move(base)) {}

        BaseIterator base_;
        mutable std::optional<reference_proxy> proxy_;
    };
    using const_iterator  = typename Tree::iterator;

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

    [[nodiscard]] iterator find(const K& key) { return iterator(tree_.find(key)); }
    [[nodiscard]] const_iterator find(const K& key) const { return tree_.find(key); }
    [[nodiscard]] bool contains(const K& key) const { return tree_.contains(key); }

    V& operator[](const K& key) {
        auto it = find(key);
        if (it == end()) {
            auto [new_it, _] = insert({key, V{}});
            return new_it->second;
        }
        return it->second;
    }

    // ── Insert ──────────────────────────────────────────────────────

    std::pair<iterator, bool> insert(const value_type& v) {
        auto [it, inserted] = tree_.insert(v);
        return {iterator(std::move(it)), inserted};
    }
    std::pair<iterator, bool> insert(value_type&& v) {
        auto [it, inserted] = tree_.insert(std::move(v));
        return {iterator(std::move(it)), inserted};
    }

    // ── Erase ───────────────────────────────────────────────────────

    size_t erase(const K& key) { return tree_.erase(key); }
    void   clear()             { tree_.clear(); }

    // ── Iterators ───────────────────────────────────────────────────

    [[nodiscard]] iterator begin() noexcept { return iterator(tree_.begin()); }
    [[nodiscard]] iterator end()   noexcept { return iterator(tree_.end()); }
    [[nodiscard]] const_iterator begin() const noexcept { return tree_.begin(); }
    [[nodiscard]] const_iterator end()   const noexcept { return tree_.end(); }

private:
    Tree tree_;
};

} // namespace zeta

#endif // ZETA_CONTAINER_BTREE_MAP_H
