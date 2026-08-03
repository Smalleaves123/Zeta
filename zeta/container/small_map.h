#ifndef ZETA_CONTAINER_SMALL_MAP_H
#define ZETA_CONTAINER_SMALL_MAP_H

/// @file   container/small_map.h
/// @brief  Inline-storage map optimized for a small number of entries.

#include "zeta/container/inlined_vector.h"

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace zeta {

template <
    typename Key,
    typename Value,
    std::size_t InlineCapacity = 8,
    typename KeyEqual = std::equal_to<Key>>
class SmallMap {
    static_assert(InlineCapacity > 0, "SmallMap requires InlineCapacity > 0");

    using Entry = std::pair<Key, Value>;
    using Storage = InlinedVector<Entry, InlineCapacity>;

public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = Entry;
    using size_type = std::size_t;
    using iterator = typename Storage::iterator;
    using const_iterator = typename Storage::const_iterator;

    SmallMap() = default;

    explicit SmallMap(KeyEqual equal) : equal_(std::move(equal)) {}

    [[nodiscard]] size_type size() const noexcept { return entries_.size(); }
    [[nodiscard]] bool empty() const noexcept { return entries_.empty(); }

    void reserve(size_type capacity) { entries_.reserve(capacity); }

    iterator begin() noexcept { return entries_.begin(); }
    const_iterator begin() const noexcept { return entries_.begin(); }
    const_iterator cbegin() const noexcept { return entries_.cbegin(); }
    iterator end() noexcept { return entries_.end(); }
    const_iterator end() const noexcept { return entries_.end(); }
    const_iterator cend() const noexcept { return entries_.cend(); }

    iterator find(const Key& key) noexcept {
        for (iterator it = begin(); it != end(); ++it) {
            if (equal_(it->first, key)) return it;
        }
        return end();
    }

    const_iterator find(const Key& key) const noexcept {
        for (const_iterator it = begin(); it != end(); ++it) {
            if (equal_(it->first, key)) return it;
        }
        return end();
    }

    [[nodiscard]] bool contains(const Key& key) const noexcept {
        return find(key) != end();
    }

    Value& operator[](Key key) {
        if (iterator it = find(key); it != end()) return it->second;
        entries_.emplace_back(std::move(key), Value{});
        return entries_.back().second;
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(Key key, Args&&... args) {
        if (iterator it = find(key); it != end()) return {it, false};
        entries_.emplace_back(
            std::piecewise_construct,
            std::forward_as_tuple(std::move(key)),
            std::forward_as_tuple(std::forward<Args>(args)...));
        return {entries_.end() - 1, true};
    }

    std::pair<iterator, bool> insert_or_assign(Key key, Value value) {
        if (iterator it = find(key); it != end()) {
            it->second = std::move(value);
            return {it, false};
        }
        entries_.emplace_back(std::move(key), std::move(value));
        return {entries_.end() - 1, true};
    }

    Value& at(const Key& key) {
        if (iterator it = find(key); it != end()) return it->second;
        throw std::out_of_range("SmallMap::at");
    }

    const Value& at(const Key& key) const {
        if (const_iterator it = find(key); it != end()) return it->second;
        throw std::out_of_range("SmallMap::at");
    }

    bool erase(const Key& key) noexcept {
        if (iterator it = find(key); it != end()) {
            entries_.erase(it);
            return true;
        }
        return false;
    }

    void clear() noexcept { entries_.clear(); }

private:
    Storage entries_;
    KeyEqual equal_;
};

} // namespace zeta

#endif // ZETA_CONTAINER_SMALL_MAP_H
