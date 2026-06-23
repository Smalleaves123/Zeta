#include "zeta/strings/str_utils.h"

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
    std::string input(reinterpret_cast<const char*>(data), size);
    std::string_view text(input);

    std::size_t off1 = size > 0 ? data[0] % (size + 1) : 0;
    std::size_t len1 = size > 1 ? data[1] % (size + 1) : 0;
    std::size_t off2 = size > 2 ? data[2] % (size + 1) : 0;
    std::size_t len2 = size > 3 ? data[3] % (size + 1) : 0;

    auto a = Slice(text, off1, len1);
    auto b = Slice(text, off2, len2);

    (void)zeta::StrContains(text, a);
    (void)zeta::StartsWith(text, a);
    (void)zeta::EndsWith(text, a);
    (void)zeta::StartsWithIgnoreCase(text, a);
    (void)zeta::EndsWithIgnoreCase(text, a);

    (void)zeta::StripPrefix(text, a);
    (void)zeta::StripSuffix(text, a);
    (void)zeta::StripLeadingAsciiWhitespace(text);
    (void)zeta::StripTrailingAsciiWhitespace(text);
    (void)zeta::StripAsciiWhitespace(text);

    auto lower = zeta::AsciiStrToLower(text);
    auto upper = zeta::AsciiStrToUpper(text);
    auto first = zeta::StrReplaceFirst(text, a, b);
    auto all = zeta::StrReplaceAll(text, a, b);

    volatile std::size_t sink =
        lower.size() + upper.size() + first.size() + all.size();
    (void)sink;
    return 0;
}
