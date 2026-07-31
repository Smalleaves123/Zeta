#include "zeta/service/propagation.h"

#include <catch2/catch_test_macros.hpp>

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
    context.SetRequestId("request-42");
    context.SetTraceId("trace-7");
    context.SetMetadata("tenant", "acme");
    context.SetMetadata("tenant", "globex");

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
    parent.SetRequestId("request-42");
    parent.SetTraceId("trace-7");
    parent.SetMetadata("tenant", "acme");

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
    source.SetRequestId("request-42");
    source.SetTraceId("trace-7");
    source.SetMetadata("tenant", "acme");

    zeta::RequestContext::Metadata carrier;
    zeta::InjectRequestContext(source, carrier);
    const auto target = zeta::ExtractRequestContext(carrier);

    REQUIRE(target.request_id() == "request-42");
    REQUIRE(target.trace_id() == "trace-7");
    REQUIRE(target.GetMetadata("tenant").value() == "acme");
}
