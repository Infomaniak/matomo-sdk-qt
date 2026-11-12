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

#include <MatomoQt/DispatchResult.h>
#include <MatomoQt/NetworkDispatcher.h>
#include <MatomoQt/NetworkDispatcherConfig.h>

#include <QtCore/QPointer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include <algorithm>

using namespace MatomoQt;

namespace {

/**
 * Minimal local HTTP server for integration tests.
 *
 * Listens on 127.0.0.1 and accepts a single connection at a time.  Each
 * connected socket is handled by reading the request line (to verify the
 * `rand` parameter) and sending a configurable response.
 */
class TestHttpServer : public QObject {
        Q_OBJECT

    public:
        enum class Mode {
            Respond200,
            Respond500,
            Respond304,
            RespondWithoutTls,
            HangWithoutResponse,
            CloseWithoutResponse,
        };

        explicit TestHttpServer(Mode mode = Mode::Respond200, QObject *parent = nullptr) :
            QObject(parent),
            m_mode(mode) {}

        ~TestHttpServer() override {
            if (m_server != nullptr) {
                m_server->close();
            }
        }

        bool start() {
            m_server = new QTcpServer(this);
            if (!m_server->listen(QHostAddress::LocalHost)) {
                return false;
            }
            connect(m_server, &QTcpServer::newConnection, this, &TestHttpServer::handleConnection);
            return true;
        }

        [[nodiscard]] QUrl url() const {
            QUrl u;
            u.setScheme(QStringLiteral("http"));
            u.setHost(QStringLiteral("127.0.0.1"));
            u.setPort(m_server != nullptr ? m_server->serverPort() : 0);
            u.setPath(QStringLiteral("/matomo.php"));
            return u;
        }

        [[nodiscard]] QString lastRequestLine() const { return m_lastRequestLine; }
        [[nodiscard]] QString lastRequestPath() const { return m_lastRequestPath; }
        [[nodiscard]] int requestCount() const { return m_requestCount; }

        void setMode(Mode mode) { m_mode = mode; }

        void respondToPendingRequests(Mode mode) {
            const auto sockets = m_pendingSockets;
            m_pendingSockets.clear();
            for (const auto &socket: sockets) {
                if (socket) {
                    respond(socket, mode);
                }
            }
        }

    private:
        void handleConnection() {
            while (m_server->hasPendingConnections()) {
                QTcpSocket *socket = m_server->nextPendingConnection();
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { readRequest(socket); });
            }
        }

        void readRequest(QTcpSocket *socket) {
            QByteArray data = socket->property("requestData").toByteArray();
            data += socket->readAll();
            if (m_mode == Mode::RespondWithoutTls) {
                // The client opened a TLS handshake: answer in plain HTTP right away.
                ++m_requestCount;
                respond(socket, Mode::Respond200);
                return;
            }
            if (!data.contains("\r\n\r\n")) {
                socket->setProperty("requestData", data);
                return;
            }

            const QString request = QString::fromUtf8(data);
            m_lastRequestLine = request.section(QLatin1Char('\n'), 0, 0).trimmed();
            m_lastRequestPath = m_lastRequestLine.section(QLatin1Char(' '), 1, 1);
            ++m_requestCount;
            respond(socket, m_mode);
        }

        void respond(QTcpSocket *socket, Mode mode) {
            switch (mode) {
                case Mode::Respond200:
                    socket->write(
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: 2\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "OK");
                    socket->disconnectFromHost();
                    break;
                case Mode::Respond500:
                    socket->write(
                            "HTTP/1.1 500 Internal Server Error\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: 5\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "Error");
                    socket->disconnectFromHost();
                    break;
                case Mode::Respond304:
                    socket->write(
                            "HTTP/1.1 304 Not Modified\r\n"
                            "Connection: close\r\n"
                            "\r\n");
                    socket->disconnectFromHost();
                    break;
                case Mode::RespondWithoutTls:
                    respond(socket, Mode::Respond200);
                    break;
                case Mode::HangWithoutResponse:
                    // Intentionally do nothing; let the timeout fire.
                    m_pendingSockets.append(socket);
                    break;
                case Mode::CloseWithoutResponse:
                    socket->disconnectFromHost();
                    break;
            }
        }

        QTcpServer *m_server = nullptr;
        Mode m_mode;
        QString m_lastRequestLine;
        QString m_lastRequestPath;
        int m_requestCount = 0;
        QList<QPointer<QTcpSocket>> m_pendingSockets;
};

class NetworkDispatcherTest : public QObject {
        Q_OBJECT

