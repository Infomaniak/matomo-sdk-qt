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
#include <QtCore/QSet>
#include <QtCore/QUrlQuery>

#include <utility>

namespace MatomoQt {

namespace {

RequestResult result(RequestResult::Status status, QString message = {}) {
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

} // namespace

Tracker::Tracker(QObject *parent) :
    QObject(parent),
    m_dispatcher(new NetworkDispatcher(this)),
    m_requestBuilder(m_config) {

    connect(m_dispatcher, &NetworkDispatcher::dispatchFinished, this, &Tracker::onDispatchFinished);
}

Tracker::Tracker(TrackerConfig config, QObject *parent) :
    QObject(parent),
    m_config(std::move(config)),
    m_dispatcher(new NetworkDispatcher(this)),
    m_requestBuilder(m_config) {

    connect(m_dispatcher, &NetworkDispatcher::dispatchFinished, this, &Tracker::onDispatchFinished);
}

Tracker::Tracker(TrackerConfig config, QNetworkAccessManager *nam, QObject *parent) :
    QObject(parent),
    m_config(std::move(config)),
    m_dispatcher(new NetworkDispatcher(nam, this)),
    m_requestBuilder(m_config) {

    connect(m_dispatcher, &NetworkDispatcher::dispatchFinished, this, &Tracker::onDispatchFinished);
}

Tracker::~Tracker() = default;

TrackerConfig Tracker::config() const {
    return m_config;
}

void Tracker::setConfig(const TrackerConfig &config) {
    if (m_config == config) {
        return;
    }

    m_config = config;
    m_requestBuilder.setConfig(m_config);
    emit configChanged();
}

ConsentState Tracker::consentState() const {
    return m_consentStore->consentState();
}

void Tracker::setConsentState(const ConsentState state) {
    if (m_consentStore->consentState() == state) {
        return;
    }

    m_consentStore->setConsentState(state);

    if (state == ConsentState::Denied || state == ConsentState::Withdrawn) {
        m_clientIdStore->clearClientId();
    }

    emit consentStateChanged(state);
}

bool Tracker::isEnabled() const {
    return m_enabled;
}

void Tracker::setEnabled(bool enabled) {
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    emit enabledChanged(m_enabled);
}

void Tracker::setConsentStore(ConsentStore *store) {
    const auto oldState = m_consentStore->consentState();
    m_defaultConsentStore.setConsentState(m_consentStore->consentState());
    m_consentStore = store ? store : &m_defaultConsentStore;

    const auto newState = m_consentStore->consentState();
    if (newState == ConsentState::Denied || newState == ConsentState::Withdrawn) {
        m_clientIdStore->clearClientId();
    }
    if (oldState != newState) {
        emit consentStateChanged(newState);
    }
}

void Tracker::setClientIdStore(ClientIdStore *store) {
    m_defaultClientIdStore.setClientId(m_clientIdStore->clientId());
    m_clientIdStore = store ? store : &m_defaultClientIdStore;

    if (m_consentStore->consentState() == ConsentState::Denied
        || m_consentStore->consentState() == ConsentState::Withdrawn) {
        m_clientIdStore->clearClientId();
    }
}

QString Tracker::clientId() const {
    return m_clientIdStore->clientId();
}

void Tracker::setClientId(const QString &clientId) const {
    m_clientIdStore->setClientId(clientId);
}

void Tracker::resetClientId() const {
    m_clientIdStore->clearClientId();
}

void Tracker::setCustomDimension(const int id, const QString &value) {
    if (value.isEmpty()) {
        m_customDimensions.remove(id);
    } else {
        m_customDimensions[id] = value;
    }
}

void Tracker::clearCustomDimension(const int id) {
    m_customDimensions.remove(id);
}

void Tracker::forceNewVisit() {
    m_forceNewVisit = true;
}

TrackerStats Tracker::stats() const {
    return m_stats;
}

void Tracker::resetStats() {
    m_stats = {};
    emit statsChanged();
}

void Tracker::setNetworkAccessManager(QNetworkAccessManager *nam) {
    const auto config = m_dispatcher->config();
    const bool breakerOpen = m_dispatcher->isCircuitBreakerOpen();

    delete m_dispatcher;
    m_dispatcher = new NetworkDispatcher(nam, this);
    m_dispatcher->setConfig(config);

    if (breakerOpen) {
        m_dispatcher->resetCircuitBreaker();
    }

    connect(m_dispatcher, &NetworkDispatcher::dispatchFinished, this, &Tracker::onDispatchFinished);
}

void Tracker::setUserAgent(const QString &userAgent) {
    m_userAgent = userAgent;
}

RequestResult Tracker::trackPageView(const PageView &pageView) {
    if (!pageView.isValid()) {
        recordBlocked();
        return result(RequestResult::Status::InvalidPayload, QStringLiteral("Page view path is required."));
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

    m_currentPageViewId = generatePageViewId();

    QUrl url = buildResult.request.url;
    addTrackerParameters(url);

    qCDebug(matomoSdk) << "dispatching page view";
    m_dispatcher->dispatch(url);
    recordSent();

    return result(RequestResult::Status::Accepted);
}

RequestResult Tracker::trackEvent(const Event &event) {
    if (!event.isValid()) {
        recordBlocked();
        return result(RequestResult::Status::InvalidPayload, QStringLiteral("Event category and action are required."));
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
    addTrackerParameters(url);

    qCDebug(matomoSdk) << "dispatching event";
    m_dispatcher->dispatch(url);
    recordSent();

    return result(RequestResult::Status::Accepted);
}

RequestResult Tracker::sendPing() {
    if (const auto validation = validateTrackingCall(); !validation.accepted()) {
        recordBlocked();
        return validation;
    }

    ensureClientId();

    const auto buildResult = m_requestBuilder.buildPing(QStringLiteral("ping"), buildOptions());
    if (!buildResult.accepted()) {
        recordBlocked();
        return buildResult.result;
    }

    QUrl url = buildResult.request.url;
    addTrackerParameters(url);

    qCDebug(matomoSdk) << "dispatching ping";
    m_dispatcher->dispatch(url);
    recordSent();

    return result(RequestResult::Status::Accepted);
}

RequestResult Tracker::validateTrackingCall() const {
    if (!m_enabled) {
        return result(RequestResult::Status::Disabled, QStringLiteral("Tracker is disabled."));
    }

    if (!m_config.isValid()) {
        return result(RequestResult::Status::InvalidConfig, QStringLiteral("Tracker endpoint and site ID are required."));
    }

    if (!PrivacyController::isTrackingAllowed(m_config.privacyMode, m_consentStore->consentState())) {
        return result(RequestResult::Status::BlockedByPrivacy, QStringLiteral("Tracking blocked by privacy settings."));
    }

    return result(RequestResult::Status::Accepted);
}

void Tracker::ensureClientId() {
    if (m_clientIdStore->clientId().isEmpty()) {
        m_clientIdStore->setClientId(generateClientId());
    }
}

RequestBuildOptions Tracker::buildOptions() const {
    RequestBuildOptions options;
    options.clientId = m_clientIdStore->clientId();
    options.userAgent = m_userAgent;
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

    for (auto it = m_customDimensions.constBegin(); it != m_customDimensions.constEnd(); ++it) {
        if (!usedIds.contains(it.key())) {
            merged.append({.id = it.key(), .value = it.value()});
        }
    }

    return merged;
}

void Tracker::addTrackerParameters(QUrl &url) {
    QUrlQuery query(url);

    if (!m_currentPageViewId.isEmpty()) {
        query.addQueryItem(QStringLiteral("pv_id"), m_currentPageViewId);
    }

    if (m_forceNewVisit) {
        query.addQueryItem(QStringLiteral("new_visit"), QStringLiteral("1"));
        m_forceNewVisit = false;
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
