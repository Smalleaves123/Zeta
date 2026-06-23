#include <zeta/container/btree_map.h>
#include <zeta/strings/format.h>
#include <zeta/time/timestamp.h>

#include <iostream>
#include <string>

int main() {
    zeta::btree_map<std::string, int> deploy_counts;
    deploy_counts["api"] = 12;
    deploy_counts["scheduler"] = 4;
    deploy_counts["worker"] = 18;

    std::cout << "snapshot=" << zeta::FormatNowUtc() << '\n';
    for (const auto& [name, count] : deploy_counts) {
        std::cout << zeta::Format("{} -> {}", name, count) << '\n';
    }
    return 0;
}
