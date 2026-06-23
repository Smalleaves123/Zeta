#include "zeta/hash/hash.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
    std::string storage(reinterpret_cast<const char*>(data), size);
    std::string_view input(storage);
    std::uint64_t seed = size > 0 ? data[0] : 0;

    zeta::HashState incremental(seed);
    std::size_t pos = 0;
    while (pos < size) {
        std::size_t step = 1;
        if (pos + 1 < size) step = 1 + (data[pos + 1] % 17);
        step = std::min(step, size - pos);
        incremental.Feed(data + pos, step);
        pos += step;
    }

    std::uint64_t inc_hash = incremental.Finalize();
    std::uint64_t one_shot = zeta::HashState::CombinedBytes(data, size, seed);
    if (inc_hash != one_shot) __builtin_trap();

    auto combined_a = zeta::HashState::Combined(seed, static_cast<int>(size), input);
    auto combined_b = zeta::hash_combine(seed, static_cast<int>(size), input);
    if (combined_a != combined_b) __builtin_trap();

    std::vector<std::uint8_t> bytes(data, data + size);
    auto range_hash = zeta::hash_range(bytes.begin(), bytes.end(), seed);
    auto byte_hash = zeta::hash_bytes(data, size, seed);
    auto string_hash = zeta::Hash<std::string>{}(storage);
    auto view_hash = zeta::Hash<std::string_view>{}(input);

    if (string_hash != view_hash) __builtin_trap();

    if (size == 0) {
        auto cstr_hash = zeta::Hash<const char*>{}("");
        auto empty_hash = zeta::hash_bytes("", 0, 0);
        if (cstr_hash != empty_hash) __builtin_trap();
    }

    volatile std::uint64_t sink =
        inc_hash ^ one_shot ^ combined_a ^ combined_b ^ range_hash ^ byte_hash ^
        string_hash ^ view_hash;
    (void)sink;
    return 0;
}
