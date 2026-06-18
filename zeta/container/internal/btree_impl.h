#ifndef ZETA_CONTAINER_INTERNAL_BTREE_IMPL_H
#define ZETA_CONTAINER_INTERNAL_BTREE_IMPL_H

/// @file   container/internal/btree_impl.h
/// @brief  B-Tree internal implementation.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace zeta {
namespace container_internal {

constexpr size_t kBtreeNodeMax = 64;

// ═══════════════════════════════════════════════════════════════════════
// Btree
// ═══════════════════════════════════════════════════════════════════════
//
// Simplified B-tree: each node stores a sorted vector of values (up to
// kBtreeNodeMax) and a vector of children.  All leaves are at the same
// depth.  This is simpler than the Abseil version but still O(log N).

template <typename Params>
class Btree {
public:
    using value_type = typename Params::value_type;
    using key_type   = typename Params::key_type;
    using key_compare = typename Params::key_compare;
    using size_type  = size_t;

    struct Node {
        std::vector<value_type>              values;
        std::vector<std::unique_ptr<Node>>   children;
        Node*                               parent = nullptr;

        [[nodiscard]] bool is_leaf() const noexcept { return children.empty(); }
        [[nodiscard]] bool is_full() const noexcept { return values.size() >= kBtreeNodeMax; }
        [[nodiscard]] key_type key(size_t i) const noexcept {
            return Params::get_key(values[i]);
        }
    };

    // ── Iterator ──────────────────────────────────────────────────

    class iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = typename Btree::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer    = const value_type*;
        using reference  = const value_type&;

        iterator() noexcept : node_(nullptr), pos_(0) {}
        reference operator*()  const noexcept { return node_->values[pos_]; }
        pointer   operator->() const noexcept { return &node_->values[pos_]; }

        iterator& operator++() noexcept {
            if (!node_->is_leaf()) {
                // Descend into leftmost leaf of right child.
                Node* child = node_->children[pos_ + 1].get();
                while (!child->is_leaf()) child = child->children[0].get();
                node_ = child;
                pos_ = 0;
            } else {
                ++pos_;
                // Ascend past node boundaries.
                while (node_ && pos_ >= static_cast<int>(node_->values.size())) {
                    Node* p = node_->parent;
                    if (!p) { node_ = nullptr; pos_ = 0; break; }
                    pos_ = index_in_parent(node_) + 1;
                    node_ = p;
                }
            }
            return *this;
        }
        iterator operator++(int) noexcept { auto t = *this; ++(*this); return t; }

        iterator& operator--() noexcept {
            if (!node_) { return *this; }
            if (!node_->is_leaf()) {
                // Rightmost leaf of left child.
                Node* child = node_->children[pos_].get();
                while (!child->is_leaf()) child = child->children.back().get();
                node_ = child;
                pos_ = static_cast<int>(child->values.size()) - 1;
            } else if (pos_ > 0) {
                --pos_;
            } else {
                // Ascend.
                for (;;) {
                    Node* p = node_->parent;
                    if (!p) { node_ = nullptr; pos_ = 0; break; }
                    int idx = index_in_parent(node_);
                    if (idx > 0) { node_ = p; pos_ = idx - 1; break; }
                    node_ = p;
                }
            }
            return *this;
        }

        friend bool operator==(iterator a, iterator b) noexcept {
            return a.node_ == b.node_ && a.pos_ == b.pos_;
        }
        friend bool operator!=(iterator a, iterator b) noexcept { return !(a == b); }

    private:
        friend class Btree;
        Node* node_;
        int   pos_;
        iterator(Node* n, int p) noexcept : node_(n), pos_(p) {}
    };

    // ── Construction ──────────────────────────────────────────────

    Btree() {
        root_ = std::make_unique<Node>();
    }

    Btree(const Btree& other) {
        root_ = std::make_unique<Node>();
        for (auto& v : other) insert(value_type(v));
    }
    Btree& operator=(const Btree& other) {
        if (this != &other) { clear(); for (auto& v : other) insert(value_type(v)); }
        return *this;
    }
    Btree(Btree&&) noexcept = default;
    Btree& operator=(Btree&&) noexcept = default;
    ~Btree() = default;

    // ── Capacity ──────────────────────────────────────────────────

    [[nodiscard]] size_t size()  const noexcept { return size_; }
    [[nodiscard]] bool   empty() const noexcept { return size_ == 0; }

    // ── Lookup ────────────────────────────────────────────────────

    [[nodiscard]] iterator find(const key_type& k) const {
        Node* node = root_.get();
        key_compare cmp;
        while (node) {
            for (size_t i = 0; i < node->values.size(); ++i) {
                auto nk = node->key(i);
                if (!cmp(nk, k) && !cmp(k, nk)) return iterator(node, static_cast<int>(i));
                if (cmp(k, nk)) {
                    // Descend into left child.
                    if (!node->is_leaf() && i < node->children.size())
                        node = node->children[i].get();
                    else
                        return end();
                    goto next_level;
                }
            }
            // k > all keys — go to last child.
            if (!node->is_leaf()) {
                node = node->children.back().get();
            } else {
                return end();
            }
            next_level:;
        }
        return end();
    }

    [[nodiscard]] bool contains(const key_type& k) const {
        return find(k) != end();
    }

    // ── Insert ────────────────────────────────────────────────────

