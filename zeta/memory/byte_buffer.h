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

class ByteBufferView {
public:
    static constexpr std::size_t kNpos = static_cast<std::size_t>(-1);

    constexpr ByteBufferView() noexcept = default;

    constexpr explicit ByteBufferView(
        std::span<const std::byte> bytes) noexcept
        : bytes_(bytes) {}

    explicit ByteBufferView(std::string_view bytes) noexcept
        : bytes_(reinterpret_cast<const std::byte*>(bytes.data()), bytes.size()) {}

    [[nodiscard]] constexpr std::span<const std::byte> Bytes() const noexcept {
        return bytes_;
    }

    [[nodiscard]] constexpr std::size_t Size() const noexcept {
        return bytes_.size();
    }

    [[nodiscard]] constexpr bool Empty() const noexcept {
        return bytes_.empty();
    }

    [[nodiscard]] constexpr const std::byte& operator[](
        std::size_t index) const noexcept {
        return bytes_[index];
    }

    [[nodiscard]] constexpr ByteBufferView Subspan(
        std::size_t offset, std::size_t count = kNpos) const noexcept {
        if (offset > bytes_.size()) return {};
        return ByteBufferView(bytes_.subspan(
            offset, std::min(count, bytes_.size() - offset)));
    }

    [[nodiscard]] std::string_view AsStringView() const noexcept {
        if (bytes_.empty()) return {};
        return std::string_view(
            reinterpret_cast<const char*>(bytes_.data()), bytes_.size());
    }

private:
    std::span<const std::byte> bytes_;
};

class ByteBuffer {
public:
    using value_type = std::byte;

    ByteBuffer() = default;

    explicit ByteBuffer(std::size_t capacity) {
        Reserve(capacity);
    }

    void Append(std::span<const std::byte> bytes) {
        if (bytes.empty()) return;
        // Keep the consumed prefix as reusable headroom while there is still
        // enough tail capacity.  Compact only when an append would otherwise
        // grow the vector; this avoids repeatedly moving a large readable
        // suffix in receive/consume loops.
        if (bytes.size() > storage_.capacity() - storage_.size()) Compact();
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

    [[nodiscard]] ByteBufferView ReadableView() const noexcept {
        return ByteBufferView(ReadableBytes());
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
