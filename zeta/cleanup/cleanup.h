#ifndef ZETA_CLEANUP_CLEANUP_H
#define ZETA_CLEANUP_CLEANUP_H

/// @file   cleanup/cleanup.h
/// @brief  RAII cleanup guard — invokes a callable when leaving scope.
///
/// `zeta::Cleanup` is similar to `absl::Cleanup` / `folly::ScopeGuard`:
/// it stores an arbitrary callable and invokes it exactly once when the
/// guard is destroyed (unless explicitly cancelled or invoked early).
///
/// The factory function `zeta::MakeCleanup(f)` should be used to create
/// a guard (CTAD doesn't work well here because we must reject rvalue args).
///
/// `Cleanup` is **move-only** (like `std::unique_ptr`).
///
/// Example:
///   void Process(FILE* fp) {
///     auto closer = zeta::MakeCleanup([fp] { fclose(fp); });
///     // ... use fp ...
///     // closer invokes fclose on scope exit (or earlier if moved)
///   }

#include <cassert>
#include <type_traits>
#include <utility>

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// Cleanup
// ═══════════════════════════════════════════════════════════════════════

template <typename F>
class Cleanup {
public:
    /// Construct from a callable.  Prefer `MakeCleanup` for deduction.
    template <typename G,
              typename = std::enable_if_t<std::is_constructible_v<F, G>>>
    explicit Cleanup(G&& f) noexcept(std::is_nothrow_constructible_v<F, G>)
        : f_(std::forward<G>(f)), active_(true) {}

    // ── Move-only ───────────────────────────────────────────────────

    Cleanup(Cleanup&& other) noexcept(std::is_nothrow_move_constructible_v<F>)
        : f_(std::move(other.f_)), active_(std::exchange(other.active_, false)) {}

    Cleanup& operator=(Cleanup&& other) noexcept(
        std::is_nothrow_move_assignable_v<F>) {
        if (this != &other) {
            Invoke();
            f_      = std::move(other.f_);
            active_ = std::exchange(other.active_, false);
        }
        return *this;
    }

    Cleanup(const Cleanup&) = delete;
    Cleanup& operator=(const Cleanup&) = delete;

    // ── Destruction ─────────────────────────────────────────────────

    ~Cleanup() noexcept {
        if (active_) f_();
    }

    // ── Manual control ─────────────────────────────────────────────

    /// Dismiss the guard — the callable will NOT be invoked on destruction.
    void Cancel() noexcept { active_ = false; }

    /// Invoke the callable immediately and dismiss the guard.
    void Invoke() noexcept(noexcept(std::declval<F&>()())) {
        if (active_) {
            active_ = false;
            f_();
        }
    }

    /// True if the guard is still armed (will fire on destruction).
    [[nodiscard]] bool IsActive() const noexcept { return active_; }

private:
    F    f_;
    bool active_;
};

// ── Factory ──────────────────────────────────────────────────────────

/// Create a `Cleanup` from a callable (lvalue or rvalue).
///
///   auto guard = zeta::MakeCleanup([] { std::cout << "done\n"; });
template <typename F>
[[nodiscard]] Cleanup<std::decay_t<F>> MakeCleanup(F&& f)
    noexcept(std::is_nothrow_constructible_v<std::decay_t<F>, F>) {
    return Cleanup<std::decay_t<F>>(std::forward<F>(f));
}

} // namespace zeta

#endif // ZETA_CLEANUP_CLEANUP_H
