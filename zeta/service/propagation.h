#ifndef ZETA_SERVICE_PROPAGATION_H
#define ZETA_SERVICE_PROPAGATION_H

/// @file   service/propagation.h
/// @brief  Generic metadata propagation for HTTP and RPC adapters.

#include "zeta/service/request_context.h"

#include <string>
#include <string_view>
#include <utility>

namespace zeta {

inline constexpr std::string_view kRequestIdHeader = "x-request-id";
inline constexpr std::string_view kTraceParentHeader = "traceparent";
inline constexpr std::string_view kTraceIdHeader = "x-trace-id";

namespace service_internal {

inline Status UpsertMetadata(
    RequestContext::Metadata& carrier,
    std::string_view key,
    std::string_view value) {
    const Status validation = RequestContext::ValidateMetadata(key, value);
    if (!validation.ok()) return validation;
    for (auto& entry : carrier) {
        if (entry.first == key) {
            entry.second.assign(value);
            return OkStatus();
        }
    }
    if (carrier.size() >= RequestContext::kMaxMetadataEntries) {
        return ResourceExhaustedError("metadata entry limit exceeded");
    }
    carrier.emplace_back(std::string(key), std::string(value));
    return OkStatus();
}

} // namespace service_internal

[[nodiscard]] inline Status InjectRequestContext(
    const RequestContext& context,
    RequestContext::Metadata& carrier) {
    RequestContext::Metadata updated = carrier;
    if (updated.size() > RequestContext::kMaxMetadataEntries) {
        return ResourceExhaustedError("metadata entry limit exceeded");
    }
    for (const auto& entry : updated) {
        const Status validation =
            RequestContext::ValidateMetadata(entry.first, entry.second);
        if (!validation.ok()) return validation;
    }
    Status result;

    if (!context.request_id().empty()) {
        result = service_internal::UpsertMetadata(
            updated, kRequestIdHeader, context.request_id());
        if (!result.ok()) return result;
    }

    if (const TraceContext* trace_context = context.trace_context();
        trace_context != nullptr) {
        result = service_internal::UpsertMetadata(
            updated, kTraceParentHeader, trace_context->ToTraceParent());
        if (!result.ok()) return result;
    } else if (!context.trace_id().empty()) {
        result = service_internal::UpsertMetadata(
            updated, kTraceIdHeader, context.trace_id());
        if (!result.ok()) return result;
    }

    for (const auto& entry : context.metadata()) {
        if (entry.first == kRequestIdHeader ||
            entry.first == kTraceParentHeader ||
            entry.first == kTraceIdHeader) {
            continue;
        }
        result = service_internal::UpsertMetadata(updated, entry.first, entry.second);
        if (!result.ok()) return result;
    }

    carrier.swap(updated);
    return OkStatus();
}

[[nodiscard]] inline StatusOr<RequestContext> ExtractRequestContext(
    const RequestContext::Metadata& carrier,
    Deadline deadline = Deadline::Never(),
    CancellationToken cancellation = {}) {
    RequestContext context =
        RequestContext::WithDeadline(deadline, std::move(cancellation));
    bool has_trace_parent = false;
    std::string legacy_trace_id;

    for (const auto& entry : carrier) {
        if (entry.first == kRequestIdHeader) {
            const Status result = context.SetRequestId(entry.second);
            if (!result.ok()) return result;
        } else if (entry.first == kTraceParentHeader) {
            const auto parsed = TraceContext::ParseTraceParent(entry.second);
            if (!parsed.ok()) return parsed.status();
            const Status result = context.SetTraceContext(*parsed);
            if (!result.ok()) return result;
            has_trace_parent = true;
        } else if (entry.first == kTraceIdHeader) {
            legacy_trace_id = entry.second;
        } else {
            const Status result = context.SetMetadata(entry.first, entry.second);
            if (!result.ok()) return result;
        }
    }

    if (!has_trace_parent && !legacy_trace_id.empty()) {
        const Status result = context.SetTraceId(std::move(legacy_trace_id));
        if (!result.ok()) return result;
    }
    return context;
}

} // namespace zeta

#endif // ZETA_SERVICE_PROPAGATION_H
