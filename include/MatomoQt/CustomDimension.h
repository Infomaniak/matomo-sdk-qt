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

#include <QtGlobal>
#include <QtCore/QString>

namespace MatomoQt {

/** Matomo custom dimension value for a configured dimension slot. */
struct MATOMOQT_CORE_EXPORT CustomDimension {
        using Id = qint32;

        static constexpr Id MinId = 1;
        static constexpr Id MaxId = 999;

        Id id = 0;
        QString value;

        /** Returns true when the dimension ID is within Matomo's valid slot range (1-999). */
        [[nodiscard]] bool isValid() const;
};

} // namespace MatomoQt
