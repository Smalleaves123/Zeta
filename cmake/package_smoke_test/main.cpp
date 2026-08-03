#include <zeta/algorithm/algorithm.h>
#include <zeta/base/as_const.h>
#include <zeta/container/flat_hash_map.h>
#include <zeta/container/lfu_cache.h>
#include <zeta/container/lru_cache.h>
#include <zeta/container/small_map.h>
#include <zeta/crc/crc32c.h>
#include <zeta/time/civil_time.h>
#include <zeta/debugging/assert.h>
#include <zeta/debugging/stack_trace.h>
#include <zeta/flags/flag.h>
#include <zeta/functional/pipe.h>
#include <zeta/log/formatters.h>
#include <zeta/metrics/metrics.h>
#include <zeta/memory/byte_buffer.h>
#include <zeta/memory/object_pool.h>
#include <zeta/random/random.h>
#include <zeta/status/status_chain.h>
#include <zeta/service/propagation.h>
#include <zeta/strings/str_cat.h>
#include <zeta/synchronization/channel.h>
#include <zeta/synchronization/bounded_queue.h>
#include <zeta/synchronization/lock_free_queue.h>

#include <array>
#include <string>

int main() {
    int value = 7;
    const int& ref = zeta::as_const(value);

    zeta::flat_hash_map<std::string, int> values;
    values["alpha"] = ref;

    zeta::metrics::Counter requests;
    requests.Increment();

    const int keys[] = {7};
    if (!zeta::c_contains(keys, 7)) return 1;
    if (zeta::ComputeCrc32c("alpha=7") == 0) return 1;

    const std::string message =
        zeta::StrCat("alpha=", values.at("alpha"), ", requests=", requests.value());
    if (message != "alpha=7, requests=1") return 1;
    if (zeta::pipe(1, [](int number) { return number + 1; }) != 2) return 1;
    ZETA_CHECK(value == 7);
    if (zeta::Symbolize(nullptr) != "0x0") return 1;

    zeta::ByteBuffer buffer;
    buffer.Append("package");
    if (buffer.Size() != 7 || !buffer.Consume(4) || buffer.Size() != 3) {
        return 1;
    }

    zeta::Channel<int> channel(1);
    if (!channel.TrySend(7) || channel.TrySend(8) ||
        channel.TryReceive().value_or(0) != 7) {
        return 1;
    }
    channel.Close();

    zeta::SmallMap<std::string, int, 2> small_values;
    small_values["alpha"] = 7;
    if (!small_values.contains("alpha")) return 1;
    zeta::LruCache<std::string, int> lru(1);
    if (!lru.Put("alpha", 7) || lru.Find("alpha") == nullptr) return 1;
    zeta::LfuCache<std::string, int> lfu(1);
    if (!lfu.Put("alpha", 7) || lfu.Find("alpha") == nullptr) return 1;

    struct PooledValue {
        explicit PooledValue(int value) : value(value) {}
        int value;
    };
    zeta::ObjectPool<PooledValue> objects(2);
    auto* pooled = objects.Create(7);
    if (pooled->value != 7) return 1;
    objects.Destroy(pooled);

    zeta::BoundedQueue<int> bounded(1);
    if (!bounded.TryPush(7) || bounded.TryPush(8) ||
        bounded.TryPop().value_or(0) != 7) {
        return 1;
    }
    zeta::LockFreeQueue<int> lock_free(2);
    if (!lock_free.TryPush(7) || lock_free.TryPop().value_or(0) != 7) {
        return 1;
    }

    zeta::Flag<int> local_flag("local", "local flag", __FILE__, 0);
    if (!local_flag.Parse("7") || local_flag.Get() != 7) return 1;

    zeta::BitGen random(20260714);
    const auto first_random = random();
    random.seed(20260714);
    if (random() != first_random) return 1;
    if (zeta::Exponential(random, 2.0) < 0.0) return 1;

    const auto chained = zeta::ChainStatus(zeta::InternalError("startup"))
                             .CausedBy(zeta::InvalidArgumentError("config"))
                             .ToStatus();
    if (chained.message().find("INVALID_ARGUMENT") == std::string::npos) {
        return 1;
    }

    const auto trace = zeta::TraceContext::ParseTraceParent(
        "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
    if (!trace.ok()) return 1;
    zeta::RequestContext context;
    if (!context.SetTraceContext(*trace).ok()) return 1;
    zeta::RequestContext::Metadata carrier;
    if (!zeta::InjectRequestContext(context, carrier).ok()) return 1;
    const auto extracted = zeta::ExtractRequestContext(carrier);
    if (!extracted.ok() || extracted->trace_context() == nullptr) return 1;

    const auto civil = zeta::ParseCivilDate("2024-02-29");
    if (!civil.has_value() || zeta::FormatCivilDate(*civil) != "2024-02-29") {
        return 1;
    }

    std::array<zeta::LogField, 1> fields{{{"request_id", "42"}}};
    zeta::LogRecordView record{
        zeta::log_internal::LogSeverity::INFO, "smoke.cpp", 1, "ok", fields};
    zeta::JsonLogFormatter formatter;
    const std::string json = formatter.Format(record);
    return json.find("\"request_id\":\"42\"") != std::string::npos ? 0 : 1;
}
