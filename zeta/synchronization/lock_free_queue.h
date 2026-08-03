#ifndef ZETA_SYNCHRONIZATION_LOCK_FREE_QUEUE_H
#define ZETA_SYNCHRONIZATION_LOCK_FREE_QUEUE_H

/// @file   synchronization/lock_free_queue.h
/// @brief  Fixed-capacity MPMC queue using atomic sequence slots.

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace zeta {

template <typename T>
class LockFreeQueue {
    static_assert(
        std::is_nothrow_move_constructible_v<T>,
        "LockFreeQueue requires a nothrow-move-constructible value type");

    struct Cell {
        std::atomic<std::size_t> sequence{0};
        alignas(T) std::byte storage[sizeof(T)];
    };

public:
    explicit LockFreeQueue(std::size_t capacity) {
        if (capacity == 0 ||
            capacity > (std::numeric_limits<std::size_t>::max() >> 1)) {
            throw std::invalid_argument("lock-free queue capacity is invalid");
        }
        const std::size_t normalized_capacity = capacity < 2 ? 2 : capacity;
        capacity_ = std::bit_ceil(normalized_capacity);
        if (capacity_ == 0) {
            throw std::invalid_argument("lock-free queue capacity is too large");
        }
        mask_ = capacity_ - 1;
        cells_ = std::make_unique<Cell[]>(capacity_);
        for (std::size_t index = 0; index < capacity_; ++index) {
            cells_[index].sequence.store(index, std::memory_order_relaxed);
        }
    }

    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
    LockFreeQueue(LockFreeQueue&&) = delete;
    LockFreeQueue& operator=(LockFreeQueue&&) = delete;

    ~LockFreeQueue() {
        while (TryPop().has_value()) {}
    }

    [[nodiscard]] bool TryPush(T value) noexcept {
        std::size_t position = enqueue_position_.load(std::memory_order_relaxed);
        Cell* cell = nullptr;
        for (;;) {
            cell = &cells_[position & mask_];
            const std::size_t sequence =
                cell->sequence.load(std::memory_order_acquire);
            const auto difference = static_cast<std::intptr_t>(
                sequence - position);
            if (difference == 0) {
                if (enqueue_position_.compare_exchange_weak(
                        position, position + 1,
                        std::memory_order_relaxed,
                        std::memory_order_relaxed)) {
                    break;
                }
            } else if (difference < 0) {
                return false;
            } else {
                position = enqueue_position_.load(std::memory_order_relaxed);
            }
        }

        std::construct_at(Data(*cell), std::move(value));
        cell->sequence.store(position + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] std::optional<T> TryPop() noexcept {
        std::size_t position = dequeue_position_.load(std::memory_order_relaxed);
        Cell* cell = nullptr;
        for (;;) {
            cell = &cells_[position & mask_];
            const std::size_t sequence =
                cell->sequence.load(std::memory_order_acquire);
            const auto difference = static_cast<std::intptr_t>(
                sequence - (position + 1));
            if (difference == 0) {
                if (dequeue_position_.compare_exchange_weak(
                        position, position + 1,
                        std::memory_order_relaxed,
                        std::memory_order_relaxed)) {
                    break;
                }
            } else if (difference < 0) {
                return std::nullopt;
            } else {
                position = dequeue_position_.load(std::memory_order_relaxed);
            }
        }

        T* value = Data(*cell);
        std::optional<T> result(std::in_place, std::move(*value));
        std::destroy_at(value);
        cell->sequence.store(
            position + capacity_, std::memory_order_release);
        return result;
    }

    [[nodiscard]] std::size_t Capacity() const noexcept {
        return capacity_;
    }

    [[nodiscard]] std::size_t Size() const noexcept {
        const std::size_t enqueued =
            enqueue_position_.load(std::memory_order_acquire);
        const std::size_t dequeued =
            dequeue_position_.load(std::memory_order_acquire);
        const std::size_t size = enqueued - dequeued;
        return size > capacity_ ? capacity_ : size;
    }

    [[nodiscard]] bool Empty() const noexcept { return Size() == 0; }

private:
    static T* Data(Cell& cell) noexcept {
        return std::launder(reinterpret_cast<T*>(cell.storage));
    }

    std::unique_ptr<Cell[]> cells_;
    std::size_t capacity_ = 0;
    std::size_t mask_ = 0;
    std::atomic<std::size_t> enqueue_position_{0};
    std::atomic<std::size_t> dequeue_position_{0};
};

} // namespace zeta

#endif // ZETA_SYNCHRONIZATION_LOCK_FREE_QUEUE_H
