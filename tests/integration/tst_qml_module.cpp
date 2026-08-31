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

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest/QtTest>

#include <MatomoQt/DispatchResult.h>
#include <MatomoQt/RequestStatus.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(MatomoQtPlugin)

namespace {

class TestHttpServer : public QObject {
        Q_OBJECT

    public:
        enum class Mode {
            Respond200,
            Respond500,
        };

        explicit TestHttpServer(QObject *parent = nullptr) :
            QObject(parent) {}

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

        [[nodiscard]] int requestCount() const { return m_requestCount; }
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
                              "ERROR");
            }
            socket->disconnectFromHost();
        }

        QTcpServer *m_server = nullptr;
        Mode m_mode = Mode::Respond200;
        int m_requestCount = 0;
        QString m_lastRequestPath;
};

void addQmlImportPaths(QQmlEngine *engine) {
    const QString moduleOutputDir = QStringLiteral(MATOMOQT_QML_IMPORT_PATH);
    QVERIFY2(!moduleOutputDir.isEmpty(), "MATOMOQT_QML_IMPORT_PATH is empty.");
    engine->addImportPath(moduleOutputDir);

    const QFileInfo moduleOutputInfo(moduleOutputDir);
    if (moduleOutputInfo.fileName() == QStringLiteral("MatomoQt")) {
        engine->addImportPath(moduleOutputInfo.absolutePath());
    } else {
        engine->addImportPath(QDir(moduleOutputDir).filePath(QStringLiteral("..")));
    }
}

QObject *matomoTracker(QQmlEngine *engine) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return engine->singletonInstance<QObject *>("MatomoQt", "MatomoTracker");
#else
    const int typeId = qmlTypeId("MatomoQt", 1, 0, "MatomoTracker");
    return engine->singletonInstance<QObject *>(typeId);
#endif
}

class QmlModuleIntegrationTest : public QObject {
        Q_OBJECT

    private slots:
        void qmlModuleImportsAndRespectsConsentGate();
        void qmlTrackerRecoversFromIncompleteConfig();
};

void QmlModuleIntegrationTest::qmlModuleImportsAndRespectsConsentGate() {
    TestHttpServer server;
    QVERIFY(server.start());

    QCoreApplication::setOrganizationName(QStringLiteral("MatomoQtTest"));
    QCoreApplication::setApplicationName(QStringLiteral("QmlModuleTest"));

    QSettings(QStringLiteral("MatomoQtTest"), QStringLiteral("QmlModuleTest")).remove(QString());

    QQmlEngine engine;
    addQmlImportPaths(&engine);

    const QString qmlSource = QStringLiteral(R"(
import QtQml
import MatomoQt 1.0

QtObject {
    id: root
    property int observedDispatchCount: 0
    property int observedDispatchStatus: -1
    property int observedDispatchHttpStatus: -1
    property string observedDispatchMessage
    property bool observedDispatchSucceeded: false
    property Connections trackerConnections: Connections {
        target: MatomoTracker
        function onDispatchFinished(status, httpStatus, message) {
            root.observedDispatchCount++
            root.observedDispatchStatus = status
            root.observedDispatchHttpStatus = httpStatus
            root.observedDispatchMessage = message
            root.observedDispatchSucceeded = status === DispatchStatus.Success
        }
    }

    Component.onCompleted: {
        MatomoTracker.endpoint = "%1"
        MatomoTracker.actionUrlBase = "app://qml-module-test/"
        MatomoTracker.siteId = 1
        MatomoTracker.privacyMode = PrivacyMode.RequiresConsent
    }
}
)").arg(server.url().toString());

    QQmlComponent component(&engine);
    component.setData(qmlSource.toUtf8(), QUrl(QStringLiteral("inmemory:/tst_qml_module.qml")));
    QTRY_VERIFY2(component.isReady() || component.isError(), qPrintable(component.errorString()));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root != nullptr, qPrintable(component.errorString()));

    QObject *tracker = matomoTracker(&engine);
    QVERIFY(tracker != nullptr);

    bool acceptedBeforeConsent = false;
    QVERIFY(QMetaObject::invokeMethod(tracker,
                                      "trackPageView",
                                      Q_RETURN_ARG(bool, acceptedBeforeConsent),
                                      Q_ARG(QString, QStringLiteral("settings")),
                                      Q_ARG(QString, QStringLiteral("Settings"))));
    QVERIFY(!acceptedBeforeConsent);
    QCOMPARE(server.requestCount(), 0);

    QSignalSpy consentSpy(tracker, SIGNAL(consentStateChanged()));
    QVERIFY(QMetaObject::invokeMethod(tracker, "grantConsent"));
    QCOMPARE(consentSpy.count(), 1);

    const QUrl configuredEndpoint = tracker->property("endpoint").toUrl();
    QSignalSpy endpointSpy(tracker, SIGNAL(endpointChanged()));
    QTest::ignoreMessage(QtWarningMsg,
                         "MatomoTracker: endpoint cannot be changed after the tracker is initialized; ignoring.");
    QVERIFY(tracker->setProperty("endpoint", QUrl(QStringLiteral("https://other.example.com/matomo.php"))));
    QCOMPARE(tracker->property("endpoint").toUrl(), configuredEndpoint);
    QCOMPARE(endpointSpy.count(), 0);

    bool acceptedAfterConsent = false;
    QVERIFY(QMetaObject::invokeMethod(tracker,
                                      "trackEvent",
                                      Q_RETURN_ARG(bool, acceptedAfterConsent),
                                      Q_ARG(QString, QStringLiteral("preferences")),
                                      Q_ARG(QString, QStringLiteral("click")),
                                      Q_ARG(QString, QStringLiteral("saveButton")),
                                      Q_ARG(QVariant, QVariant(1.0))));
    QVERIFY2(acceptedAfterConsent, qPrintable(tracker->property("lastRequestMessage").toString()));
    QTRY_COMPARE_WITH_TIMEOUT(server.requestCount(), 1, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("observedDispatchCount").toInt(), 1, 5000);
    QVERIFY(root->property("observedDispatchSucceeded").toBool());
    QCOMPARE(root->property("observedDispatchStatus").toInt(),
             static_cast<int>(MatomoQt::DispatchStatus::Value::Success));
    QCOMPARE(root->property("observedDispatchHttpStatus").toInt(), 200);
    QVERIFY(tracker->property("hasDispatchResult").toBool());
    QCOMPARE(tracker->property("lastDispatchStatus").toInt(),
             static_cast<int>(MatomoQt::DispatchStatus::Value::Success));
    QCOMPARE(tracker->property("lastDispatchHttpStatus").toInt(), 200);

    const QUrl sentUrl = QUrl(QStringLiteral("http://127.0.0.1%1").arg(server.lastRequestPath()));
    const QUrlQuery sentQuery(sentUrl);
    QCOMPARE(sentQuery.queryItemValue(QStringLiteral("e_c"), QUrl::FullyDecoded), QStringLiteral("preferences"));
    QCOMPARE(sentQuery.queryItemValue(QStringLiteral("e_a"), QUrl::FullyDecoded), QStringLiteral("click"));

    server.setMode(TestHttpServer::Mode::Respond500);
    QVERIFY(QMetaObject::invokeMethod(tracker,
                                      "trackEvent",
                                      Q_RETURN_ARG(bool, acceptedAfterConsent),
                                      Q_ARG(QString, QStringLiteral("preferences")),
                                      Q_ARG(QString, QStringLiteral("retry")),
                                      Q_ARG(QString, QString()),
                                      Q_ARG(QVariant, QVariant())));
    QVERIFY(acceptedAfterConsent);
    QTRY_COMPARE_WITH_TIMEOUT(root->property("observedDispatchCount").toInt(), 2, 5000);
    QVERIFY(!root->property("observedDispatchSucceeded").toBool());
    QCOMPARE(root->property("observedDispatchStatus").toInt(),
             static_cast<int>(MatomoQt::DispatchStatus::Value::NetworkError));
    QCOMPARE(root->property("observedDispatchHttpStatus").toInt(), 500);
    QVERIFY(!root->property("observedDispatchMessage").toString().isEmpty());
    QCOMPARE(tracker->property("lastDispatchStatus").toInt(),
             static_cast<int>(MatomoQt::DispatchStatus::Value::NetworkError));
    QCOMPARE(tracker->property("lastDispatchHttpStatus").toInt(), 500);
    QVERIFY(!tracker->property("lastDispatchMessage").toString().isEmpty());
}

