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

/** User consent state known by the tracker. */
enum class ConsentState {
    Unknown,
    Granted,
    Denied,
    Withdrawn,
};

[[nodiscard]] inline QString enumToString(ConsentState state) {
    switch (state) {
        case ConsentState::Unknown:
            return QStringLiteral("Unknown");
        case ConsentState::Granted:
            return QStringLiteral("Granted");
        case ConsentState::Denied:
            return QStringLiteral("Denied");
        case ConsentState::Withdrawn:
            return QStringLiteral("Withdrawn");
    }
    return QStringLiteral("Unknown");
}

} // namespace MatomoQt
