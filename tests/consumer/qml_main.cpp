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

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(MatomoQtPlugin)

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import MatomoQt 1.0

        QtObject {
            property int successStatus: DispatchStatus.Success
            property int acceptedStatus: RequestStatus.RequestAccepted
        }
    )",
                      QUrl(QStringLiteral("inmemory:/matomoqt_consumer.qml")));

    if (component.isLoading()) {
        QEventLoop loop;
        QObject::connect(&component, &QQmlComponent::statusChanged, &loop, [&loop](QQmlComponent::Status status) {
            if (status != QQmlComponent::Loading) loop.quit();
        });
        QTimer::singleShot(5000, &loop, &QEventLoop::quit);
        loop.exec();
    }

    if (!component.isReady()) {
        qCritical().noquote() << component.errorString();
        return 1;
    }

    const std::unique_ptr<QObject> root(component.create());
    if (!root) {
        qCritical().noquote() << component.errorString();
        return 1;
    }

    return 0;
}