    private slots:
        void initTestCase();
        void cleanupTestCase();

        void dispatchSucceedsOn2xx();
        void dispatchFailsWhenServerClosesConnection();
        void dispatchFailsOnTimeout();
        void dispatchFailsOnNon2xxResponse();
        void non2xxResponseWithoutQtErrorIsFailure();
        void tlsHandshakeFailureReportsSslError();
        void circuitBreakerOpensAfterConsecutiveFailures();
        void circuitBreakerBlocksDispatchWhenOpen();
        void circuitBreakerResetsOnManualReset();
        void circuitBreakerResetsOnConfigChange();
        void circuitBreakerResetsOnSuccess();
        void circuitBreakerRecoversAfterCooldown();
        void failedProbeRestartsCooldown_data();
        void failedProbeRestartsCooldown();
        void onlyOneRecoveryProbeIsAllowed();
        void zeroCooldownRequiresManualReset();
        void manualResetStopsCooldown();
        void manualResetInvalidatesPendingProbe();
        void configCanDisableRecovery_data();
        void configCanDisableRecovery();
        void cooldownChangeAppliesToNextCooldown();
        void pendingRepliesDoNotAffectRecovery_data();
        void pendingRepliesDoNotAffectRecovery();
        void invalidUrlDoesNotConsumeRecoveryProbe();
        void cancelledProbeReleasesRecoverySlot();
        void enablingCooldownRecoversOpenBreaker();
        void staleFailureCannotReopenClosedBreaker_data();
        void staleFailureCannotReopenClosedBreaker();
        void cacheBusterRandIsAdded();
        void customNetworkAccessManagerIsUsed();
        void configDefaultsAreSafe();
        void destructionWithPendingRepliesDoesNotCrash();
        void destructionDeletesPendingRepliesFromExternalManager();
};

void NetworkDispatcherTest::initTestCase() {
    // Ensure the logging category is registered even if QT_LOGGING_RULES is not set.
    QLoggingCategory::setFilterRules(QStringLiteral("matomo.sdk.debug=true"));
}

void NetworkDispatcherTest::cleanupTestCase() {}

void NetworkDispatcherTest::dispatchSucceedsOn2xx() {
    TestHttpServer server(TestHttpServer::Mode::Respond200);
    QVERIFY(server.start());

    NetworkDispatcher dispatcher;
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    dispatcher.dispatch(server.url());

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    const auto result = finishedSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(result.status, DispatchStatus::Value::Success);
    QCOMPARE(result.httpStatus, 200);
    QVERIFY(result.success());
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
}

void NetworkDispatcherTest::dispatchFailsWhenServerClosesConnection() {
    TestHttpServer server(TestHttpServer::Mode::CloseWithoutResponse);
    QVERIFY(server.start());

    NetworkDispatcher dispatcher;
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    dispatcher.dispatch(server.url());

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    const auto result = finishedSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(result.status, DispatchStatus::Value::NetworkError);
    QCOMPARE(result.httpStatus, 0);
    QVERIFY(!result.success());
}

void NetworkDispatcherTest::dispatchFailsOnTimeout() {
    TestHttpServer server(TestHttpServer::Mode::HangWithoutResponse);
    QVERIFY(server.start());

    NetworkDispatcherConfig config;
    config.timeoutMs = 500; // Short timeout for tests
    config.maxConsecutiveFailures = 100; // Avoid circuit breaker opening during this test

    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);

    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    dispatcher.dispatch(server.url());

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    const auto result = finishedSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(result.status, DispatchStatus::Value::Timeout);
    QCOMPARE(result.httpStatus, 0);
    QVERIFY(!result.success());
}

void NetworkDispatcherTest::dispatchFailsOnNon2xxResponse() {
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());

    NetworkDispatcherConfig config;
    config.maxConsecutiveFailures = 100; // Avoid circuit breaker opening during this test

    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);

    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    dispatcher.dispatch(server.url());

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    const auto result = finishedSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(result.status, DispatchStatus::Value::NetworkError);
    QCOMPARE(result.httpStatus, 500);
    QVERIFY(!result.success());
}

