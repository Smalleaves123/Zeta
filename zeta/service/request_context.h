#ifndef ZETA_SERVICE_REQUEST_CONTEXT_H
#define ZETA_SERVICE_REQUEST_CONTEXT_H

/// @file   service/request_context.h
/// @brief  Request-scoped metadata, deadline, and cancellation state.

#include "zeta/futures/cancellation.h"
#include "zeta/service/trace_context.h"
#include "zeta/status/status.h"
#include "zeta/time/stopwatch.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace zeta {

class RequestContext {
public:
    using MetadataEntry = std::pair<std::string, std::string>;
    using Metadata = std::vector<MetadataEntry>;

    static constexpr std::size_t kMaxMetadataEntries = 64;
    static constexpr std::size_t kMaxMetadataKeyLength = 128;
    static constexpr std::size_t kMaxMetadataValueLength = 4096;
    static constexpr std::size_t kMaxRequestIdLength = 128;
    static constexpr std::size_t kMaxTraceIdLength = 128;

    RequestContext() noexcept
        : deadline_(Deadline::Never()) {}

    explicit RequestContext(
        Deadline deadline, CancellationToken cancellation = {}) noexcept
        : deadline_(deadline), cancellation_(std::move(cancellation)) {}

    [[nodiscard]] static RequestContext WithDeadline(
        Deadline deadline, CancellationToken cancellation = {}) noexcept {
        return RequestContext(deadline, std::move(cancellation));
    }

    [[nodiscard]] static RequestContext WithTimeout(
        Duration timeout, CancellationToken cancellation = {}) noexcept {
        return RequestContext(Deadline::After(timeout), std::move(cancellation));
    }

    [[nodiscard]] RequestContext Child() const {
        return ChildWithTimeout(Duration::Infinite());
    }

    [[nodiscard]] RequestContext ChildWithTimeout(Duration timeout) const {
        const Deadline child_deadline = Deadline::After(timeout);
        RequestContext child(
            deadline_.time_point() < child_deadline.time_point()
                ? deadline_
                : child_deadline,
            cancellation_);
        child.request_id_ = request_id_;
        child.trace_id_ = trace_id_;
        child.trace_context_ = trace_context_;
        child.metadata_ = metadata_;
        return child;
    }

    [[nodiscard]] const Deadline& deadline() const noexcept {
        return deadline_;
    }

    [[nodiscard]] Duration Remaining() const noexcept {
        return deadline_.Remaining();
    }

    [[nodiscard]] bool IsCancelled() const noexcept {
        return cancellation_.IsCancellationRequested();
    }

    [[nodiscard]] bool IsExpired() const noexcept {
        return deadline_.Expired();
    }

    [[nodiscard]] bool IsDone() const noexcept {
        return IsCancelled() || IsExpired();
    }

    [[nodiscard]] Status Check() const {
        if (IsCancelled()) return CancelledError("request cancelled");
        if (IsExpired()) return DeadlineExceededError("request deadline exceeded");
        return OkStatus();
    }

    [[nodiscard]] const CancellationToken& cancellation() const noexcept {
        return cancellation_;
    }

    [[nodiscard]] Status SetRequestId(std::string request_id) {
        if (request_id.size() > kMaxRequestIdLength) {
            return InvalidArgumentError("request id is too long");
        }
        request_id_ = std::move(request_id);
        return OkStatus();
    }

    [[nodiscard]] std::string_view request_id() const noexcept {
        return request_id_;
    }

    [[nodiscard]] Status SetTraceId(std::string trace_id) {
        if (trace_id.size() > kMaxTraceIdLength) {
            return InvalidArgumentError("trace id is too long");
        }
        trace_id_ = std::move(trace_id);
        trace_context_.reset();
        return OkStatus();
    }

    [[nodiscard]] std::string_view trace_id() const noexcept {
        return trace_id_;
    }

    [[nodiscard]] const TraceContext* trace_context() const noexcept {
        return trace_context_.has_value() ? &*trace_context_ : nullptr;
    }

    [[nodiscard]] static Status ValidateMetadata(
        std::string_view key, std::string_view value) {
        if (key.empty()) return InvalidArgumentError("metadata key is empty");
        if (key.size() > kMaxMetadataKeyLength) {
            return InvalidArgumentError("metadata key is too long");
        }
        if (value.size() > kMaxMetadataValueLength) {
            return InvalidArgumentError("metadata value is too long");
        }
        return OkStatus();
    }

    [[nodiscard]] Status SetMetadata(std::string key, std::string value) {
        const Status validation = ValidateMetadata(key, value);
        if (!validation.ok()) return validation;
        for (auto& entry : metadata_) {
            if (entry.first == key) {
                entry.second = std::move(value);
                return OkStatus();
            }
        }
        if (metadata_.size() >= kMaxMetadataEntries) {
            return ResourceExhaustedError("metadata entry limit exceeded");
        }
        metadata_.emplace_back(std::move(key), std::move(value));
        return OkStatus();
    }

    [[nodiscard]] Status SetTraceContext(TraceContext trace_context) {
        const Status validation = trace_context.Validate();
        if (!validation.ok()) return validation;
        trace_id_ = std::string(trace_context.trace_id());
        trace_context_ = std::move(trace_context);
        return OkStatus();
    }

    [[nodiscard]] std::optional<std::string_view> GetMetadata(
        std::string_view key) const noexcept {
        for (const auto& entry : metadata_) {
            if (entry.first == key) return entry.second;
        }
        return std::nullopt;
    }

    [[nodiscard]] const Metadata& metadata() const noexcept {
        return metadata_;
    }

private:
    Deadline deadline_;
    CancellationToken cancellation_;
    std::string request_id_;
    std::string trace_id_;
    std::optional<TraceContext> trace_context_;
    Metadata metadata_;
};

} // namespace zeta

#endif // ZETA_SERVICE_REQUEST_CONTEXT_H
