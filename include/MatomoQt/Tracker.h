#pragma once

#include <MatomoQt/ClientIdStore.h>
#include <MatomoQt/ConsentState.h>
#include <MatomoQt/ConsentStore.h>
#include <MatomoQt/Event.h>
#include <MatomoQt/Export.h>
#include <MatomoQt/InMemoryClientIdStore.h>
#include <MatomoQt/InMemoryConsentStore.h>
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

        /** Returns the current consent state, read from the active store. */
        [[nodiscard]] ConsentState consentState() const;

        /** Updates the consent state, written to the active store. */
        void setConsentState(ConsentState state);

        /** Returns whether tracking is enabled locally. */
        [[nodiscard]] bool isEnabled() const;

        /** Enables or disables local tracking validation. */
        void setEnabled(bool enabled);

        /** Sets an optional persistent consent store. nullptr resets to the default in-memory store. */
        void setConsentStore(ConsentStore *store);

        /** Sets an optional persistent client ID store. nullptr resets to the default in-memory store. */
        void setClientIdStore(ClientIdStore *store);

        /** Resets the client ID, clearing the active store. */
        void resetClientId() const;

        /** Validates a page view tracking call without sending a request. */
        [[nodiscard]] RequestResult trackPageView(const PageView &pageView) const;

        /** Validates an event tracking call without sending a request. */
        [[nodiscard]] RequestResult trackEvent(const Event &event) const;

        /** Validates a ping tracking call without sending a request. */
        [[nodiscard]] RequestResult sendPing() const;

    signals:
        void configChanged();
        void consentStateChanged(ConsentState state);
        void enabledChanged(bool enabled);

    private:
        [[nodiscard]] RequestResult validateTrackingCall() const;

        TrackerConfig m_config;
        InMemoryConsentStore m_defaultConsentStore;
        InMemoryClientIdStore m_defaultClientIdStore;
        ConsentStore *m_consentStore = &m_defaultConsentStore;
        ClientIdStore *m_clientIdStore = &m_defaultClientIdStore;
        bool m_enabled = true;
};

} // namespace MatomoQt
