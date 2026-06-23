#include "zeta/strings/str_cat.h"

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
    long long n1 = size > 12
                       ? static_cast<long long>(
                             static_cast<std::int64_t>(
                                 (static_cast<std::uint64_t>(data[5]) << 56) |
                                 (static_cast<std::uint64_t>(data[6]) << 48) |
                                 (static_cast<std::uint64_t>(data[7]) << 40) |
                                 (static_cast<std::uint64_t>(data[8]) << 32) |
                                 (static_cast<std::uint64_t>(data[9]) << 24) |
                                 (static_cast<std::uint64_t>(data[10]) << 16) |
                                 (static_cast<std::uint64_t>(data[11]) << 8) |
                                 static_cast<std::uint64_t>(data[12])))
                       : 0LL;
    bool flag = size > 13 ? ((data[13] & 1U) != 0) : false;
    char ch = size > 14 ? static_cast<char>(data[14]) : 'x';

    auto joined = zeta::StrCat(a, b, n0, n1, flag, ch);
    auto reference = std::string(a) + std::string(b) + std::to_string(n0) +
                     std::to_string(n1) + (flag ? "true" : "false") +
                     std::string(1, ch);
    if (joined != reference) __builtin_trap();

    std::string appended = std::string(a);
    zeta::StrAppend(appended, b, n0, flag, ch);
    auto appended_ref = std::string(a) + std::string(b) + std::to_string(n0) +
                        (flag ? "true" : "false") + std::string(1, ch);
    if (appended != appended_ref) __builtin_trap();

    volatile std::size_t sink = joined.size() + appended.size();
    (void)sink;
    return 0;
}
