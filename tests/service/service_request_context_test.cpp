#include "zeta/service/context_executor.h"
#include "zeta/service/propagation.h"
#include "zeta/service/log_adapter.h"
#include "zeta/futures/future.h"
#include "zeta/log/formatters.h"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
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

TEST_CASE("RequestContext: enriches structured logs through an adapter",
          "[service][log]") {
    auto path = std::filesystem::temp_directory_path() /
        "zeta_service_log_context_test.log";
    std::filesystem::remove(path);

    const auto trace = zeta::TraceContext::ParseTraceParent(
        "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
    REQUIRE(trace.ok());

    zeta::RequestContext context;
    REQUIRE(context.SetRequestId("request-42").ok());
    REQUIRE(context.SetTraceContext(*trace).ok());

    {
        zeta::JsonLogFormatter formatter;
        zeta::log_internal::FileLogSink sink(path, 0, false);
        zeta::log_internal::ScopedLogSink scoped_sink(&sink);
        zeta::log_internal::ScopedLogFormatter scoped_formatter(&formatter);
        zeta::WithRequestContext(
            zeta::LogMessage(zeta::log_internal::LogSeverity::INFO,
                             "context.cpp", 21),
            context)
            << "request completed";
    }

    std::ifstream in(path);
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    REQUIRE(content.find("\"request_id\":\"request-42\"") !=
            std::string::npos);
    REQUIRE(content.find("\"trace_id\":\"4bf92f3577b34da6a3ce929d0e0e4736\"") !=
            std::string::npos);
    REQUIRE(content.find("\"span_id\":\"00f067aa0ba902b7\"") !=
            std::string::npos);

    std::filesystem::remove(path);
}

TEST_CASE("RequestContextScope: installs and restores nested contexts",
          "[service][context]") {
    REQUIRE(zeta::CurrentRequestContext() == nullptr);

    zeta::RequestContext outer;
    REQUIRE(outer.SetRequestId("outer").ok());
    zeta::RequestContext inner;
    REQUIRE(inner.SetRequestId("inner").ok());

    {
        zeta::RequestContextScope outer_scope(outer);
        REQUIRE(zeta::CurrentRequestContext() == &outer);
        {
            zeta::RequestContextScope inner_scope(inner);
            REQUIRE(zeta::CurrentRequestContext() == &inner);
        }
        REQUIRE(zeta::CurrentRequestContext() == &outer);
    }
    REQUIRE(zeta::CurrentRequestContext() == nullptr);
}

TEST_CASE("ContextExecutor: propagates context and cleans up worker scope",
          "[service][context][concurrency]") {
    zeta::RequestContext context = zeta::RequestContext::WithTimeout(
        zeta::Duration::Seconds(5));
    REQUIRE(context.SetRequestId("request-42").ok());
    REQUIRE(context.SetTraceId("trace-7").ok());
    REQUIRE(context.SetMetadata("tenant", "acme").ok());

    zeta::ThreadPoolExecutor pool(1);
    zeta::ContextExecutor executor(pool, context);

    std::promise<std::string> observed;
    auto observed_result = observed.get_future();
    executor.Add([&observed] {
        const auto* current = zeta::CurrentRequestContext();
        if (current == nullptr) {
            observed.set_value("missing");
            return;
        }
        observed.set_value(
            std::string(current->request_id()) + ":" +
            std::string(current->trace_id()) + ":" +
            std::string(current->GetMetadata("tenant").value_or("")));
    });

    REQUIRE(observed_result.wait_for(std::chrono::seconds(1)) ==
            std::future_status::ready);
    REQUIRE(observed_result.get() == "request-42:trace-7:acme");

    std::promise<bool> cleaned;
    auto cleaned_result = cleaned.get_future();
    pool.Add([&cleaned] {
        cleaned.set_value(zeta::CurrentRequestContext() == nullptr);
    });
    REQUIRE(cleaned_result.wait_for(std::chrono::seconds(1)) ==
            std::future_status::ready);
    REQUIRE(cleaned_result.get());
}

TEST_CASE("ContextExecutor: propagates context through Future continuation",
          "[service][context][futures]") {
    zeta::CancellationSource cancellation;
    zeta::RequestContext context = zeta::RequestContext::WithTimeout(
        zeta::Duration::Seconds(5), cancellation.GetToken());
    REQUIRE(context.SetRequestId("request-42").ok());
    REQUIRE(context.SetTraceId("trace-7").ok());

    zeta::ThreadPoolExecutor pool(1);
    zeta::ContextExecutor executor(pool, context);
    auto [promise, future] = zeta::makePromiseContract<int>();
    std::promise<bool> observed;
    auto observed_result = observed.get_future();

    auto next = std::move(future).Via(executor).Then([&observed](int value) {
        const auto* current = zeta::CurrentRequestContext();
        observed.set_value(
            current != nullptr && current->request_id() == "request-42" &&
            current->trace_id() == "trace-7" && !current->IsCancelled() &&
            !current->IsExpired());
        return value + 1;
    });

    REQUIRE(promise.SetValue(41).ok());
    const auto output = std::move(next).Get();
    REQUIRE(output.ok());
    REQUIRE(*output == 42);
    REQUIRE(observed_result.wait_for(std::chrono::seconds(1)) ==
            std::future_status::ready);
    REQUIRE(observed_result.get());
}
