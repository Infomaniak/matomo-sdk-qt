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
#include <QtCore/QUrlQuery>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest/QtTest>

#include <memory>

Q_IMPORT_QML_PLUGIN(MatomoQtPlugin)

namespace {

class TestHttpServer : public QObject {
        Q_OBJECT

    public:
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

            socket->write("HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: 2\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "OK");
            socket->disconnectFromHost();
        }

        QTcpServer *m_server = nullptr;
        int m_requestCount = 0;
        QString m_lastRequestPath;
};

class QmlModuleIntegrationTest : public QObject {
        Q_OBJECT

    private slots:
        void qmlModuleImportsAndRespectsConsentGate();
};

void QmlModuleIntegrationTest::qmlModuleImportsAndRespectsConsentGate() {
    TestHttpServer server;
    QVERIFY(server.start());

    QQmlEngine engine;

    const QString moduleOutputDir = QStringLiteral(MATOMOQT_QML_IMPORT_PATH);
    QVERIFY2(!moduleOutputDir.isEmpty(), "MATOMOQT_QML_IMPORT_PATH is empty.");
    engine.addImportPath(moduleOutputDir);

    const QFileInfo moduleOutputInfo(moduleOutputDir);
    if (moduleOutputInfo.fileName() == QStringLiteral("MatomoQt")) {
        engine.addImportPath(moduleOutputInfo.absolutePath());
    } else {
        engine.addImportPath(QDir(moduleOutputDir).filePath(QStringLiteral("..")));
    }

    const QString qmlSource = QStringLiteral(R"(
import QtQml
import MatomoQt 1.0

QtObject {
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

    QObject *tracker = engine.singletonInstance<QObject *>("MatomoQt", "MatomoTracker");
    QVERIFY(tracker != nullptr);

    bool acceptedBeforeConsent = false;
    QVERIFY(QMetaObject::invokeMethod(tracker,
                                      "trackPageView",
                                      Q_RETURN_ARG(bool, acceptedBeforeConsent),
                                      Q_ARG(QString, QStringLiteral("settings")),
                                      Q_ARG(QString, QStringLiteral("Settings"))));
    QVERIFY(!acceptedBeforeConsent);
    QCOMPARE(server.requestCount(), 0);

    QVERIFY(QMetaObject::invokeMethod(tracker, "grantConsent"));

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

    const QUrl sentUrl = QUrl(QStringLiteral("http://127.0.0.1%1").arg(server.lastRequestPath()));
    const QUrlQuery sentQuery(sentUrl);
    QCOMPARE(sentQuery.queryItemValue(QStringLiteral("e_c"), QUrl::FullyDecoded), QStringLiteral("preferences"));
    QCOMPARE(sentQuery.queryItemValue(QStringLiteral("e_a"), QUrl::FullyDecoded), QStringLiteral("click"));
}

} // namespace

QTEST_MAIN(QmlModuleIntegrationTest)
#include "tst_qml_module.moc"
