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
#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>

// The MatomoQt QML module is built as a static plugin in this example
// (MatomoQtQml is a static library target). Statically linked QML plugins
// must be registered explicitly with Q_IMPORT_QML_PLUGIN so the QML engine
// can resolve "import MatomoQt" without loading a shared library at runtime.
Q_IMPORT_QML_PLUGIN(MatomoQtPlugin)

int main(int argc, char *argv[]) {
    const QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("MatomoQt QML basic example"));
    const QCommandLineOption exitOnCompletedOption(
            QStringList() << QStringLiteral("exit-on-completed"));
    parser.addOption(exitOnCompletedOption);
    parser.parse(QCoreApplication::arguments());

    QQmlApplicationEngine engine;

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("MatomoQtExamples.QmlBasic", "Main");
#else
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [](QObject *obj, const QUrl &) {
            if (obj == nullptr) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/MatomoQtExamples/QmlBasic/Main.qml")));
#endif

    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    if (parser.isSet(exitOnCompletedOption)) {
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    }

    return QGuiApplication::exec();
}
