#include <zeta/log/log.h>
#include <zeta/random/random.h>

#include <array>
#include <iostream>

int main() {
    zeta::BitGen gen(42);
    std::array<int, 5> rolls{};
    for (auto& value : rolls) {
        value = zeta::Uniform(gen, 1, 6);
    }

    ZETA_LOG(INFO) << "generated deterministic dice rolls";

    std::cout << "rolls:";
    for (int value : rolls) {
        std::cout << ' ' << value;
    }
    std::cout << '\n';

    std::cout << "coin_flip=" << (zeta::Bernoulli(gen, 0.5) ? "heads" : "tails")
              << '\n';
    return 0;
}
