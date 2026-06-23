#include "zeta/strings/str_join.h"

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

std::vector<std::string_view> BuildStringViews(std::string_view input,
                                               const std::uint8_t* data,
                                               std::size_t size) {
    std::vector<std::string_view> parts;
    std::size_t count = size > 0 ? (data[0] % 8) : 0;
    parts.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::size_t base = 1 + i * 2;
        std::size_t off = base < size ? data[base] % (size + 1) : 0;
        std::size_t len = (base + 1) < size ? data[base + 1] % (size + 1) : 0;
        parts.push_back(Slice(input, off, len));
    }
    return parts;
}

std::vector<int> BuildInts(const std::uint8_t* data, std::size_t size) {
    std::vector<int> values;
    std::size_t count = size > 1 ? (data[1] % 8) : 0;
    values.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::size_t idx = 2 + i;
        int value = idx < size ? static_cast<int>(static_cast<int8_t>(data[idx]))
                               : static_cast<int>(i);
        values.push_back(value);
    }
    return values;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
    std::string storage(reinterpret_cast<const char*>(data), size);
    std::string_view input(storage);

    std::size_t off = size > 18 ? data[18] % (size + 1) : 0;
    std::size_t len = size > 19 ? data[19] % (size + 1) : 0;
    std::string_view delim = Slice(input, off, len);

    auto string_views = BuildStringViews(input, data, size);
    auto ints = BuildInts(data, size);

    auto joined_sv = zeta::StrJoin(string_views, delim);
    auto joined_ints = zeta::StrJoin(ints, delim);
    auto joined_chars =
        zeta::StrJoin({static_cast<char>(size > 20 ? data[20] : 'a'),
                       static_cast<char>(size > 21 ? data[21] : 'b'),
                       static_cast<char>(size > 22 ? data[22] : 'c')},
                      delim);
    auto joined_init = zeta::StrJoin({Slice(input, 0, size / 2), Slice(input, size / 2, size)}, delim);

    if (!string_views.empty()) {
        (void)joined_sv.starts_with(std::string(string_views.front()));
        (void)joined_sv.ends_with(std::string(string_views.back()));
    }
    if (string_views.size() <= 1 || delim.empty()) {
        (void)joined_sv.size();
    } else {
        (void)joined_sv.find(std::string(delim));
    }

    volatile std::size_t sink =
        joined_sv.size() + joined_ints.size() + joined_chars.size() + joined_init.size();
    (void)sink;
    return 0;
}
