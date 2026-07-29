#include <MatomoQt/QSettingsClientIdStore.h>

#include <QtCore/QSettings>

namespace MatomoQt {

QSettingsClientIdStore::QSettingsClientIdStore(QSettings *settings) :
    m_settings(settings) {}

QString QSettingsClientIdStore::clientId() const {
    if (!m_settings) {
        return QString{};
    }

    return m_settings->value(QStringLiteral("clientId")).toString();
}

void QSettingsClientIdStore::setClientId(const QString &clientId) {
    if (!m_settings) {
        return;
    }

    m_settings->setValue(QStringLiteral("clientId"), clientId);
}

void QSettingsClientIdStore::clearClientId() {
    if (!m_settings) {
        return;
    }

    m_settings->remove(QStringLiteral("clientId"));
}

} // namespace MatomoQt
