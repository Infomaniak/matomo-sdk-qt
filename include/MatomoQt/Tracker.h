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

#pragma once

#include <MatomoQt/ClientIdStore.h>
#include <MatomoQt/ConsentState.h>
#include <MatomoQt/ConsentStore.h>
#include <MatomoQt/DispatchResult.h>
#include <MatomoQt/Event.h>
#include <MatomoQt/Export.h>
#include <MatomoQt/InMemoryClientIdStore.h>
#include <MatomoQt/InMemoryConsentStore.h>
#include <MatomoQt/NetworkDispatcher.h>
#include <MatomoQt/NetworkDispatcherConfig.h>
#include <MatomoQt/PageView.h>
#include <MatomoQt/RequestBuilder.h>
#include <MatomoQt/RequestResult.h>
#include <MatomoQt/TrackerConfig.h>
#include <MatomoQt/TrackerStats.h>

#include <QtCore/QMap>
#include <QtCore/QObject>

class QNetworkAccessManager;

namespace MatomoQt {

/**
 * Main entry point for tracking calls.
 *
 * The Tracker orchestrates privacy checks, request building and network
 * dispatch.  It owns an internal NetworkDispatcher and RequestBuilder.
 *
 * Tracking calls return RequestResult synchronously to indicate whether the
 * call was accepted or rejected.  The asynchronous network result is reported
 * via the dispatchFinished() signal.
 */
class MATOMOQT_CORE_EXPORT Tracker : public QObject {
        Q_OBJECT

    public:
        explicit Tracker(QObject *parent = nullptr);
        explicit Tracker(TrackerConfig config, QObject *parent = nullptr);
        explicit Tracker(TrackerConfig config, QNetworkAccessManager *nam, QObject *parent);
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

        /** Returns the current client ID, read from the active store. */
        [[nodiscard]] QString clientId() const;

        /** Persists the client ID, written to the active store. */
        void setClientId(const QString &clientId) const;

        /** Resets the client ID, clearing the active store. */
        void resetClientId();

        /** Builds and dispatches a page view tracking request. */
        [[nodiscard]] RequestResult trackPageView(const PageView &pageView);

        /** Builds and dispatches an event tracking request. */
        [[nodiscard]] RequestResult trackEvent(const Event &event);

        /** Builds and dispatches a ping tracking request. */
        [[nodiscard]] RequestResult sendPing();

        /** Sets a tracker-level custom dimension merged into every outgoing request.
         *
         * Per-call dimensions in PageView or Event take precedence over
         * tracker-level dimensions with the same ID.
         */
        void setCustomDimension(int id, const QString &value);

        /** Removes a tracker-level custom dimension. */
        void clearCustomDimension(int id);

        /** Forces new_visit=1 on the next dispatched request. */
        void forceNewVisit();

        /** Returns the current runtime statistics. */
        [[nodiscard]] TrackerStats stats() const;

        /** Resets all runtime statistics counters to zero. */
        void resetStats();

        /** Injects a custom QNetworkAccessManager for the internal dispatcher.
         *
         * The Tracker does not take ownership of @p nam.  Pass nullptr to
         * revert to the internal default.
         */
        void setNetworkAccessManager(QNetworkAccessManager *nam);

        /** Sets the User-Agent string included in outgoing requests.
         *
         * Leave empty to omit the ua parameter.  The host application can
         * build this with UserAgentBuilder or supply its own.
         */
        void setUserAgent(const QString &userAgent);

        /** Sets the network dispatcher configuration (timeout, circuit breaker).
         *
         * Changing the configuration resets the circuit breaker.
         */
        void setNetworkDispatcherConfig(const NetworkDispatcherConfig &config);

    signals:
        void configChanged();
        void consentStateChanged(ConsentState state);
        void enabledChanged(bool enabled);
        void dispatchFinished(const DispatchResult &result);
        void statsChanged();

    private:
        [[nodiscard]] RequestResult validateTrackingCall() const;

        void clearVisitorIdentity();

        RequestBuildOptions buildOptions() const;
        QList<CustomDimension> mergeDimensions(const QList<CustomDimension> &callDimensions) const;
        void addTrackerParameters(QUrl &url);
        void ensureClientId();
        void recordBlocked();
        void recordSent();
        void onDispatchFinished(const DispatchResult &result);

        TrackerConfig m_config;
        InMemoryConsentStore m_defaultConsentStore;
        InMemoryClientIdStore m_defaultClientIdStore;
        ConsentStore *m_consentStore = &m_defaultConsentStore;
        ClientIdStore *m_clientIdStore = &m_defaultClientIdStore;
        bool m_enabled = true;

        NetworkDispatcher *m_dispatcher;
        RequestBuilder m_requestBuilder;
        QMap<int, QString> m_customDimensions;
        QString m_currentPageViewId;
        bool m_forceNewVisit = false;
        TrackerStats m_stats;
        QString m_userAgent;
};

} // namespace MatomoQt
