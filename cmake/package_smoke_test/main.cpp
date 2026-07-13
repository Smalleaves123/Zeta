#include <zeta/algorithm/algorithm.h>
#include <zeta/base/as_const.h>
#include <zeta/container/flat_hash_map.h>
#include <zeta/crc/crc32c.h>
#include <zeta/log/formatters.h>
#include <zeta/metrics/metrics.h>
#include <zeta/strings/str_cat.h>

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

    std::array<zeta::LogField, 1> fields{{{"request_id", "42"}}};
    zeta::LogRecordView record{
        zeta::log_internal::LogSeverity::INFO, "smoke.cpp", 1, "ok", fields};
    zeta::JsonLogFormatter formatter;
    const std::string json = formatter.Format(record);
    return json.find("\"request_id\":\"42\"") != std::string::npos ? 0 : 1;
}
