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

/** Runtime statistics for a Tracker instance.
 *
 * Counters are updated synchronously by the Tracker and are safe to read
 * from the same thread that owns the Tracker.
 */
struct MATOMOQT_CORE_EXPORT TrackerStats {
    int32_t sentCount = 0;
    int32_t blockedCount = 0;
    int32_t failedCount = 0;

    [[nodiscard]] bool operator==(const TrackerStats &other) const = default;
};

} // namespace MatomoQt
