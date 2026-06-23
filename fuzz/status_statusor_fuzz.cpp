#include "zeta/status/statusor.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace {

zeta::Status MakeError(std::uint8_t byte, std::string msg) {
    switch (byte % 4U) {
    case 0: return zeta::InvalidArgumentError(std::move(msg));
    case 1: return zeta::NotFoundError(std::move(msg));
    case 2: return zeta::InternalError(std::move(msg));
    default: return zeta::UnavailableError(std::move(msg));
    }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
    std::string payload(reinterpret_cast<const char*>(data), size);
    int value = size > 0 ? static_cast<int>(static_cast<int8_t>(data[0])) : 0;
    bool choose_value = size > 1 ? ((data[1] & 1U) != 0) : false;

    zeta::StatusOr<int> int_or =
        choose_value ? zeta::StatusOr<int>(value)
                     : zeta::StatusOr<int>(MakeError(size > 2 ? data[2] : 0, payload));
    if (int_or.ok()) {
        if (*int_or != value) __builtin_trap();
        if (!int_or.status().ok()) __builtin_trap();
        if (int_or.value_or(7) != value) __builtin_trap();
    } else {
        if (int_or.status().ok()) __builtin_trap();
        if (int_or.value_or(7) != 7) __builtin_trap();
    }

    zeta::StatusOr<std::string> str_or =
        choose_value ? zeta::StatusOr<std::string>(payload)
                     : zeta::StatusOr<std::string>(MakeError(size > 3 ? data[3] : 0, payload));
    zeta::StatusOr<std::string> copied = str_or;
    if (copied.ok() != str_or.ok()) __builtin_trap();
    if (copied.ok() && copied.value() != str_or.value()) __builtin_trap();
    if (!copied.ok() && copied.status() != str_or.status()) __builtin_trap();

    zeta::StatusOr<void> void_or =
        choose_value ? zeta::StatusOr<void>() : zeta::StatusOr<void>(MakeError(size > 4 ? data[4] : 0, payload));
    if (void_or.ok()) {
        void_or.value();
    } else if (void_or.status().ok()) {
        __builtin_trap();
    }

    zeta::StatusOr<int> moved = std::move(int_or);
    if (moved.ok() && moved.value() != value) __builtin_trap();

    volatile std::size_t sink =
        payload.size() + static_cast<std::size_t>(copied.ok() ? copied->size() : 0);
    (void)sink;
    return 0;
}
