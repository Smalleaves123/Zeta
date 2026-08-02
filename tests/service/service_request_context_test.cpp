#include "zeta/service/propagation.h"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>

TEST_CASE("RequestContext: defaults to an unbounded active request", "[service]") {
    const zeta::RequestContext context;

    REQUIRE(!context.IsCancelled());
    REQUIRE(!context.IsExpired());
    REQUIRE(!context.IsDone());
    REQUIRE(context.Check().ok());
    REQUIRE(context.Remaining() == zeta::Duration::Infinite());
}

TEST_CASE("RequestContext: reports deadline state", "[service]") {
    const auto context = zeta::RequestContext::WithTimeout(zeta::Duration());

    REQUIRE(context.IsExpired());
    REQUIRE(context.IsDone());
    REQUIRE(context.Check().code() == zeta::StatusCode::kDeadlineExceeded);
}

TEST_CASE("RequestContext: reports cooperative cancellation", "[service]") {
    zeta::CancellationSource source;
    const auto context = zeta::RequestContext::WithDeadline(
        zeta::Deadline::Never(), source.GetToken());

    REQUIRE(source.RequestCancellation());
    REQUIRE(!source.RequestCancellation());
    REQUIRE(context.IsCancelled());
    REQUIRE(context.Check().code() == zeta::StatusCode::kCancelled);
}

TEST_CASE("RequestContext: stores and updates request metadata", "[service]") {
    zeta::RequestContext context;
    REQUIRE(context.SetRequestId("request-42").ok());
    REQUIRE(context.SetTraceId("trace-7").ok());
    REQUIRE(context.SetMetadata("tenant", "acme").ok());
    REQUIRE(context.SetMetadata("tenant", "globex").ok());

    REQUIRE(context.request_id() == "request-42");
    REQUIRE(context.trace_id() == "trace-7");
    REQUIRE(context.GetMetadata("tenant").value() == "globex");
    REQUIRE(!context.GetMetadata("missing").has_value());
    REQUIRE(context.metadata().size() == 1);
}

TEST_CASE("RequestContext: child inherits request state and tightens deadline", "[service]") {
    zeta::CancellationSource source;
    const auto parent_deadline = zeta::Deadline(
        zeta::Clock::Now() + zeta::Duration::Seconds(5).ToRaw());
    auto parent = zeta::RequestContext::WithDeadline(
        parent_deadline, source.GetToken());
    REQUIRE(parent.SetRequestId("request-42").ok());
    REQUIRE(parent.SetTraceId("trace-7").ok());
    REQUIRE(parent.SetMetadata("tenant", "acme").ok());

    const auto child = parent.ChildWithTimeout(zeta::Duration::Seconds(1));

    REQUIRE(child.request_id() == "request-42");
    REQUIRE(child.trace_id() == "trace-7");
    REQUIRE(child.GetMetadata("tenant").value() == "acme");
    REQUIRE(child.deadline().time_point() < parent.deadline().time_point());
    REQUIRE(&child.cancellation() != &parent.cancellation());

    (void)source.RequestCancellation();
    REQUIRE(child.IsCancelled());
}

TEST_CASE("RequestContext: propagates through a generic metadata carrier", "[service]") {
    zeta::RequestContext source;
    REQUIRE(source.SetRequestId("request-42").ok());
    REQUIRE(source.SetTraceId("trace-7").ok());
    REQUIRE(source.SetMetadata("tenant", "acme").ok());

    zeta::RequestContext::Metadata carrier;
    REQUIRE(zeta::InjectRequestContext(source, carrier).ok());
    const auto target_result = zeta::ExtractRequestContext(carrier);

    REQUIRE(target_result.ok());
    const auto& target = *target_result;
    REQUIRE(target.request_id() == "request-42");
    REQUIRE(target.trace_id() == "trace-7");
    REQUIRE(target.GetMetadata("tenant").value() == "acme");
}

TEST_CASE("RequestContext: validates W3C traceparent", "[service][trace]") {
    const auto parsed = zeta::TraceContext::ParseTraceParent(
        "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");

    REQUIRE(parsed.ok());
    REQUIRE(parsed->trace_id() == "4bf92f3577b34da6a3ce929d0e0e4736");
    REQUIRE(parsed->span_id() == "00f067aa0ba902b7");
    REQUIRE(parsed->sampled());
    REQUIRE(parsed->ToTraceParent() ==
            "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");

    zeta::RequestContext context;
    REQUIRE(context.SetTraceContext(*parsed).ok());
    zeta::RequestContext::Metadata carrier;
    REQUIRE(zeta::InjectRequestContext(context, carrier).ok());
    REQUIRE(carrier.front().first == zeta::kTraceParentHeader);
    REQUIRE(carrier.front().second == parsed->ToTraceParent());
}

TEST_CASE("RequestContext: rejects untrusted metadata limits", "[service]") {
    zeta::RequestContext context;
    REQUIRE_FALSE(context.SetMetadata("", "value").ok());
    REQUIRE_FALSE(context.SetMetadata(
        std::string(zeta::RequestContext::kMaxMetadataKeyLength + 1, 'k'),
        "value").ok());
    REQUIRE_FALSE(context.SetMetadata(
        "key",
        std::string(zeta::RequestContext::kMaxMetadataValueLength + 1, 'v')).ok());

    for (std::size_t index = 0;
         index < zeta::RequestContext::kMaxMetadataEntries;
         ++index) {
        REQUIRE(context.SetMetadata("key-" + std::to_string(index), "value").ok());
    }
    REQUIRE(context.SetMetadata("overflow", "value").code() ==
            zeta::StatusCode::kResourceExhausted);
}
