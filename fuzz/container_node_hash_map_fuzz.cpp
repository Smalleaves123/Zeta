#include "zeta/container/node_hash_map.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace {

void CheckEqual(const zeta::node_hash_map<int, int>& actual,
                const std::unordered_map<int, int>& expected) {
    if (actual.size() != expected.size()) __builtin_trap();
    for (const auto& [k, v] : expected) {
        auto it = actual.find(k);
        if (it == actual.end()) __builtin_trap();
        if (it->second != v) __builtin_trap();
    }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
    zeta::node_hash_map<int, int> actual;
    std::unordered_map<int, int> expected;

    for (std::size_t i = 0; i < size; ++i) {
        std::uint8_t op = data[i] % 6U;
        int key = static_cast<int>(static_cast<int8_t>(data[i]));
        int value = static_cast<int>((i + 1) < size ? static_cast<int8_t>(data[i + 1]) : 0);

        switch (op) {
        case 0:
            actual.insert({key, value});
            expected.insert({key, value});
            break;
        case 1:
            actual[key] = value;
            expected[key] = value;
            break;
        case 2:
            if (actual.contains(key) != (expected.find(key) != expected.end())) {
                __builtin_trap();
            }
            break;
        case 3:
            if (actual.erase(key) != expected.erase(key)) {
                __builtin_trap();
            }
            break;
        case 4:
            actual.clear();
            expected.clear();
            break;
        case 5: {
            if (!expected.empty()) {
                auto sample = expected.begin();
                auto a = actual.find(sample->first);
                auto b = expected.find(sample->first);
                if ((a == actual.end()) != (b == expected.end())) __builtin_trap();
            }
            break;
        }
        default:
            break;
        }

        CheckEqual(actual, expected);
    }

    volatile std::size_t sink = actual.size();
    (void)sink;
    return 0;
}
