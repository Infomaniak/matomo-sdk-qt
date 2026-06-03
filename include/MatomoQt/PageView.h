#pragma once

#include <MatomoQt/CustomDimension.h>

#include <QtCore/QList>
#include <QtCore/QString>

namespace MatomoQt {

/** Page view tracking payload. */
struct PageView {
        QString path;
        QString actionName;
        QList<CustomDimension> customDimensions;

        /** Returns true when the page path is present. */
        [[nodiscard]] bool isValid() const;
};

} // namespace MatomoQt
