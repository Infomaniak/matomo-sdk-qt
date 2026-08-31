#include <MatomoQt/Tracker.h>
#include <MatomoQt/PrivacyController.h>

#include <utility>

namespace MatomoQt {

namespace {

RequestResult result(RequestResult::Status status, QString message = {}) {
    return RequestResult{status, std::move(message)};
}

} // namespace

Tracker::Tracker(QObject *parent) :
    QObject(parent) {}

Tracker::Tracker(TrackerConfig config, QObject *parent) :
    QObject(parent),
    m_config(std::move(config)) {}

Tracker::~Tracker() = default;

TrackerConfig Tracker::config() const {
    return m_config;
}

void Tracker::setConfig(const TrackerConfig &config) {
    if (m_config == config) {
        return;
    }

    m_config = config;
    emit configChanged();
}

ConsentState Tracker::consentState() const {
    return m_consentStore->consentState();
}

void Tracker::setConsentState(const ConsentState state) {
    const auto previousState = m_consentStore->consentState();
    if (previousState == state) {
        return;
    }

    m_consentStore->setConsentState(state);

    // Consent withdrawal or denial must clear persisted client ID
    if (state == ConsentState::Denied || state == ConsentState::Withdrawn) {
        m_clientIdStore->clearClientId();
    }

    if (const auto persistedState = m_consentStore->consentState(); persistedState != previousState) {
        emit consentStateChanged(persistedState);
    }
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

RequestResult Tracker::trackPageView(const PageView &pageView) const {
    if (!pageView.isValid()) {
        return result(RequestResult::Status::InvalidPayload, QStringLiteral("Page view path is required."));
    }

    if (const auto validation = validateTrackingCall(); !validation.accepted()) {
        return validation;
    }

    return result(RequestResult::Status::Accepted);
}

RequestResult Tracker::trackEvent(const Event &event) const {
    if (!event.isValid()) {
        return result(RequestResult::Status::InvalidPayload, QStringLiteral("Event category and action are required."));
    }

    if (const auto validation = validateTrackingCall(); !validation.accepted()) {
        return validation;
    }

    return result(RequestResult::Status::Accepted);
}

RequestResult Tracker::sendPing() const {
    return validateTrackingCall();
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

} // namespace MatomoQt
