#ifndef ZETA_CONTAINER_NODE_HASH_MAP_H
#define ZETA_CONTAINER_NODE_HASH_MAP_H

/// @file   container/node_hash_map.h
/// @brief  Node-based hash map — pointer-stable iterators.
///
/// Unlike `flat_hash_map`, elements are individually heap-allocated so
/// references and iterators remain valid across rehash / insert.
/// Performance is slightly lower due to the pointer indirection.
///
/// Example:
///   zeta::node_hash_map<std::string, int> m;
///   m["hello"] = 42;
///   auto it = m.find("hello");  // iterator stays valid after more inserts

#include "zeta/container/internal/raw_hash_set.h"

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <utility>

namespace zeta {

template <typename K, typename V,
          typename Hash  = std::hash<K>,
          typename KeyEq = std::equal_to<K>>
class node_hash_map {
    using Policy = container_internal::MapPolicy<K, V>;
    using Node   = std::pair<const K, V>;

    struct NodePolicy {
        using key_type   = K;
        using value_type = std::unique_ptr<Node>;
        static const key_type& get_key(const value_type& v) noexcept {
            return Policy::get_key(*v);
        }
    };

    using Table = container_internal::raw_hash_set<NodePolicy, Hash, KeyEq>;

public:
    using key_type        = K;
    using mapped_type     = V;
    using value_type      = std::pair<const K, V>;
    using size_type       = size_t;
    using hasher          = Hash;
    using key_equal       = KeyEq;

    class iterator {
        friend class node_hash_map;
        typename Table::iterator it_;
        explicit iterator(typename Table::iterator it) noexcept : it_(it) {}
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const K, V>;
        using difference_type = std::ptrdiff_t;
        using pointer    = value_type*;
        using reference  = value_type&;

        iterator() noexcept = default;
        reference operator*()  const noexcept { return *it_->get(); }
        pointer   operator->() const noexcept { return it_->get(); }
        iterator& operator++() noexcept { ++it_; return *this; }
        iterator  operator++(int) noexcept { auto t = *this; ++(*this); return t; }
        friend bool operator==(iterator a, iterator b) noexcept { return a.it_ == b.it_; }
        friend bool operator!=(iterator a, iterator b) noexcept { return a.it_ != b.it_; }
    };

    class const_iterator {
        friend class node_hash_map;
        typename Table::const_iterator it_;
        explicit const_iterator(typename Table::const_iterator it) noexcept : it_(it) {}
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const K, V>;
        using difference_type = std::ptrdiff_t;
        using pointer    = const value_type*;
        using reference  = const value_type&;

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

    node_hash_map() = default;
    ~node_hash_map() = default;

    node_hash_map(std::initializer_list<value_type> ilist) {
        for (const auto& v : ilist) insert(v);
    }

    node_hash_map(const node_hash_map& other) {
        for (auto& kv : other) insert(kv);
    }
    node_hash_map& operator=(const node_hash_map& other) {
        if (this != &other) { clear(); for (auto& kv : other) insert(kv); }
        return *this;
    }
    node_hash_map(node_hash_map&&) noexcept = default;
    node_hash_map& operator=(node_hash_map&&) noexcept = default;

    // ── Capacity ────────────────────────────────────────────────────

    [[nodiscard]] size_t size()      const noexcept { return table_.size(); }
    [[nodiscard]] bool   empty()     const noexcept { return table_.empty(); }
    [[nodiscard]] size_t capacity()  const noexcept { return table_.capacity(); }
    void reserve(size_t n) { table_.reserve(n); }

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

    template <typename LK = K>
    [[nodiscard]] V& at(const LK& key) {
        auto it = find(key);
        if (it == end()) throw std::out_of_range("node_hash_map::at");
        return it->second;
    }
    template <typename LK = K>
    [[nodiscard]] const V& at(const LK& key) const {
        auto it = find(key);
        if (it == end()) throw std::out_of_range("node_hash_map::at");
        return it->second;
    }

    V& operator[](const K& key) {
        auto it = find(key);
        if (it == end()) {
            auto node = std::make_unique<Node>(key, V{});
            auto [tbl_it, ok] = insert_node(std::move(node));
            return tbl_it->second;
        }
        return it->second;
    }

    // ── Insert / emplace ────────────────────────────────────────────

    std::pair<iterator, bool> insert(const value_type& v) {
        auto node = std::make_unique<Node>(v);
        return insert_node(std::move(node));
    }
    std::pair<iterator, bool> insert(value_type&& v) {
        auto node = std::make_unique<Node>(std::move(v));
        return insert_node(std::move(node));
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(const K& key, Args&&... args) {
        auto it = find(key);
        if (it != end()) return {it, false};
        auto node = std::make_unique<Node>(
            std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple(std::forward<Args>(args)...));
        return insert_node(std::move(node));
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

private:
    Table table_;

    std::pair<iterator, bool> insert_node(std::unique_ptr<Node> node) {
        auto result = table_.insert(std::move(node));
        return {iterator(result.first), result.second};
    }
};

} // namespace zeta

#endif // ZETA_CONTAINER_NODE_HASH_MAP_H
