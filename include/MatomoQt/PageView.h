#pragma once

#include <MatomoQt/CustomDimension.h>
#include <MatomoQt/Export.h>

#include <QtCore/QList>
#include <QtCore/QString>

namespace MatomoQt {

/** Page view tracking payload. */
struct MATOMOQT_CORE_EXPORT PageView {
        QString path;
        QString actionName;
        QList<CustomDimension> customDimensions;

        /** Returns true when the page path is present. */
        [[nodiscard]] bool isValid() const;
};

} // namespace MatomoQt
