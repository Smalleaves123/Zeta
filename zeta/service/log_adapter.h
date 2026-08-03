#ifndef ZETA_SERVICE_LOG_ADAPTER_H
#define ZETA_SERVICE_LOG_ADAPTER_H

/// @file   service/log_adapter.h
/// @brief  Optional structured-log enrichment for request contexts.

#include "zeta/log/log.h"
#include "zeta/service/request_context.h"

#include <utility>

namespace zeta {

inline LogMessage& WithRequestContext(
    LogMessage& message, const RequestContext& context) {
    if (!context.request_id().empty()) {
        message.WithField("request_id", context.request_id());
    }
    if (!context.trace_id().empty()) {
        message.WithField("trace_id", context.trace_id());
    }
    if (const TraceContext* trace_context = context.trace_context();
        trace_context != nullptr) {
        message.WithField("span_id", trace_context->span_id());
        message.WithField("traceparent", trace_context->ToTraceParent());
    }
    return message;
}

inline LogMessage&& WithRequestContext(
    LogMessage&& message, const RequestContext& context) {
    WithRequestContext(message, context);
    return std::move(message);
}

} // namespace zeta

#endif // ZETA_SERVICE_LOG_ADAPTER_H
