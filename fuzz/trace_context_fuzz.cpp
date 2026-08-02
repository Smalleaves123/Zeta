#include "zeta/service/trace_context.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size) {
    const std::string input(
        reinterpret_cast<const char*>(data), size);
    const auto parsed = zeta::TraceContext::ParseTraceParent(input);
    if (parsed.ok()) {
        const auto round_trip =
            zeta::TraceContext::ParseTraceParent(parsed->ToTraceParent());
        if (!round_trip.ok()) std::abort();
    }
    return 0;
}
