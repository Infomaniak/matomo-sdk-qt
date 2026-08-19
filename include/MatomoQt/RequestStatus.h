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

#include <QtCore/QString>
#include <QtCore/qtmetamacros.h>

namespace MatomoQt::RequestStatus {

Q_NAMESPACE_EXPORT(MATOMOQT_CORE_EXPORT)

/** Public-facing status of a tracking request. */
enum class Value {
    Accepted,
    RequestDisabled,
    RequestBlockedByPrivacy,
    RequestInvalidConfig,
    RequestInvalidPayload,
};
Q_ENUM_NS(Value)

[[nodiscard]] inline QString enumToString(Value status) {
    switch (status) {
        case Value::Accepted:
            return QStringLiteral("Accepted");
        case Value::RequestDisabled:
            return QStringLiteral("RequestDisabled");
        case Value::RequestBlockedByPrivacy:
            return QStringLiteral("RequestBlockedByPrivacy");
        case Value::RequestInvalidConfig:
            return QStringLiteral("RequestInvalidConfig");
        case Value::RequestInvalidPayload:
            return QStringLiteral("RequestInvalidPayload");
    }
    return QStringLiteral("RequestInvalidConfig");
}

} // namespace MatomoQt::RequestStatus
