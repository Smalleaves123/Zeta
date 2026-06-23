#include "zeta/strings/str_split.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string_view Slice(std::string_view input, std::size_t offset,
                       std::size_t length) {
    if (offset > input.size()) offset = input.size();
    length = std::min(length, input.size() - offset);
    return input.substr(offset, length);
}

void ConsumeRange(auto&& range, std::size_t limit = 256) {
    std::size_t count = 0;
    for (auto part : range) {
        volatile std::size_t sink = part.size();
        (void)sink;
        if (++count >= limit) break;
    }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size) {
    std::string input(reinterpret_cast<const char*>(data), size);
    std::string_view sv(input);

    char delim = size > 0 ? static_cast<char>(data[0]) : ',';
    std::size_t off1 = size > 1 ? data[1] % (size + 1) : 0;
    std::size_t len1 = size > 2 ? data[2] % (size + 1) : 0;
    std::size_t off2 = size > 3 ? data[3] % (size + 1) : 0;
    std::size_t len2 = size > 4 ? data[4] % (size + 1) : 0;
    std::size_t max_splits = size > 5 ? data[5] : 8;

    auto delim_string = Slice(sv, off1, len1);
    auto any_chars = Slice(sv, off2, len2);

    ConsumeRange(zeta::StrSplit(sv, delim));
    ConsumeRange(zeta::StrSplit(sv, delim, zeta::SkipEmpty));
    ConsumeRange(zeta::StrSplit(sv, delim, zeta::MaxSplits_t{max_splits}));
    ConsumeRange(zeta::StrSplit(
        sv, delim, zeta::SkipEmpty, zeta::MaxSplits_t{max_splits}));

    ConsumeRange(zeta::StrSplit(sv, delim_string));
    ConsumeRange(zeta::StrSplit(sv, delim_string, zeta::SkipEmpty));

    ConsumeRange(zeta::StrSplit(sv, zeta::ByAnyChar{any_chars}));
    ConsumeRange(zeta::StrSplit(
        sv, zeta::ByAnyChar{any_chars}, zeta::SkipEmpty));

    return 0;
}
