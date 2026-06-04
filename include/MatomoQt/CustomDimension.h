#pragma once

#include <MatomoQt/Export.h>

#include <QtCore/QString>
#include <QtCore/QtTypes>

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
