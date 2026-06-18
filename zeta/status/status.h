#ifndef ZETA_STATUS_STATUS_H
#define ZETA_STATUS_STATUS_H

/// @file   status/status.h
/// @brief  Lightweight error status with small-message optimization.
///
/// `zeta::Status` is a 16-byte value type carrying an error code and message.
/// Messages up to 15 characters are stored inline — no heap allocation.
/// Only messages longer than 15 characters fall back to heap storage.
///
/// Status is designed for all environments:
///   - No exceptions, no RTTI required
///   - Safe to copy, cheap to move
///   - OK status is all-zeros (trivially initialized)
///   - Thread-safe to read; mutation is not thread-safe by design
///
/// Example:
///   zeta::Status DoWork() {
///     if (bad) return zeta::InvalidArgumentError("bad input");
///     return zeta::OkStatus();
///   }

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace zeta {

// ── Status Code Enum ─────────────────────────────────────────────────

/// Status codes mirror the gRPC code set, the lingua franca of modern
/// service error reporting.  17 codes fit in 6 bits.
enum class StatusCode : uint8_t {
    kOk                  = 0,
    kCancelled           = 1,
    kUnknown             = 2,
    kInvalidArgument     = 3,
    kDeadlineExceeded    = 4,
    kNotFound            = 5,
    kAlreadyExists       = 6,
    kPermissionDenied    = 7,
    kResourceExhausted   = 8,
    kFailedPrecondition  = 9,
    kAborted             = 10,
    kOutOfRange          = 11,
    kUnimplemented       = 12,
    kInternal            = 13,
    kUnavailable         = 14,
    kDataLoss            = 15,
    kUnauthenticated     = 16,
};

/// Human-readable label for each code.
constexpr const char* StatusCodeToString(StatusCode code) noexcept {
    switch (code) {
    case StatusCode::kOk                 : return "OK";
    case StatusCode::kCancelled          : return "CANCELLED";
    case StatusCode::kUnknown            : return "UNKNOWN";
    case StatusCode::kInvalidArgument    : return "INVALID_ARGUMENT";
    case StatusCode::kDeadlineExceeded   : return "DEADLINE_EXCEEDED";
    case StatusCode::kNotFound           : return "NOT_FOUND";
    case StatusCode::kAlreadyExists      : return "ALREADY_EXISTS";
    case StatusCode::kPermissionDenied   : return "PERMISSION_DENIED";
    case StatusCode::kResourceExhausted  : return "RESOURCE_EXHAUSTED";
    case StatusCode::kFailedPrecondition : return "FAILED_PRECONDITION";
    case StatusCode::kAborted            : return "ABORTED";
    case StatusCode::kOutOfRange         : return "OUT_OF_RANGE";
    case StatusCode::kUnimplemented      : return "UNIMPLEMENTED";
    case StatusCode::kInternal           : return "INTERNAL";
    case StatusCode::kUnavailable        : return "UNAVAILABLE";
    case StatusCode::kDataLoss           : return "DATA_LOSS";
    case StatusCode::kUnauthenticated    : return "UNAUTHENTICATED";
    }
    return "UNKNOWN";
}

// ── Status ───────────────────────────────────────────────────────────

/// A 16-byte error object with **small-message optimization**.
///
/// ## Storage layout (16 bytes, endian-independent):
///
///     raw_[0]        = tag and code
///       bits [1:0]   = tag   (0=OK, 1=inline, 2=heap)
///       bits [7:2]   = StatusCode
///     raw_[1..15]    = inline message (null-terminated, up to 15 chars)
///                       — OR — raw_[8..15] = HeapRep* (tag=2)
///
/// ## Key properties:
///   - OK status is 16 zero bytes → `Status()` is trivially zeroed.
///   - Messages ≤ 15 chars: zero heap allocations (embedded-friendly).
///   - Messages > 15 chars: one heap allocation for code + message.
///   - Copy is cheap (memcpy for inline, deep-copy for heap).
///   - Move is always cheap (memcpy + invalidate source).
class Status {
    // ── Low-level constants ──────────────────────────────────────
    static constexpr uint8_t kTagOk     = 0;
    static constexpr uint8_t kTagInline = 1;
    static constexpr uint8_t kTagHeap   = 2;
    static constexpr uint8_t kTagMask   = 0x03;
    static constexpr uint8_t kCodeShift = 2;
    static constexpr size_t  kInlineMax = 15; // chars (not counting null)

    // ── Heap representation ──────────────────────────────────────
    struct HeapRep {
        StatusCode code;
        std::string message;

        HeapRep(StatusCode c, std::string msg)
            : code(c), message(std::move(msg)) {}
    };

    // ── Raw storage ──────────────────────────────────────────────
    // 16 bytes, alignas(8) so raw_[8..15] is pointer-aligned.
    alignas(alignof(void*)) unsigned char raw_[16];

    // ── Tag helpers ──────────────────────────────────────────────
    uint8_t tag()       const noexcept { return raw_[0] & kTagMask; }

    // ── Inline helpers ───────────────────────────────────────────
    void set_inline(StatusCode code, const char* msg, size_t len) {
        raw_[0] = static_cast<uint8_t>(
            (static_cast<uint8_t>(code) << kCodeShift) | kTagInline);
        size_t n = len < kInlineMax ? len : kInlineMax;
        if (n > 0) std::memcpy(raw_ + 1, msg, n);
        std::memset(raw_ + 1 + n, 0, kInlineMax - n);
    }

    StatusCode inline_code() const noexcept {
        return static_cast<StatusCode>(raw_[0] >> kCodeShift);
    }

    const char* inline_msg() const noexcept {
        return reinterpret_cast<const char*>(raw_ + 1);
    }

    // Returns inline message length (scans for null terminator).
    size_t inline_len() const noexcept {
        for (size_t i = 0; i < kInlineMax; ++i)
            if (raw_[1 + i] == 0) return i;
        return kInlineMax;
    }

    // ── Heap helpers ─────────────────────────────────────────────
    void set_heap(HeapRep* ptr) noexcept {
        raw_[0] = kTagHeap;
        std::memset(raw_ + 1, 0, 7); // zero padding
        std::memcpy(raw_ + 8, &ptr, sizeof(ptr));
    }

    HeapRep* heap_ptr() const noexcept {
        void* ptr;
        std::memcpy(&ptr, raw_ + 8, sizeof(ptr));
        return static_cast<HeapRep*>(ptr);
    }

    // ── Destroys current payload (called before overwriting) ─────
    void destroy() noexcept {
        if (tag() == kTagHeap) {
            delete heap_ptr();
        }
        // Mark as OK to be safe (callers usually reassign immediately).
        std::memset(raw_, 0, sizeof(raw_));
    }

public:
    // ── Construction ─────────────────────────────────────────────

    /// Constructs an OK status (all bytes zero).
    constexpr Status() noexcept : raw_{} {}

    /// Constructs from code + message.
    /// Accepts anything convertible to `std::string`.
    /// kOk code → OK status (message ignored).
    Status(StatusCode code, std::string message) noexcept(false) {
        if (code == StatusCode::kOk) {
            std::memset(raw_, 0, sizeof(raw_));
        } else if (message.size() <= kInlineMax) {
            set_inline(code, message.data(), message.size());
        } else {
            set_heap(new HeapRep(code, std::move(message)));
        }
    }

    // ── Copy ─────────────────────────────────────────────────────

    Status(const Status& other) noexcept(false) {
        if (other.tag() == kTagHeap) {
            auto* rep = other.heap_ptr();
            set_heap(new HeapRep(rep->code, rep->message));
        } else {
            std::memcpy(raw_, other.raw_, sizeof(raw_));
        }
    }

    Status& operator=(const Status& other) noexcept(false) {
        if (this != &other) {
            destroy();
            if (other.tag() == kTagHeap) {
                auto* rep = other.heap_ptr();
                set_heap(new HeapRep(rep->code, rep->message));
            } else {
                std::memcpy(raw_, other.raw_, sizeof(raw_));
            }
        }
        return *this;
    }

    // ── Move ─────────────────────────────────────────────────────

    Status(Status&& other) noexcept {
        std::memcpy(raw_, other.raw_, sizeof(raw_));
        // Source becomes OK.
        std::memset(other.raw_, 0, sizeof(other.raw_));
    }

    Status& operator=(Status&& other) noexcept {
        if (this != &other) {
            destroy();
            std::memcpy(raw_, other.raw_, sizeof(raw_));
            std::memset(other.raw_, 0, sizeof(other.raw_));
        }
        return *this;
    }

    ~Status() noexcept { destroy(); }

    // ── Observers ────────────────────────────────────────────────

    /// Returns true if the status is OK (no error).
    [[nodiscard]] bool ok() const noexcept { return tag() == kTagOk; }

    /// Returns the status code.  kOk for an OK status.
    [[nodiscard]] StatusCode code() const noexcept {
        switch (tag()) {
        case kTagInline: return inline_code();
        case kTagHeap:   return heap_ptr()->code;
        default:         return StatusCode::kOk;
        }
    }

    /// Returns the message. Empty for an OK status.
    /// @warning The returned view is valid for the lifetime of this Status.
    [[nodiscard]] std::string_view message() const noexcept {
        switch (tag()) {
        case kTagInline: return std::string_view(inline_msg(), inline_len());
        case kTagHeap:   return heap_ptr()->message;
        default:         return {};
        }
    }

    // ── Comparison ───────────────────────────────────────────────

    [[nodiscard]] bool operator==(const Status& other) const noexcept {
        if (ok() && other.ok()) return true;
        if (ok() != other.ok()) return false;
        return code() == other.code() && message() == other.message();
    }

    [[nodiscard]] bool operator!=(const Status& other) const noexcept {
        return !(*this == other);
    }

    // ── String representation ────────────────────────────────────

    /// Returns "OK" or "CODE: message".
    [[nodiscard]] std::string ToString() const {
        if (ok()) return "OK";
        auto msg = message();
        std::string result(StatusCodeToString(code()));
        result += ": ";
        result.append(msg.data(), msg.size());
        return result;
    }
};

// Ensure Status is exactly 16 bytes.
static_assert(sizeof(Status) == 16, "Status must be 16 bytes");

// ── Free Functions ───────────────────────────────────────────────────

/// Returns an OK status.
inline Status OkStatus() noexcept { return Status(); }

/// Swallows a Status, suppressing "unused return value" warnings.
/// Use when you intentionally discard a status.
inline void IgnoreStatus(const Status& /*s*/) noexcept {}
inline void IgnoreStatus(Status&& /*s*/) noexcept {}

// ── Status factory helpers ───────────────────────────────────────────

inline Status CancelledError(std::string msg)          { return Status(StatusCode::kCancelled,          std::move(msg)); }
inline Status UnknownError(std::string msg)            { return Status(StatusCode::kUnknown,             std::move(msg)); }
inline Status InvalidArgumentError(std::string msg)    { return Status(StatusCode::kInvalidArgument,     std::move(msg)); }
inline Status DeadlineExceededError(std::string msg)   { return Status(StatusCode::kDeadlineExceeded,    std::move(msg)); }
inline Status NotFoundError(std::string msg)           { return Status(StatusCode::kNotFound,            std::move(msg)); }
inline Status AlreadyExistsError(std::string msg)      { return Status(StatusCode::kAlreadyExists,       std::move(msg)); }
inline Status PermissionDeniedError(std::string msg)   { return Status(StatusCode::kPermissionDenied,    std::move(msg)); }
inline Status ResourceExhaustedError(std::string msg)  { return Status(StatusCode::kResourceExhausted,   std::move(msg)); }
inline Status FailedPreconditionError(std::string msg) { return Status(StatusCode::kFailedPrecondition,  std::move(msg)); }
inline Status AbortedError(std::string msg)            { return Status(StatusCode::kAborted,             std::move(msg)); }
inline Status OutOfRangeError(std::string msg)         { return Status(StatusCode::kOutOfRange,          std::move(msg)); }
inline Status UnimplementedError(std::string msg)      { return Status(StatusCode::kUnimplemented,       std::move(msg)); }
inline Status InternalError(std::string msg)           { return Status(StatusCode::kInternal,            std::move(msg)); }
inline Status UnavailableError(std::string msg)        { return Status(StatusCode::kUnavailable,         std::move(msg)); }
inline Status DataLossError(std::string msg)           { return Status(StatusCode::kDataLoss,            std::move(msg)); }
inline Status UnauthenticatedError(std::string msg)    { return Status(StatusCode::kUnauthenticated,     std::move(msg)); }

} // namespace zeta

#endif // ZETA_STATUS_STATUS_H
