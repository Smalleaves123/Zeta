#include <array>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <string>
#include <string_view>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size);

namespace {

std::vector<std::uint8_t> MakeInput(std::mt19937_64& rng, std::size_t size) {
    std::vector<std::uint8_t> input(size);
    for (auto& byte : input) {
        byte = static_cast<std::uint8_t>(rng() & 0xFFu);
    }
    return input;
}

void RunOne(const std::vector<std::uint8_t>& input) {
    (void)LLVMFuzzerTestOneInput(input.data(), input.size());
}

} // namespace

int main(int argc, char** argv) {
    std::array<unsigned, 5> seeds{
        0x5A, 0x45, 0x54, 0x41, static_cast<unsigned>(argc),
    };
    std::seed_seq seed(seeds.begin(), seeds.end());
    std::mt19937_64 rng(seed);

    // Optional file input keeps the harness useful in CI and local smoke runs.
    if (argc > 1 && argv[1] != nullptr && std::strlen(argv[1]) > 0) {
        std::string path = argv[1];
        if (FILE* file = std::fopen(path.c_str(), "rb")) {
            std::vector<std::uint8_t> buffer;
            std::fseek(file, 0, SEEK_END);
            long len = std::ftell(file);
            std::rewind(file);
            if (len > 0) {
                buffer.resize(static_cast<std::size_t>(len));
                if (std::fread(buffer.data(), 1, buffer.size(), file) ==
                    buffer.size()) {
                    RunOne(buffer);
                }
            }
            std::fclose(file);
        }
    }

    const std::vector<std::vector<std::uint8_t>> corpus = {
        {}, {0}, {0, 1, 2, 3}, {0xFF, 0x00, 0x7F, 0x80},
    };
    for (const auto& entry : corpus) {
        RunOne(entry);
    }

    for (int i = 0; i < 256; ++i) {
        std::size_t size = static_cast<std::size_t>(rng() % 512u);
        RunOne(MakeInput(rng, size));
    }

    return 0;
}
