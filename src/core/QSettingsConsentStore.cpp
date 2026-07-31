#include <MatomoQt/QSettingsConsentStore.h>

#include <QtCore/QSettings>

namespace MatomoQt {

namespace {
    const auto kConsentStateKey = QStringLiteral("MatomoQt/consentState");
}

QSettingsConsentStore::QSettingsConsentStore(QSettings *settings) :
    m_settings(settings) {}

ConsentState QSettingsConsentStore::consentState() const {
    if (!m_settings) {
        return ConsentState::Unknown;
    }

    if (const bool exists = m_settings->contains(kConsentStateKey); !exists) {
        return ConsentState::Unknown;
    }

    bool ok = false;
    const int value = m_settings->value(kConsentStateKey).toInt(&ok);
    if (!ok || value < 0 || value > 3) {
        return ConsentState::Unknown;
    }

    return static_cast<ConsentState>(value);
}

void QSettingsConsentStore::setConsentState(const ConsentState state) {
    if (!m_settings) {
        return;
    }
    m_settings->setValue(kConsentStateKey, static_cast<int32_t>(state));
}

} // namespace MatomoQt
