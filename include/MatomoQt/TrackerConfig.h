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

        /** Absolute URL that page-view and event paths are resolved against to build Matomo's `url`.
         *
         *  A base made of a scheme only, such as `app://`, is used as a prefix: the path
         *  `homePage` gives `app://homePage`, with its case preserved. Any other base follows
         *  standard URL resolution, e.g. `app://desktop/` and `homePage` give `app://desktop/homePage`.
         */
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

        /** Screen resolution sent as Matomo's `res` parameter, e.g. `1920x1080`.
         *
         *  Leave empty to omit the res parameter.  The core library only depends on
         *  QtCore, so the host application reads the value from its own screen API.
         */
        QString screenResolution;

        /** User-Agent Client Hints sent as Matomo's `uadata` parameter, as a JSON object.
         *
         *  Matomo reads the version of a Linux distribution from them only, e.g. to
         *  report "Ubuntu 26.04" instead of "Ubuntu". Build them with UserAgentBuilder::buildClientHints()
         *  or supply your own. Leave empty to omit the uadata parameter.
         *
         *  A non-empty value that is not a JSON object makes every tracking call fail
         *  with RequestInvalidPayload: Matomo would silently ignore it, so the
         *  configuration error would otherwise go unnoticed.
         */
        QString clientHints;

        /** Network dispatcher configuration (timeout, circuit breaker). */
        NetworkDispatcherConfig networkDispatcherConfig;

        /** Returns true when the endpoint, action URL base and site ID are usable. */
        [[nodiscard]] bool isValid() const;

        [[nodiscard]] bool operator==(const TrackerConfig &other) const = default;
};

} // namespace MatomoQt
