#include <zeta/time/backoff.h>
#include <zeta/time/periodic_timer.h>
#include <zeta/time/stopwatch.h>

#include <iostream>

int main() {
    zeta::Stopwatch sw;
    zeta::PeriodicTimer timer = zeta::PeriodicTimer::Every(zeta::Duration::Milliseconds(5));
    zeta::ExponentialBackoff backoff(
        zeta::Duration::Milliseconds(5),
        zeta::Duration::Milliseconds(20));

    int ticks = 0;
    while (ticks < 3) {
        timer.WaitNext();
        std::cout << "tick=" << ticks
                  << " elapsed_ms=" << sw.Elapsed().ToMilliseconds()
                  << " retry_delay_ms=" << backoff.NextDelay().ToMilliseconds()
                  << '\n';
        ++ticks;
    }

    return 0;
}
