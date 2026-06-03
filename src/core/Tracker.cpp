#include <MatomoQt/Tracker.h>

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
    if (m_config.endpoint == config.endpoint && m_config.siteId == config.siteId && m_config.privacyMode == config.privacyMode) {
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

RequestResult Tracker::trackPageView(const PageView &pageView) const {
    if (const auto validation = validateTrackingCall(); !validation.accepted()) {
        return validation;
    }

    if (!pageView.isValid()) {
        return result(RequestResult::Status::InvalidPayload, QStringLiteral("Page view path is required."));
    }

    return result(RequestResult::Status::Accepted);
}

RequestResult Tracker::trackEvent(const Event &event) const {
    if (const auto validation = validateTrackingCall(); !validation.accepted()) {
        return validation;
    }

    if (!event.isValid()) {
        return result(RequestResult::Status::InvalidPayload, QStringLiteral("Event category and action are required."));
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

    switch (m_config.privacyMode) {
        using enum PrivacyMode;
        using enum ConsentState;
        case Disabled:
            return result(RequestResult::Status::BlockedByPrivacy, QStringLiteral("Tracking is disabled by privacy mode."));
        case RequiresConsent:
            if (m_consentState != Granted) {
                return result(RequestResult::Status::BlockedByPrivacy, QStringLiteral("Tracking consent is required."));
            }
            break;
        case ConsentExemptWithOptOut:
            if (m_consentState == Denied || m_consentState == Withdrawn) {
                return result(RequestResult::Status::BlockedByPrivacy, QStringLiteral("User has opted out."));
            }
            break;
    }

    return result(RequestResult::Status::Accepted);
}

} // namespace MatomoQt
