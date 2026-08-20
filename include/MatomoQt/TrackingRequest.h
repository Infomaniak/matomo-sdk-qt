#pragma once

#include <MatomoQt/Export.h>

#include <QtCore/QUrl>

namespace MatomoQt {

/** Tracking request built for the Matomo HTTP Tracking API. */
struct MATOMOQT_CORE_EXPORT TrackingRequest {
        QUrl url;
};

} // namespace MatomoQt