void NetworkDispatcherTest::non2xxResponseWithoutQtErrorIsFailure() {
    // Qt reports a 304 with NoError. Catches a dispatcher that trusts NoError alone: the request
    // would be reported as sent and a recovery probe would close the breaker on a broken server.
    TestHttpServer server(TestHttpServer::Mode::Respond304);
    QVERIFY(server.start());
    NetworkDispatcherConfig config;
    config.maxConsecutiveFailures = 1;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    QVERIFY(dispatcher.dispatch(server.url()));
    QVERIFY(finishedSpy.wait(5000));

    const auto result = finishedSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(result.status, DispatchStatus::Value::NetworkError);
    QCOMPARE(result.httpStatus, 304);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
}

void NetworkDispatcherTest::tlsHandshakeFailureReportsSslError() {
    // Catches a TLS failure reported as a generic network error, or not counted as a failure:
    // the host could not tell a certificate or proxy problem from a server outage.
    if (!QSslSocket::supportsSsl()) {
        QSKIP("No TLS backend available.");
    }
    TestHttpServer server(TestHttpServer::Mode::RespondWithoutTls);
    QVERIFY(server.start());
    NetworkDispatcherConfig config;
    config.maxConsecutiveFailures = 1;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    QUrl url = server.url();
    url.setScheme(QStringLiteral("https"));
    QVERIFY(dispatcher.dispatch(url));
    QVERIFY(finishedSpy.wait(5000));

    QCOMPARE(finishedSpy.at(0).at(0).value<DispatchResult>().status, DispatchStatus::Value::SslError);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
}

void NetworkDispatcherTest::circuitBreakerOpensAfterConsecutiveFailures() {
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());

    NetworkDispatcherConfig config;
    config.timeoutMs = 1000;
    config.maxConsecutiveFailures = 3;

    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);

    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);

    // Send 3 failing requests
    for (int i = 0; i < 3; ++i) {
        QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
        dispatcher.dispatch(server.url());
        QVERIFY(finishedSpy.wait(5000));
    }

    // The circuit breaker should have opened after the 3rd failure
    QCOMPARE(breakerSpy.count(), 1);
    QCOMPARE(breakerSpy.at(0).at(0).toBool(), true);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
}

void NetworkDispatcherTest::circuitBreakerBlocksDispatchWhenOpen() {
    NetworkDispatcherConfig config;
    config.timeoutMs = 500;
    config.maxConsecutiveFailures = 1; // Open after first failure

    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);

    // Trigger one failure to open the circuit breaker
    {
        QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
        dispatcher.dispatch(QUrl(QStringLiteral("http://127.0.0.1:1/matomo.php")));
        QVERIFY(finishedSpy.wait(5000));
    }

    QVERIFY(dispatcher.isCircuitBreakerOpen());

    // Now dispatch should be blocked by the circuit breaker (no network call)
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
    dispatcher.dispatch(QUrl(QStringLiteral("http://127.0.0.1:1/matomo.php")));

    // The CircuitBreakerOpen result is emitted immediately (synchronously via
    // queued signal); we still need to process the event loop.
    QVERIFY(finishedSpy.count() == 1 || finishedSpy.wait(1000));
    QCOMPARE(finishedSpy.count(), 1);

    const auto result = finishedSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(result.status, DispatchStatus::Value::CircuitBreakerOpen);
    QVERIFY(!result.success());
}

void NetworkDispatcherTest::circuitBreakerResetsOnManualReset() {
    NetworkDispatcherConfig config;
    config.timeoutMs = 500;
    config.maxConsecutiveFailures = 1;

    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);

    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);

    // Trigger one failure to open the circuit breaker
    {
        QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
        dispatcher.dispatch(QUrl(QStringLiteral("http://127.0.0.1:1/matomo.php")));
        QVERIFY(finishedSpy.wait(5000));
    }

    QVERIFY(dispatcher.isCircuitBreakerOpen());

    // Manually reset
    dispatcher.resetCircuitBreaker();

    QCOMPARE(breakerSpy.count(), 2);
    QCOMPARE(breakerSpy.at(1).at(0).toBool(), false);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
}

void NetworkDispatcherTest::circuitBreakerResetsOnConfigChange() {
    NetworkDispatcherConfig config;
    config.timeoutMs = 500;
    config.maxConsecutiveFailures = 1;

    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);

    // Trigger one failure to open the circuit breaker
    {
        QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
        dispatcher.dispatch(QUrl(QStringLiteral("http://127.0.0.1:1/matomo.php")));
        QVERIFY(finishedSpy.wait(5000));
    }

    QVERIFY(dispatcher.isCircuitBreakerOpen());

    QSignalSpy configSpy(&dispatcher, &NetworkDispatcher::configChanged);
    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);

    // Change config (this should reset the circuit breaker)
    NetworkDispatcherConfig newConfig;
    newConfig.timeoutMs = 2000;
    newConfig.maxConsecutiveFailures = 10;
    dispatcher.setConfig(newConfig);

    QCOMPARE(configSpy.count(), 1);
    QCOMPARE(breakerSpy.count(), 1);
    QCOMPARE(breakerSpy.at(0).at(0).toBool(), false);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
    QCOMPARE(dispatcher.config().timeoutMs, 2000);
    QCOMPARE(dispatcher.config().maxConsecutiveFailures, 10);
}

