#include "zeta/metrics/metrics.h"

#include <chrono>
#include <iostream>

int main() {
    zeta::metrics::Counter requests;
    zeta::metrics::Gauge in_flight;
    zeta::metrics::Histogram latency({
        1'000'000,
        10'000'000,
        100'000'000,
    });

    in_flight.Add(1);
    requests.Increment();
    {
        zeta::metrics::ScopedTimer timer(latency);
        in_flight.Add(-1);
    }

    const auto snapshot = latency.Snapshot();
    std::cout << "requests=" << requests.value()
              << " in_flight=" << in_flight.value()
              << " samples=" << snapshot.count << "\n";
}
