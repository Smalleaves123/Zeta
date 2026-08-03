#ifndef ZETA_MEMORY_MEMORY_POOL_H
#define ZETA_MEMORY_MEMORY_POOL_H

/// @file   memory/memory_pool.h
/// @brief  Fixed-size raw block allocator with chunked backing storage.

#include <algorithm>
#include <bit>
#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace zeta {

class MemoryPool {
public:
    static constexpr std::size_t kDefaultBlocksPerChunk = 64;

    explicit MemoryPool(
        std::size_t block_size,
        std::size_t block_alignment = alignof(std::max_align_t),
        std::size_t blocks_per_chunk = kDefaultBlocksPerChunk)
        : block_alignment_(
              std::max(block_alignment, alignof(FreeBlock)))
        , block_size_(RoundUp(
              std::max(block_size, sizeof(FreeBlock)), block_alignment_))
        , blocks_per_chunk_(blocks_per_chunk) {
        if (block_alignment == 0 || !std::has_single_bit(block_alignment)) {
            throw std::invalid_argument("memory pool alignment must be a power of two");
        }
        if (blocks_per_chunk == 0) {
            throw std::invalid_argument("memory pool chunk size must not be zero");
        }
    }

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool& operator=(MemoryPool&&) = delete;

    ~MemoryPool() {
        Clear();
    }

    [[nodiscard]] void* Allocate() {
        if (free_list_ == nullptr) AddChunk();
        FreeBlock* block = free_list_;
        free_list_ = block->next;
        ++allocated_blocks_;
        return block;
    }

    void Deallocate(void* memory) noexcept {
        if (memory == nullptr) return;
        auto* block = static_cast<FreeBlock*>(memory);
        block->next = free_list_;
        free_list_ = block;
        --allocated_blocks_;
    }

    void Reserve(std::size_t block_count) {
        while (capacity_ < block_count) AddChunk();
    }

    void Clear() noexcept {
        for (std::byte* chunk : chunks_) {
            ::operator delete(chunk, std::align_val_t(block_alignment_));
        }
        chunks_.clear();
        free_list_ = nullptr;
        capacity_ = 0;
        allocated_blocks_ = 0;
    }

    [[nodiscard]] std::size_t BlockSize() const noexcept {
        return block_size_;
    }

    [[nodiscard]] std::size_t BlockAlignment() const noexcept {
        return block_alignment_;
    }

    [[nodiscard]] std::size_t Size() const noexcept {
        return allocated_blocks_;
    }

    [[nodiscard]] std::size_t Capacity() const noexcept {
        return capacity_;
    }

    [[nodiscard]] std::size_t Available() const noexcept {
        return capacity_ - allocated_blocks_;
    }

private:
    struct FreeBlock {
        FreeBlock* next = nullptr;
    };

    static std::size_t RoundUp(
        std::size_t value, std::size_t alignment) {
        const std::size_t remainder = value % alignment;
        if (remainder == 0) return value;
        const std::size_t increment = alignment - remainder;
        if (value > std::numeric_limits<std::size_t>::max() - increment) {
            throw std::bad_alloc();
        }
        return value + increment;
    }

    void AddChunk() {
        if (blocks_per_chunk_ >
            std::numeric_limits<std::size_t>::max() / block_size_) {
            throw std::bad_alloc();
        }
        const std::size_t bytes = block_size_ * blocks_per_chunk_;
        auto* chunk = static_cast<std::byte*>(
            ::operator new(bytes, std::align_val_t(block_alignment_)));
        try {
            chunks_.push_back(chunk);
        } catch (...) {
            ::operator delete(chunk, std::align_val_t(block_alignment_));
            throw;
        }

        for (std::size_t index = 0; index < blocks_per_chunk_; ++index) {
            auto* block = reinterpret_cast<FreeBlock*>(
                chunk + index * block_size_);
            block->next = free_list_;
            free_list_ = block;
        }
        capacity_ += blocks_per_chunk_;
    }

    const std::size_t block_alignment_;
    const std::size_t block_size_;
    const std::size_t blocks_per_chunk_;
    std::vector<std::byte*> chunks_;
    FreeBlock* free_list_ = nullptr;
    std::size_t capacity_ = 0;
    std::size_t allocated_blocks_ = 0;
};

} // namespace zeta

#endif // ZETA_MEMORY_MEMORY_POOL_H
