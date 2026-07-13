#include <zeta/algorithm/algorithm.h>
#include <zeta/base/as_const.h>
#include <zeta/container/flat_hash_map.h>
#include <zeta/metrics/metrics.h>
#include <zeta/strings/str_cat.h>

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

    const std::string message =
        zeta::StrCat("alpha=", values.at("alpha"), ", requests=", requests.value());
    return message == "alpha=7, requests=1" ? 0 : 1;
}
