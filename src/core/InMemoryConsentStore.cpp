#include <MatomoQt/InMemoryConsentStore.h>

namespace MatomoQt {

ConsentState InMemoryConsentStore::consentState() const {
    return m_state;
}

void InMemoryConsentStore::setConsentState(ConsentState state) {
    m_state = state;
}

} // namespace MatomoQt
