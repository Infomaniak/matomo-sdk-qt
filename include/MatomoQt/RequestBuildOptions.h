#pragma once

#include <MatomoQt/Export.h>

#include <QtCore/QString>

namespace MatomoQt {

/** Optional request-building inputs supplied by the tracker orchestration layer. */
struct MATOMOQT_CORE_EXPORT RequestBuildOptions {
        QString clientId;
};

} // namespace MatomoQt
