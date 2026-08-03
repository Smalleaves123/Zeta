#ifndef ZETA_CONTAINER_LRU_CACHE_H
#define ZETA_CONTAINER_LRU_CACHE_H

/// @file   container/lru_cache.h
/// @brief  Non-thread-safe least-recently-used cache.

#include <cstddef>
#include <functional>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

namespace zeta {

template <
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>>
class LruCache {
    using Order = std::list<const Key*>;

    struct Entry {
        Value value;
        typename Order::iterator position;
    };

    using Map = std::unordered_map<Key, Entry, Hash, KeyEqual>;

public:
    explicit LruCache(std::size_t capacity) : capacity_(capacity) {}

    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] bool empty() const noexcept { return entries_.empty(); }

    bool Put(Key key, Value value) {
        if (auto it = entries_.find(key); it != entries_.end()) {
            it->second.value = std::move(value);
            Touch(it);
            return false;
        }
        if (capacity_ == 0) return false;

        auto [it, inserted] = entries_.try_emplace(
            std::move(key), Entry{std::move(value), order_.end()});
        if (!inserted) return false;
        try {
            order_.push_front(&it->first);
            it->second.position = order_.begin();
        } catch (...) {
            entries_.erase(it);
            throw;
        }
        if (entries_.size() > capacity_) EvictOldest();
        return true;
    }

    Value* Find(const Key& key) {
        auto it = entries_.find(key);
        if (it == entries_.end()) return nullptr;
        Touch(it);
        return &it->second.value;
    }

    const Value* Find(const Key& key) const {
        auto it = entries_.find(key);
        return it == entries_.end() ? nullptr : &it->second.value;
    }

    std::optional<Value> Get(const Key& key) {
        if (Value* value = Find(key); value != nullptr) return *value;
        return std::nullopt;
    }

    [[nodiscard]] bool Contains(const Key& key) const {
        return entries_.find(key) != entries_.end();
    }

    bool Erase(const Key& key) {
        auto it = entries_.find(key);
        if (it == entries_.end()) return false;
        order_.erase(it->second.position);
        entries_.erase(it);
        return true;
    }

    void Clear() noexcept {
        order_.clear();
        entries_.clear();
    }

private:
    void Touch(typename Map::iterator it) {
        order_.splice(order_.begin(), order_, it->second.position);
    }

    void EvictOldest() {
        const Key* key = order_.back();
        auto it = entries_.find(*key);
        order_.pop_back();
        entries_.erase(it);
    }

    std::size_t capacity_;
    Order order_;
    Map entries_;
};

template <
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>>
using LRUCache = LruCache<Key, Value, Hash, KeyEqual>;

} // namespace zeta

#endif // ZETA_CONTAINER_LRU_CACHE_H