void NetworkDispatcherTest::circuitBreakerResetsOnSuccess() {
    TestHttpServer server(TestHttpServer::Mode::Respond200);
    QVERIFY(server.start());

    NetworkDispatcherConfig config;
    config.timeoutMs = 500;
    config.maxConsecutiveFailures = 5;

    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);

    // Fail enough to get close to the threshold but not open the breaker
    for (int i = 0; i < 4; ++i) {
        QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
        dispatcher.dispatch(QUrl(QStringLiteral("http://127.0.0.1:1/matomo.php")));
        QVERIFY(finishedSpy.wait(5000));
    }

    QVERIFY(!dispatcher.isCircuitBreakerOpen());

    // Now a success should reset the consecutive failure count
    {
        QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
        dispatcher.dispatch(server.url());
        QVERIFY(finishedSpy.wait(5000));
        const auto result = finishedSpy.at(0).at(0).value<DispatchResult>();
        QCOMPARE(result.status, DispatchStatus::Value::Success);
    }

    QVERIFY(!dispatcher.isCircuitBreakerOpen());

    // After the success, we should be able to fail at least once without
    // opening the breaker (the count was reset).
    {
        QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
        dispatcher.dispatch(QUrl(QStringLiteral("http://127.0.0.1:1/matomo.php")));
        QVERIFY(finishedSpy.wait(5000));
    }

    QVERIFY(!dispatcher.isCircuitBreakerOpen());
}

void NetworkDispatcherTest::circuitBreakerRecoversAfterCooldown() {
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());

    NetworkDispatcherConfig config;
    config.maxConsecutiveFailures = 2;
    config.circuitBreakerCooldownMs = 20;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);

    for (int i = 0; i < config.maxConsecutiveFailures; ++i) {
        QVERIFY(dispatcher.dispatch(server.url()));
        QTRY_COMPARE(finishedSpy.count(), i + 1);
    }
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    QCOMPARE(breakerSpy.count(), 1);
    QCOMPARE(breakerSpy.at(0).at(0).toBool(), true);

    server.setMode(TestHttpServer::Mode::Respond200);
    QTest::qWait(50);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    QCOMPARE(server.requestCount(), 2); // Recovery waits for the next dispatch.
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 3);
    QCOMPARE(finishedSpy.at(2).at(0).value<DispatchResult>().status, DispatchStatus::Value::Success);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
    QCOMPARE(breakerSpy.count(), 2);
    QCOMPARE(breakerSpy.at(1).at(0).toBool(), false);

    // Recovery also resets the consecutive failure count.
    server.setMode(TestHttpServer::Mode::Respond500);
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 4);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
    QCOMPARE(breakerSpy.count(), 2);
}

void NetworkDispatcherTest::failedProbeRestartsCooldown_data() {
    QTest::addColumn<int>("mode");
    QTest::addColumn<DispatchStatus::Value>("expectedStatus");
    QTest::newRow("http-error") << int(TestHttpServer::Mode::Respond500) << DispatchStatus::Value::NetworkError;
    QTest::newRow("network-error") << int(TestHttpServer::Mode::CloseWithoutResponse) << DispatchStatus::Value::NetworkError;
    QTest::newRow("timeout") << int(TestHttpServer::Mode::HangWithoutResponse) << DispatchStatus::Value::Timeout;
}

