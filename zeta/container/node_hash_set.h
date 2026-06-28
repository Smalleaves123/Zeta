#ifndef ZETA_CONTAINER_NODE_HASH_SET_H
#define ZETA_CONTAINER_NODE_HASH_SET_H

/// @file   container/node_hash_set.h
/// @brief  Node-based hash set — pointer-stable iterators.
///
/// Like `flat_hash_set` but with individually heap-allocated elements.
///
/// Example:
///   zeta::node_hash_set<std::string> s;
///   s.insert("hello");

#include "zeta/container/internal/raw_hash_set.h"

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>

namespace zeta {

template <typename K,
          typename Hash  = std::hash<K>,
          typename KeyEq = std::equal_to<K>>
class node_hash_set {
    using Node = K;

    struct NodePolicy {
        using key_type   = K;
        using value_type = std::unique_ptr<Node>;
        static const key_type& get_key(const value_type& v) noexcept { return *v; }
    };

    using Table = container_internal::raw_hash_set<NodePolicy, Hash, KeyEq>;

public:
    using key_type        = K;
    using value_type      = K;
    using size_type       = size_t;
    using hasher          = Hash;
    using key_equal       = KeyEq;

    class iterator {
        friend class node_hash_set;
        typename Table::iterator it_;
        explicit iterator(typename Table::iterator it) noexcept : it_(it) {}
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = K;
        using difference_type = std::ptrdiff_t;
        using pointer    = const K*;
        using reference  = const K&;

        iterator() noexcept = default;
        reference operator*()  const noexcept { return *it_->get(); }
        pointer   operator->() const noexcept { return it_->get(); }
        iterator& operator++() noexcept { ++it_; return *this; }
        iterator  operator++(int) noexcept { auto t = *this; ++(*this); return t; }
        friend bool operator==(iterator a, iterator b) noexcept { return a.it_ == b.it_; }
        friend bool operator!=(iterator a, iterator b) noexcept { return a.it_ != b.it_; }
    };

    class const_iterator {
        friend class node_hash_set;
        typename Table::const_iterator it_;
        explicit const_iterator(typename Table::const_iterator it) noexcept : it_(it) {}
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = K;
        using difference_type = std::ptrdiff_t;
        using pointer    = const K*;
        using reference  = const K&;

        const_iterator() noexcept = default;
        const_iterator(iterator it) noexcept : it_(it.it_) {}
        reference operator*()  const noexcept { return *it_->get(); }
        pointer   operator->() const noexcept { return it_->get(); }
        const_iterator& operator++() noexcept { ++it_; return *this; }
        const_iterator  operator++(int) noexcept { auto t = *this; ++(*this); return t; }
        friend bool operator==(const const_iterator& a, const const_iterator& b) noexcept { return a.it_ == b.it_; }
        friend bool operator!=(const const_iterator& a, const const_iterator& b) noexcept { return a.it_ != b.it_; }
    };

    // ── Construction ────────────────────────────────────────────────

    node_hash_set() = default;
    ~node_hash_set() = default;

    node_hash_set(std::initializer_list<value_type> ilist) {
        for (const auto& v : ilist) insert(v);
    }

    node_hash_set(const node_hash_set& other) {
        for (auto& v : other) insert(v);
    }
    node_hash_set& operator=(const node_hash_set& other) {
        if (this != &other) { clear(); for (auto& v : other) insert(v); }
        return *this;
    }
    node_hash_set(node_hash_set&&) noexcept = default;
    node_hash_set& operator=(node_hash_set&&) noexcept = default;

    // ── Capacity ────────────────────────────────────────────────────

    [[nodiscard]] size_t size()      const noexcept { return table_.size(); }
    [[nodiscard]] bool   empty()     const noexcept { return table_.empty(); }
    [[nodiscard]] size_t capacity()  const noexcept { return table_.capacity(); }
    [[nodiscard]] size_t max_size()  const noexcept { return table_.max_size(); }
    void reserve(size_t n) { table_.reserve(n); }
    void rehash(size_t n) { table_.rehash(n); }

    // ── Lookup ──────────────────────────────────────────────────────

    template <typename LK = K>
    [[nodiscard]] iterator find(const LK& key) {
        return iterator(table_.find(key));
    }
    template <typename LK = K>
    [[nodiscard]] const_iterator find(const LK& key) const {
        return const_iterator(table_.find(key));
    }
    template <typename LK = K>
    [[nodiscard]] bool contains(const LK& key) const {
        return table_.contains(key);
    }

    // ── Insert ──────────────────────────────────────────────────────

    std::pair<iterator, bool> insert(const K& v) {
        auto result = table_.insert(std::make_unique<K>(v));
        return {iterator(result.first), result.second};
    }
    std::pair<iterator, bool> insert(K&& v) {
        auto result = table_.insert(std::make_unique<K>(std::move(v)));
        return {iterator(result.first), result.second};
    }

    // ── Erase ───────────────────────────────────────────────────────

    template <typename LK = K>
    size_t erase(const LK& key) {
        return table_.erase(key);
    }
    void clear() { table_.clear(); }

    // ── Iterators ───────────────────────────────────────────────────

    [[nodiscard]] iterator begin() noexcept { return iterator(table_.begin()); }
    [[nodiscard]] iterator end()   noexcept { return iterator(table_.end()); }
    [[nodiscard]] const_iterator begin() const noexcept { return const_iterator(table_.begin()); }
    [[nodiscard]] const_iterator end() const noexcept { return const_iterator(table_.end()); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return end(); }

    void swap(node_hash_set& other) noexcept { table_.swap(other.table_); }
    friend void swap(node_hash_set& a, node_hash_set& b) noexcept { a.swap(b); }

    bool operator==(const node_hash_set& other) const {
        if (size() != other.size()) return false;
        for (const auto& v : *this) {
            if (!other.contains(v)) return false;
        }
        return true;
    }
    bool operator!=(const node_hash_set& other) const { return !(*this == other); }

private:
    Table table_;
};

} // namespace zeta

#endif // ZETA_CONTAINER_NODE_HASH_SET_H
