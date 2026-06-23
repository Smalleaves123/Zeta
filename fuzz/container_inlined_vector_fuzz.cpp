#include "zeta/container/inlined_vector.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

void CheckEqual(const zeta::InlinedVector<int, 4>& actual,
                const std::vector<int>& expected) {
    if (actual.size() != expected.size()) __builtin_trap();
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (actual[i] != expected[i]) __builtin_trap();
    }
    if (actual.empty() != expected.empty()) __builtin_trap();
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
    zeta::InlinedVector<int, 4> actual;
    std::vector<int> expected;

    for (std::size_t i = 0; i < size; ++i) {
        std::uint8_t op = data[i] % 8U;
        int value = static_cast<int>(static_cast<int8_t>(data[i]));

        switch (op) {
        case 0:
            actual.push_back(value);
            expected.push_back(value);
            break;
        case 1:
            if (!expected.empty()) {
                actual.pop_back();
                expected.pop_back();
            }
            break;
        case 2: {
            std::size_t idx = expected.empty() ? 0 : (data[i] % (expected.size() + 1));
            actual.insert(actual.begin() + static_cast<std::ptrdiff_t>(idx), value);
            expected.insert(expected.begin() + static_cast<std::ptrdiff_t>(idx), value);
            break;
        }
        case 3:
            if (!expected.empty()) {
                std::size_t idx = data[i] % expected.size();
                actual.erase(actual.begin() + static_cast<std::ptrdiff_t>(idx));
                expected.erase(expected.begin() + static_cast<std::ptrdiff_t>(idx));
            }
            break;
        case 4: {
            std::size_t new_size = data[i] % 16U;
            actual.resize(new_size, value);
            expected.resize(new_size, value);
            break;
        }
        case 5:
            actual.clear();
            expected.clear();
            break;
        case 6:
            actual.reserve(data[i] % 32U);
            break;
        case 7:
            actual.shrink_to_fit();
            break;
        default:
            break;
        }

        CheckEqual(actual, expected);
        if (!expected.empty()) {
            if (actual.front() != expected.front()) __builtin_trap();
            if (actual.back() != expected.back()) __builtin_trap();
        }
    }

    volatile std::size_t sink = actual.size() + actual.capacity();
    (void)sink;
    return 0;
}
