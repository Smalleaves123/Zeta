#ifndef ZETA_FLAGS_INTERNAL_REGISTRY_H
#define ZETA_FLAGS_INTERNAL_REGISTRY_H

/// @file   flags/internal/registry.h
/// @brief  Global flag registry — lock-free intrusive linked list.
///
/// Uses `std::atomic` with constant-initialisation to avoid all
/// function-local-static guard variables and cross-TU ordering issues.
/// Safe to register flags during static-initialisation (before main).

#include <atomic>
#include <cstddef>
#include <string>
#include <string_view>

namespace zeta {
namespace flags_internal {

// ═══════════════════════════════════════════════════════════════════════
// Global registry head
// ═══════════════════════════════════════════════════════════════════════

/// ODR-safe single-instance head pointer.
///
/// `std::atomic<FlagEntry*>` has a constexpr constructor from `nullptr`,
/// so this is constant-initialised — no runtime guard, no initialisation
/// ordering issues, safe to read/write during static init.
class FlagEntry;
inline std::atomic<FlagEntry*> g_registry_head{nullptr};

// ═══════════════════════════════════════════════════════════════════════
// FlagEntry
// ═══════════════════════════════════════════════════════════════════════

class FlagEntry {
public:
    constexpr FlagEntry(const char* name, const char* help,
                        const char* filename, const char* env_name = nullptr,
                        bool required = false) noexcept
        : name_(name), help_(help), filename_(filename), env_name_(env_name),
          required_(required), next_(nullptr) {}

    /// Self-register into the global linked list.  Safe to call from
    /// static constructors because `g_registry_head` is constant-init.
    void Register() noexcept {
        // No lock needed — static-init is single-threaded, and after
        // main() the list is read-only.
        next_ = g_registry_head.load(std::memory_order_relaxed);
        g_registry_head.store(this, std::memory_order_release);
    }

    FlagEntry(const FlagEntry&) = delete;
    FlagEntry& operator=(const FlagEntry&) = delete;

    [[nodiscard]] std::string_view Name()     const noexcept { return name_; }
    [[nodiscard]] std::string_view Help()     const noexcept { return help_; }
    [[nodiscard]] std::string_view Filename() const noexcept { return filename_; }
    [[nodiscard]] std::string_view EnvName() const noexcept {
        return env_name_ != nullptr ? std::string_view(env_name_) : std::string_view{};
    }
    [[nodiscard]] bool Required() const noexcept { return required_; }
    [[nodiscard]] bool IsSet() const noexcept {
        return is_set_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] FlagEntry*       Next()     const noexcept { return next_; }

    virtual bool Parse(std::string_view /*value*/) noexcept { return false; }
    [[nodiscard]] virtual std::string_view CurrentValue() const noexcept { return ""; }
    [[nodiscard]] virtual std::string_view DefaultValue() const noexcept { return ""; }
    [[nodiscard]] virtual std::string_view TypeName() const noexcept { return ""; }
    virtual bool Validate(std::string* /*error*/) const { return true; }

    /// Return the head of the global registry list.
    [[nodiscard]] static FlagEntry* Head() noexcept {
        return g_registry_head.load(std::memory_order_acquire);
    }

protected:
    void MarkSet() noexcept { is_set_.store(true, std::memory_order_relaxed); }
    ~FlagEntry() = default;

private:
    const char* name_;
    const char* help_;
    const char* filename_;
    const char* env_name_;
    bool        required_;
    std::atomic<bool> is_set_{false};
    FlagEntry*  next_;
};

} // namespace flags_internal
} // namespace zeta

#endif // ZETA_FLAGS_INTERNAL_REGISTRY_H
