#pragma once

#include <MatomoQt/ConsentState.h>
#include <MatomoQt/Event.h>
#include <MatomoQt/Export.h>
#include <MatomoQt/PageView.h>
#include <MatomoQt/RequestResult.h>
#include <MatomoQt/TrackerConfig.h>

#include <QtCore/QObject>

namespace MatomoQt {

/**
 * Main entry point for tracking calls.
 *
 * This milestone only validates local state and emits no network requests.
 */
class MATOMOQT_CORE_EXPORT Tracker : public QObject {
        Q_OBJECT

    public:
        explicit Tracker(QObject *parent = nullptr);
        explicit Tracker(TrackerConfig config, QObject *parent = nullptr);
        ~Tracker() override;

        /** Returns the current tracker configuration. */
        [[nodiscard]] TrackerConfig config() const;

        /** Replaces the tracker configuration. */
        void setConfig(const TrackerConfig &config);

        /** Returns the current volatile consent state. */
        [[nodiscard]] ConsentState consentState() const;

        /** Updates the current volatile consent state. */
        void setConsentState(ConsentState state);

        /** Returns whether tracking is enabled locally. */
        [[nodiscard]] bool isEnabled() const;

        /** Enables or disables local tracking validation. */
        void setEnabled(bool enabled);

        /** Validates a page view tracking call without sending a request. */
        [[nodiscard]] RequestResult trackPageView(const PageView &pageView);

        /** Validates an event tracking call without sending a request. */
        [[nodiscard]] RequestResult trackEvent(const Event &event);

        /** Validates a ping tracking call without sending a request. */
        [[nodiscard]] RequestResult sendPing();

    signals:
        void configChanged();
        void consentStateChanged(MatomoQt::ConsentState state);
        void enabledChanged(bool enabled);

    private:
        [[nodiscard]] RequestResult validateTrackingCall() const;

        TrackerConfig m_config;
        ConsentState m_consentState = ConsentState::Unknown;
        bool m_enabled = true;
};

} // namespace MatomoQt
