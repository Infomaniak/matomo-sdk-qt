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
