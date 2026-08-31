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

#include <MatomoQt/CustomDimension.h>
#include <MatomoQt/Export.h>

#include <QtCore/QList>
#include <QtCore/QString>

#include <optional>

namespace MatomoQt {

/** Event tracking payload. */
struct MATOMOQT_CORE_EXPORT Event {
        QString category;
        QString action;
        QString name;
        std::optional<double> value;
        QList<CustomDimension> customDimensions;

        /** Returns true when the required Matomo event fields are present. */
        [[nodiscard]] bool isValid() const;
};

} // namespace MatomoQt
