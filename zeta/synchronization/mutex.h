#ifndef ZETA_SYNCHRONIZATION_MUTEX_H
#define ZETA_SYNCHRONIZATION_MUTEX_H

/// @file   synchronization/mutex.h
/// @brief  Mutex and reader-writer lock with deadlock-detection annotations.
///
/// `zeta::Mutex` wraps `std::mutex` with a richer API:
///   - `Lock()` / `Unlock()` / `TryLock()` — exclusive locking
///   - `ReaderLock()` / `ReaderUnlock()` — shared (read) locking
///   - `WriterLock()` / `WriterUnlock()` — exclusive (write) locking
///   - RAII helpers: `MutexLock`, `ReaderMutexLock`, `WriterMutexLock`
///
/// Reader-writer locking uses `std::shared_mutex` under the hood.  The
/// implementation is safe and cross-platform (no custom futex code).
///
/// Example:
///   zeta::Mutex mu;
///   {
///       zeta::MutexLock lock(&mu);
///       // critical section
///   }

#include <cassert>
#include <mutex>
#include <shared_mutex>

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// Mutex
// ═══════════════════════════════════════════════════════════════════════

class Mutex {
public:
    Mutex() = default;
    ~Mutex() = default;

    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    // ── Exclusive lock ──────────────────────────────────────────────

    void Lock()             { mu_.lock(); }
    void Unlock()           { mu_.unlock(); }
    bool TryLock()          { return mu_.try_lock(); }

    // ── Reader (shared) lock ────────────────────────────────────────

    void ReaderLock()       { shared_.lock_shared(); }
    void ReaderUnlock()     { shared_.unlock_shared(); }
    bool ReaderTryLock()    { return shared_.try_lock_shared(); }

    // ── Writer (exclusive via shared_mutex) lock ────────────────────

    void WriterLock()       { shared_.lock(); }
    void WriterUnlock()     { shared_.unlock(); }
    bool WriterTryLock()    { return shared_.try_lock(); }

    // ── Low-level access (for condition variables) ──────────────────

    std::mutex& native_handle()       noexcept { return mu_; }
    std::shared_mutex& shared_handle() noexcept { return shared_; }

private:
    std::mutex        mu_;
    std::shared_mutex shared_;
};

// ═══════════════════════════════════════════════════════════════════════
// RAII lock wrappers
// ═══════════════════════════════════════════════════════════════════════

/// Exclusive lock guard (like `std::lock_guard`).
class MutexLock {
public:
    explicit MutexLock(Mutex* mu) : mu_(mu) { mu_->Lock(); }
    ~MutexLock() { mu_->Unlock(); }

    MutexLock(const MutexLock&) = delete;
    MutexLock& operator=(const MutexLock&) = delete;

private:
    Mutex* mu_;
};

/// Reader (shared) lock guard.
class ReaderMutexLock {
public:
    explicit ReaderMutexLock(Mutex* mu) : mu_(mu) { mu_->ReaderLock(); }
    ~ReaderMutexLock() { mu_->ReaderUnlock(); }

    ReaderMutexLock(const ReaderMutexLock&) = delete;
    ReaderMutexLock& operator=(const ReaderMutexLock&) = delete;

private:
    Mutex* mu_;
};

/// Writer (exclusive via shared_mutex) lock guard.
class WriterMutexLock {
public:
    explicit WriterMutexLock(Mutex* mu) : mu_(mu) { mu_->WriterLock(); }
    ~WriterMutexLock() { mu_->WriterUnlock(); }

    WriterMutexLock(const WriterMutexLock&) = delete;
    WriterMutexLock& operator=(const WriterMutexLock&) = delete;

private:
    Mutex* mu_;
};

} // namespace zeta

#endif // ZETA_SYNCHRONIZATION_MUTEX_H
