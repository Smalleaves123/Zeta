#include "zeta/strings/numbers.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace {

std::string_view Slice(std::string_view input, std::size_t offset,
                       std::size_t length) {
    if (offset > input.size()) offset = input.size();
    length = std::min(length, input.size() - offset);
    return input.substr(offset, length);
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
    std::string storage(reinterpret_cast<const char*>(data), size);
    std::string_view input(storage);

    int i = 0;
    unsigned u = 0;
    long long ll = 0;
    double d = 0.0;
    float f = 0.0f;

    (void)zeta::SimpleAtoi(input, &i);
    (void)zeta::SimpleAtoi(input, &u);
    (void)zeta::SimpleAtoi(input, &ll);
    (void)zeta::SimpleAtoi(input, &d);
    (void)zeta::SimpleAtoi(input, &f);

    std::int64_t seed = 0;
    for (std::size_t idx = 0; idx < size && idx < 8; ++idx) {
        seed = (seed << 8) | data[idx];
    }
    int roundtrip = static_cast<int>(seed);
    unsigned roundtrip_u = static_cast<unsigned>(seed);
    char buf[32];

    auto text_i = zeta::FastIntToBuffer(roundtrip, buf);
    int parsed_i = 0;
    if (!zeta::SimpleAtoi(text_i, &parsed_i) || parsed_i != roundtrip) {
        __builtin_trap();
    }

    auto text_u = zeta::FastIntToBuffer(roundtrip_u, buf);
    unsigned parsed_u = 0;
    if (!zeta::SimpleAtoi(text_u, &parsed_u) || parsed_u != roundtrip_u) {
        __builtin_trap();
    }

    auto part = Slice(input, size > 0 ? data[0] % (size + 1) : 0,
                      size > 1 ? data[1] % (size + 1) : 0);
    long parsed_ll = 0;
    (void)zeta::SimpleAtoi(part, &parsed_ll);

    volatile std::size_t sink =
        text_i.size() + text_u.size() + part.size() +
        static_cast<std::size_t>(parsed_i == 0 ? 0 : 1) +
        static_cast<std::size_t>(parsed_u == 0 ? 0 : 1) +
        static_cast<std::size_t>(parsed_ll == 0 ? 0 : 1);
    (void)sink;
    return 0;
}