void NetworkDispatcherTest::failedProbeRestartsCooldown() {
    QFETCH(int, mode);
    QFETCH(DispatchStatus::Value, expectedStatus);
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());

    NetworkDispatcherConfig config;
    config.timeoutMs = 50;
    config.maxConsecutiveFailures = 1;
    config.circuitBreakerCooldownMs = 20;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);

    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 1);
    server.setMode(static_cast<TestHttpServer::Mode>(mode));
    QTest::qWait(50);
    QVERIFY(dispatcher.dispatch(server.url()));
    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 2);
    QCOMPARE(finishedSpy.at(1).at(0).value<DispatchResult>().status, expectedStatus);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    QCOMPARE(breakerSpy.count(), 1);

    QVERIFY(!dispatcher.dispatch(server.url()));
    QCOMPARE(finishedSpy.count(), 3);
    QCOMPARE(finishedSpy.last().at(0).value<DispatchResult>().status, DispatchStatus::Value::CircuitBreakerOpen);
    server.setMode(TestHttpServer::Mode::Respond200);
    QTest::qWait(50);
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 4);
    QCOMPARE(finishedSpy.at(3).at(0).value<DispatchResult>().status, DispatchStatus::Value::Success);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
    QCOMPARE(breakerSpy.count(), 2);
    QCOMPARE(breakerSpy.at(1).at(0).toBool(), false);
}

void NetworkDispatcherTest::onlyOneRecoveryProbeIsAllowed() {
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());
    NetworkDispatcherConfig config;
    config.timeoutMs = 0;
    config.maxConsecutiveFailures = 1;
    config.circuitBreakerCooldownMs = 20;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);

    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 1);
    server.setMode(TestHttpServer::Mode::HangWithoutResponse);
    QTest::qWait(50);
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(server.requestCount(), 2);
    QVERIFY(!dispatcher.dispatch(server.url()));
    QCOMPARE(finishedSpy.count(), 2);
    QCOMPARE(finishedSpy.at(1).at(0).value<DispatchResult>().status, DispatchStatus::Value::CircuitBreakerOpen);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    QCOMPARE(breakerSpy.count(), 1);

    server.respondToPendingRequests(TestHttpServer::Mode::Respond200);
    QTRY_COMPARE(finishedSpy.count(), 3);
    QCOMPARE(server.requestCount(), 2);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
}

void NetworkDispatcherTest::zeroCooldownRequiresManualReset() {
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());
    NetworkDispatcherConfig config;
    config.maxConsecutiveFailures = 1;
    config.circuitBreakerCooldownMs = 0;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);

    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 1);
    server.setMode(TestHttpServer::Mode::Respond200);
    for (int i = 0; i < 3; ++i) {
        QTest::qWait(50);
        QVERIFY(!dispatcher.dispatch(server.url()));
        QCOMPARE(finishedSpy.last().at(0).value<DispatchResult>().status, DispatchStatus::Value::CircuitBreakerOpen);
    }
    QCOMPARE(server.requestCount(), 1);
    QCOMPARE(breakerSpy.count(), 1);
    QVERIFY(dispatcher.isCircuitBreakerOpen());

    dispatcher.resetCircuitBreaker();
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 5);
    QCOMPARE(finishedSpy.last().at(0).value<DispatchResult>().status, DispatchStatus::Value::Success);
    QCOMPARE(breakerSpy.count(), 2);
}

void NetworkDispatcherTest::manualResetStopsCooldown() {
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());
    NetworkDispatcherConfig config;
    config.maxConsecutiveFailures = 1;
    config.circuitBreakerCooldownMs = 50;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);

    QVERIFY(dispatcher.dispatch(server.url()));
    QVERIFY(finishedSpy.wait(5000));
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    dispatcher.resetCircuitBreaker();
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
    QCOMPARE(breakerSpy.count(), 2);
    QCOMPARE(breakerSpy.at(1).at(0).toBool(), false);
    QTest::qWait(100);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
    QCOMPARE(server.requestCount(), 1);
    QCOMPARE(finishedSpy.count(), 1);

    server.setMode(TestHttpServer::Mode::Respond200);
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 2);
    QCOMPARE(breakerSpy.count(), 2);
}

void NetworkDispatcherTest::manualResetInvalidatesPendingProbe() {
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());
    NetworkDispatcherConfig config;
    config.timeoutMs = 0;
    config.maxConsecutiveFailures = 1;
    config.circuitBreakerCooldownMs = 20;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);

    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 1);
    QTest::qWait(50);
    server.setMode(TestHttpServer::Mode::HangWithoutResponse);
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(server.requestCount(), 2);
    dispatcher.resetCircuitBreaker();
    QVERIFY(!dispatcher.isCircuitBreakerOpen());

    server.setMode(TestHttpServer::Mode::Respond500);
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 2);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    server.respondToPendingRequests(TestHttpServer::Mode::Respond200);
    QTRY_COMPARE(finishedSpy.count(), 3);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    QCOMPARE(breakerSpy.count(), 3); // The old probe cannot close the new circuit.

    QTest::qWait(50);
    server.setMode(TestHttpServer::Mode::Respond200);
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 4);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
    QCOMPARE(breakerSpy.count(), 4);
}