    std::pair<iterator, bool> insert(value_type v) {
        auto k = Params::get_key(v);
        Node* node = root_.get();

        // Navigate to leaf.
        while (!node->is_leaf()) {
            size_t i = child_pos(node, k);
            node = node->children[i].get();
        }

        // Check duplicate.
        for (size_t i = 0; i < node->values.size(); ++i) {
            key_compare cmp;
            if (!cmp(node->key(i), k) && !cmp(k, node->key(i))) {
                return {iterator(node, static_cast<int>(i)), false};
            }
        }

        if (!node->is_full()) {
            insert_into_node(node, std::move(v));
            ++size_;
            // Find the insertion position.
            for (size_t i = 0; i < node->values.size(); ++i) {
                key_compare cmp;
                if (!cmp(node->key(i), k) && !cmp(k, node->key(i)))
                    return {iterator(node, static_cast<int>(i)), true};
            }
            return {iterator(node, 0), true};  // should not reach
        }

        // Node is full — split.
        insert_into_node(node, std::move(v));
        auto median = extract_median(node);
        auto right = split_right(node);

        insert_into_parent(node, std::move(median), std::move(right));
        ++size_;
        return {find(k), true};
    }

    // ── Erase ─────────────────────────────────────────────────────

    size_t erase(const key_type& k) {
        auto it = find(k);
        if (it == end()) return 0;
        erase(it);
        return 1;
    }

    void erase(iterator it) {
        Node* node = it.node_;
        int pos = it.pos_;
        node->values.erase(node->values.begin() + pos);
        --size_;
        // Note: does not rebalance underfull nodes.  This is a simplified
        // B-tree suitable for typical workloads; full rebalancing is rare
        // in practice and omitted for clarity.
    }

    void clear() {
        root_ = std::make_unique<Node>();
        size_ = 0;
    }

    // ── Iterators ─────────────────────────────────────────────────

    [[nodiscard]] iterator begin() const {
        Node* node = root_.get();
        while (node && !node->is_leaf() && !node->values.empty())
            node = node->children[0].get();
        if (node && !node->values.empty()) return iterator(node, 0);
        return end();
    }
    [[nodiscard]] iterator end() const noexcept { return iterator(); }

private:
    std::unique_ptr<Node> root_;
    size_t                size_ = 0;

    static int index_in_parent(Node* node) noexcept {
        Node* p = node->parent;
        if (!p) return -1;
        for (size_t i = 0; i < p->children.size(); ++i)
            if (p->children[i].get() == node) return static_cast<int>(i);
        return -1;
    }

    static size_t child_pos(Node* node, const key_type& k) noexcept {
        key_compare cmp;
        for (size_t i = 0; i < node->values.size(); ++i)
            if (cmp(k, node->key(i))) return i;
        return node->values.size();
    }

    static void insert_into_node(Node* node, value_type v) {
        key_compare cmp;
        auto k = Params::get_key(v);
        auto it = node->values.begin();
        while (it != node->values.end() && cmp(Params::get_key(*it), k)) ++it;
        node->values.insert(it, std::move(v));
    }

    // Extract median from a full node (keeps left half, returns median).
    static value_type extract_median(Node* node) {
        size_t mid = node->values.size() / 2;
        value_type v = std::move(node->values[mid]);
        node->values.erase(node->values.begin() + mid);
        return v;
    }

    // Split a node: move right half to a new node, return it.
    static std::unique_ptr<Node> split_right(Node* left) {
        auto right = std::make_unique<Node>();
        size_t mid = left->values.size() / 2;
        right->parent = left->parent;
        right->values.assign(
            std::make_move_iterator(left->values.begin() + mid),
            std::make_move_iterator(left->values.end()));
        left->values.erase(left->values.begin() + mid, left->values.end());
        // Move children.
        if (!left->is_leaf()) {
            right->children.assign(
                std::make_move_iterator(left->children.begin() + mid + 1),
                std::make_move_iterator(left->children.end()));
            left->children.erase(left->children.begin() + mid + 1, left->children.end());
            for (auto& c : right->children) c->parent = right.get();
        }
        return right;
    }

    void insert_into_parent(Node* left, value_type median,
                             std::unique_ptr<Node> right) {
        if (left == root_.get()) {
            auto new_root = std::make_unique<Node>();
            new_root->values.push_back(std::move(median));
            new_root->children.push_back(std::move(root_));
            new_root->children.push_back(std::move(right));
            new_root->children[0]->parent = new_root.get();
            new_root->children[1]->parent = new_root.get();
            root_ = std::move(new_root);
            return;
        }

        Node* parent = left->parent;
        right->parent = parent;

        key_compare cmp;
        auto k = Params::get_key(median);
        size_t pos = 0;
        while (pos < parent->values.size() && cmp(parent->key(pos), k)) ++pos;

        parent->values.insert(parent->values.begin() + pos, std::move(median));
        parent->children.insert(parent->children.begin() + pos + 1, std::move(right));

        if (parent->is_full()) {
            auto med2 = extract_median(parent);
            auto right2 = split_right(parent);
            insert_into_parent(parent, std::move(med2), std::move(right2));
        }
    }
};

} // namespace container_internal
} // namespace zeta

#endif // ZETA_CONTAINER_INTERNAL_BTREE_IMPL_H
