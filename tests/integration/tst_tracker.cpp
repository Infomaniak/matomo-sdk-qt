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
#include <MatomoQt/Event.h>
#include <MatomoQt/InMemoryClientIdStore.h>
#include <MatomoQt/InMemoryConsentStore.h>
#include <MatomoQt/PageView.h>
#include <MatomoQt/RequestResult.h>
#include <MatomoQt/Tracker.h>
#include <MatomoQt/TrackerConfig.h>
#include <MatomoQt/TrackerStats.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>
#include <QtCore/QUrlQuery>

using namespace MatomoQt;

namespace {

class TestHttpServer : public QObject {
        Q_OBJECT

    public:
        enum class Mode {
            Respond200,
            Respond500,
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

        [[nodiscard]] QString lastRequestPath() const { return m_lastRequestPath; }
        [[nodiscard]] int requestCount() const { return m_requestCount; }

        void setMode(const Mode mode) { m_mode = mode; }

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
            const QString requestLine = request.section(QLatin1Char('\n'), 0, 0).trimmed();
            m_lastRequestPath = requestLine.section(QLatin1Char(' '), 1, 1);
            m_requestCount++;

            if (m_mode == Mode::Respond200) {
                socket->write("HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 2\r\n"
                              "Connection: close\r\n"
                              "\r\n"
                              "OK");
            } else {
                socket->write("HTTP/1.1 500 Internal Server Error\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 5\r\n"
                              "Connection: close\r\n"
                              "\r\n"
                              "Error");
            }
            socket->disconnectFromHost();
        }

        QTcpServer *m_server = nullptr;
        Mode m_mode;
        QString m_lastRequestPath;
        int m_requestCount = 0;
};

TrackerConfig validConfig(const QUrl &endpoint) {
    TrackerConfig config;
    config.endpoint = endpoint;
    config.actionUrlBase = QUrl(QStringLiteral("app://desktop/"));
    config.siteId = 1;
    config.privacyMode = PrivacyMode::Value::ConsentExemptWithOptOut;
    return config;
}

struct DefaultStores {
    InMemoryConsentStore consent;
    InMemoryClientIdStore clientId;
};

QUrlQuery queryFor(const QString &requestPath) {
    const int queryStart = requestPath.indexOf(QLatin1Char('?'));
    if (queryStart < 0) {
        return {};
    }
    return QUrlQuery(requestPath.mid(queryStart + 1));
}

QString queryValue(const QUrlQuery &query, const QString &key) {
    const auto items = query.queryItems(QUrl::FullyDecoded);
    for (const auto &[k, v]: items) {
        if (k == key) {
            return v;
        }
    }
    return {};
}

bool hasQueryItem(const QUrlQuery &query, const QString &key) {
    const auto items = query.queryItems(QUrl::FullyDecoded);
    for (const auto &[k, v]: items) {
        if (k == key) {
            return true;
        }
    }
    return false;
}

class TrackerIntegrationTest : public QObject {
        Q_OBJECT

    private slots:
        void initTestCase();
        void cleanupTestCase();

