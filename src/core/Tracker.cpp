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
    return m_consentState;
}

void Tracker::setConsentState(ConsentState state) {
    if (m_consentState == state) {
        return;
    }

    m_consentState = state;
    emit consentStateChanged(m_consentState);
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

RequestResult Tracker::trackPageView(const PageView &pageView) {
    if (!pageView.isValid()) {
        return result(RequestResult::Status::InvalidPayload, QStringLiteral("Page view path is required."));
    }

    if (const auto validation = validateTrackingCall(); !validation.accepted()) {
        return validation;
    }

    return result(RequestResult::Status::Accepted);
}

RequestResult Tracker::trackEvent(const Event &event) {
    if (!event.isValid()) {
        return result(RequestResult::Status::InvalidPayload, QStringLiteral("Event category and action are required."));
    }

    if (const auto validation = validateTrackingCall(); !validation.accepted()) {
        return validation;
    }

    return result(RequestResult::Status::Accepted);
}

RequestResult Tracker::sendPing() {
    return validateTrackingCall();
}

RequestResult Tracker::validateTrackingCall() const {
    if (!m_enabled) {
        return result(RequestResult::Status::Disabled, QStringLiteral("Tracker is disabled."));
    }

    if (!m_config.isValid()) {
        return result(RequestResult::Status::InvalidConfig, QStringLiteral("Tracker endpoint and site ID are required."));
    }

    if (!PrivacyController::isTrackingAllowed(m_config.privacyMode, m_consentState)) {
        return result(RequestResult::Status::BlockedByPrivacy, QStringLiteral("Tracking blocked by privacy settings."));
    }

    return result(RequestResult::Status::Accepted);
}

} // namespace MatomoQt
