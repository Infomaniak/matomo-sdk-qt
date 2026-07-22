#pragma once

#include <MatomoQt/ConsentState.h>
#include <MatomoQt/PrivacyMode.h>

namespace MatomoQt {

/**
 * Stateless decision utility for whether a tracking request may proceed.
 *
 * Every call to track must evaluate PrivacyController::isTrackingAllowed()
 * before any request may be built or dispatched.
 *
 * Rules:
 * - Disabled                 -> always blocked
 * - RequiresConsent          -> allowed only when Granted
 * - ConsentExemptWithOptOut  -> allowed unless Denied or Withdrawn
 */
struct PrivacyController {
    /** Returns true only if the given mode and state allow tracking. */
    [[nodiscard]] static bool isTrackingAllowed(PrivacyMode mode, ConsentState state);
};

} // namespace MatomoQt
