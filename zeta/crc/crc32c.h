#ifndef ZETA_CRC_CRC32C_H
#define ZETA_CRC_CRC32C_H

/// @file   crc/crc32c.h
/// @brief  Portable CRC32C (Castagnoli) checksum computation.
///
/// The implementation uses the reflected Castagnoli polynomial and the
/// standard CRC32C initial/final XOR values. It is suitable for checksums in
/// storage, caches, network frames, and test fixtures. Hardware-accelerated
/// backends can be added behind this API without changing callers.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace zeta {

using crc32c_t = std::uint32_t;

namespace crc_internal {

constexpr std::array<crc32c_t, 256> MakeCrc32cTable() {
    std::array<crc32c_t, 256> table{};
    for (std::size_t i = 0; i < table.size(); ++i) {
        crc32c_t value = static_cast<crc32c_t>(i);
        for (int bit = 0; bit < 8; ++bit) {
            value = (value & 1U) != 0U
                        ? (value >> 1U) ^ 0x82F63B78U
                        : value >> 1U;
        }
        table[i] = value;
    }
    return table;
}

inline constexpr auto kCrc32cTable = MakeCrc32cTable();

} // namespace crc_internal

/// Incremental CRC32C accumulator.
class Crc32cAccumulator {
public:
    constexpr Crc32cAccumulator() noexcept = default;

    void Reset() noexcept { state_ = kInitialState; }

    void Update(const void* data, std::size_t size) noexcept {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        for (std::size_t i = 0; i < size; ++i) {
            state_ = crc_internal::kCrc32cTable[
                          (state_ ^ bytes[i]) & 0xFFU] ^
                      (state_ >> 8U);
        }
    }

    void Update(std::string_view data) noexcept {
        Update(data.data(), data.size());
    }

    [[nodiscard]] crc32c_t Finalize() const noexcept {
        return ~state_;
    }

private:
    static constexpr crc32c_t kInitialState = 0xFFFFFFFFU;
    crc32c_t state_ = kInitialState;
};

/// Computes the CRC32C checksum of a byte buffer.
[[nodiscard]] inline crc32c_t ComputeCrc32c(
    const void* data, std::size_t size) noexcept {
    Crc32cAccumulator accumulator;
    accumulator.Update(data, size);
    return accumulator.Finalize();
}

/// Computes the CRC32C checksum of a string view.
[[nodiscard]] inline crc32c_t ComputeCrc32c(std::string_view data) noexcept {
    return ComputeCrc32c(data.data(), data.size());
}

/// Extends an existing finalized CRC32C with more bytes.
[[nodiscard]] inline crc32c_t ExtendCrc32c(
    crc32c_t crc, const void* data, std::size_t size) noexcept {
    // Convert the finalized checksum back to the accumulator state.
    crc32c_t running = ~crc;
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < size; ++i) {
        running = crc_internal::kCrc32cTable[
                      (running ^ bytes[i]) & 0xFFU] ^
                  (running >> 8U);
    }
    return ~running;
}

/// Extends an existing finalized CRC32C with more bytes.
[[nodiscard]] inline crc32c_t ExtendCrc32c(
    crc32c_t crc, std::string_view data) noexcept {
    return ExtendCrc32c(crc, data.data(), data.size());
}

} // namespace zeta

#endif // ZETA_CRC_CRC32C_H
