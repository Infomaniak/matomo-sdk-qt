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

#include <MatomoQt/ConsentState.h>
#include <MatomoQt/Export.h>
#include <MatomoQt/PrivacyMode.h>

namespace MatomoQt {

/**
 * Stateless decision utility for whether a tracking request may proceed.
 *
 * Every call to track must evaluate PrivacyController::isTrackingAllowed()
 * before any request may be built or dispatched.
 *
 * Rules:
 * - Disabled                 -> always blocked
 * - RequiresConsent          -> allowed only when Granted
 * - ConsentExemptWithOptOut  -> allowed unless Denied or Withdrawn
 */
struct MATOMOQT_CORE_EXPORT PrivacyController {
    /** Returns true only if the given mode and state allow tracking. */
    [[nodiscard]] static bool isTrackingAllowed(PrivacyMode mode, ConsentState state);
};

} // namespace MatomoQt
