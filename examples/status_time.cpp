#include <zeta/status/result.h>
#include <zeta/status/status.h>
#include <zeta/strings/numbers.h>
#include <zeta/time/stopwatch.h>
#include <zeta/time/timestamp.h>

#include <iostream>
#include <string_view>

namespace {

zeta::Result<int> ParsePort(std::string_view text) {
    int port = 0;
    if (!zeta::SimpleAtoi(text, &port)) {
        return zeta::InvalidArgumentError("port must be an integer");
    }
    if (port <= 0 || port > 65535) {
        return zeta::OutOfRangeError("port must be in 1..65535");
    }
    return port;
}

} // namespace

int main() {
    zeta::Stopwatch sw;
    auto ok = ParsePort("8080");
    if (!ok.ok()) {
        std::cerr << ok.status().ToString() << '\n';
        return 1;
    }

    auto bad = ParsePort("abc");
    auto deadline = zeta::Deadline::After(zeta::Duration::Milliseconds(5));
    std::cout << "now(local): " << zeta::FormatNow() << '\n';
    std::cout << "now(utc):   " << zeta::FormatNowUtc() << '\n';
    std::cout << "parsed port: " << *ok << '\n';
    std::cout << "bad parse:   " << bad.status().ToString() << '\n';
    std::cout << "remaining(ms): "
              << deadline.Remaining().ToMilliseconds() << '\n';
    std::cout << "setup(us):   "
              << sw.Elapsed().ToMicroseconds() << '\n';
    return 0;
}
