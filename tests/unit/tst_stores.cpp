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

#include <MatomoQt/InMemoryClientIdStore.h>
#include <MatomoQt/InMemoryConsentStore.h>
#include <MatomoQt/QSettingsClientIdStore.h>
#include <MatomoQt/QSettingsConsentStore.h>

#include <QtCore/QSettings>
#include <QtCore/QTemporaryFile>
#include <QtTest/QtTest>

namespace {

QSettings *createTempQSettings(QObject *parent) {
    auto file = new QTemporaryFile(parent);
    if (!file->open()) {
        qFatal("Failed to create temporary file for QSettings");
    }
    const QString path = file->fileName();
    file->close();

    return new QSettings(path, QSettings::IniFormat, parent);
}

class StoresTest : public QObject {
        Q_OBJECT

    private slots:
        void inMemoryConsentStoreDefaultsToUnknown();
        void inMemoryConsentStoreRetainsValue();

        void inMemoryClientIdStoreDefaultsToEmpty();
        void inMemoryClientIdStoreRetainsValue();
        void inMemoryClientIdStoreClears();

        void qSettingsConsentStorePersistsState();
        void qSettingsConsentStoreInvalidValueReturnsUnknown();
        void qSettingsConsentStoreMissingValueReturnsUnknown();
        void qSettingsConsentStoreOverwritesOldValue();

        void qSettingsClientIdStorePersistsValue();
        void qSettingsClientIdStoreReturnsEmptyWhenMissing();
        void qSettingsClientIdStoreClears();
};
} // namespace

void StoresTest::inMemoryConsentStoreDefaultsToUnknown() {
    MatomoQt::InMemoryConsentStore store;
    QCOMPARE(store.consentState(), MatomoQt::ConsentState::Value::Unknown);
}

void StoresTest::inMemoryConsentStoreRetainsValue() {
    MatomoQt::InMemoryConsentStore store;
    store.setConsentState(MatomoQt::ConsentState::Value::Granted);
    QCOMPARE(store.consentState(), MatomoQt::ConsentState::Value::Granted);

    store.setConsentState(MatomoQt::ConsentState::Value::Denied);
    QCOMPARE(store.consentState(), MatomoQt::ConsentState::Value::Denied);

    store.setConsentState(MatomoQt::ConsentState::Value::Withdrawn);
    QCOMPARE(store.consentState(), MatomoQt::ConsentState::Value::Withdrawn);
}

void StoresTest::inMemoryClientIdStoreDefaultsToEmpty() {
    MatomoQt::InMemoryClientIdStore store;
    QVERIFY(store.clientId().isEmpty());
}

void StoresTest::inMemoryClientIdStoreRetainsValue() {
    MatomoQt::InMemoryClientIdStore store;
    store.setClientId(QStringLiteral("abc123"));
    QCOMPARE(store.clientId(), QStringLiteral("abc123"));
}

void StoresTest::inMemoryClientIdStoreClears() {
    MatomoQt::InMemoryClientIdStore store;
    store.setClientId(QStringLiteral("abc123"));
    store.clearClientId();
    QVERIFY(store.clientId().isEmpty());
}

void StoresTest::qSettingsConsentStorePersistsState() {
    auto settings = createTempQSettings(this);
    MatomoQt::QSettingsConsentStore store(settings);

    store.setConsentState(MatomoQt::ConsentState::Value::Granted);
    QCOMPARE(store.consentState(), MatomoQt::ConsentState::Value::Granted);

    // Simulate a fresh store reading the same settings
    MatomoQt::QSettingsConsentStore freshStore(settings);
    QCOMPARE(freshStore.consentState(), MatomoQt::ConsentState::Value::Granted);

    store.setConsentState(MatomoQt::ConsentState::Value::Denied);
    QCOMPARE(freshStore.consentState(), MatomoQt::ConsentState::Value::Denied);
}

void StoresTest::qSettingsConsentStoreInvalidValueReturnsUnknown() {
    const auto settings = createTempQSettings(this);
    settings->setValue(QStringLiteral("MatomoQt/consentState"), 999);

    const MatomoQt::QSettingsConsentStore store(settings);
    QCOMPARE(store.consentState(), MatomoQt::ConsentState::Value::Unknown);

    settings->setValue(QStringLiteral("MatomoQt/consentState"), QStringLiteral("bogus"));
    QCOMPARE(store.consentState(), MatomoQt::ConsentState::Value::Unknown);

    settings->setValue(QStringLiteral("MatomoQt/consentState"), -1);
    QCOMPARE(store.consentState(), MatomoQt::ConsentState::Value::Unknown);
}

void StoresTest::qSettingsConsentStoreMissingValueReturnsUnknown() {
    auto settings = createTempQSettings(this);
    MatomoQt::QSettingsConsentStore store(settings);

    QCOMPARE(store.consentState(), MatomoQt::ConsentState::Value::Unknown);
}

void StoresTest::qSettingsConsentStoreOverwritesOldValue() {
    auto settings = createTempQSettings(this);
    MatomoQt::QSettingsConsentStore store(settings);

    store.setConsentState(MatomoQt::ConsentState::Value::Granted);
    QCOMPARE(store.consentState(), MatomoQt::ConsentState::Value::Granted);

    store.setConsentState(MatomoQt::ConsentState::Value::Withdrawn);
    QCOMPARE(store.consentState(), MatomoQt::ConsentState::Value::Withdrawn);

    MatomoQt::QSettingsConsentStore freshStore(settings);
    QCOMPARE(freshStore.consentState(), MatomoQt::ConsentState::Value::Withdrawn);
}

void StoresTest::qSettingsClientIdStorePersistsValue() {
    auto settings = createTempQSettings(this);
    MatomoQt::QSettingsClientIdStore store(settings);

    store.setClientId(QStringLiteral("client-id-42"));
    QCOMPARE(store.clientId(), QStringLiteral("client-id-42"));

    MatomoQt::QSettingsClientIdStore freshStore(settings);
    QCOMPARE(freshStore.clientId(), QStringLiteral("client-id-42"));
}

void StoresTest::qSettingsClientIdStoreReturnsEmptyWhenMissing() {
    auto settings = createTempQSettings(this);
    MatomoQt::QSettingsClientIdStore store(settings);

    QVERIFY(store.clientId().isEmpty());
}

void StoresTest::qSettingsClientIdStoreClears() {
    auto settings = createTempQSettings(this);
    MatomoQt::QSettingsClientIdStore store(settings);

    store.setClientId(QStringLiteral("client-id-42"));
    QCOMPARE(store.clientId(), QStringLiteral("client-id-42"));

    store.clearClientId();
    QVERIFY(store.clientId().isEmpty());

    MatomoQt::QSettingsClientIdStore freshStore(settings);
    QVERIFY(freshStore.clientId().isEmpty());
}

QTEST_MAIN(StoresTest)
#include "tst_stores.moc"
