#pragma once

#include <MatomoQt/Export.h>
#include <MatomoQt/PrivacyMode.h>

#include <QtCore/QUrl>

namespace MatomoQt {

/** Configuration shared by all tracking calls of a tracker instance. */
struct MATOMOQT_CORE_EXPORT TrackerConfig {
        QUrl endpoint;
        int siteId = 0;
        PrivacyMode privacyMode = PrivacyMode::RequiresConsent;

        /** Returns true when the endpoint and site ID can identify a Matomo site. */
        [[nodiscard]] bool isValid() const;
};

} // namespace MatomoQt
