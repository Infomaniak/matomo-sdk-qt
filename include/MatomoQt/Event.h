#pragma once

#include <MatomoQt/CustomDimension.h>
#include <MatomoQt/Export.h>

#include <QtCore/QList>
#include <QtCore/QString>

#include <optional>

namespace MatomoQt {

/** Event tracking payload. */
struct MATOMOQT_CORE_EXPORT Event {
        QString category;
        QString action;
        QString name;
        std::optional<double> value;
        QList<CustomDimension> customDimensions;

        /** Returns true when the required Matomo event fields are present. */
        [[nodiscard]] bool isValid() const;
};

} // namespace MatomoQt
