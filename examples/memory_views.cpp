#include <zeta/memory/function_ref.h>
#include <zeta/memory/span.h>

#include <iostream>
#include <vector>

namespace {

void ForEach(zeta::Span<int> values, zeta::function_ref<void(int&)> fn) {
    for (int& value : values) fn(value);
}

int Sum(zeta::Span<const int> values) {
    int total = 0;
    for (int value : values) total += value;
    return total;
}

} // namespace

int main() {
    std::vector<int> values = {1, 2, 3, 4, 5};

    auto double_value = [](int& value) { value *= 2; };
    ForEach(values, double_value);

    zeta::Span<const int> tail(values.data() + 2, values.size() - 2);
    std::cout << "sum_all=" << Sum(values) << '\n';
    std::cout << "sum_tail=" << Sum(tail) << '\n';
    return 0;
}
