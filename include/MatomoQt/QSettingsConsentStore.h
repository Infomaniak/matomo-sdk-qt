#pragma once

#include <MatomoQt/ConsentStore.h>
#include <MatomoQt/Export.h>

class QSettings;

namespace MatomoQt {

/**
 * QSettings-based persistent consent store.
 *
 * The provided QSettings pointer is not owned by this instance.
 * The caller must ensure the QSettings outlives the store.
 */
class MATOMOQT_CORE_EXPORT QSettingsConsentStore : public ConsentStore {
    public:
        explicit QSettingsConsentStore(QSettings *settings);
        ~QSettingsConsentStore() override = default;

        [[nodiscard]] ConsentState consentState() const override;
        void setConsentState(ConsentState state) override;

    private:
        QSettings *m_settings = nullptr;
};

} // namespace MatomoQt
