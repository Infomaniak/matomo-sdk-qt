#include <MatomoQt/InMemoryClientIdStore.h>

namespace MatomoQt {

QString InMemoryClientIdStore::clientId() const {
    return m_clientId;
}

void InMemoryClientIdStore::setClientId(const QString &clientId) {
    m_clientId = clientId;
}

void InMemoryClientIdStore::clearClientId() {
    m_clientId.clear();
}

} // namespace MatomoQt
