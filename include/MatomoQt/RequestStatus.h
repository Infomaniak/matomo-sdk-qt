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

/** Public-facing status of a tracking request, suitable for QML and C++ consumers. */
enum class RequestStatus {
    Accepted,
    RequestDisabled,
    RequestBlockedByPrivacy,
    RequestInvalidConfig,
    RequestInvalidPayload,
};

[[nodiscard]] inline QString enumToString(const RequestStatus status) {
    switch (status) {
        case RequestStatus::Accepted:
            return QStringLiteral("Accepted");
        case RequestStatus::RequestDisabled:
            return QStringLiteral("RequestDisabled");
        case RequestStatus::RequestBlockedByPrivacy:
            return QStringLiteral("RequestBlockedByPrivacy");
        case RequestStatus::RequestInvalidConfig:
            return QStringLiteral("RequestInvalidConfig");
        case RequestStatus::RequestInvalidPayload:
            return QStringLiteral("RequestInvalidPayload");
    }
    return QStringLiteral("RequestInvalidConfig");
}

} // namespace MatomoQt
