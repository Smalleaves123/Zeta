#ifndef ZETA_FLAGS_INTERNAL_REGISTRY_H
#define ZETA_FLAGS_INTERNAL_REGISTRY_H

/// @file   flags/internal/registry.h
/// @brief  Global flag registry (intrusive linked list).

#include <cstddef>
#include <string_view>

namespace zeta {
namespace flags_internal {

class FlagEntry;

/// Global registry head — `inline` ensures single instance across all TUs.
/// Zero-initialized at program load, before any dynamic initialization.
inline FlagEntry* g_head = nullptr;

// ═══════════════════════════════════════════════════════════════════════
// FlagEntry — base class for all registered flags
// ═══════════════════════════════════════════════════════════════════════

class FlagEntry {
public:
    constexpr FlagEntry(const char* name, const char* help,
                        const char* filename) noexcept
        : name_(name), help_(help), filename_(filename), next_(nullptr) {}

    /// Register this flag in the global list. Safe to call from static
    /// constructors because `g_head` is zero-initialized before them.
    void Register() noexcept {
        next_ = g_head;
        g_head = this;
    }

    FlagEntry(const FlagEntry&) = delete;
    FlagEntry& operator=(const FlagEntry&) = delete;

    [[nodiscard]] std::string_view Name()     const noexcept { return name_; }
    [[nodiscard]] std::string_view Help()     const noexcept { return help_; }
    [[nodiscard]] std::string_view Filename() const noexcept { return filename_; }
    [[nodiscard]] FlagEntry*       Next()     const noexcept { return next_; }

    virtual bool Parse(std::string_view /*value*/) noexcept { return false; }
    [[nodiscard]] virtual std::string_view CurrentValue() const noexcept { return ""; }
    [[nodiscard]] virtual std::string_view TypeName() const noexcept { return ""; }

protected:
    ~FlagEntry() = default;

private:
    const char* name_;
    const char* help_;
    const char* filename_;
    FlagEntry*  next_;
};

} // namespace flags_internal
} // namespace zeta

#endif // ZETA_FLAGS_INTERNAL_REGISTRY_H
