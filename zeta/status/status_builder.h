#ifndef ZETA_STATUS_STATUS_BUILDER_H
#define ZETA_STATUS_STATUS_BUILDER_H

/// @file   status/status_builder.h
/// @brief  Helpers for enriching and inspecting Status values.

#include "zeta/status/internal/message_join.h"
#include "zeta/status/status.h"

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace zeta {

[[nodiscard]] inline bool IsCancelled(const Status& status) noexcept {
    return status.code() == StatusCode::kCancelled;
}
[[nodiscard]] inline bool IsUnknown(const Status& status) noexcept {
    return status.code() == StatusCode::kUnknown;
}
[[nodiscard]] inline bool IsInvalidArgument(const Status& status) noexcept {
    return status.code() == StatusCode::kInvalidArgument;
}
[[nodiscard]] inline bool IsDeadlineExceeded(const Status& status) noexcept {
    return status.code() == StatusCode::kDeadlineExceeded;
}
[[nodiscard]] inline bool IsNotFound(const Status& status) noexcept {
    return status.code() == StatusCode::kNotFound;
}
[[nodiscard]] inline bool IsAlreadyExists(const Status& status) noexcept {
    return status.code() == StatusCode::kAlreadyExists;
}
[[nodiscard]] inline bool IsPermissionDenied(const Status& status) noexcept {
    return status.code() == StatusCode::kPermissionDenied;
}
[[nodiscard]] inline bool IsResourceExhausted(const Status& status) noexcept {
    return status.code() == StatusCode::kResourceExhausted;
}
[[nodiscard]] inline bool IsFailedPrecondition(const Status& status) noexcept {
    return status.code() == StatusCode::kFailedPrecondition;
}
[[nodiscard]] inline bool IsAborted(const Status& status) noexcept {
    return status.code() == StatusCode::kAborted;
}
[[nodiscard]] inline bool IsOutOfRange(const Status& status) noexcept {
    return status.code() == StatusCode::kOutOfRange;
}
[[nodiscard]] inline bool IsUnimplemented(const Status& status) noexcept {
    return status.code() == StatusCode::kUnimplemented;
}
[[nodiscard]] inline bool IsInternal(const Status& status) noexcept {
    return status.code() == StatusCode::kInternal;
}
[[nodiscard]] inline bool IsUnavailable(const Status& status) noexcept {
    return status.code() == StatusCode::kUnavailable;
}
[[nodiscard]] inline bool IsDataLoss(const Status& status) noexcept {
    return status.code() == StatusCode::kDataLoss;
}
[[nodiscard]] inline bool IsUnauthenticated(const Status& status) noexcept {
    return status.code() == StatusCode::kUnauthenticated;
}

[[nodiscard]] inline Status AppendMessage(
    Status status, std::string_view message,
    std::string_view separator = ": ") {
    if (status.ok() || message.empty()) return status;
    return Status(status.code(), status_internal::JoinStatusMessage(
                                   status.message(), separator, message));
}

[[nodiscard]] inline Status PrependMessage(
    Status status, std::string_view message,
    std::string_view separator = ": ") {
    if (status.ok() || message.empty()) return status;
    return Status(status.code(), status_internal::JoinStatusMessage(
                                   message, separator, status.message()));
}

[[nodiscard]] inline Status Annotate(
    Status status, std::string_view context,
    std::string_view separator = ": ") {
    return AppendMessage(std::move(status), context, separator);
}

class StatusBuilder {
public:
    explicit StatusBuilder(Status status) : status_(std::move(status)) {}

    template <typename T>
    StatusBuilder& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

    StatusBuilder& SetPrepend() noexcept {
        prepend_ = true;
        return *this;
    }

    StatusBuilder& SetAppend() noexcept {
        prepend_ = false;
        return *this;
    }

    StatusBuilder& SetSeparator(std::string separator) {
        separator_ = std::move(separator);
        return *this;
    }

    [[nodiscard]] Status Build() const& {
        if (status_.ok()) return status_;
        const std::string extra = stream_.str();
        if (extra.empty()) return status_;
        return prepend_ ? PrependMessage(status_, extra, separator_)
                        : AppendMessage(status_, extra, separator_);
    }

    [[nodiscard]] Status Build() && {
        if (status_.ok()) return std::move(status_);
        const std::string extra = stream_.str();
        if (extra.empty()) return std::move(status_);
        return prepend_ ? PrependMessage(std::move(status_), extra, separator_)
                        : AppendMessage(std::move(status_), extra, separator_);
    }

    [[nodiscard]] operator Status() const& {  // NOLINT
        return Build();
    }

    [[nodiscard]] operator Status() && {  // NOLINT
        return std::move(*this).Build();
    }

private:
    Status status_;
    std::ostringstream stream_;
    std::string separator_ = ": ";
    bool prepend_ = false;
};

} // namespace zeta

#endif // ZETA_STATUS_STATUS_BUILDER_H
