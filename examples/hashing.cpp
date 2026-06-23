#include <zeta/hash/hash.h>
#include <zeta/strings/str_split.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
    const std::string payload = "tenant=blue;region=us-west;service=api";
    std::vector<std::string> fields;
    for (auto part : zeta::StrSplit(payload, ';', zeta::SkipEmpty)) {
        fields.emplace_back(part);
    }

    const std::uint64_t payload_hash = zeta::hash_bytes(payload.data(), payload.size());
    const std::uint64_t field_hash = zeta::hash_range(fields.begin(), fields.end());

    std::cout << "payload_hash=" << payload_hash << '\n';
    std::cout << "field_hash=" << field_hash << '\n';
    return 0;
}
