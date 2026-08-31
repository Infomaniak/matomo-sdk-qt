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
#include <MatomoQt/NetworkDispatcherConfig.h>
#include <MatomoQt/PrivacyMode.h>

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtGlobal>

namespace MatomoQt {

/** Configuration shared by all tracking calls of a tracker instance. */
struct MATOMOQT_CORE_EXPORT TrackerConfig {
        using SiteId = qint32;

        QUrl endpoint;
        QUrl actionUrlBase;
        SiteId siteId = 0;
        PrivacyMode::Value privacyMode = PrivacyMode::Value::RequiresConsent;

        /** Tracker-level custom dimensions merged into page-view and event
         *  tracking requests (per-call dimensions take precedence on duplicate IDs).
         *
         *  Tracker-level dimensions are not sent with sendPing()
         */
        QMap<int, QString> customDimensions;

        /** Full User-Agent sent as Matomo's `ua` Tracking HTTP API parameter.
         *
         *  Leave empty to omit the ua parameter.  The host application can
         *  build this with UserAgentBuilder or supply its own.
         */
        QString userAgent;

        /** Network dispatcher configuration (timeout, circuit breaker). */
        NetworkDispatcherConfig networkDispatcherConfig;

        /** Returns true when the endpoint, action URL base and site ID are usable. */
        [[nodiscard]] bool isValid() const;

        [[nodiscard]] bool operator==(const TrackerConfig &other) const = default;
};

} // namespace MatomoQt
