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
    context.SetRequestId("request-42");
    context.SetTraceId("trace-7");
    context.SetMetadata("tenant", "acme");

    zeta::RequestContext::Metadata carrier;
    zeta::InjectRequestContext(context, carrier);
    const auto downstream = zeta::ExtractRequestContext(
        carrier, zeta::Deadline::After(zeta::Duration::Milliseconds(500)));

    const zeta::Status result = HandleRequest(downstream);
    if (!result.ok()) {
        std::cerr << result.ToString() << std::endl;
        return 1;
    }
    return 0;
}
