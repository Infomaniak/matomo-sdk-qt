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

#include "QmlMatomoTracker.h"

#include <MatomoQt/Event.h>
#include <MatomoQt/PageView.h>

#include <QtCore/QtGlobal>

#include <optional>

namespace MatomoQt::Qml {

MatomoTracker *MatomoTracker::create(QQmlEngine *engine, QJSEngine *) {
    auto *tracker = new MatomoTracker(engine);
    QQmlEngine::setObjectOwnership(tracker, QQmlEngine::CppOwnership);
    return tracker;
}

MatomoTracker::MatomoTracker(QObject *parent) :
    QObject(parent) {

    recreateTracker();
}

QUrl MatomoTracker::endpoint() const {
    return m_config.endpoint;
}

void MatomoTracker::setEndpoint(const QUrl &endpoint) {
    if (m_config.endpoint == endpoint) {
        return;
    }
    m_config.endpoint = endpoint;
    recreateTracker();
    emit endpointChanged();
}

QUrl MatomoTracker::actionUrlBase() const {
    return m_config.actionUrlBase;
}

void MatomoTracker::setActionUrlBase(const QUrl &actionUrlBase) {
    if (m_config.actionUrlBase == actionUrlBase) {
        return;
    }
    m_config.actionUrlBase = actionUrlBase;
    recreateTracker();
    emit actionUrlBaseChanged();
}

int MatomoTracker::siteId() const {
    return m_config.siteId;
}

void MatomoTracker::setSiteId(const int siteId) {
    if (m_config.siteId == siteId) {
        return;
    }
    m_config.siteId = siteId;
    recreateTracker();
    emit siteIdChanged();
}

PrivacyMode::Value MatomoTracker::privacyMode() const {
    return m_config.privacyMode;
}

void MatomoTracker::setPrivacyMode(const PrivacyMode::Value mode) {
    if (m_config.privacyMode == mode) {
        return;
    }
    m_config.privacyMode = mode;
    recreateTracker();
    emit privacyModeChanged();
}

bool MatomoTracker::isEnabled() const {
    return m_enabled;
}

void MatomoTracker::setEnabled(const bool enabled) {
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    if (m_tracker) {
        m_tracker->setEnabled(m_enabled);
    }
}

ConsentState::Value MatomoTracker::consentState() const {
    return m_consentStore.consentState();
}

void MatomoTracker::setConsentState(const ConsentState::Value state) {
    if (m_consentStore.consentState() == state) {
        return;
    }
    if (m_tracker) {
        m_tracker->setConsentState(state);
    } else {
        m_consentStore.setConsentState(state);
    }
    emit consentStateChanged();
}

RequestStatus::Value MatomoTracker::lastRequestStatus() const {
    return m_lastRequestResult.status;
}

QString MatomoTracker::lastRequestMessage() const {
    return m_lastRequestResult.message;
}

bool MatomoTracker::hasDispatchResult() const {
    return m_hasDispatchResult;
}

DispatchStatus::Value MatomoTracker::lastDispatchStatus() const {
    return m_lastDispatchResult.status;
}

int MatomoTracker::lastDispatchHttpStatus() const {
    return m_lastDispatchResult.httpStatus;
}

QString MatomoTracker::lastDispatchMessage() const {
    return m_lastDispatchResult.message;
}

bool MatomoTracker::trackPageView(const QString &path, const QString &actionName) {
    if (!m_tracker) {
        applyRequestResult({.status = RequestStatus::Value::RequestInvalidConfig,
                            .message = QStringLiteral("Tracker endpoint and site ID are required.")});
        return false;
    }
    const auto result = m_tracker->trackPageView({
        .path = path,
        .actionName = actionName,
    });
    applyRequestResult(result);
    return result.accepted();
}

bool MatomoTracker::trackEvent(const QString &category,
                               const QString &action,
                               const QString &name,
                               const QVariant &value) {
    std::optional<double> numericValue;
    if (value.isValid() && !value.isNull()) {
        bool ok = false;
        const auto parsed = value.toDouble(&ok);
        if (!ok) {
            applyRequestResult({
                .status = RequestStatus::Value::RequestInvalidPayload,
                .message = QStringLiteral("trackEvent value must be numeric or omitted."),
            });
            return false;
        }
        numericValue = parsed;
    }

    if (!m_tracker) {
        applyRequestResult({.status = RequestStatus::Value::RequestInvalidConfig,
                            .message = QStringLiteral("Tracker endpoint and site ID are required.")});
        return false;
    }
    const auto result = m_tracker->trackEvent({
        .category = category,
        .action = action,
        .name = name,
        .value = numericValue,
    });
    applyRequestResult(result);
    return result.accepted();
}

bool MatomoTracker::sendPing() {
    if (!m_tracker) {
        applyRequestResult({.status = RequestStatus::Value::RequestInvalidConfig,
                            .message = QStringLiteral("Tracker endpoint and site ID are required.")});
        return false;
    }
    const auto result = m_tracker->sendPing();
    applyRequestResult(result);
    return result.accepted();
}

void MatomoTracker::grantConsent() {
    setConsentState(ConsentState::Value::Granted);
}

void MatomoTracker::denyConsent() {
    setConsentState(ConsentState::Value::Denied);
}

void MatomoTracker::withdrawConsent() {
    setConsentState(ConsentState::Value::Withdrawn);
}

void MatomoTracker::resetClientId() {
    if (m_tracker) {
        m_tracker->resetClientId();
    } else {
        m_clientIdStore.clearClientId();
    }
}

void MatomoTracker::recreateTracker() {
    m_tracker = std::make_unique<Tracker>(m_config, &m_consentStore, &m_clientIdStore, this);
    m_tracker->setEnabled(m_enabled);

    connect(m_tracker.get(), &Tracker::enabledChanged, this, &MatomoTracker::enabledChanged);
    connect(m_tracker.get(), &Tracker::consentStateChanged, this, [this](ConsentState::Value) {
        emit consentStateChanged();
    });
    connect(m_tracker.get(), &Tracker::dispatchFinished, this, &MatomoTracker::applyDispatchResult);
}

void MatomoTracker::applyRequestResult(const RequestResult &result) {
    const bool statusChanged = m_lastRequestResult.status != result.status;
    const bool messageChanged = m_lastRequestResult.message != result.message;
    m_lastRequestResult = result;
    if (statusChanged) emit lastRequestStatusChanged();
    if (messageChanged) emit lastRequestMessageChanged();
}

void MatomoTracker::applyDispatchResult(const DispatchResult &result) {
    const bool firstResult = !m_hasDispatchResult;
    const bool statusChanged = firstResult || m_lastDispatchResult.status != result.status;
    const bool httpStatusChanged = firstResult || m_lastDispatchResult.httpStatus != result.httpStatus;
    const bool messageChanged = firstResult || m_lastDispatchResult.message != result.message;

    m_lastDispatchResult = result;
    m_hasDispatchResult = true;

    if (firstResult) emit hasDispatchResultChanged();
    if (statusChanged) emit lastDispatchStatusChanged();
    if (httpStatusChanged) emit lastDispatchHttpStatusChanged();
    if (messageChanged) emit lastDispatchMessageChanged();
    emit dispatchFinished(result.status, result.httpStatus, result.message);
}

} // namespace MatomoQt::Qml
