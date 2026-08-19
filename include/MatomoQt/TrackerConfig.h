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
#include <MatomoQt/PrivacyMode.h>

#include <QtGlobal>
#include <QtCore/QUrl>

namespace MatomoQt {

/** Configuration shared by all tracking calls of a tracker instance. */
struct MATOMOQT_CORE_EXPORT TrackerConfig {
        using SiteId = qint32;

        QUrl endpoint;
        QUrl actionUrlBase;
        SiteId siteId = 0;
        PrivacyMode::Value privacyMode = PrivacyMode::Value::RequiresConsent;

        /** Returns true when the endpoint and site ID can identify a Matomo site. */
        [[nodiscard]] bool isValid() const;

        [[nodiscard]] bool operator==(const TrackerConfig &other) const = default;
};

} // namespace MatomoQt
