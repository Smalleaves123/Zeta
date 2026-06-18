#ifndef ZETA_SYNCHRONIZATION_MUTEX_H
#define ZETA_SYNCHRONIZATION_MUTEX_H

/// @file   synchronization/mutex.h
/// @brief  Mutex and reader-writer lock.
///
/// `zeta::Mutex` wraps `std::shared_mutex` — all locking methods
/// (exclusive, shared, reader, writer) operate on the same underlying
/// synchronisation object, guaranteeing proper mutual exclusion.
///
/// Example:
///   zeta::Mutex mu;
///   {
///       zeta::MutexLock lock(&mu);
///       // exclusive critical section
///   }

#include <cassert>
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

    void ReaderLock()       { mu_.lock_shared(); }
    void ReaderUnlock()     { mu_.unlock_shared(); }
    bool ReaderTryLock()    { return mu_.try_lock_shared(); }

    // ── Writer (exclusive) lock ─────────────────────────────────────

    void WriterLock()       { mu_.lock(); }
    void WriterUnlock()     { mu_.unlock(); }
    bool WriterTryLock()    { return mu_.try_lock(); }

    // ── Low-level access ────────────────────────────────────────────

    std::shared_mutex& native_handle() noexcept { return mu_; }

private:
    std::shared_mutex mu_;
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

/// Writer (exclusive) lock guard.
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
