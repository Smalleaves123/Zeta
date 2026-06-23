#include "zeta/status/status.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace {

zeta::StatusCode PickCode(std::uint8_t byte) {
    return static_cast<zeta::StatusCode>(byte % 17U);
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
    std::string message(reinterpret_cast<const char*>(data), size);
    zeta::StatusCode code = PickCode(size > 0 ? data[0] : 0);

    zeta::Status a(code, message);
    zeta::Status b = a;
    if ((a == b) != true) __builtin_trap();
    if (a.ok() != (a.code() == zeta::StatusCode::kOk)) __builtin_trap();

    zeta::Status moved = std::move(b);
    if (a != moved) __builtin_trap();

    zeta::Status assigned = zeta::InternalError("seed");
    assigned = a;
    if (assigned != a) __builtin_trap();

    zeta::Status move_assigned = zeta::NotFoundError("tmp");
    move_assigned = std::move(assigned);
    if (move_assigned != a) __builtin_trap();

    auto code_name = zeta::StatusCodeToString(code);
    if (code_name[0] == '\0') __builtin_trap();

    auto text = a.ToString();
    if (text.empty()) __builtin_trap();
    if (a.ok() && text != "OK") __builtin_trap();

    volatile std::size_t sink = text.size() + a.message().size();
    (void)sink;
    return 0;
}
