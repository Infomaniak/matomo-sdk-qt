#pragma once

#include <MatomoQt/CustomDimension.h>

#include <QtCore/QList>
#include <QtCore/QString>

#include <optional>

namespace MatomoQt {

/** Event tracking payload. */
struct Event {
        QString category;
        QString action;
        QString name;
        std::optional<double> value;
        QList<CustomDimension> customDimensions;

        /** Returns true when the required Matomo event fields are present. */
        [[nodiscard]] bool isValid() const;
};

} // namespace MatomoQt
