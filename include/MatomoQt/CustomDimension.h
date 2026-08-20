#pragma once

#include <MatomoQt/Export.h>

#include <QtGlobal>
#include <QtCore/QString>

namespace MatomoQt {

/** Matomo custom dimension value for a configured dimension slot. */
struct MATOMOQT_CORE_EXPORT CustomDimension {
        using Id = qint32;

        static constexpr Id MinId = 1;
        static constexpr Id MaxId = 999;

        Id id = 0;
        QString value;

        /** Returns true when the dimension ID is within Matomo's valid slot range (1-999). */
        [[nodiscard]] bool isValid() const;
};

} // namespace MatomoQt