void NetworkDispatcherTest::configCanDisableRecovery_data() {
    QTest::addColumn<bool>("halfOpen");
    QTest::newRow("during-cooldown") << false;
    QTest::newRow("after-cooldown") << true;
}

void NetworkDispatcherTest::configCanDisableRecovery() {
    QFETCH(bool, halfOpen);
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());
    NetworkDispatcherConfig config;
    config.maxConsecutiveFailures = 1;
    config.circuitBreakerCooldownMs = 20;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);
    QSignalSpy configSpy(&dispatcher, &NetworkDispatcher::configChanged);

    QVERIFY(dispatcher.dispatch(server.url()));
    QVERIFY(finishedSpy.wait(5000));
    if (halfOpen) {
        QTest::qWait(50);
    }
    config.circuitBreakerCooldownMs = 0;
    dispatcher.setConfig(config);
    QCOMPARE(configSpy.count(), 1);
    QCOMPARE(dispatcher.config().circuitBreakerCooldownMs, 0);
    QTest::qWait(100);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    QVERIFY(!dispatcher.dispatch(server.url()));
    QCOMPARE(finishedSpy.last().at(0).value<DispatchResult>().status, DispatchStatus::Value::CircuitBreakerOpen);
    QCOMPARE(breakerSpy.count(), 1);
    QCOMPARE(server.requestCount(), 1);
}

void NetworkDispatcherTest::cooldownChangeAppliesToNextCooldown() {
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());
    NetworkDispatcherConfig config;
    config.maxConsecutiveFailures = 1;
    config.circuitBreakerCooldownMs = 20;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);

    QVERIFY(dispatcher.dispatch(server.url()));
    QVERIFY(finishedSpy.wait(5000));
    config.circuitBreakerCooldownMs = 60000;
    dispatcher.setConfig(config);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    QTest::qWait(50);
    QVERIFY(dispatcher.dispatch(server.url())); // The original 20 ms cooldown still applies.
    QTRY_COMPARE(finishedSpy.count(), 2);
    QTest::qWait(50);
    QVERIFY(!dispatcher.dispatch(server.url())); // The failed probe started the new cooldown.
    QCOMPARE(finishedSpy.last().at(0).value<DispatchResult>().status, DispatchStatus::Value::CircuitBreakerOpen);
    QCOMPARE(breakerSpy.count(), 1);
    QCOMPARE(server.requestCount(), 2);
}

void NetworkDispatcherTest::pendingRepliesDoNotAffectRecovery_data() {
    QTest::addColumn<int>("mode");
    QTest::newRow("late-success") << int(TestHttpServer::Mode::Respond200);
    QTest::newRow("late-failure") << int(TestHttpServer::Mode::Respond500);
}

void NetworkDispatcherTest::pendingRepliesDoNotAffectRecovery() {
    QFETCH(int, mode);
    TestHttpServer pendingServer(TestHttpServer::Mode::HangWithoutResponse);
    TestHttpServer probeServer(TestHttpServer::Mode::Respond500);
    QVERIFY(pendingServer.start());
    QVERIFY(probeServer.start());
    NetworkDispatcherConfig config;
    config.timeoutMs = 0;
    config.maxConsecutiveFailures = 1;
    config.circuitBreakerCooldownMs = 20;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);

    QVERIFY(dispatcher.dispatch(pendingServer.url()));
    QTRY_COMPARE(pendingServer.requestCount(), 1);
    QVERIFY(dispatcher.dispatch(probeServer.url()));
    QTRY_COMPARE(finishedSpy.count(), 1);
    QTest::qWait(50);
    probeServer.setMode(TestHttpServer::Mode::HangWithoutResponse);
    QVERIFY(dispatcher.dispatch(probeServer.url()));
    QTRY_COMPARE(probeServer.requestCount(), 2);

    pendingServer.respondToPendingRequests(static_cast<TestHttpServer::Mode>(mode));
    QTRY_COMPARE(finishedSpy.count(), 2);
    QTest::qWait(50);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    QVERIFY(!dispatcher.dispatch(probeServer.url()));
    QCOMPARE(finishedSpy.last().at(0).value<DispatchResult>().status, DispatchStatus::Value::CircuitBreakerOpen);
    QCOMPARE(breakerSpy.count(), 1);

    probeServer.respondToPendingRequests(TestHttpServer::Mode::Respond200);
    QTRY_COMPARE(finishedSpy.count(), 4);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
    QCOMPARE(breakerSpy.count(), 2);
    QCOMPARE(probeServer.requestCount(), 2);
}

