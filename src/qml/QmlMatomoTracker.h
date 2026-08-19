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
#include <MatomoQt/QtMeta.h>
#include <MatomoQt/RequestStatus.h>
#include <MatomoQt/Tracker.h>

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtQmlIntegration/qqmlintegration.h>

namespace MatomoQt::Qml {

namespace ForeignNamespace {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(MatomoQt)
QML_NAMED_ELEMENT(Matomo)
} // namespace ForeignNamespace

class MatomoTracker : public QObject {
        Q_OBJECT
        QML_NAMED_ELEMENT(MatomoTracker)
        Q_PROPERTY(QUrl endpoint READ endpoint WRITE setEndpoint NOTIFY endpointChanged)
        Q_PROPERTY(QUrl actionUrlBase READ actionUrlBase WRITE setActionUrlBase NOTIFY actionUrlBaseChanged)
        Q_PROPERTY(int siteId READ siteId WRITE setSiteId NOTIFY siteIdChanged)
        Q_PROPERTY(MatomoQt::PrivacyMode privacyMode READ privacyMode WRITE setPrivacyMode NOTIFY privacyModeChanged)
        Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
        Q_PROPERTY(MatomoQt::ConsentState consentState READ consentState WRITE setConsentState NOTIFY consentStateChanged)
        Q_PROPERTY(MatomoQt::RequestStatus lastRequestStatus READ lastRequestStatus NOTIFY lastRequestStatusChanged)
        Q_PROPERTY(QString lastRequestMessage READ lastRequestMessage NOTIFY lastRequestMessageChanged)

    public:
        explicit MatomoTracker(QObject *parent = nullptr);

        [[nodiscard]] QUrl endpoint() const;
        void setEndpoint(const QUrl &endpoint);

        [[nodiscard]] QUrl actionUrlBase() const;
        void setActionUrlBase(const QUrl &actionUrlBase);

        [[nodiscard]] int siteId() const;
        void setSiteId(int siteId);

        [[nodiscard]] PrivacyMode privacyMode() const;
        void setPrivacyMode(PrivacyMode mode);

        [[nodiscard]] bool isEnabled() const;
        void setEnabled(bool enabled);

        [[nodiscard]] ConsentState consentState() const;
        void setConsentState(ConsentState state);

        [[nodiscard]] RequestStatus lastRequestStatus() const;
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
        [[nodiscard]] static RequestStatus toRequestStatus(RequestResult::Status status);

        void onTrackerConfigChanged();
        void applyRequestResult(const RequestResult &result);

        Tracker m_tracker;
        RequestStatus m_lastRequestStatus = RequestStatus::RequestInvalidConfig;
        QString m_lastRequestMessage;
};

} // namespace MatomoQt::Qml