        void trackPageViewDispatchesToServer();
        void trackEventDispatchesToServer();
        void sendPingDispatchesToServer();
        void pingCarriesLastPageViewPath();
        void pingWithoutPageViewFallsBackToBase();
        void blockedByPrivacyDoesNotReachDispatcher();
        void disabledTrackerDoesNotDispatch();
        void pageViewIdIsPresentOnPageView();
        void pageViewIdIsReusedOnEvent();
        void pageViewIdClearedAfterConsentDenial();
        void forceNewVisitAddsNewVisitParam();
        void customDimensionAppearsInRequest();
        void clearCustomDimensionRemovesFromRequest();
        void pingDoesNotCarryCustomDimensions();
        void runtimeStatsAreAccurate();
        void clientIdIsAutoGenerated();
        void dispatchFinishedSignalEmitted();
        void failedCountIncrementedOnFailure();
        void circuitBreakerDoesNotCommitUndispatchedState();
};

void TrackerIntegrationTest::initTestCase() {
    QLoggingCategory::setFilterRules(QStringLiteral("matomo.sdk.debug=true"));
}

void TrackerIntegrationTest::cleanupTestCase() {}

void TrackerIntegrationTest::trackPageViewDispatchesToServer() {
    TestHttpServer server;
    QVERIFY(server.start());

    DefaultStores stores;
    Tracker tracker(validConfig(server.url()), stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    const auto result = tracker.trackPageView({.path = QStringLiteral("settings"), .actionName = QStringLiteral("Settings")});

    QVERIFY(result.accepted());
    QVERIFY(dispatchSpy.wait(5000));
    QCOMPARE(dispatchSpy.count(), 1);

    const auto dispatchResult = dispatchSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(dispatchResult.status, DispatchStatus::Value::Success);

    QVERIFY(!server.lastRequestPath().isEmpty());
    const auto query = queryFor(server.lastRequestPath());
    QCOMPARE(queryValue(query, QStringLiteral("action_name")), QStringLiteral("Settings"));
    QCOMPARE(queryValue(query, QStringLiteral("idsite")), QStringLiteral("1"));
}

void TrackerIntegrationTest::trackEventDispatchesToServer() {
    TestHttpServer server;
    QVERIFY(server.start());

    DefaultStores stores;
    Tracker tracker(validConfig(server.url()), stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    const auto result = tracker.trackEvent({
        .category = QStringLiteral("preferences"),
        .action = QStringLiteral("click"),
        .name = QStringLiteral("saveButton"),
    });

    QVERIFY(result.accepted());
    QVERIFY(dispatchSpy.wait(5000));
    QCOMPARE(dispatchSpy.count(), 1);

    const auto dispatchResult = dispatchSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(dispatchResult.status, DispatchStatus::Value::Success);

    const auto query = queryFor(server.lastRequestPath());
    QCOMPARE(queryValue(query, QStringLiteral("e_c")), QStringLiteral("preferences"));
    QCOMPARE(queryValue(query, QStringLiteral("e_a")), QStringLiteral("click"));
    QCOMPARE(queryValue(query, QStringLiteral("e_n")), QStringLiteral("saveButton"));
}

void TrackerIntegrationTest::sendPingDispatchesToServer() {
    TestHttpServer server;
    QVERIFY(server.start());

    DefaultStores stores;
    Tracker tracker(validConfig(server.url()), stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    const auto result = tracker.sendPing();

    QVERIFY(result.accepted());
    QVERIFY(dispatchSpy.wait(5000));
    QCOMPARE(dispatchSpy.count(), 1);

    const auto dispatchResult = dispatchSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(dispatchResult.status, DispatchStatus::Value::Success);

    const auto query = queryFor(server.lastRequestPath());
    QVERIFY(hasQueryItem(query, QStringLiteral("ping")));
}

void TrackerIntegrationTest::pingCarriesLastPageViewPath() {
    TestHttpServer server;
    QVERIFY(server.start());

    DefaultStores stores;
    Tracker tracker(validConfig(server.url()), stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    (void) tracker.trackPageView({.path = QStringLiteral("settings")});
    QVERIFY(dispatchSpy.wait(5000));

    const auto pvQuery = queryFor(server.lastRequestPath());
    const QString pvUrl = queryValue(pvQuery, QStringLiteral("url"));
    QVERIFY(pvUrl.endsWith(QStringLiteral("/settings")));

    dispatchSpy.clear();
    (void) tracker.sendPing();
    QVERIFY(dispatchSpy.wait(5000));

    const auto pingQuery = queryFor(server.lastRequestPath());
    QVERIFY(hasQueryItem(pingQuery, QStringLiteral("ping")));
    const QString pingUrl = queryValue(pingQuery, QStringLiteral("url"));
    QCOMPARE(pingUrl, pvUrl);
}

void TrackerIntegrationTest::pingWithoutPageViewFallsBackToBase() {
    TestHttpServer server;
    QVERIFY(server.start());

    DefaultStores stores;
    Tracker tracker(validConfig(server.url()), stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    const auto res = tracker.sendPing();
    QVERIFY(res.accepted());
    QVERIFY(dispatchSpy.wait(5000));

    const auto query = queryFor(server.lastRequestPath());
    QVERIFY(hasQueryItem(query, QStringLiteral("ping")));
    const QString url = queryValue(query, QStringLiteral("url"));
    QCOMPARE(url, QStringLiteral("app://desktop/"));
}

void TrackerIntegrationTest::blockedByPrivacyDoesNotReachDispatcher() {
    TestHttpServer server;
    QVERIFY(server.start());

    TrackerConfig config = validConfig(server.url());
    config.privacyMode = PrivacyMode::Value::RequiresConsent;

    DefaultStores stores;
    Tracker tracker(config, stores.consent, stores.clientId);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    QSignalSpy statsSpy(&tracker, &Tracker::statsChanged);
    const auto result = tracker.trackPageView({.path = QStringLiteral("settings")});

    QVERIFY(!result.accepted());
    QCOMPARE(result.status, RequestStatus::Value::RequestBlockedByPrivacy);

    QCOMPARE(dispatchSpy.count(), 0);
    QCOMPARE(server.requestCount(), 0);

    const auto stats = tracker.stats();
    QCOMPARE(stats.blockedCount, 1);
    QCOMPARE(stats.sentCount, 0);
    QCOMPARE(statsSpy.count(), 1);
}

void TrackerIntegrationTest::disabledTrackerDoesNotDispatch() {
    TestHttpServer server;
    QVERIFY(server.start());

    DefaultStores stores;
    Tracker tracker(validConfig(server.url()), stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);
    tracker.setEnabled(false);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    const auto result = tracker.trackPageView({.path = QStringLiteral("settings")});

    QVERIFY(!result.accepted());
    QCOMPARE(result.status, RequestStatus::Value::RequestDisabled);

    QCOMPARE(dispatchSpy.count(), 0);
    QCOMPARE(server.requestCount(), 0);

    QCOMPARE(tracker.stats().blockedCount, 1);
    QCOMPARE(tracker.stats().sentCount, 0);
}

void TrackerIntegrationTest::pageViewIdIsPresentOnPageView() {
    TestHttpServer server;
    QVERIFY(server.start());

    DefaultStores stores;
    Tracker tracker(validConfig(server.url()), stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    (void) tracker.trackPageView({.path = QStringLiteral("page1")});

    QVERIFY(dispatchSpy.wait(5000));

    const auto query = queryFor(server.lastRequestPath());
    const QString pvId = queryValue(query, QStringLiteral("pv_id"));
    QVERIFY(!pvId.isEmpty());
    QCOMPARE(pvId.length(), 6);
}

void TrackerIntegrationTest::pageViewIdIsReusedOnEvent() {
    TestHttpServer server;
    QVERIFY(server.start());

    DefaultStores stores;
    Tracker tracker(validConfig(server.url()), stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    (void) tracker.trackPageView({.path = QStringLiteral("page1")});
    QVERIFY(dispatchSpy.wait(5000));

    const auto pvQuery = queryFor(server.lastRequestPath());
    const QString pageViewPvId = queryValue(pvQuery, QStringLiteral("pv_id"));

    dispatchSpy.clear();

    (void) tracker.trackEvent({.category = QStringLiteral("cat"), .action = QStringLiteral("act")});
    QVERIFY(dispatchSpy.wait(5000));

    const auto eventQuery = queryFor(server.lastRequestPath());
    const QString eventPvId = queryValue(eventQuery, QStringLiteral("pv_id"));
    QCOMPARE(eventPvId, pageViewPvId);
}

void TrackerIntegrationTest::pageViewIdClearedAfterConsentDenial() {
    TestHttpServer server;
    QVERIFY(server.start());

    TrackerConfig config = validConfig(server.url());
    config.privacyMode = PrivacyMode::Value::RequiresConsent;

    DefaultStores stores;
    Tracker tracker(config, stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    (void) tracker.trackPageView({.path = QStringLiteral("page1")});
    QVERIFY(dispatchSpy.wait(5000));

    const auto pvQuery = queryFor(server.lastRequestPath());
    const QString pageViewPvId = queryValue(pvQuery, QStringLiteral("pv_id"));
    QVERIFY(!pageViewPvId.isEmpty());

    tracker.setConsentState(ConsentState::Value::Denied);
    tracker.setConsentState(ConsentState::Value::Granted);

    dispatchSpy.clear();
    (void) tracker.trackEvent({.category = QStringLiteral("cat"), .action = QStringLiteral("act")});
    QVERIFY(dispatchSpy.wait(5000));

    const auto eventQuery = queryFor(server.lastRequestPath());
    QVERIFY(!hasQueryItem(eventQuery, QStringLiteral("pv_id")));
}

void TrackerIntegrationTest::forceNewVisitAddsNewVisitParam() {
    TestHttpServer server;
    QVERIFY(server.start());

    DefaultStores stores;
    Tracker tracker(validConfig(server.url()), stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    tracker.forceNewVisit();
    (void) tracker.trackPageView({.path = QStringLiteral("page1")});

    QVERIFY(dispatchSpy.wait(5000));

    const auto query = queryFor(server.lastRequestPath());
    QCOMPARE(queryValue(query, QStringLiteral("new_visit")), QStringLiteral("1"));

    dispatchSpy.clear();
    (void) tracker.trackPageView({.path = QStringLiteral("page2")});
    QVERIFY(dispatchSpy.wait(5000));

    const auto query2 = queryFor(server.lastRequestPath());
    QVERIFY(!hasQueryItem(query2, QStringLiteral("new_visit")));
}

void TrackerIntegrationTest::customDimensionAppearsInRequest() {
    TestHttpServer server;
    QVERIFY(server.start());

    auto config = validConfig(server.url());
    config.customDimensions[3] = QStringLiteral("beta");

    DefaultStores stores;
    Tracker tracker(config, stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    (void) tracker.trackPageView({.path = QStringLiteral("settings")});

    QVERIFY(dispatchSpy.wait(5000));

    const auto query = queryFor(server.lastRequestPath());
    QCOMPARE(queryValue(query, QStringLiteral("dimension3")), QStringLiteral("beta"));
}

void TrackerIntegrationTest::clearCustomDimensionRemovesFromRequest() {
    TestHttpServer server;
    QVERIFY(server.start());

    auto configWithoutDim = validConfig(server.url());
    DefaultStores storesWithoutDim;
    Tracker trackerWithoutDim(configWithoutDim, storesWithoutDim.consent, storesWithoutDim.clientId);
    trackerWithoutDim.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&trackerWithoutDim, &Tracker::dispatchFinished);
    (void) trackerWithoutDim.trackPageView({.path = QStringLiteral("settings")});

    QVERIFY(dispatchSpy.wait(5000));

    const auto query = queryFor(server.lastRequestPath());
    QVERIFY(!hasQueryItem(query, QStringLiteral("dimension3")));
}

void TrackerIntegrationTest::pingDoesNotCarryCustomDimensions() {
    TestHttpServer server;
    QVERIFY(server.start());

    auto config = validConfig(server.url());
    config.customDimensions[3] = QStringLiteral("beta");

    DefaultStores stores;
    Tracker tracker(config, stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    (void) tracker.sendPing();

    QVERIFY(dispatchSpy.wait(5000));

    const auto query = queryFor(server.lastRequestPath());
    QVERIFY(hasQueryItem(query, QStringLiteral("ping")));
    QVERIFY(!hasQueryItem(query, QStringLiteral("dimension3")));
}

void TrackerIntegrationTest::runtimeStatsAreAccurate() {
    TestHttpServer server;
    QVERIFY(server.start());

    TrackerConfig config = validConfig(server.url());
    config.privacyMode = PrivacyMode::Value::RequiresConsent;

    DefaultStores stores;
    Tracker tracker(config, stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    (void) tracker.trackPageView({.path = QStringLiteral("page1")});
    (void) tracker.trackPageView({.path = QStringLiteral("page2")});

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    QVERIFY(dispatchSpy.wait(5000));

    tracker.setConsentState(ConsentState::Value::Denied);
    (void) tracker.trackPageView({.path = QStringLiteral("blocked")});

    QTRY_COMPARE_WITH_TIMEOUT(dispatchSpy.count(), 2, 5000);

    const auto stats = tracker.stats();
    QCOMPARE(stats.sentCount, 2);
    QCOMPARE(stats.blockedCount, 1);
    QCOMPARE(stats.failedCount, 0);
}

void TrackerIntegrationTest::clientIdIsAutoGenerated() {
    TestHttpServer server;
    QVERIFY(server.start());

    DefaultStores stores;
    Tracker tracker(validConfig(server.url()), stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QVERIFY(tracker.clientId().isEmpty());

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    (void) tracker.trackPageView({.path = QStringLiteral("settings")});

    QVERIFY(dispatchSpy.wait(5000));

    const auto clientId = tracker.clientId();
    QVERIFY(!clientId.isEmpty());
    QCOMPARE(clientId.length(), 16);

    static const QRegularExpression hexPattern(QStringLiteral("^[0-9a-f]{16}$"));
    QVERIFY(hexPattern.match(clientId).hasMatch());

    const auto query = queryFor(server.lastRequestPath());
    QCOMPARE(queryValue(query, QStringLiteral("_id")), clientId);
}

void TrackerIntegrationTest::dispatchFinishedSignalEmitted() {
    TestHttpServer server;
    QVERIFY(server.start());

    DefaultStores stores;
    Tracker tracker(validConfig(server.url()), stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    (void) tracker.trackEvent({.category = QStringLiteral("cat"), .action = QStringLiteral("act")});

    QVERIFY(dispatchSpy.wait(5000));
    QCOMPARE(dispatchSpy.count(), 1);

    const auto result = dispatchSpy.at(0).at(0).value<DispatchResult>();
    QCOMPARE(result.status, DispatchStatus::Value::Success);
    QCOMPARE(result.httpStatus, 200);
    QVERIFY(result.success());
}

void TrackerIntegrationTest::failedCountIncrementedOnFailure() {
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());

    DefaultStores stores;
    Tracker tracker(validConfig(server.url()), stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    (void) tracker.trackPageView({.path = QStringLiteral("settings")});

    QVERIFY(dispatchSpy.wait(5000));

    const auto result = dispatchSpy.at(0).at(0).value<DispatchResult>();
    QVERIFY(!result.success());

    const auto stats = tracker.stats();
    QCOMPARE(stats.sentCount, 1);
    QCOMPARE(stats.failedCount, 1);
    QCOMPARE(stats.blockedCount, 0);
}

void TrackerIntegrationTest::circuitBreakerDoesNotCommitUndispatchedState() {
    TestHttpServer server(TestHttpServer::Mode::Respond500);
    QVERIFY(server.start());

    auto config = validConfig(server.url());
    config.networkDispatcherConfig = {.timeoutMs = 1000, .maxConsecutiveFailures = 1};
    DefaultStores stores;
    Tracker tracker(config, stores.consent, stores.clientId);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy dispatchSpy(&tracker, &Tracker::dispatchFinished);
    (void) tracker.trackEvent({.category = QStringLiteral("cat"), .action = QStringLiteral("open-breaker")});
    QVERIFY(dispatchSpy.wait(5000));
    QCOMPARE(dispatchSpy.constLast().constFirst().value<DispatchResult>().status, DispatchStatus::Value::NetworkError);
    QCOMPARE(server.requestCount(), 1);
    QCOMPARE(tracker.stats().sentCount, 1);

    tracker.forceNewVisit();
    dispatchSpy.clear();
    const auto blockedResult = tracker.trackPageView({.path = QStringLiteral("not-dispatched")});
    QVERIFY(blockedResult.accepted());
    QCOMPARE(dispatchSpy.count(), 1);
    QCOMPARE(dispatchSpy.constFirst().constFirst().value<DispatchResult>().status,
             DispatchStatus::Value::CircuitBreakerOpen);
    QCOMPARE(server.requestCount(), 1);
    QCOMPARE(tracker.stats().sentCount, 1);

    server.setMode(TestHttpServer::Mode::Respond200);
    tracker.resetCircuitBreaker();
    dispatchSpy.clear();
    (void) tracker.trackEvent({.category = QStringLiteral("cat"), .action = QStringLiteral("after-reset")});
    QVERIFY(dispatchSpy.wait(5000));
    QCOMPARE(dispatchSpy.constLast().constFirst().value<DispatchResult>().status, DispatchStatus::Value::Success);

    const auto query = queryFor(server.lastRequestPath());
    QCOMPARE(queryValue(query, QStringLiteral("new_visit")), QStringLiteral("1"));
    QVERIFY(!hasQueryItem(query, QStringLiteral("pv_id")));
    QCOMPARE(server.requestCount(), 2);
    QCOMPARE(tracker.stats().sentCount, 2);
}

} // namespace

QTEST_MAIN(TrackerIntegrationTest)
#include "tst_tracker.moc"