void NetworkDispatcherTest::invalidUrlDoesNotConsumeRecoveryProbe() {
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());
    NetworkDispatcherConfig config;
    config.maxConsecutiveFailures = 1;
    config.circuitBreakerCooldownMs = 20;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 1);
    QTest::qWait(50);
    QVERIFY(!dispatcher.dispatch(QUrl()));
    QCOMPARE(finishedSpy.count(), 2);
    QCOMPARE(finishedSpy.last().at(0).value<DispatchResult>().status, DispatchStatus::Value::NetworkError);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    server.setMode(TestHttpServer::Mode::Respond200);
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 3);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
}

void NetworkDispatcherTest::cancelledProbeReleasesRecoverySlot() {
    // Catches a breaker stuck open forever: a probe aborted from outside the dispatcher used to
    // keep the probe slot taken, so every later dispatch was rejected.
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());
    QNetworkAccessManager networkAccessManager;
    NetworkDispatcherConfig config;
    config.timeoutMs = 0;
    config.maxConsecutiveFailures = 1;
    config.circuitBreakerCooldownMs = 20;
    NetworkDispatcher dispatcher(&networkAccessManager);
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 1);
    QVERIFY(dispatcher.isCircuitBreakerOpen());

    server.setMode(TestHttpServer::Mode::HangWithoutResponse);
    QTest::qWait(50);
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(server.requestCount(), 2);

    const auto replies = networkAccessManager.findChildren<QNetworkReply *>();
    const auto probe = std::ranges::find_if(replies, [](const QNetworkReply *reply) { return !reply->isFinished(); });
    QVERIFY(probe != replies.end());
    (*probe)->abort();
    QTRY_COMPARE(finishedSpy.count(), 2);
    QCOMPARE(finishedSpy.last().at(0).value<DispatchResult>().status, DispatchStatus::Value::NetworkError);
    QVERIFY(dispatcher.isCircuitBreakerOpen());

    server.setMode(TestHttpServer::Mode::Respond200);
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 3);
    QCOMPARE(finishedSpy.last().at(0).value<DispatchResult>().status, DispatchStatus::Value::Success);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
}

void NetworkDispatcherTest::enablingCooldownRecoversOpenBreaker() {
    // Catches a breaker that opened while recovery was disabled and never recovered after a
    // positive cooldown was configured, leaving manual reset as the only way out.
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());
    NetworkDispatcherConfig config;
    config.maxConsecutiveFailures = 1;
    config.circuitBreakerCooldownMs = 0;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 1);
    QVERIFY(dispatcher.isCircuitBreakerOpen());

    config.circuitBreakerCooldownMs = 20;
    dispatcher.setConfig(config);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    server.setMode(TestHttpServer::Mode::Respond200);
    QTest::qWait(50);
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 2);
    QCOMPARE(finishedSpy.last().at(0).value<DispatchResult>().status, DispatchStatus::Value::Success);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
}

void NetworkDispatcherTest::staleFailureCannotReopenClosedBreaker_data() {
    QTest::addColumn<bool>("manualReset");
    QTest::newRow("sent-before-opening-then-probe-succeeds") << false;
    QTest::newRow("probe-pending-during-manual-reset") << true;
}

