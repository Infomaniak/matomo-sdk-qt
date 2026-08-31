#include <MatomoQt/QSettingsClientIdStore.h>

#include <QtCore/QSettings>

namespace MatomoQt {

namespace {
    const auto kClientIdKey = QStringLiteral("MatomoQt/clientId");
}

QSettingsClientIdStore::QSettingsClientIdStore(QSettings *settings) :
    m_settings(settings) {}

QString QSettingsClientIdStore::clientId() const {
    if (!m_settings) {
        return QString{};
    }

    return m_settings->value(kClientIdKey).toString();
}

void QSettingsClientIdStore::setClientId(const QString &clientId) {
    if (!m_settings) {
        return;
    }

    m_settings->setValue(kClientIdKey, clientId);
}

void QSettingsClientIdStore::clearClientId() {
    if (!m_settings) {
        return;
    }

    m_settings->remove(kClientIdKey);
}

} // namespace MatomoQt
