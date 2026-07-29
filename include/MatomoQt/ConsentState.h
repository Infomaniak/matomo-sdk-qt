#pragma once

#include <QtCore/QString>

namespace MatomoQt {

/** User consent state known by the tracker. */
enum class ConsentState {
    Unknown,
    Granted,
    Denied,
    Withdrawn,
};

[[nodiscard]] inline QString enumToString(ConsentState state) {
    switch (state) {
        case ConsentState::Unknown:
            return QStringLiteral("Unknown");
        case ConsentState::Granted:
            return QStringLiteral("Granted");
        case ConsentState::Denied:
            return QStringLiteral("Denied");
        case ConsentState::Withdrawn:
            return QStringLiteral("Withdrawn");
    }
    return QStringLiteral("Unknown");
}

} // namespace MatomoQt
