#pragma once

#include <QtCore/QString>

namespace MatomoQt {

/** Privacy gate applied before any tracking request may be built. */
enum class PrivacyMode {
    Disabled,
    RequiresConsent,
    ConsentExemptWithOptOut,
};

[[nodiscard]] inline QString enumToString(PrivacyMode mode) {
    switch (mode) {
        case PrivacyMode::Disabled:
            return QStringLiteral("Disabled");
        case PrivacyMode::RequiresConsent:
            return QStringLiteral("RequiresConsent");
        case PrivacyMode::ConsentExemptWithOptOut:
            return QStringLiteral("ConsentExemptWithOptOut");
    }
    return QStringLiteral("Disabled");
}

} // namespace MatomoQt
