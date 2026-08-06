#ifndef ZETA_MEMORY_OBJECT_POOL_H
#define ZETA_MEMORY_OBJECT_POOL_H

/// @file   memory/object_pool.h
/// @brief  Reusable storage for individually constructed objects.

#include "zeta/memory/memory_pool.h"

#include <cstddef>
#include <memory>
#include <unordered_set>
#include <utility>

namespace zeta {

template <typename T>
class ObjectPool {
public:
    explicit ObjectPool(
        std::size_t objects_per_chunk = MemoryPool::kDefaultBlocksPerChunk)
        : storage_(sizeof(T), alignof(T), objects_per_chunk) {}

    ~ObjectPool() {
        Clear();
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    template <typename... Args>
    [[nodiscard]] T* Create(Args&&... args) {
        void* memory = storage_.Allocate();
        T* object = nullptr;
        try {
            object = std::construct_at(
                static_cast<T*>(memory), std::forward<Args>(args)...);
            active_.emplace(object);
            return object;
        } catch (...) {
            if (object != nullptr) std::destroy_at(object);
            storage_.Deallocate(memory);
            throw;
        }
    }

    void Destroy(T* object) noexcept {
        if (object == nullptr) return;
        const auto it = active_.find(object);
        if (it == active_.end()) return;
        active_.erase(it);
        std::destroy_at(object);
        storage_.Deallocate(object);
    }

    void Clear() noexcept {
        for (T* object : active_) std::destroy_at(object);
        active_.clear();
        storage_.Clear();
    }

    void Reserve(std::size_t object_count) {
        storage_.Reserve(object_count);
    }

    [[nodiscard]] std::size_t Size() const noexcept {
        return storage_.Size();
    }

    [[nodiscard]] std::size_t Capacity() const noexcept {
        return storage_.Capacity();
    }

    [[nodiscard]] std::size_t Available() const noexcept {
        return storage_.Available();
    }

private:
    MemoryPool storage_;
    std::unordered_set<T*> active_;
};

} // namespace zeta

#endif // ZETA_MEMORY_OBJECT_POOL_H
