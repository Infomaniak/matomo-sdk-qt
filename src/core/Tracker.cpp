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

#include <MatomoQt/Tracker.h>

#include <MatomoQt/CustomDimension.h>
#include <MatomoQt/Logging.h>
#include <MatomoQt/PrivacyController.h>
#include <MatomoQt/RequestBuildOptions.h>
#include <MatomoQt/RequestBuildResult.h>
#include <MatomoQt/TrackingRequest.h>

#include <QtCore/QLocale>
#include <QtCore/QRandomGenerator>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/QUrlQuery>

#include <utility>

namespace MatomoQt {

namespace {

RequestResult result(RequestStatus::Value status, QString message = {}) {
    return RequestResult{status, std::move(message)};
}

QString generateClientId() {
    const quint64 part = QRandomGenerator::global()->generate64();
    return QString::number(part, 16).rightJustified(16, u'0').toLower();
}

QString generatePageViewId() {
    const quint32 part = QRandomGenerator::global()->generate();
    return QString::number(part, 16).rightJustified(6, u'0').right(6).toUpper();
}

bool isValidClientId(const QString &clientId) {
    static const QRegularExpression clientIdPattern(QStringLiteral("^[0-9A-Fa-f]{16}$"));
    return clientId.isEmpty() || clientIdPattern.match(clientId).hasMatch();
}

} // namespace

Tracker::Tracker(TrackerConfig config, ConsentStore *consentStore, ClientIdStore *clientIdStore, QObject *parent) :
    Tracker(std::move(config), nullptr, consentStore, clientIdStore, parent) {
}

Tracker::Tracker(TrackerConfig config, QNetworkAccessManager *nam, ConsentStore *consentStore, ClientIdStore *clientIdStore, QObject *parent) :
    QObject(parent),
    m_config(std::move(config)),
    m_consentStore(consentStore),
    m_clientIdStore(clientIdStore),
    m_dispatcher(new NetworkDispatcher(nam, this)),
    m_requestBuilder(m_config) {

    m_dispatcher->setConfig(m_config.networkDispatcherConfig);

    connect(m_dispatcher, &NetworkDispatcher::dispatchFinished, this, &Tracker::onDispatchFinished);

    if (m_consentStore->consentState() == ConsentState::Value::Denied
        || m_consentStore->consentState() == ConsentState::Value::Withdrawn) {
        clearVisitorIdentity();
    }
}

Tracker::~Tracker() = default;

TrackerConfig Tracker::config() const {
    return m_config;
}


ConsentState::Value Tracker::consentState() const {
    return m_consentStore->consentState();
}

void Tracker::setConsentState(const ConsentState::Value state) {
    const auto previousState = m_consentStore->consentState();
    if (previousState == state) {
        return;
    }

    m_consentStore->setConsentState(state);

    if (state == ConsentState::Value::Denied || state == ConsentState::Value::Withdrawn) {
        clearVisitorIdentity();
    }

    if (const auto persistedState = m_consentStore->consentState(); persistedState != previousState) {
        emit consentStateChanged(persistedState);
    }
}

bool Tracker::isEnabled() const {
    return m_enabled;
}

void Tracker::setEnabled(const bool enabled) {
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    emit enabledChanged(m_enabled);
}

QString Tracker::clientId() const {
    return m_clientIdStore->clientId();
}

bool Tracker::setClientId(const QString &clientId) const {
    if (!isValidClientId(clientId)) {
        return false;
    }

    m_clientIdStore->setClientId(clientId);
    return true;
}

void Tracker::resetClientId() {
    clearVisitorIdentity();
}

void Tracker::forceNewVisit() {
    m_forceNewVisit = true;
}

TrackerStats Tracker::stats() const {
    return m_stats;
}

RequestResult Tracker::trackPageView(const PageView &pageView) {
    if (!pageView.isValid()) {
        recordBlocked();
        return result(RequestStatus::Value::RequestInvalidPayload, QStringLiteral("Page view path is required."));
    }

    if (const auto validation = validateTrackingCall(); !validation.accepted()) {
        recordBlocked();
        return validation;
    }

    ensureClientId();

    PageView enriched = pageView;
    enriched.customDimensions = mergeDimensions(pageView.customDimensions);

    const auto buildResult = m_requestBuilder.buildPageView(enriched, buildOptions());
    if (!buildResult.accepted()) {
        recordBlocked();
        return buildResult.result;
    }

    const QString pageViewId = generatePageViewId();
    const bool forceNewVisit = m_forceNewVisit;

    QUrl url = buildResult.request.url;
    addTrackerParameters(url, pageViewId, forceNewVisit);

    qCDebug(matomoSdk) << "dispatching page view";
    if (m_dispatcher->dispatch(url)) {
        m_currentPageViewId = pageViewId;
        m_lastPageViewPath = enriched.path;
        if (forceNewVisit) {
            m_forceNewVisit = false;
        }
        recordSent();
    }

    return result(RequestStatus::Value::RequestAccepted);
}

RequestResult Tracker::trackEvent(const Event &event) {
    if (!event.isValid()) {
        recordBlocked();
        return result(RequestStatus::Value::RequestInvalidPayload, QStringLiteral("Event category and action are required."));
    }

    if (const auto validation = validateTrackingCall(); !validation.accepted()) {
        recordBlocked();
        return validation;
    }

    ensureClientId();

    Event enriched = event;
    enriched.customDimensions = mergeDimensions(event.customDimensions);

    const auto buildResult = m_requestBuilder.buildEvent(enriched, buildOptions());
    if (!buildResult.accepted()) {
        recordBlocked();
        return buildResult.result;
    }

    QUrl url = buildResult.request.url;
    const bool forceNewVisit = m_forceNewVisit;
    addTrackerParameters(url, m_currentPageViewId, forceNewVisit);

    qCDebug(matomoSdk) << "dispatching event";
    if (m_dispatcher->dispatch(url)) {
        if (forceNewVisit) {
            m_forceNewVisit = false;
        }
        recordSent();
    }

    return result(RequestStatus::Value::RequestAccepted);
}

RequestResult Tracker::sendPing() {
    if (const auto validation = validateTrackingCall(); !validation.accepted()) {
        recordBlocked();
        return validation;
    }

    ensureClientId();

    const QString pingPath = m_lastPageViewPath.isEmpty() ? QStringLiteral("/") : m_lastPageViewPath;
    const auto buildResult = m_requestBuilder.buildPing(pingPath, buildOptions());
    if (!buildResult.accepted()) {
        recordBlocked();
        return buildResult.result;
    }

    QUrl url = buildResult.request.url;
    const bool forceNewVisit = m_forceNewVisit;
    addTrackerParameters(url, m_currentPageViewId, forceNewVisit);

    qCDebug(matomoSdk) << "dispatching ping";
    if (m_dispatcher->dispatch(url)) {
        if (forceNewVisit) {
            m_forceNewVisit = false;
        }
        recordSent();
    }

    return result(RequestStatus::Value::RequestAccepted);
}

RequestResult Tracker::validateTrackingCall() const {
    if (!m_enabled) {
        return result(RequestStatus::Value::RequestDisabled, QStringLiteral("Tracker is disabled."));
    }

    if (!m_config.isValid()) {
        return result(RequestStatus::Value::RequestInvalidConfig, QStringLiteral("Tracker endpoint and site ID are required."));
    }

    if (!PrivacyController::isTrackingAllowed(m_config.privacyMode, m_consentStore->consentState())) {
        return result(RequestStatus::Value::RequestBlockedByPrivacy, QStringLiteral("Tracking blocked by privacy settings."));
    }

    return result(RequestStatus::Value::RequestAccepted);
}

void Tracker::clearVisitorIdentity() {
    m_clientIdStore->clearClientId();
    m_currentPageViewId.clear();
    m_lastPageViewPath.clear();
}

void Tracker::ensureClientId() {
    if (m_clientIdStore->clientId().isEmpty()) {
        m_clientIdStore->setClientId(generateClientId());
    }
}

RequestBuildOptions Tracker::buildOptions() const {
    RequestBuildOptions options;
    options.clientId = m_clientIdStore->clientId();
    options.userAgent = m_config.userAgent;
    options.language = QLocale().name();
    options.screenResolution = {};
    return options;
}

QList<CustomDimension> Tracker::mergeDimensions(const QList<CustomDimension> &callDimensions) const {
    QSet<int> usedIds;
    QList<CustomDimension> merged;

    for (const auto &dim: callDimensions) {
        merged.append(dim);
        usedIds.insert(dim.id);
    }

    for (auto it = m_config.customDimensions.constBegin(); it != m_config.customDimensions.constEnd(); ++it) {
        if (!usedIds.contains(it.key())) {
            merged.append({.id = it.key(), .value = it.value()});
        }
    }

    return merged;
}

void Tracker::addTrackerParameters(QUrl &url, const QString &pageViewId, const bool forceNewVisit) const {
    QUrlQuery query(url);

    if (!pageViewId.isEmpty()) {
        query.addQueryItem(QStringLiteral("pv_id"), pageViewId);
    }

    if (forceNewVisit) {
        query.addQueryItem(QStringLiteral("new_visit"), QStringLiteral("1"));
    }

    url.setQuery(query);
}

void Tracker::recordBlocked() {
    m_stats.blockedCount++;
    emit statsChanged();
}

void Tracker::recordSent() {
    m_stats.sentCount++;
    emit statsChanged();
}

void Tracker::onDispatchFinished(const DispatchResult &dispatchResult) {
    if (!dispatchResult.success()) {
        m_stats.failedCount++;
        emit statsChanged();
    }

    emit dispatchFinished(dispatchResult);
}

} // namespace MatomoQt
