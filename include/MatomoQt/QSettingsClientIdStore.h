#pragma once

#include <MatomoQt/ClientIdStore.h>
#include <MatomoQt/Export.h>

#include <QtCore/QString>

class QSettings;

namespace MatomoQt {

/**
 * QSettings-based persistent client ID store.
 *
 * The provided QSettings pointer is not owned by this instance.
 * The caller must ensure the QSettings outlives the store.
 */
class MATOMOQT_CORE_EXPORT QSettingsClientIdStore : public ClientIdStore {
    public:
        explicit QSettingsClientIdStore(QSettings *settings);
        ~QSettingsClientIdStore() override = default;

        [[nodiscard]] QString clientId() const override;
        void setClientId(const QString &clientId) override;
        void clearClientId() override;

    private:
        QSettings *m_settings = nullptr;
};

} // namespace MatomoQt