void QmlModuleIntegrationTest::qmlTrackerRecoversFromIncompleteConfig() {
    TestHttpServer server;
    QVERIFY(server.start());

    QCoreApplication::setOrganizationName(QStringLiteral("MatomoQtTest"));
    QCoreApplication::setApplicationName(QStringLiteral("QmlRecoveryTest"));
    QSettings(QStringLiteral("MatomoQtTest"), QStringLiteral("QmlRecoveryTest")).remove(QString());

    QQmlEngine engine;
    addQmlImportPaths(&engine);

    QQmlComponent component(&engine);
    component.setData("import QtQml\nimport MatomoQt 1.0\nQtObject {}\n",
                      QUrl(QStringLiteral("inmemory:/tst_qml_recovery.qml")));
    QTRY_VERIFY2(component.isReady() || component.isError(), qPrintable(component.errorString()));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root != nullptr, qPrintable(component.errorString()));

    QObject *tracker = matomoTracker(&engine);
    QVERIFY(tracker != nullptr);
    QVERIFY(tracker->setProperty("endpoint", server.url()));
    QVERIFY(tracker->setProperty("siteId", 1));

    bool accepted = true;
    QVERIFY(QMetaObject::invokeMethod(tracker,
                                      "trackPageView",
                                      Q_RETURN_ARG(bool, accepted),
                                      Q_ARG(QString, QStringLiteral("settings")),
                                      Q_ARG(QString, QStringLiteral("Settings"))));
    QVERIFY(!accepted);
    QCOMPARE(tracker->property("lastRequestStatus").toInt(),
             static_cast<int>(MatomoQt::RequestStatus::Value::RequestInvalidConfig));
    QCOMPARE(server.requestCount(), 0);

    QVERIFY(tracker->setProperty("actionUrlBase", QUrl(QStringLiteral("app://qml-recovery-test/"))));
    QVERIFY(QMetaObject::invokeMethod(tracker, "grantConsent"));
    QVERIFY(QMetaObject::invokeMethod(tracker,
                                      "trackEvent",
                                      Q_RETURN_ARG(bool, accepted),
                                      Q_ARG(QString, QStringLiteral("preferences")),
                                      Q_ARG(QString, QStringLiteral("recover")),
                                      Q_ARG(QString, QString()),
                                      Q_ARG(QVariant, QVariant())));
    QVERIFY2(accepted, qPrintable(tracker->property("lastRequestMessage").toString()));
    QTRY_COMPARE_WITH_TIMEOUT(server.requestCount(), 1, 5000);
}

} // namespace

QTEST_MAIN(QmlModuleIntegrationTest)
#include "tst_qml_module.moc"
