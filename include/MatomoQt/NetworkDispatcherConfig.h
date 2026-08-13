/*
 * Infomaniak - matomo-sdk-qt
 * Copyright (C) 2026 Infomaniak Network SA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
