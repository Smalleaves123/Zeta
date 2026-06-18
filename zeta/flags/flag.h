#ifndef ZETA_FLAGS_FLAG_H
#define ZETA_FLAGS_FLAG_H

/// @file   flags/flag.h
/// @brief  Type-safe command-line flags with automatic registration.
///
/// Flags are registered at static-init time (before `main`).  Use the
/// `ZETA_FLAG(type, name, default, help)` macro to declare a flag.
///
/// Example:
///   ZETA_FLAG(int32, port, 8080, "Server port number");
///   // In main():
///   zeta::ParseCommandLine(argc, argv);
///   // Use: int p = *FLAGS_port;  // 8080 or whatever --port value

#include <cstdint>
#include <string>
#include <string_view>

#include "zeta/flags/internal/registry.h"
#include "zeta/strings/numbers.h"

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// Flag<T> — a single typed flag
// ═══════════════════════════════════════════════════════════════════════

template <typename T>
class Flag : public flags_internal::FlagEntry {
public:
    Flag(const char* name, const char* help, const char* filename, T def)
        : FlagEntry(name, help, filename), value_(def), default_(def) {
        Register();  // safe: called after base and members are constructed
    }

    [[nodiscard]] const T& Get()      const noexcept { return value_; }
    [[nodiscard]] const T& Default()  const noexcept { return default_; }

    void Set(T v) noexcept { value_ = v; }

    bool Parse(std::string_view s) noexcept override {
        if constexpr (std::is_same_v<T, bool>) {
            if (s == "true" || s == "1" || s == "yes" || s == "y") {
                value_ = true; return true;
            }
            if (s == "false" || s == "0" || s == "no" || s == "n") {
                value_ = false; return true;
            }
            return false;
        } else if constexpr (std::is_same_v<T, std::string>) {
            value_ = std::string(s);
            return true;
        } else {
            return SimpleAtoi(s, &value_);
        }
    }

    [[nodiscard]] std::string_view CurrentValue() const noexcept override {
        buf_ = ToString(value_);
        return buf_;
    }

    [[nodiscard]] std::string_view TypeName() const noexcept override {
        if constexpr (std::is_same_v<T, bool>)          return "bool";
        else if constexpr (std::is_same_v<T, int32_t>)  return "int32";
        else if constexpr (std::is_same_v<T, int64_t>)  return "int64";
        else if constexpr (std::is_same_v<T, uint32_t>) return "uint32";
        else if constexpr (std::is_same_v<T, uint64_t>) return "uint64";
        else if constexpr (std::is_same_v<T, double>)   return "double";
        else if constexpr (std::is_same_v<T, std::string>) return "string";
        else return "unknown";
    }

private:
    T value_;
    T default_;
    mutable std::string buf_;  // scratch for CurrentValue()

    template <typename U>
    static std::string ToString(U v) {
        if constexpr (std::is_same_v<U, bool>)
            return v ? "true" : "false";
        else if constexpr (std::is_same_v<U, std::string>)
            return v;
        else
            return std::to_string(v);
    }
};

} // namespace zeta

// ═══════════════════════════════════════════════════════════════════════
// ZETA_FLAG macro
// ═══════════════════════════════════════════════════════════════════════

/// Declare a global flag.
///
/// Usage:
///   ZETA_FLAG(int32, port, 8080, "Server port number");
///
/// Generates a `FLAGS_port` global variable of type `zeta::Flag<int32_t>`.
/// The flag is automatically registered and can be queried after
/// `zeta::ParseCommandLine()`.
///
/// Supported types: bool, int32, int64, uint32, uint64, double, string.
#define ZETA_FLAG(type, name, default_value, help_text)                 \
    static ::zeta::Flag<::zeta::internal::FlagType_##type>              \
        FLAGS_##name(#name, help_text, __FILE__, (default_value))

namespace zeta {
namespace internal {
    using FlagType_bool   = bool;
    using FlagType_int32  = int32_t;
    using FlagType_int64  = int64_t;
    using FlagType_uint32 = uint32_t;
    using FlagType_uint64 = uint64_t;
    using FlagType_double = double;
    using FlagType_string = std::string;
} // namespace internal
} // namespace zeta

#endif // ZETA_FLAGS_FLAG_H
