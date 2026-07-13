#include "zeta/strings/escaping.h"
#include "zeta/strings/match.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size) {
    const std::string_view input(reinterpret_cast<const char*>(data), size);
    const auto escaped = zeta::CEscape(input);
    const auto restored = zeta::CUnescape(escaped);
    if (!restored.ok() || restored.value() != input) __builtin_trap();

    (void)zeta::StrMatch(input, "*");
    return 0;
}
