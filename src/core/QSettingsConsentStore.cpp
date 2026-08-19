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

#include <MatomoQt/QSettingsConsentStore.h>

#include <QtCore/QSettings>

namespace MatomoQt {

namespace {
    const auto kConsentStateKey = QStringLiteral("MatomoQt/consentState");
}

QSettingsConsentStore::QSettingsConsentStore(QSettings *settings) :
    m_settings(settings) {}

ConsentState::Value QSettingsConsentStore::consentState() const {
    if (!m_settings) {
        return ConsentState::Value::Unknown;
    }

    if (const bool exists = m_settings->contains(kConsentStateKey); !exists) {
        return ConsentState::Value::Unknown;
    }

    bool ok = false;
    const int value = m_settings->value(kConsentStateKey).toInt(&ok);
    if (!ok || value < 0 || value > 3) {
        return ConsentState::Value::Unknown;
    }

    return static_cast<ConsentState::Value>(value);
}

void QSettingsConsentStore::setConsentState(const ConsentState::Value state) {
    if (!m_settings) {
        return;
    }
    m_settings->setValue(kConsentStateKey, static_cast<int32_t>(state));
}

} // namespace MatomoQt
