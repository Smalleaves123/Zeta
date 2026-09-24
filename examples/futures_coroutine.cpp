#include <zeta/futures/coroutine.h>

#include <iostream>
#include <utility>

namespace {

zeta::Coroutine<int> AddOne(zeta::Future<int> input) {
    auto result = co_await std::move(input);
    if (!result.ok()) co_return result.status();
    co_return result.value() + 1;
}

}  // namespace

int main() {
    auto [promise, input] = zeta::makePromiseContract<int>();
    auto output = AddOne(std::move(input)).GetFuture();

    if (!promise.SetValue(41).ok()) return 1;

    auto result = std::move(output).Get();
    if (!result.ok()) {
        std::cerr << result.status().ToString() << '\n';
        return 1;
    }

    std::cout << "coroutine result=" << result.value() << '\n';
    return result.value() == 42 ? 0 : 1;
}
