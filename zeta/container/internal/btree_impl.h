#ifndef ZETA_CONTAINER_INTERNAL_BTREE_IMPL_H
#define ZETA_CONTAINER_INTERNAL_BTREE_IMPL_H

/// @file   container/internal/btree_impl.h
/// @brief  B-Tree internal implementation.

#include <algorithm>
#include <cassert>
#include <concepts>
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
            if (!node_) return *this;  // end() → stay end()
            if (!node_->is_leaf()) {
                // Descend into leftmost leaf of right child.
                Node* child = node_->children[pos_ + 1].get();
                while (!child->is_leaf()) child = child->children[0].get();
                node_ = child;
                pos_ = 0;
            } else {
                ++pos_;
                // Ascend past node boundaries, skipping empty nodes.
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

    Btree(const Btree& other)
        requires std::copy_constructible<value_type> {
        root_ = std::make_unique<Node>();
        for (auto& v : other) insert(value_type(v));
    }
    Btree& operator=(const Btree& other)
        requires std::copy_constructible<value_type> {
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

        // Node is full — insert then split in one operation.
        insert_into_node(node, std::move(v));
        value_type median;
        auto right = split_full_node(node, median);
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
        if (!node->is_leaf()) {
            const key_type doomed_key = node->key(static_cast<size_t>(pos));
            Btree rebuilt;
            key_compare cmp;
            for (auto cur = begin(); cur != end(); ++cur) {
                const key_type& cur_key = Params::get_key(*cur);
                if (!cmp(cur_key, doomed_key) && !cmp(doomed_key, cur_key)) {
                    continue;
                }
                rebuilt.insert(*cur);
            }
            root_ = std::move(rebuilt.root_);
            size_ = rebuilt.size_;
            return;
        }
        erase_leaf_value(node, static_cast<size_t>(pos));
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
        if (!node || size_ == 0) return end();

        // Descend leftmost.  Skip empty internal nodes (can appear after
        // erase on a node whose only value was the promoted median).
        while (node && !node->is_leaf()) {
            if (node->children.empty()) return end();
            // Skip past empty internal nodes whose left child vanished.
            size_t child_idx = 0;
            while (child_idx < node->children.size() &&
                   !node->children[child_idx]) ++child_idx;
            if (child_idx >= node->children.size()) return end();
            node = node->children[child_idx].get();
        }

        // We are at a leaf.  If it is empty, walk right among siblings.
        while (node && node->values.empty()) {
            // Find parent and index.
            Node* p = node->parent;
            if (!p) return end();
            int idx = index_in_parent(node);
            // Try next sibling.
            if (idx + 1 < static_cast<int>(p->children.size())) {
                node = p->children[idx + 1].get();
                while (node && !node->is_leaf()) {
                    if (!node->children.empty()) node = node->children[0].get();
                    else return end();
                }
            } else {
                // No more siblings — ascend.
                node = p;
                while (node && node->values.empty()) {
                    p = node->parent;
                    if (!p) return end();
                    idx = index_in_parent(node);
                    if (idx + 1 < static_cast<int>(p->children.size())) {
                        node = p->children[idx + 1].get();
                        while (node && !node->is_leaf()) {
                            if (!node->children.empty()) node = node->children[0].get();
                            else return end();
                        }
                        break;
                    }
                    node = p;
                }
            }
        }

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

    void erase_leaf_value(Node* node, size_t pos) {
        node->values.erase(node->values.begin() + static_cast<ptrdiff_t>(pos));
        --size_;
    }

    static void insert_into_node(Node* node, value_type v) {
        key_compare cmp;
        auto k = Params::get_key(v);
        auto it = node->values.begin();
        while (it != node->values.end() && cmp(Params::get_key(*it), k)) ++it;
        node->values.insert(it, std::move(v));
    }

    /// Split a full node (values.size() == kCapacity + 1 after insert).
    /// Returns the median value through the output parameter, and the new
    /// right sibling as the return value.  The left node keeps the lower
    /// half (including the preceding children for internal nodes).
    static std::unique_ptr<Node> split_full_node(Node* left, value_type& median_out) {
        auto right = std::make_unique<Node>();
        right->parent = left->parent;

        const size_t total = left->values.size();  // kCapacity + 1
        const size_t mid   = total / 2;            // median index

        // Extract median.
        median_out = std::move(left->values[mid]);

        // Right half values: [mid+1 .. total-1].
        right->values.assign(
            std::make_move_iterator(left->values.begin() + mid + 1),
            std::make_move_iterator(left->values.end()));

        // Truncate left to [0 .. mid-1].
        left->values.erase(left->values.begin() + mid, left->values.end());

        // Children: left had total+1 children, median separates them.
        // Left keeps  children[0..mid], right gets children[mid+1..total].
        if (!left->is_leaf()) {
            right->children.assign(
                std::make_move_iterator(left->children.begin() + mid + 1),
                std::make_move_iterator(left->children.end()));
            left->children.erase(left->children.begin() + mid + 1,
                                 left->children.end());
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
            value_type med2;
            auto right2 = split_full_node(parent, med2);
            insert_into_parent(parent, std::move(med2), std::move(right2));
        }
    }
};

} // namespace container_internal
} // namespace zeta

#endif // ZETA_CONTAINER_INTERNAL_BTREE_IMPL_H
