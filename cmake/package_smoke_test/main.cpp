#include <zeta/container/flat_hash_map.h>
#include <zeta/strings/str_cat.h>

#include <string>

int main() {
    zeta::flat_hash_map<std::string, int> values;
    values["alpha"] = 7;

    const std::string message = zeta::StrCat("alpha=", values.at("alpha"));
    return message == "alpha=7" ? 0 : 1;
}
