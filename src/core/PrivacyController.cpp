#include <MatomoQt/PrivacyController.h>

namespace MatomoQt {

bool PrivacyController::isTrackingAllowed(const PrivacyMode mode, const ConsentState state) {
    switch (mode) {
        using enum PrivacyMode;
        case Disabled:
            return false;
        case RequiresConsent:
            return state == ConsentState::Granted;
        case ConsentExemptWithOptOut:
            return state != ConsentState::Denied && state != ConsentState::Withdrawn;
    }
    return false;
}

} // namespace MatomoQt
