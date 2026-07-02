#include <zeta/time/retry.h>

#include <iostream>

int main() {
    zeta::RetryPolicy policy(
        zeta::ExponentialBackoff(zeta::Duration::Milliseconds(5),
                                 zeta::Duration::Milliseconds(20)),
        3,
        zeta::Deadline::After(zeta::Duration::Milliseconds(25)));

    while (policy.CanRetry()) {
        std::cout << "attempt=" << policy.attempts()
                  << " next_delay_ms=" << policy.NextDelay().ToMilliseconds()
                  << '\n';
        policy.Advance();
    }

    return 0;
}
