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

#include <MatomoQt/NetworkDispatcher.h>

#include <MatomoQt/Logging.h>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <QtCore/QRandomGenerator>
#include <QtCore/QTimer>
#include <QtCore/QUrlQuery>

#include <utility>

namespace MatomoQt {

namespace {

DispatchResult makeResult(const DispatchStatus::Value status, QString message, const int httpStatus = 0) {
    return DispatchResult{.status = status, .httpStatus = httpStatus, .message = std::move(message)};
}

QUrl addCacheBuster(QUrl url) {
    QUrlQuery query(url);
    query.addQueryItem(QStringLiteral("rand"), QString::number(QRandomGenerator::global()->generate()));
    url.setQuery(query);
    return url;
}

} // namespace

NetworkDispatcher::NetworkDispatcher(QObject *parent) :
    QObject(parent),
    m_nam(new QNetworkAccessManager(this)) {}

NetworkDispatcher::NetworkDispatcher(QNetworkAccessManager *nam, QObject *parent) :
    QObject(parent),
    m_nam(nam ? nam : new QNetworkAccessManager(this)) {}

NetworkDispatcher::~NetworkDispatcher() {
    for (auto it = m_pendingReplies.begin(); it != m_pendingReplies.end(); ++it) {
        QNetworkReply *reply = it.key();
        if (it.value().timer != nullptr) {
            it.value().timer->stop();
        }
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
    }
}

NetworkDispatcherConfig NetworkDispatcher::config() const {
    return m_config;
}

void NetworkDispatcher::setConfig(const NetworkDispatcherConfig &config) {
    if (m_config == config) {
        return;
    }

    m_config = config;
    resetCircuitBreaker();
    emit configChanged();
}

bool NetworkDispatcher::isCircuitBreakerOpen() const {
    return m_circuitBreakerOpen;
}

void NetworkDispatcher::resetCircuitBreaker() {
    m_consecutiveFailures = 0;
    setOpen(false);
}

bool NetworkDispatcher::dispatch(const QUrl &url) {
    qCDebug(matomoSdk) << "dispatching request";
    if (m_circuitBreakerOpen) {
        qCDebug(matomoSdk) << "dispatch blocked by circuit breaker";
        emit dispatchFinished(makeResult(DispatchStatus::Value::CircuitBreakerOpen,
                                          QStringLiteral("Circuit breaker is open.")));
        return false;
    }

    if (!url.isValid() || url.isEmpty()) {
        qCDebug(matomoSdk) << "dispatch rejected: invalid URL";
        emit dispatchFinished(makeResult(DispatchStatus::Value::NetworkError,
                                          QStringLiteral("Tracking URL is invalid.")));
        return false;
    }

    const QUrl dispatchUrl = addCacheBuster(url);
    QNetworkRequest request(dispatchUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_nam->get(request);

    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    if (m_config.timeoutMs > 0) {
        timer->start(m_config.timeoutMs);
    }

    m_pendingReplies.insert(reply, PendingReply{timer});

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleReplyFinished(reply);
    });

    connect(timer, &QTimer::timeout, this, [this, reply]() {
        handleTimeout(reply);
    });

    connect(reply, &QNetworkReply::sslErrors, this, [](const QList<QSslError> &) {
        // SSL errors are never ignored. The reply will fail when finished.
        qCDebug(matomoSdk) << "SSL errors detected on reply";
    });

    return true;
}

