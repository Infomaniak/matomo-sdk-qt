#pragma once

#include <MatomoQt/Export.h>

#include <QtGlobal>
#include <QtCore/QString>

namespace MatomoQt {

/** Matomo custom dimension value for a configured dimension slot. */
struct MATOMOQT_CORE_EXPORT CustomDimension {
        using Id = qint32;

        Id id = 0;
        QString value;

        /** Returns true when the dimension has a positive Matomo slot ID. */
        [[nodiscard]] bool isValid() const;
};

} // namespace MatomoQt
