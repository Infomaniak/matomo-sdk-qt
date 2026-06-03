#pragma once

#include <MatomoQt/Export.h>

#include <QtCore/QString>

namespace MatomoQt {

/**
 * Interface used by applications that want SDK-managed client ID persistence.
 */
class MATOMOQT_CORE_EXPORT ClientIdStore {
    public:
        ClientIdStore() = default;
        ClientIdStore(const ClientIdStore &) = delete;
        ClientIdStore &operator=(const ClientIdStore &) = delete;
        ClientIdStore(ClientIdStore &&) = delete;
        ClientIdStore &operator=(ClientIdStore &&) = delete;
        virtual ~ClientIdStore();

        /** Returns the stored client ID, or an empty string when none exists. */
        virtual QString clientId() const = 0;

        /** Persists the client ID chosen by the application or SDK. */
        virtual void setClientId(const QString &clientId) = 0;

        /** Removes the stored client ID. */
        virtual void clearClientId() = 0;
};

} // namespace MatomoQt
