#pragma once

#include <MatomoQt/ConsentState.h>
#include <MatomoQt/Export.h>

namespace MatomoQt {

/**
 * Interface used by applications that want SDK-managed consent persistence.
 */
class MATOMOQT_CORE_EXPORT ConsentStore {
    public:
        virtual ~ConsentStore();

        /** Returns the stored consent state, or Unknown when none exists. */
        virtual ConsentState consentState() const = 0;

        /** Persists the consent state chosen by the application. */
        virtual void setConsentState(ConsentState state) = 0;
};

} // namespace MatomoQt
