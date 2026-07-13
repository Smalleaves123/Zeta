#include "zeta/crc/crc32c.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size) {
    const auto one_shot = zeta::ComputeCrc32c(data, size);

    zeta::Crc32cAccumulator accumulator;
    const std::size_t split = size / 2;
    accumulator.Update(data, split);
    accumulator.Update(data + split, size - split);
    if (accumulator.Finalize() != one_shot) __builtin_trap();

    const auto prefix = zeta::ComputeCrc32c(data, split);
    const auto extended = zeta::ExtendCrc32c(prefix, data + split, size - split);
    if (extended != one_shot) __builtin_trap();

    (void)std::string_view(reinterpret_cast<const char*>(data), size);
    return 0;
}
