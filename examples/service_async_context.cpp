#include <zeta/futures/future.h>
#include <zeta/service/context_executor.h>

#include <iostream>

int main() {
    zeta::ThreadPoolExecutor pool(1);
    auto context = zeta::RequestContext::WithTimeout(
        zeta::Duration::Seconds(1));
    if (!context.SetRequestId("request-42").ok() ||
        !context.SetTraceId("trace-7").ok() ||
        !context.SetMetadata("tenant", "acme").ok()) {
        std::cerr << "invalid request context" << std::endl;
        return 1;
    }

    zeta::ContextExecutor executor(pool, context);
    auto [promise, future] = zeta::makePromiseContract<int>();
    auto result = std::move(future).Via(executor).Then([](int value) {
        const zeta::RequestContext* current = zeta::CurrentRequestContext();
        if (current == nullptr) return -1;

        std::cout << "request=" << current->request_id()
                  << " trace=" << current->trace_id()
                  << " tenant=" << current->GetMetadata("tenant").value_or("")
                  << " remaining_ms=" << current->Remaining().ToMilliseconds()
                  << std::endl;
        return value + 1;
    });

    (void)promise.SetValue(41);
    const auto output = std::move(result).Get();
    if (!output.ok() || *output != 42) return 1;
    return 0;
}
