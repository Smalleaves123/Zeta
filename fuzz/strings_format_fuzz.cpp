#include "zeta/strings/format.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
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

    std::size_t off1 = size > 0 ? data[0] % (size + 1) : 0;
    std::size_t len1 = size > 1 ? data[1] % (size + 1) : 0;
    std::size_t off2 = size > 2 ? data[2] % (size + 1) : 0;
    std::size_t len2 = size > 3 ? data[3] % (size + 1) : 0;

    auto a = Slice(input, off1, len1);
    auto b = Slice(input, off2, len2);
    int n0 = size > 4 ? static_cast<int>(static_cast<int8_t>(data[4])) : 0;
    unsigned n1 = size > 5 ? static_cast<unsigned>(data[5]) : 0U;

    auto r0 = zeta::Format(storage);
    auto r1 = zeta::Format(storage, a);
    auto r2 = zeta::Format(storage, a, b);
    auto r3 = zeta::Format(storage, a, n0, n1);
    auto r4 = zeta::Format(std::string_view(input), a, b, storage);
    auto r5 = zeta::Format("{{{0}}}:{1}:{}", a, b, n0);

    volatile std::size_t sink =
        r0.size() + r1.size() + r2.size() + r3.size() + r4.size() + r5.size();
    (void)sink;
    return 0;
}
