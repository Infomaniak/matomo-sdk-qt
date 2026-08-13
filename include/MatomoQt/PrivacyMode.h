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

#include <QtCore/QString>

namespace MatomoQt {

/** Privacy gate applied before any tracking request may be built. */
enum class PrivacyMode {
    Disabled,
    RequiresConsent,
    ConsentExemptWithOptOut,
};

[[nodiscard]] inline QString enumToString(PrivacyMode mode) {
    switch (mode) {
        case PrivacyMode::Disabled:
            return QStringLiteral("Disabled");
        case PrivacyMode::RequiresConsent:
            return QStringLiteral("RequiresConsent");
        case PrivacyMode::ConsentExemptWithOptOut:
            return QStringLiteral("ConsentExemptWithOptOut");
    }
    return QStringLiteral("Disabled");
}

} // namespace MatomoQt
