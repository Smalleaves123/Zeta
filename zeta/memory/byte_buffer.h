#ifndef ZETA_MEMORY_BYTE_BUFFER_H
#define ZETA_MEMORY_BYTE_BUFFER_H

/// @file   memory/byte_buffer.h
/// @brief  Owning byte storage with prefix consumption.

#include <algorithm>
#include <cstddef>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace zeta {

class ByteBuffer {
public:
    using value_type = std::byte;

    ByteBuffer() = default;

    explicit ByteBuffer(std::size_t capacity) {
        Reserve(capacity);
    }

    void Append(std::span<const std::byte> bytes) {
        if (bytes.empty()) return;
        Compact();
        storage_.insert(storage_.end(), bytes.begin(), bytes.end());
    }

    void Append(std::string_view bytes) {
        Append(std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(bytes.data()), bytes.size()));
    }

    [[nodiscard]] std::span<const std::byte> ReadableBytes() const noexcept {
        if (Empty()) return {};
        return std::span<const std::byte>(
            storage_.data() + read_offset_, Size());
    }

    [[nodiscard]] std::size_t Size() const noexcept {
        return storage_.size() - read_offset_;
    }

    [[nodiscard]] bool Empty() const noexcept {
        return Size() == 0;
    }

    [[nodiscard]] bool Consume(std::size_t bytes) noexcept {
        if (bytes > Size()) return false;
        read_offset_ += bytes;
        if (read_offset_ == storage_.size()) {
            Clear();
        }
        return true;
    }

    void Clear() noexcept {
        storage_.clear();
        read_offset_ = 0;
    }

    void Reserve(std::size_t capacity) {
        Compact();
        storage_.reserve(capacity);
    }

private:
    void Compact() {
        if (read_offset_ == 0) return;
        if (read_offset_ == storage_.size()) {
            Clear();
            return;
        }

        const auto readable_begin = storage_.begin() + read_offset_;
        std::move(readable_begin, storage_.end(), storage_.begin());
        storage_.resize(Size());
        read_offset_ = 0;
    }

    std::vector<std::byte> storage_;
    std::size_t read_offset_ = 0;
};

} // namespace zeta

#endif // ZETA_MEMORY_BYTE_BUFFER_H
