/*
 * Infomaniak - matomo-sdk-qt
 * Copyright (C) 2026 Infomaniak Network SA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
