#ifndef ZETA_CONTAINER_LFU_CACHE_H
#define ZETA_CONTAINER_LFU_CACHE_H

/// @file   container/lfu_cache.h
/// @brief  Non-thread-safe least-frequently-used cache with LRU ties.

#include <cstddef>
#include <functional>
#include <limits>
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
class LfuCache {
    using Order = std::list<const Key*>;

    struct Entry {
        Value value;
        std::size_t frequency = 1;
        typename Order::iterator position;
    };

    using Map = std::unordered_map<Key, Entry, Hash, KeyEqual>;
    using FrequencyBuckets = std::unordered_map<std::size_t, Order>;

public:
    explicit LfuCache(std::size_t capacity) : capacity_(capacity) {}

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
            std::move(key), Entry{std::move(value), 1, {}});
        if (!inserted) return false;
        try {
            auto& bucket = buckets_[1];
            bucket.push_front(&it->first);
            it->second.position = bucket.begin();
            min_frequency_ = 1;
        } catch (...) {
            auto bucket_it = buckets_.find(1);
            if (bucket_it != buckets_.end() && bucket_it->second.empty()) {
                buckets_.erase(bucket_it);
            }
            entries_.erase(it);
            throw;
        }
        if (entries_.size() > capacity_) EvictLeastUsed();
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
        RemoveFromBucket(it);
        entries_.erase(it);
        return true;
    }

    void Clear() noexcept {
        buckets_.clear();
        entries_.clear();
        min_frequency_ = 1;
    }

private:
    void Touch(typename Map::iterator it) {
        const std::size_t old_frequency = it->second.frequency;
        const std::size_t new_frequency =
            old_frequency == std::numeric_limits<std::size_t>::max()
                ? old_frequency
                : old_frequency + 1;

        // Insert the new list node first. If allocation fails, the entry is
        // still linked into its old bucket and the cache remains valid.
        auto [new_bucket_it, inserted] = buckets_.try_emplace(new_frequency);
        try {
            new_bucket_it->second.push_front(&it->first);
        } catch (...) {
            if (inserted && new_bucket_it->second.empty()) {
                buckets_.erase(new_bucket_it);
            }
            throw;
        }

        const auto new_position = new_bucket_it->second.begin();
        const auto old_position = it->second.position;
        if (old_frequency == new_frequency) {
            new_bucket_it->second.erase(old_position);
        } else {
            auto old_bucket_it = buckets_.find(old_frequency);
            old_bucket_it->second.erase(old_position);
            if (old_bucket_it->second.empty()) {
                buckets_.erase(old_bucket_it);
            }
        }

        it->second.frequency = new_frequency;
        it->second.position = new_position;
        if (old_frequency == min_frequency_ && old_frequency != new_frequency &&
            buckets_.find(old_frequency) == buckets_.end()) {
            RecomputeMinFrequency();
        }
        if (new_frequency < min_frequency_) min_frequency_ = new_frequency;
    }

    void RemoveFromBucket(typename Map::iterator it) {
        auto bucket_it = buckets_.find(it->second.frequency);
        bucket_it->second.erase(it->second.position);
        if (bucket_it->second.empty()) buckets_.erase(bucket_it);
        RecomputeMinFrequency();
    }

    void EvictLeastUsed() {
        auto bucket_it = buckets_.find(min_frequency_);
        const Key* key = bucket_it->second.back();
        auto it = entries_.find(*key);
        bucket_it->second.pop_back();
        if (bucket_it->second.empty()) buckets_.erase(bucket_it);
        entries_.erase(it);
        RecomputeMinFrequency();
    }

    void RecomputeMinFrequency() noexcept {
        if (buckets_.empty()) {
            min_frequency_ = 1;
            return;
        }
        min_frequency_ = std::numeric_limits<std::size_t>::max();
        for (const auto& [frequency, bucket] : buckets_) {
            (void)bucket;
            if (frequency < min_frequency_) min_frequency_ = frequency;
        }
    }

    std::size_t capacity_;
    std::size_t min_frequency_ = 1;
    FrequencyBuckets buckets_;
    Map entries_;
};

template <
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>>
using LFUCache = LfuCache<Key, Value, Hash, KeyEqual>;

} // namespace zeta

#endif // ZETA_CONTAINER_LFU_CACHE_H