void NetworkDispatcher::handleReplyFinished(QNetworkReply *reply) {
    if (!m_pendingReplies.contains(reply)) {
        return;
    }

    cleanupReply(reply);

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() == QNetworkReply::NoError) {
        if (httpStatus >= 200 && httpStatus < 300) {
            qCDebug(matomoSdk) << "dispatch succeeded with HTTP" << httpStatus;
            recordSuccess();
            emit dispatchFinished(makeResult(DispatchStatus::Value::Success, {}, httpStatus));
        } else {
            qCDebug(matomoSdk) << "dispatch failed with HTTP" << httpStatus;
            recordFailure(DispatchStatus::Value::NetworkError,
                          QStringLiteral("Server returned HTTP %1.").arg(httpStatus),
                          httpStatus);
            emit dispatchFinished(makeResult(DispatchStatus::Value::NetworkError,
                                             QStringLiteral("Server returned HTTP %1.").arg(httpStatus),
                                             httpStatus));
        }
    } else if (reply->error() == QNetworkReply::OperationCanceledError) {
        // Timeout already handled by handleTimeout; if the reply was aborted
        // during destruction, do not emit a result.
        qCDebug(matomoSdk) << "dispatch cancelled";
    } else if (reply->error() == QNetworkReply::SslHandshakeFailedError) {
        qCDebug(matomoSdk) << "dispatch failed: SSL error";
        recordFailure(DispatchStatus::Value::SslError, QStringLiteral("SSL handshake failed."));
        emit dispatchFinished(makeResult(DispatchStatus::Value::SslError, QStringLiteral("SSL handshake failed.")));
    } else if (httpStatus > 0) {
        // HTTP response received but with an error status (e.g. 4xx, 5xx).
        qCDebug(matomoSdk) << "dispatch failed with HTTP" << httpStatus;
        recordFailure(DispatchStatus::Value::NetworkError,
                      QStringLiteral("Server returned HTTP %1.").arg(httpStatus),
                      httpStatus);
        emit dispatchFinished(makeResult(DispatchStatus::Value::NetworkError,
                                         QStringLiteral("Server returned HTTP %1.").arg(httpStatus),
                                         httpStatus));
    } else {
        qCDebug(matomoSdk) << "dispatch failed: network error";
        recordFailure(DispatchStatus::Value::NetworkError, reply->errorString());
        emit dispatchFinished(makeResult(DispatchStatus::Value::NetworkError, reply->errorString()));
    }

    reply->deleteLater();
}

void NetworkDispatcher::handleTimeout(QNetworkReply *reply) {
    if (!m_pendingReplies.contains(reply)) {
        return;
    }

    qCDebug(matomoSdk) << "dispatch timed out";
    cleanupReply(reply);
    reply->abort();

    recordFailure(DispatchStatus::Value::Timeout, QStringLiteral("Request timed out."));
    emit dispatchFinished(makeResult(DispatchStatus::Value::Timeout, QStringLiteral("Request timed out.")));

    // The abort will trigger finished(); handleReplyFinished will see
    // OperationCanceledError and skip emitting a duplicate result.
    reply->deleteLater();
}

void NetworkDispatcher::recordSuccess() {
    m_consecutiveFailures = 0;
    setOpen(false);
}

void NetworkDispatcher::recordFailure(const DispatchStatus::Value status, const QString &message, const int httpStatus) {
    //Following parameters might be use later for better handling
    Q_UNUSED(status)
    Q_UNUSED(message)
    Q_UNUSED(httpStatus)

    ++m_consecutiveFailures;

    if (m_config.maxConsecutiveFailures > 0 && m_consecutiveFailures >= m_config.maxConsecutiveFailures) {
        setOpen(true);
    }
}

void NetworkDispatcher::setOpen(const bool open) {
    if (m_circuitBreakerOpen == open) {
        return;
    }

    m_circuitBreakerOpen = open;
    emit circuitBreakerChanged(m_circuitBreakerOpen);
}

void NetworkDispatcher::cleanupReply(QNetworkReply *reply) {
    const auto it = m_pendingReplies.find(reply);
    if (it == m_pendingReplies.end()) {
        return;
    }

    if (it.value().timer != nullptr) {
        it.value().timer->stop();
        it.value().timer->deleteLater();
    }
    m_pendingReplies.erase(it);
}

} // namespace MatomoQt
