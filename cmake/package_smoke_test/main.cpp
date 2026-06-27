#include <zeta/base/as_const.h>
#include <zeta/container/flat_hash_map.h>
#include <zeta/strings/str_cat.h>

#include <string>

int main() {
    int value = 7;
    const int& ref = zeta::as_const(value);

    zeta::flat_hash_map<std::string, int> values;
    values["alpha"] = ref;

    const std::string message = zeta::StrCat("alpha=", values.at("alpha"));
    return message == "alpha=7" ? 0 : 1;
}
