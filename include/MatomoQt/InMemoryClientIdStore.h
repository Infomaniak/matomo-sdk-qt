#pragma once

#include <MatomoQt/ClientIdStore.h>
#include <MatomoQt/Export.h>

#include <QtCore/QString>

namespace MatomoQt {

/**
 * Volatile in-memory client ID store.
 *
 * The ID is lost when the instance is destroyed.
 * Suitable for unit tests and for applications that manage their own persistence.
 */
class MATOMOQT_CORE_EXPORT InMemoryClientIdStore : public ClientIdStore {
    public:
        InMemoryClientIdStore() = default;

        [[nodiscard]] QString clientId() const override;
        void setClientId(const QString &clientId) override;
        void clearClientId() override;

    private:
        QString m_clientId;
};

} // namespace MatomoQt
