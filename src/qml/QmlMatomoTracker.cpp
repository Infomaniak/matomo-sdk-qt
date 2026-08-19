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
    QObject(parent),
    m_tracker(this),
    m_lastConfig(m_tracker.config()) {

    connect(&m_tracker, &Tracker::configChanged, this, &MatomoTracker::onTrackerConfigChanged);
    connect(&m_tracker, &Tracker::enabledChanged, this, &MatomoTracker::enabledChanged);
    connect(&m_tracker, &Tracker::consentStateChanged, this, [this](ConsentState::Value) {
        emit consentStateChanged();
    });
}

QUrl MatomoTracker::endpoint() const {
    return m_tracker.config().endpoint;
}

void MatomoTracker::setEndpoint(const QUrl &endpoint) {
    auto config = m_tracker.config();
    if (config.endpoint == endpoint) {
        return;
    }
    config.endpoint = endpoint;
    m_tracker.setConfig(config);
}

QUrl MatomoTracker::actionUrlBase() const {
    return m_tracker.config().actionUrlBase;
}

void MatomoTracker::setActionUrlBase(const QUrl &actionUrlBase) {
    auto config = m_tracker.config();
    if (config.actionUrlBase == actionUrlBase) {
        return;
    }
    config.actionUrlBase = actionUrlBase;
    m_tracker.setConfig(config);
}

int MatomoTracker::siteId() const {
    return m_tracker.config().siteId;
}

void MatomoTracker::setSiteId(const int siteId) {
    auto config = m_tracker.config();
    if (config.siteId == siteId) {
        return;
    }
    config.siteId = siteId;
    m_tracker.setConfig(config);
}

PrivacyMode::Value MatomoTracker::privacyMode() const {
    return m_tracker.config().privacyMode;
}

void MatomoTracker::setPrivacyMode(const PrivacyMode::Value mode) {
    auto config = m_tracker.config();
    if (config.privacyMode == mode) {
        return;
    }
    config.privacyMode = mode;
    m_tracker.setConfig(config);
}

bool MatomoTracker::isEnabled() const {
    return m_tracker.isEnabled();
}

void MatomoTracker::setEnabled(const bool enabled) {
    m_tracker.setEnabled(enabled);
}

ConsentState::Value MatomoTracker::consentState() const {
    return m_tracker.consentState();
}

void MatomoTracker::setConsentState(const ConsentState::Value state) {
    m_tracker.setConsentState(state);
}

RequestStatus::Value MatomoTracker::lastRequestStatus() const {
    return m_lastRequestResult.status;
}

QString MatomoTracker::lastRequestMessage() const {
    return m_lastRequestResult.message;
}

bool MatomoTracker::trackPageView(const QString &path, const QString &actionName) {
    const auto result = m_tracker.trackPageView({
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

    const auto result = m_tracker.trackEvent({
        .category = category,
        .action = action,
        .name = name,
        .value = numericValue,
    });
    applyRequestResult(result);
    return result.accepted();
}

bool MatomoTracker::sendPing() {
    const auto result = m_tracker.sendPing();
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
    m_tracker.resetClientId();
}

void MatomoTracker::onTrackerConfigChanged() {
    const auto config = m_tracker.config();
    if (m_lastConfig.endpoint != config.endpoint) emit endpointChanged();
    if (m_lastConfig.actionUrlBase != config.actionUrlBase) emit actionUrlBaseChanged();
    if (m_lastConfig.siteId != config.siteId) emit siteIdChanged();
    if (m_lastConfig.privacyMode != config.privacyMode) emit privacyModeChanged();
    m_lastConfig = config;
}

void MatomoTracker::applyRequestResult(const RequestResult &result) {
    m_lastRequestResult = result;
    emit lastRequestStatusChanged();
    emit lastRequestMessageChanged();
}

} // namespace MatomoQt::Qml
