#ifndef ZETA_BASE_THREAD_ANNOTATIONS_H
#define ZETA_BASE_THREAD_ANNOTATIONS_H

/// @file   base/thread_annotations.h
/// @brief  Clang thread-safety annotations with portable fallbacks.

#if defined(__clang__) && __has_attribute(guarded_by)
#define ZETA_THREAD_ANNOTATION(attribute) __attribute__((attribute))
#define ZETA_GUARDED_BY(mutex) ZETA_THREAD_ANNOTATION(guarded_by(mutex))
#define ZETA_PT_GUARDED_BY(mutex) ZETA_THREAD_ANNOTATION(pt_guarded_by(mutex))
#define ZETA_ACQUIRED_AFTER(...) ZETA_THREAD_ANNOTATION(acquired_after(__VA_ARGS__))
#define ZETA_ACQUIRED_BEFORE(...) ZETA_THREAD_ANNOTATION(acquired_before(__VA_ARGS__))
#define ZETA_EXCLUSIVE_LOCKS_REQUIRED(...) \
    ZETA_THREAD_ANNOTATION(exclusive_locks_required(__VA_ARGS__))
#define ZETA_SHARED_LOCKS_REQUIRED(...) \
    ZETA_THREAD_ANNOTATION(shared_locks_required(__VA_ARGS__))
#define ZETA_LOCKS_EXCLUDED(...) ZETA_THREAD_ANNOTATION(locks_excluded(__VA_ARGS__))
#define ZETA_EXCLUSIVE_LOCK_FUNCTION(...) \
    ZETA_THREAD_ANNOTATION(exclusive_lock_function(__VA_ARGS__))
#define ZETA_SHARED_LOCK_FUNCTION(...) \
    ZETA_THREAD_ANNOTATION(shared_lock_function(__VA_ARGS__))
#define ZETA_UNLOCK_FUNCTION(...) ZETA_THREAD_ANNOTATION(unlock_function(__VA_ARGS__))
#define ZETA_SCOPED_LOCKABLE ZETA_THREAD_ANNOTATION(scoped_lockable)
#define ZETA_ASSERT_EXCLUSIVE_LOCK(...) \
    ZETA_THREAD_ANNOTATION(assert_exclusive_lock(__VA_ARGS__))
#define ZETA_NO_THREAD_SAFETY_ANALYSIS \
    ZETA_THREAD_ANNOTATION(no_thread_safety_analysis)
#else
#define ZETA_GUARDED_BY(mutex)
#define ZETA_PT_GUARDED_BY(mutex)
#define ZETA_ACQUIRED_AFTER(...)
#define ZETA_ACQUIRED_BEFORE(...)
#define ZETA_EXCLUSIVE_LOCKS_REQUIRED(...)
#define ZETA_SHARED_LOCKS_REQUIRED(...)
#define ZETA_LOCKS_EXCLUDED(...)
#define ZETA_EXCLUSIVE_LOCK_FUNCTION(...)
#define ZETA_SHARED_LOCK_FUNCTION(...)
#define ZETA_UNLOCK_FUNCTION(...)
#define ZETA_SCOPED_LOCKABLE
#define ZETA_ASSERT_EXCLUSIVE_LOCK(...)
#define ZETA_NO_THREAD_SAFETY_ANALYSIS
#endif

#endif // ZETA_BASE_THREAD_ANNOTATIONS_H
