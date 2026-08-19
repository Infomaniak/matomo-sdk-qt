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

MatomoTracker::MatomoTracker(QObject *parent) :
    QObject(parent),
    m_tracker(this) {

    connect(&m_tracker, &Tracker::configChanged, this, &MatomoTracker::onTrackerConfigChanged);
    connect(&m_tracker, &Tracker::enabledChanged, this, &MatomoTracker::enabledChanged);
    connect(&m_tracker, &Tracker::consentStateChanged, this, [this](ConsentState) {
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

PrivacyMode MatomoTracker::privacyMode() const {
    return m_tracker.config().privacyMode;
}

void MatomoTracker::setPrivacyMode(const PrivacyMode mode) {
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

ConsentState MatomoTracker::consentState() const {
    return m_tracker.consentState();
}

void MatomoTracker::setConsentState(const ConsentState state) {
    m_tracker.setConsentState(state);
}

RequestStatus MatomoTracker::lastRequestStatus() const {
    return m_lastRequestStatus;
}

QString MatomoTracker::lastRequestMessage() const {
    return m_lastRequestMessage;
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
                .status = RequestResult::Status::InvalidPayload,
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
    setConsentState(ConsentState::Granted);
}

void MatomoTracker::denyConsent() {
    setConsentState(ConsentState::Denied);
}

void MatomoTracker::withdrawConsent() {
    setConsentState(ConsentState::Withdrawn);
}

void MatomoTracker::resetClientId() {
    m_tracker.resetClientId();
}

RequestStatus MatomoTracker::toRequestStatus(const RequestResult::Status status) {
    switch (status) {
        case RequestResult::Status::Accepted:
            return RequestStatus::Accepted;
        case RequestResult::Status::Disabled:
            return RequestStatus::RequestDisabled;
        case RequestResult::Status::BlockedByPrivacy:
            return RequestStatus::RequestBlockedByPrivacy;
        case RequestResult::Status::InvalidConfig:
            return RequestStatus::RequestInvalidConfig;
        case RequestResult::Status::InvalidPayload:
            return RequestStatus::RequestInvalidPayload;
    }
    return RequestStatus::RequestInvalidConfig;
}

void MatomoTracker::onTrackerConfigChanged() {
    emit endpointChanged();
    emit actionUrlBaseChanged();
    emit siteIdChanged();
    emit privacyModeChanged();
}

void MatomoTracker::applyRequestResult(const RequestResult &result) {
    if (const auto status = toRequestStatus(result.status); m_lastRequestStatus != status) {
        m_lastRequestStatus = status;
        emit lastRequestStatusChanged();
    }
    if (m_lastRequestMessage != result.message) {
        m_lastRequestMessage = result.message;
        emit lastRequestMessageChanged();
    }
}

} // namespace MatomoQt::Qml
