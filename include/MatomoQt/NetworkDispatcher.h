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

#include <MatomoQt/DispatchResult.h>
#include <MatomoQt/Export.h>
#include <MatomoQt/NetworkDispatcherConfig.h>

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace MatomoQt {

/**
 * Sends Matomo tracking requests over HTTP.
 *
 * The dispatcher is a standalone component: it receives a fully-built URL
 * (typically from RequestBuilder) and handles the network I/O.  It does not
 * know about privacy, consent or the Tracker.
 *
 * Features:
 * - Dependency injection of a custom QNetworkAccessManager.
 * - Per-request timeout (default 10 s).
 * - Circuit breaker that opens after a configurable number of consecutive
 *   failures and blocks further dispatch until reset.
 * - `rand` cache-busting parameter added to every outgoing request.
 * - SSL errors are never silently ignored.
 * - Safe destruction while replies are pending.
 *
 * The result of each dispatch is reported via the dispatchFinished() signal.
 */
class MATOMOQT_CORE_EXPORT NetworkDispatcher : public QObject {
        Q_OBJECT

    public:
        /** Creates a dispatcher with an internal QNetworkAccessManager. */
        explicit NetworkDispatcher(QObject *parent = nullptr);

        /** Creates a dispatcher using the supplied QNetworkAccessManager.
         *
         * The dispatcher does not take ownership of @p nam.  The caller must
         * ensure @p nam outlives the dispatcher, or pass a child of the
         * dispatcher.
         */
        explicit NetworkDispatcher(QNetworkAccessManager *nam, QObject *parent = nullptr);

        ~NetworkDispatcher() override;

        NetworkDispatcher(const NetworkDispatcher &) = delete;
        NetworkDispatcher &operator=(const NetworkDispatcher &) = delete;
        NetworkDispatcher(NetworkDispatcher &&) = delete;
        NetworkDispatcher &operator=(NetworkDispatcher &&) = delete;

        /** Returns the current dispatcher configuration. */
        [[nodiscard]] NetworkDispatcherConfig config() const;

        /** Replaces the dispatcher configuration.
         *
         * Changing the configuration resets the circuit breaker.
         */
        void setConfig(const NetworkDispatcherConfig &config);

        /** Returns true when the circuit breaker is open and blocking dispatch. */
        [[nodiscard]] bool isCircuitBreakerOpen() const;

        /** Resets the circuit breaker to the closed state. */
        void resetCircuitBreaker();

        /** Dispatches a GET request to @p url.
         *
         * The @p url should be a fully-built Matomo tracking URL.  A `rand`
         * cache-busting parameter is added before the request is sent.
         *
         * If the circuit breaker is open, no network request is made and a
         * DispatchResult with status CircuitBreakerOpen is emitted.
         *
         * The result is always reported via the dispatchFinished() signal.
         * Returns true only when a network reply was created.
         */
        bool dispatch(const QUrl &url);

    signals:
        void dispatchFinished(const DispatchResult &result);
        void circuitBreakerChanged(bool open);
        void configChanged();

    private:
        void handleReplyFinished(QNetworkReply *reply);
        void handleTimeout(QNetworkReply *reply);
        void recordSuccess();
        void recordFailure(DispatchStatus::Value status, const QString &message, int httpStatus = 0);
        void setOpen(bool open);
        void cleanupReply(QNetworkReply *reply);

        struct PendingReply {
                QTimer *timer = nullptr;
        };

        NetworkDispatcherConfig m_config;
        QNetworkAccessManager *m_nam = nullptr;
        int m_consecutiveFailures = 0;
        bool m_circuitBreakerOpen = false;
        QHash<QNetworkReply *, PendingReply> m_pendingReplies;
};

} // namespace MatomoQt