void NetworkDispatcherTest::staleFailureCannotReopenClosedBreaker() {
    // Catches a closed breaker reopened by a reply sent before it last opened or was reset: with
    // maxConsecutiveFailures = 1, that late failure blocked healthy requests for a whole cooldown.
    QFETCH(bool, manualReset);
    TestHttpServer staleServer(TestHttpServer::Mode::HangWithoutResponse);
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(staleServer.start());
    QVERIFY(server.start());
    NetworkDispatcherConfig config;
    config.timeoutMs = 0;
    config.maxConsecutiveFailures = 1;
    config.circuitBreakerCooldownMs = 20;
    NetworkDispatcher dispatcher;
    dispatcher.setConfig(config);
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);
    QSignalSpy breakerSpy(&dispatcher, &NetworkDispatcher::circuitBreakerChanged);

    if (!manualReset) {
        QVERIFY(dispatcher.dispatch(staleServer.url()));
        QTRY_COMPARE(staleServer.requestCount(), 1);
    }
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), 1);
    QVERIFY(dispatcher.isCircuitBreakerOpen());
    QTest::qWait(50);

    if (manualReset) {
        QVERIFY(dispatcher.dispatch(staleServer.url())); // Recovery probe.
        QTRY_COMPARE(staleServer.requestCount(), 1);
        dispatcher.resetCircuitBreaker();
    } else {
        server.setMode(TestHttpServer::Mode::Respond200);
        QVERIFY(dispatcher.dispatch(server.url())); // Recovery probe.
        QTRY_COMPARE(finishedSpy.count(), 2);
    }
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
    QCOMPARE(breakerSpy.count(), 2);

    const qsizetype resultsBeforeStaleFailure = finishedSpy.count();
    staleServer.respondToPendingRequests(TestHttpServer::Mode::Respond500);
    QTRY_COMPARE(finishedSpy.count(), resultsBeforeStaleFailure + 1);
    QCOMPARE(finishedSpy.last().at(0).value<DispatchResult>().status, DispatchStatus::Value::NetworkError);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
    QCOMPARE(breakerSpy.count(), 2);

    server.setMode(TestHttpServer::Mode::Respond200);
    QVERIFY(dispatcher.dispatch(server.url()));
    QTRY_COMPARE(finishedSpy.count(), resultsBeforeStaleFailure + 2);
    QCOMPARE(finishedSpy.last().at(0).value<DispatchResult>().status, DispatchStatus::Value::Success);
}

void NetworkDispatcherTest::cacheBusterRandIsAdded() {
    TestHttpServer server(TestHttpServer::Mode::Respond200);
    QVERIFY(server.start());

    NetworkDispatcher dispatcher;
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    dispatcher.dispatch(server.url());

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    // Verify the server received a request with the rand parameter
    const QString requestPath = server.lastRequestPath();
    QVERIFY(requestPath.contains(QStringLiteral("rand="), Qt::CaseInsensitive));
}

void NetworkDispatcherTest::customNetworkAccessManagerIsUsed() {
    TestHttpServer server(TestHttpServer::Mode::Respond200);
    QVERIFY(server.start());

    auto *networkAccessManager = new QNetworkAccessManager;
    NetworkDispatcher dispatcher(networkAccessManager);

    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    dispatcher.dispatch(server.url());

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    const auto result = finishedSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(result.status, DispatchStatus::Value::Success);
    QCOMPARE(result.httpStatus, 200);

    // The dispatcher should not own the externally provided QNetworkAccessManager.
    delete networkAccessManager;
}

void NetworkDispatcherTest::configDefaultsAreSafe() {
    NetworkDispatcherConfig config;
    QCOMPARE(config.timeoutMs, 10000);
    QCOMPARE(config.maxConsecutiveFailures, 5);
    QCOMPARE(config.circuitBreakerCooldownMs, 60000);

    NetworkDispatcher dispatcher;
    QCOMPARE(dispatcher.config().timeoutMs, 10000);
    QCOMPARE(dispatcher.config().maxConsecutiveFailures, 5);
    QCOMPARE(dispatcher.config().circuitBreakerCooldownMs, 60000);
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
}

void NetworkDispatcherTest::destructionWithPendingRepliesDoesNotCrash() {
    TestHttpServer server(TestHttpServer::Mode::HangWithoutResponse);
    QVERIFY(server.start());

    {
        NetworkDispatcherConfig config;
        config.timeoutMs = 30000; // Long timeout so replies are still pending on destruction

        NetworkDispatcher dispatcher;
        dispatcher.setConfig(config);

        // Dispatch multiple requests
        dispatcher.dispatch(server.url());
        dispatcher.dispatch(server.url());
        dispatcher.dispatch(server.url());

        // Destroy the dispatcher while replies are still pending.
        // This should not crash.
    }

    QVERIFY(true); // If we reach here, the test passed.
}

void NetworkDispatcherTest::destructionDeletesPendingRepliesFromExternalManager() {
    TestHttpServer server(TestHttpServer::Mode::HangWithoutResponse);
    QVERIFY(server.start());

    QNetworkAccessManager networkAccessManager;
    QPointer<QNetworkReply> pendingReply;
    {
        NetworkDispatcher dispatcher(&networkAccessManager);
        dispatcher.dispatch(server.url());

        const auto replies = networkAccessManager.findChildren<QNetworkReply *>();
        QCOMPARE(replies.size(), 1);
        pendingReply = replies.constFirst();
    }

    QTRY_VERIFY(pendingReply.isNull());
}

} // namespace

QTEST_MAIN(NetworkDispatcherTest)
#include "tst_networkdispatcher.moc"
