#pragma once

namespace MatomoQt {

/** Privacy gate applied before any tracking request may be built. */
enum class PrivacyMode {
    Disabled,
    RequiresConsent,
    ConsentExemptWithOptOut,
};

} // namespace MatomoQt
