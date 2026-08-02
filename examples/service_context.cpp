#include <zeta/service/propagation.h>

#include <iostream>

namespace {

zeta::Status HandleRequest(const zeta::RequestContext& context) {
    const zeta::Status state = context.Check();
    if (!state.ok()) return state;

    const auto tenant = context.GetMetadata("tenant");
    if (!tenant.has_value()) {
        return zeta::InvalidArgumentError("tenant metadata is required");
    }

    std::cout << "request=" << context.request_id()
              << " tenant=" << *tenant
              << " remaining_ms=" << context.Remaining().ToMilliseconds()
              << std::endl;
    return zeta::OkStatus();
}

} // namespace

int main() {
    zeta::CancellationSource cancellation;
    auto context = zeta::RequestContext::WithTimeout(
        zeta::Duration::Seconds(1), cancellation.GetToken());
    if (!context.SetRequestId("request-42").ok() ||
        !context.SetTraceId("trace-7").ok() ||
        !context.SetMetadata("tenant", "acme").ok()) {
        std::cerr << "invalid request context" << std::endl;
        return 1;
    }

    zeta::RequestContext::Metadata carrier;
    const zeta::Status injected = zeta::InjectRequestContext(context, carrier);
    if (!injected.ok()) {
        std::cerr << injected.ToString() << std::endl;
        return 1;
    }
    const auto downstream_result = zeta::ExtractRequestContext(
        carrier, zeta::Deadline::After(zeta::Duration::Milliseconds(500)));
    if (!downstream_result.ok()) {
        std::cerr << downstream_result.status().ToString() << std::endl;
        return 1;
    }

    const zeta::Status result = HandleRequest(*downstream_result);
    if (!result.ok()) {
        std::cerr << result.ToString() << std::endl;
        return 1;
    }
    return 0;
}
