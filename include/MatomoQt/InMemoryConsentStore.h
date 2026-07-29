#pragma once

#include <MatomoQt/ConsentState.h>
#include <MatomoQt/ConsentStore.h>
#include <MatomoQt/Export.h>

namespace MatomoQt {

/**
 * Volatile in-memory consent store.
 *
 * State is lost when the instance is destroyed.
 * Suitable for unit tests and for applications that manage their own persistence.
 */
class MATOMOQT_CORE_EXPORT InMemoryConsentStore : public ConsentStore {
    public:
        InMemoryConsentStore() = default;

        [[nodiscard]] ConsentState consentState() const override;
        void setConsentState(ConsentState state) override;

    private:
        ConsentState m_state = ConsentState::Unknown;
};

} // namespace MatomoQt
