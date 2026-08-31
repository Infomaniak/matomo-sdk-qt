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

namespace MatomoQt {

/** Optional request-building inputs supplied by the tracker orchestration layer. */
struct MATOMOQT_CORE_EXPORT RequestBuildOptions {
        QString clientId;
        /** Full User-Agent sent as Matomo's `ua` Tracking HTTP API parameter.
         *
         * Matomo uses this value for bot/client/device/OS classification. It can
         * be supplied directly by the host application or generated with
         * UserAgentBuilder.
         *
         * References:
         * - https://developer.matomo.org/api-reference/tracking-api
         * - https://github.com/matomo-org/device-detector
         */
        QString userAgent;
        QString language;
        QString screenResolution;
};

} // namespace MatomoQt
