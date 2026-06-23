#include <zeta/container/flat_hash_map.h>
#include <zeta/container/inlined_vector.h>
#include <zeta/strings/str_cat.h>
#include <zeta/strings/str_join.h>
#include <zeta/strings/str_split.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
    zeta::flat_hash_map<std::string, int> scores;
    scores["alpha"] = 7;
    scores["beta"] = 11;

    zeta::InlinedVector<std::string, 4> names = {"alpha", "beta", "gamma"};
    names.push_back("delta");

    std::vector<std::string> present;
    for (const auto& name : names) {
        if (scores.contains(name)) present.push_back(name);
    }

    const std::string joined = zeta::StrJoin(present, ", ");
    const std::string summary =
        zeta::StrCat("known users: ", joined, " (count=", present.size(), ")");

    std::cout << summary << '\n';

    for (auto part : zeta::StrSplit("prod:us-west:api", ':')) {
        std::cout << "segment=" << part << '\n';
    }

    return 0;
}
