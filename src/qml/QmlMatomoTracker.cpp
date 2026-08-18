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

    connect(&m_tracker, &MatomoQt::Tracker::configChanged, this, &MatomoTracker::onTrackerConfigChanged);
    connect(&m_tracker, &MatomoQt::Tracker::enabledChanged, this, &MatomoTracker::enabledChanged);
    connect(&m_tracker, &MatomoQt::Tracker::consentStateChanged, this, [this](MatomoQt::ConsentState) {
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

Matomo::PrivacyMode MatomoTracker::privacyMode() const {
    return toQmlPrivacyMode(m_tracker.config().privacyMode);
}

void MatomoTracker::setPrivacyMode(const Matomo::PrivacyMode mode) {
    auto config = m_tracker.config();
    const auto converted = toCorePrivacyMode(mode);
    if (config.privacyMode == converted) {
        return;
    }
    config.privacyMode = converted;
    m_tracker.setConfig(config);
}

bool MatomoTracker::isEnabled() const {
    return m_tracker.isEnabled();
}

void MatomoTracker::setEnabled(const bool enabled) {
    m_tracker.setEnabled(enabled);
}

Matomo::ConsentState MatomoTracker::consentState() const {
    return toQmlConsentState(m_tracker.consentState());
}

void MatomoTracker::setConsentState(const Matomo::ConsentState state) {
    m_tracker.setConsentState(toCoreConsentState(state));
}

Matomo::RequestStatus MatomoTracker::lastRequestStatus() const {
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
                .status = MatomoQt::RequestResult::Status::InvalidPayload,
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
    setConsentState(Matomo::ConsentState::Granted);
}

void MatomoTracker::denyConsent() {
    setConsentState(Matomo::ConsentState::Denied);
}

void MatomoTracker::withdrawConsent() {
    setConsentState(Matomo::ConsentState::Withdrawn);
}

void MatomoTracker::resetClientId() {
    m_tracker.resetClientId();
}

Matomo::PrivacyMode MatomoTracker::toQmlPrivacyMode(const MatomoQt::PrivacyMode mode) {
    switch (mode) {
        case MatomoQt::PrivacyMode::Disabled:
            return Matomo::PrivacyMode::Disabled;
        case MatomoQt::PrivacyMode::RequiresConsent:
            return Matomo::PrivacyMode::RequiresConsent;
        case MatomoQt::PrivacyMode::ConsentExemptWithOptOut:
            return Matomo::PrivacyMode::ConsentExemptWithOptOut;
    }
    return Matomo::PrivacyMode::Disabled;
}

MatomoQt::PrivacyMode MatomoTracker::toCorePrivacyMode(const Matomo::PrivacyMode mode) {
    switch (mode) {
        case Matomo::PrivacyMode::Disabled:
            return MatomoQt::PrivacyMode::Disabled;
        case Matomo::PrivacyMode::RequiresConsent:
            return MatomoQt::PrivacyMode::RequiresConsent;
        case Matomo::PrivacyMode::ConsentExemptWithOptOut:
            return MatomoQt::PrivacyMode::ConsentExemptWithOptOut;
    }
    return MatomoQt::PrivacyMode::Disabled;
}

Matomo::ConsentState MatomoTracker::toQmlConsentState(const MatomoQt::ConsentState state) {
    switch (state) {
        case MatomoQt::ConsentState::Unknown:
            return Matomo::ConsentState::Unknown;
        case MatomoQt::ConsentState::Granted:
            return Matomo::ConsentState::Granted;
        case MatomoQt::ConsentState::Denied:
            return Matomo::ConsentState::Denied;
        case MatomoQt::ConsentState::Withdrawn:
            return Matomo::ConsentState::Withdrawn;
    }
    return Matomo::ConsentState::Unknown;
}

MatomoQt::ConsentState MatomoTracker::toCoreConsentState(const Matomo::ConsentState state) {
    switch (state) {
        case Matomo::ConsentState::Unknown:
            return MatomoQt::ConsentState::Unknown;
        case Matomo::ConsentState::Granted:
            return MatomoQt::ConsentState::Granted;
        case Matomo::ConsentState::Denied:
            return MatomoQt::ConsentState::Denied;
        case Matomo::ConsentState::Withdrawn:
            return MatomoQt::ConsentState::Withdrawn;
    }
    return MatomoQt::ConsentState::Unknown;
}

Matomo::RequestStatus MatomoTracker::toQmlRequestStatus(const MatomoQt::RequestResult::Status status) {
    switch (status) {
        case MatomoQt::RequestResult::Status::Accepted:
            return Matomo::RequestStatus::Accepted;
        case MatomoQt::RequestResult::Status::Disabled:
            return Matomo::RequestStatus::RequestDisabled;
        case MatomoQt::RequestResult::Status::BlockedByPrivacy:
            return Matomo::RequestStatus::RequestBlockedByPrivacy;
        case MatomoQt::RequestResult::Status::InvalidConfig:
            return Matomo::RequestStatus::RequestInvalidConfig;
        case MatomoQt::RequestResult::Status::InvalidPayload:
            return Matomo::RequestStatus::RequestInvalidPayload;
    }
    return Matomo::RequestStatus::RequestInvalidConfig;
}

void MatomoTracker::onTrackerConfigChanged() {
    emit endpointChanged();
    emit actionUrlBaseChanged();
    emit siteIdChanged();
    emit privacyModeChanged();
}

void MatomoTracker::applyRequestResult(const MatomoQt::RequestResult &result) {
    const auto status = toQmlRequestStatus(result.status);
    if (m_lastRequestStatus != status) {
        m_lastRequestStatus = status;
        emit lastRequestStatusChanged();
    }
    if (m_lastRequestMessage != result.message) {
        m_lastRequestMessage = result.message;
        emit lastRequestMessageChanged();
    }
}

} // namespace MatomoQt::Qml
