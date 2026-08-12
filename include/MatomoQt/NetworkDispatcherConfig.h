#pragma once

#include <MatomoQt/Export.h>

namespace MatomoQt {

/** Configuration for the network dispatcher. */
struct MATOMOQT_CORE_EXPORT NetworkDispatcherConfig {
        /** Per-request timeout in milliseconds. 0 disables the timeout. */
        int timeoutMs = 10000;

        /** Number of consecutive failures before the circuit breaker opens. 0 disables the circuit breaker. */
        int maxConsecutiveFailures = 5;

        [[nodiscard]] bool operator==(const NetworkDispatcherConfig &other) const = default;
};

} // namespace MatomoQt
