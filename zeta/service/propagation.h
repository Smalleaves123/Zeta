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
inline constexpr std::string_view kTraceIdHeader = "x-trace-id";

namespace service_internal {

inline void UpsertMetadata(
    RequestContext::Metadata& carrier,
    std::string_view key,
    std::string_view value) {
    for (auto& entry : carrier) {
        if (entry.first == key) {
            entry.second.assign(value);
            return;
        }
    }
    carrier.emplace_back(std::string(key), std::string(value));
}

} // namespace service_internal

inline void InjectRequestContext(
    const RequestContext& context,
    RequestContext::Metadata& carrier) {
    if (!context.request_id().empty()) {
        service_internal::UpsertMetadata(
            carrier, kRequestIdHeader, context.request_id());
    }
    if (!context.trace_id().empty()) {
        service_internal::UpsertMetadata(
            carrier, kTraceIdHeader, context.trace_id());
    }
    for (const auto& entry : context.metadata()) {
        if (entry.first == kRequestIdHeader || entry.first == kTraceIdHeader) {
            continue;
        }
        service_internal::UpsertMetadata(carrier, entry.first, entry.second);
    }
}

[[nodiscard]] inline RequestContext ExtractRequestContext(
    const RequestContext::Metadata& carrier,
    Deadline deadline = Deadline::Never(),
    CancellationToken cancellation = {}) {
    RequestContext context =
        RequestContext::WithDeadline(deadline, std::move(cancellation));
    for (const auto& entry : carrier) {
        if (entry.first == kRequestIdHeader) {
            context.SetRequestId(entry.second);
        } else if (entry.first == kTraceIdHeader) {
            context.SetTraceId(entry.second);
        } else {
            context.SetMetadata(entry.first, entry.second);
        }
    }
    return context;
}

} // namespace zeta

#endif // ZETA_SERVICE_PROPAGATION_H
