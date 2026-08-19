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

#include <MatomoQt/Event.h>
#include <MatomoQt/PageView.h>
#include <MatomoQt/Tracker.h>
#include <MatomoQt/TrackerConfig.h>
#include <MatomoQt/Version.h>

#include <QtCore/QString>
#include <QtCore/QUrl>

int main() {
    MatomoQt::TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.actionUrlBase = QUrl(QStringLiteral("app://desktop/"));
    config.siteId = 1;
    config.privacyMode = MatomoQt::PrivacyMode::Value::ConsentExemptWithOptOut;

    MatomoQt::Tracker tracker(config);
    tracker.setConsentState(MatomoQt::ConsentState::Value::Granted);

    const auto pageViewResult = tracker.trackPageView({.path = QStringLiteral("preferences")});
    const auto eventResult = tracker.trackEvent({
            .category = QStringLiteral("preferences"),
            .action = QStringLiteral("click"),
    });

    return MatomoQt::versionString().isEmpty() || !pageViewResult.accepted() || !eventResult.accepted() ? 1 : 0;
}
