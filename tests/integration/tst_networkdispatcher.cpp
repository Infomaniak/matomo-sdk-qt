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

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

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
            HangWithoutResponse,
        };

        explicit TestHttpServer(Mode mode = Mode::Respond200, QObject *parent = nullptr)
            : QObject(parent), m_mode(mode) {}

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

        void setMode(Mode mode) { m_mode = mode; }

    private:
        void handleConnection() {
            while (m_server->hasPendingConnections()) {
                QTcpSocket *socket = m_server->nextPendingConnection();
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                    readRequest(socket);
                });
            }
        }

        void readRequest(QTcpSocket *socket) {
            QByteArray data = socket->property("requestData").toByteArray();
            data += socket->readAll();
            if (!data.contains("\r\n\r\n")) {
                socket->setProperty("requestData", data);
                return;
            }

            const QString request = QString::fromUtf8(data);
            m_lastRequestLine = request.section(QLatin1Char('\n'), 0, 0).trimmed();
            m_lastRequestPath = m_lastRequestLine.section(QLatin1Char(' '), 1, 1);

            switch (m_mode) {
                case Mode::Respond200:
                    socket->write("HTTP/1.1 200 OK\r\n"
                                 "Content-Type: text/plain\r\n"
                                 "Content-Length: 2\r\n"
                                 "Connection: close\r\n"
                                 "\r\n"
                                 "OK");
                    socket->disconnectFromHost();
                    break;
                case Mode::Respond500:
                    socket->write("HTTP/1.1 500 Internal Server Error\r\n"
                                 "Content-Type: text/plain\r\n"
                                 "Content-Length: 5\r\n"
                                 "Connection: close\r\n"
                                 "\r\n"
                                 "Error");
                    socket->disconnectFromHost();
                    break;
                case Mode::HangWithoutResponse:
                    // Intentionally do nothing; let the timeout fire.
                    break;
            }
        }

        QTcpServer *m_server = nullptr;
        Mode m_mode;
        QString m_lastRequestLine;
        QString m_lastRequestPath;
};

class NetworkDispatcherTest : public QObject {
        Q_OBJECT

    private slots:
        void initTestCase();
        void cleanupTestCase();

        void dispatchSucceedsOn2xx();
        void dispatchFailsOnConnectionRefused();
        void dispatchFailsOnTimeout();
        void dispatchFailsOnNon2xxResponse();
        void circuitBreakerOpensAfterConsecutiveFailures();
        void circuitBreakerBlocksDispatchWhenOpen();
        void circuitBreakerResetsOnManualReset();
        void circuitBreakerResetsOnConfigChange();
        void circuitBreakerResetsOnSuccess();
        void cacheBusterRandIsAdded();
        void customNetworkAccessManagerIsUsed();
        void configDefaultsAreSafe();
        void destructionWithPendingRepliesDoesNotCrash();
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
    QCOMPARE(result.status, DispatchResult::Status::Success);
    QCOMPARE(result.httpStatus, 200);
    QVERIFY(result.success());
    QVERIFY(!dispatcher.isCircuitBreakerOpen());
}

void NetworkDispatcherTest::dispatchFailsOnConnectionRefused() {
    NetworkDispatcher dispatcher;
    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    // Dispatch to a port where nothing is listening (port 1 is reserved and
    // typically not in use).
    QUrl url(QStringLiteral("http://127.0.0.1:1/matomo.php"));
    dispatcher.dispatch(url);

    // Connection refused may be reported quickly; allow up to 5 s.
    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    const auto result = finishedSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(result.status, DispatchResult::Status::NetworkError);
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
    QCOMPARE(result.status, DispatchResult::Status::Timeout);
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
    QCOMPARE(result.status, DispatchResult::Status::NetworkError);
    QCOMPARE(result.httpStatus, 500);
    QVERIFY(!result.success());
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
    QCOMPARE(result.status, DispatchResult::Status::CircuitBreakerOpen);
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
        QCOMPARE(result.status, DispatchResult::Status::Success);
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

    auto *nam = new QNetworkAccessManager;
    NetworkDispatcher dispatcher(nam);

    QSignalSpy finishedSpy(&dispatcher, &NetworkDispatcher::dispatchFinished);

    dispatcher.dispatch(server.url());

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    const auto result = finishedSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(result.status, DispatchResult::Status::Success);
    QCOMPARE(result.httpStatus, 200);

    // The dispatcher should not own the externally provided NAM.
    delete nam;
}

void NetworkDispatcherTest::configDefaultsAreSafe() {
    NetworkDispatcherConfig config;
    QCOMPARE(config.timeoutMs, 10000);
    QCOMPARE(config.maxConsecutiveFailures, 5);

    NetworkDispatcher dispatcher;
    QCOMPARE(dispatcher.config().timeoutMs, 10000);
    QCOMPARE(dispatcher.config().maxConsecutiveFailures, 5);
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

} // namespace

QTEST_MAIN(NetworkDispatcherTest)
#include "tst_networkdispatcher.moc"
