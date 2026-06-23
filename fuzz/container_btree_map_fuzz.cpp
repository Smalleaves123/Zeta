#include "zeta/container/btree_map.h"

#include <cstddef>
#include <cstdint>
#include <map>

namespace {

void CheckEqual(const zeta::btree_map<int, int>& actual,
                const std::map<int, int>& expected) {
    if (actual.size() != expected.size()) __builtin_trap();
    auto ait = actual.begin();
    auto bit = expected.begin();
    for (; bit != expected.end(); ++bit, ++ait) {
        if (ait == actual.end()) __builtin_trap();
        if (ait->first != bit->first || ait->second != bit->second) {
            __builtin_trap();
        }
    }
    if (ait != actual.end()) __builtin_trap();
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
    zeta::btree_map<int, int> actual;
    std::map<int, int> expected;

    for (std::size_t i = 0; i < size; ++i) {
        std::uint8_t op = data[i] % 6U;
        int key = static_cast<int>(static_cast<int8_t>(data[i]));
        int value = static_cast<int>((i + 1) < size ? static_cast<int8_t>(data[i + 1]) : 0);

        switch (op) {
        case 0: {
            auto a = actual.insert({key, value});
            auto b = expected.insert({key, value});
            if (a.second != b.second) __builtin_trap();
            break;
        }
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
            if ((actual.find(key) == actual.end()) != (expected.find(key) == expected.end())) {
                __builtin_trap();
            }
            break;
        case 4:
            if (actual.erase(key) != expected.erase(key)) __builtin_trap();
            break;
        case 5:
            actual.clear();
            expected.clear();
            break;
        default:
            break;
        }

        CheckEqual(actual, expected);
    }

    volatile std::size_t sink = actual.size();
    (void)sink;
    return 0;
}
