#include <zeta/strings/format.h>
#include <zeta/strings/str_join.h>
#include <zeta/strings/str_split.h>
#include <zeta/strings/str_utils.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
    const std::string input = "  api,worker, scheduler ,, cron  ";
    std::vector<std::string> cleaned;

    for (auto part : zeta::StrSplit(input, ',', zeta::SkipEmpty)) {
        auto trimmed = zeta::StripAsciiWhitespace(part);
        if (!trimmed.empty()) {
            cleaned.push_back(zeta::AsciiStrToLower(trimmed));
        }
    }

    const std::string normalized = zeta::StrJoin(cleaned, "|");
    std::cout << zeta::Format("normalized={}", normalized) << '\n';
    std::cout << "has_worker="
              << (zeta::StrContains(normalized, "worker") ? "yes" : "no")
              << '\n';
    return 0;
}
