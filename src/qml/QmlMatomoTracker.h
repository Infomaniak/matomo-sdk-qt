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

#pragma once

#include <MatomoQt/ConsentState.h>
#include <MatomoQt/PrivacyMode.h>
#include <MatomoQt/RequestStatus.h>
#include <MatomoQt/Tracker.h>

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtQml/QQmlEngine>
#include <QtQmlIntegration/qqmlintegration.h>

namespace MatomoQt::Qml {

namespace PrivacyModeForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(MatomoQt::PrivacyMode)
QML_NAMED_ELEMENT(PrivacyMode)
} // namespace PrivacyModeForeign

namespace ConsentStateForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(MatomoQt::ConsentState)
QML_NAMED_ELEMENT(ConsentState)
} // namespace ConsentStateForeign

namespace RequestStatusForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(MatomoQt::RequestStatus)
QML_NAMED_ELEMENT(RequestStatus)
} // namespace RequestStatusForeign

class MatomoTracker : public QObject {
        Q_OBJECT
        QML_NAMED_ELEMENT(MatomoTracker)
        QML_SINGLETON
        Q_PROPERTY(QUrl endpoint READ endpoint WRITE setEndpoint NOTIFY endpointChanged)
        Q_PROPERTY(QUrl actionUrlBase READ actionUrlBase WRITE setActionUrlBase NOTIFY actionUrlBaseChanged)
        Q_PROPERTY(int siteId READ siteId WRITE setSiteId NOTIFY siteIdChanged)
        Q_PROPERTY(PrivacyMode::Value privacyMode READ privacyMode WRITE setPrivacyMode NOTIFY privacyModeChanged)
        Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
        Q_PROPERTY(ConsentState::Value consentState READ consentState WRITE setConsentState NOTIFY consentStateChanged)
        Q_PROPERTY(RequestStatus::Value lastRequestStatus READ lastRequestStatus NOTIFY lastRequestStatusChanged)
        Q_PROPERTY(QString lastRequestMessage READ lastRequestMessage NOTIFY lastRequestMessageChanged)

    public:
        explicit MatomoTracker(QObject *parent = nullptr);

        static MatomoTracker *create(QQmlEngine *, QJSEngine *);

        [[nodiscard]] QUrl endpoint() const;
        void setEndpoint(const QUrl &endpoint);

        [[nodiscard]] QUrl actionUrlBase() const;
        void setActionUrlBase(const QUrl &actionUrlBase);

        [[nodiscard]] int siteId() const;
        void setSiteId(int siteId);

        [[nodiscard]] PrivacyMode::Value privacyMode() const;
        void setPrivacyMode(PrivacyMode::Value mode);

        [[nodiscard]] bool isEnabled() const;
        void setEnabled(bool enabled);

        [[nodiscard]] ConsentState::Value consentState() const;
        void setConsentState(ConsentState::Value state);

        [[nodiscard]] RequestStatus::Value lastRequestStatus() const;
        [[nodiscard]] QString lastRequestMessage() const;

        Q_INVOKABLE bool trackPageView(const QString &path, const QString &actionName = {});
        Q_INVOKABLE bool trackEvent(const QString &category,
                                    const QString &action,
                                    const QString &name = {},
                                    const QVariant &value = {});
        Q_INVOKABLE bool sendPing();
        Q_INVOKABLE void grantConsent();
        Q_INVOKABLE void denyConsent();
        Q_INVOKABLE void withdrawConsent();
        Q_INVOKABLE void resetClientId();

    signals:
        void endpointChanged();
        void actionUrlBaseChanged();
        void siteIdChanged();
        void privacyModeChanged();
        void enabledChanged();
        void consentStateChanged();
        void lastRequestStatusChanged();
        void lastRequestMessageChanged();

    private:
        void onTrackerConfigChanged();
        void applyRequestResult(const RequestResult &result);

        Tracker m_tracker;
        TrackerConfig m_lastConfig;
        RequestResult m_lastRequestResult;
};

} // namespace MatomoQt::Qml
