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

#include <MatomoQt/ClientIdStore.h>
#include <MatomoQt/ConsentStore.h>
#include <MatomoQt/CustomDimension.h>
#include <MatomoQt/Event.h>
#include <MatomoQt/InMemoryClientIdStore.h>
#include <MatomoQt/InMemoryConsentStore.h>
#include <MatomoQt/PageView.h>
#include <MatomoQt/PrivacyMode.h>
#include <MatomoQt/QSettingsConsentStore.h>
#include <MatomoQt/RequestResult.h>
#include <MatomoQt/Tracker.h>
#include <MatomoQt/TrackerConfig.h>

#include <QtTest/QtTest>

#include <limits>

using namespace MatomoQt;

namespace {
class TrackerTest : public QObject {
        Q_OBJECT

    private slots:
        static void defaultConfigIsPrivacySafe();
        static void configValidityRequiresEndpointAndSiteId();

        static void payloadValidityUsesRequiredFields();
        static void trackerDoesNotAcceptWithoutConsentByDefault();

        static void trackerAcceptsCallsAfterConsent();
        static void trackerRejectsInvalidPayloadBeforeTrackerState();
        static void trackerRejectsInvalidConfigForValidTrackingCalls();
        static void trackerRejectsInvalidPayloadAfterConsent();
        static void trackerDoesNotEmitSignalsForUnchangedValues();

        static void trackerSupportsDisabledAndOptOutModes();

        static void trackerPersistsConsentStateToStore();
        static void trackerDoesNotEmitUnpersistedConsentState();
        static void trackerReadsConsentStateFromStore();
        static void trackerResetClientIdClearsStore();
        static void trackerDenialClearsPersistedClientId();
        static void trackerWithdrawalClearsPersistedClientId();

        static void trackerReadsClientIdFromStore();
        static void trackerWritesClientIdToStore();
        static void trackerRejectsInvalidClientId();
        static void trackerConstructsWithDeniedStoreClearsClientId();
        static void trackerConstructsWithWithdrawnStoreClearsClientId();
};
} // namespace

void TrackerTest::defaultConfigIsPrivacySafe() {
    const TrackerConfig config;

    QVERIFY(!config.isValid());
    QCOMPARE(config.siteId, 0);
    QCOMPARE(config.privacyMode, PrivacyMode::Value::RequiresConsent);

    const Tracker tracker;
    QCOMPARE(tracker.consentState(), ConsentState::Value::Unknown);
    QVERIFY(tracker.isEnabled());
}

void TrackerTest::configValidityRequiresEndpointAndSiteId() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("matomo.example.com/matomo.php"));

    config.siteId = 1;
    QVERIFY(!config.isValid());

    config.endpoint = QUrl(QStringLiteral("file:///tmp/matomo.php"));
    QVERIFY(!config.isValid());

    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    QVERIFY(config.isValid());
}

void TrackerTest::payloadValidityUsesRequiredFields() {
    QVERIFY(!PageView{}.isValid());
    QVERIFY(PageView{.path = QStringLiteral("preferences")}.isValid());
    const PageView pageViewWithInvalidDimension{
            .path = QStringLiteral("preferences"),
            .customDimensions = {{.id = 1}, {.id = 0}},
    };
    QVERIFY(!pageViewWithInvalidDimension.isValid());
    const PageView pageViewWithValidDimension{
            .path = QStringLiteral("preferences"),
            .customDimensions = {{.id = 1}, {.id = 2}},
    };
    QVERIFY(pageViewWithValidDimension.isValid());

    QVERIFY(!Event{}.isValid());
    QVERIFY(!Event{.category = QStringLiteral("preferences")}.isValid());
    const Event eventWithNaN{.category = QStringLiteral("preferences"),
                             .action = QStringLiteral("click"),
                             .value = std::numeric_limits<double>::quiet_NaN()};
    QVERIFY(!eventWithNaN.isValid());
    const Event eventWithInfinity{.category = QStringLiteral("preferences"),
                                  .action = QStringLiteral("click"),
                                  .value = std::numeric_limits<double>::infinity()};
    QVERIFY(!eventWithInfinity.isValid());
    const Event eventWithInvalidDimension{
            .category = QStringLiteral("preferences"),
            .action = QStringLiteral("click"),
            .customDimensions = {{.id = 1}, {.id = 0}},
    };
    QVERIFY(!eventWithInvalidDimension.isValid());
    const Event validEvent{.category = QStringLiteral("preferences"), .action = QStringLiteral("click")};
    QVERIFY(validEvent.isValid());
    const Event eventWithValidOptionalFields{
            .category = QStringLiteral("preferences"),
            .action = QStringLiteral("click"),
            .value = 0.0,
            .customDimensions = {{.id = 1}, {.id = 2}},
    };
    QVERIFY(eventWithValidOptionalFields.isValid());

    QVERIFY(!CustomDimension{}.isValid());
    const CustomDimension validDimension{.id = 1, .value = QStringLiteral("enabled")};
    QVERIFY(validDimension.isValid());

    QVERIFY(!RequestResult{}.accepted());
    QCOMPARE(RequestResult{}.status, RequestStatus::Value::RequestInvalidConfig);
}

void TrackerTest::trackerDoesNotAcceptWithoutConsentByDefault() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.siteId = 1;

    Tracker tracker(config);
    const auto result = tracker.trackPageView({.path = QStringLiteral("preferences")});

    QVERIFY(!result.accepted());
    QCOMPARE(result.status, RequestStatus::Value::RequestBlockedByPrivacy);
}

void TrackerTest::trackerAcceptsCallsAfterConsent() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.actionUrlBase = QUrl(QStringLiteral("app://desktop/"));
    config.siteId = 1;

    Tracker tracker(config);
    QSignalSpy consentSpy(&tracker, &Tracker::consentStateChanged);
    tracker.setConsentState(ConsentState::Value::Granted);

    QCOMPARE(consentSpy.count(), 1);
    QCOMPARE(tracker.trackPageView({.path = QStringLiteral("preferences")}).status, RequestStatus::Value::Accepted);
    QCOMPARE(tracker.trackEvent({.category = QStringLiteral("preferences"), .action = QStringLiteral("click")}).status,
             RequestStatus::Value::RequestAccepted);
    QCOMPARE(tracker.sendPing().status, RequestStatus::Value::RequestAccepted);
}

void TrackerTest::trackerRejectsInvalidPayloadBeforeTrackerState() {
    Tracker tracker;

    QCOMPARE(tracker.trackPageView({}).status, RequestStatus::Value::RequestInvalidPayload);
    QCOMPARE(tracker.trackEvent({}).status, RequestStatus::Value::RequestInvalidPayload);
}

void TrackerTest::trackerRejectsInvalidConfigForValidTrackingCalls() {
    Tracker tracker;
    tracker.setConsentState(ConsentState::Value::Granted);

    QCOMPARE(tracker.trackPageView({.path = QStringLiteral("preferences")}).status, RequestStatus::Value::RequestInvalidConfig);
    QCOMPARE(tracker.trackEvent({.category = QStringLiteral("preferences"), .action = QStringLiteral("click")}).status,
             RequestStatus::Value::RequestInvalidConfig);
    QCOMPARE(tracker.sendPing().status, RequestStatus::Value::RequestInvalidConfig);
}

void TrackerTest::trackerRejectsInvalidPayloadAfterConsent() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.siteId = 1;

    Tracker tracker(config);
    tracker.setConsentState(ConsentState::Value::Granted);

    QCOMPARE(tracker.trackPageView({}).status, RequestStatus::Value::RequestInvalidPayload);
    QCOMPARE(tracker.trackEvent({}).status, RequestStatus::Value::RequestInvalidPayload);
    QCOMPARE(tracker.trackPageView({.path = QStringLiteral("preferences"), .customDimensions = {{.id = 1}, {.id = 0}}}).status,
             RequestStatus::Value::RequestInvalidPayload);
    QCOMPARE(tracker.trackEvent({.category = QStringLiteral("preferences"),
                                 .action = QStringLiteral("click"),
                                 .customDimensions = {{.id = 1}, {.id = 0}}})
                     .status,
             RequestStatus::Value::RequestInvalidPayload);
    QCOMPARE(tracker.trackEvent({.category = QStringLiteral("preferences"),
                                 .action = QStringLiteral("click"),
                                 .value = std::numeric_limits<double>::quiet_NaN()})
                     .status,
             RequestStatus::Value::RequestInvalidPayload);
}

void TrackerTest::trackerDoesNotEmitSignalsForUnchangedValues() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.siteId = 1;

    Tracker tracker(config);
    tracker.setConsentState(ConsentState::Value::Granted);

    QSignalSpy consentSpy(&tracker, &Tracker::consentStateChanged);
    QSignalSpy enabledSpy(&tracker, &Tracker::enabledChanged);

    tracker.setConsentState(ConsentState::Value::Granted);
    tracker.setEnabled(true);

    QCOMPARE(consentSpy.count(), 0);
    QCOMPARE(enabledSpy.count(), 0);
}

void TrackerTest::trackerSupportsDisabledAndOptOutModes() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.actionUrlBase = QUrl(QStringLiteral("app://desktop/"));
    config.siteId = 1;
    config.privacyMode = PrivacyMode::Value::Disabled;

    Tracker tracker(config);
    QCOMPARE(tracker.sendPing().status, RequestStatus::Value::RequestBlockedByPrivacy);

    TrackerConfig optOutConfig = config;
    optOutConfig.privacyMode = PrivacyMode::Value::ConsentExemptWithOptOut;
    Tracker optOutTracker(optOutConfig);
    QCOMPARE(optOutTracker.sendPing().status, RequestStatus::Value::RequestAccepted);

    tracker.setConsentState(ConsentState::Value::Denied);
    QCOMPARE(tracker.sendPing().status, RequestStatus::Value::RequestBlockedByPrivacy);

    optOutTracker.setEnabled(false);
    QCOMPARE(optOutTracker.sendPing().status, RequestStatus::Value::RequestDisabled);
}

void TrackerTest::trackerPersistsConsentStateToStore() {
    InMemoryConsentStore store;
    Tracker tracker(TrackerConfig{}, &store, nullptr);

    tracker.setConsentState(ConsentState::Value::Granted);
    QCOMPARE(store.consentState(), ConsentState::Value::Granted);
    QCOMPARE(tracker.consentState(), ConsentState::Value::Granted);

    tracker.setConsentState(ConsentState::Value::Denied);
    QCOMPARE(store.consentState(), ConsentState::Value::Denied);
    QCOMPARE(tracker.consentState(), ConsentState::Value::Denied);
}

void TrackerTest::trackerDoesNotEmitUnpersistedConsentState() {
    QSettingsConsentStore store(nullptr);
    Tracker tracker(TrackerConfig{}, &store, nullptr);

    QSignalSpy consentSpy(&tracker, &Tracker::consentStateChanged);
    tracker.setConsentState(ConsentState::Value::Granted);

    QCOMPARE(tracker.consentState(), ConsentState::Value::Unknown);
    QCOMPARE(consentSpy.count(), 0);
}

void TrackerTest::trackerReadsConsentStateFromStore() {
    InMemoryConsentStore store;
    store.setConsentState(ConsentState::Value::Granted);

    Tracker tracker(TrackerConfig{}, &store, nullptr);
    QCOMPARE(tracker.consentState(), ConsentState::Value::Granted);
}

void TrackerTest::trackerResetClientIdClearsStore() {
    InMemoryClientIdStore store;
    store.setClientId(QStringLiteral("abc123"));

    Tracker tracker(TrackerConfig{}, nullptr, &store);
    tracker.resetClientId();

    QVERIFY(store.clientId().isEmpty());
}

void TrackerTest::trackerDenialClearsPersistedClientId() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.actionUrlBase = QUrl(QStringLiteral("app://desktop/"));
    config.siteId = 1;
    config.privacyMode = PrivacyMode::Value::ConsentExemptWithOptOut;

    InMemoryClientIdStore store;
    store.setClientId(QStringLiteral("0123456789abcdef"));

    Tracker tracker(config, nullptr, &store);
    tracker.setConsentState(ConsentState::Value::Granted);

    QVERIFY(tracker.trackPageView({.path = QStringLiteral("preferences")}).accepted());

    tracker.setConsentState(ConsentState::Value::Denied);
    QVERIFY(store.clientId().isEmpty());
    QCOMPARE(tracker.trackPageView({.path = QStringLiteral("preferences")}).status, RequestStatus::Value::RequestBlockedByPrivacy);
}

void TrackerTest::trackerWithdrawalClearsPersistedClientId() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.actionUrlBase = QUrl(QStringLiteral("app://desktop/"));
    config.siteId = 1;

    InMemoryClientIdStore store;
    store.setClientId(QStringLiteral("0123456789abcdef"));

    Tracker tracker(config, nullptr, &store);
    tracker.setConsentState(ConsentState::Value::Granted);

    QVERIFY(tracker.trackPageView({.path = QStringLiteral("preferences")}).accepted());

    tracker.setConsentState(ConsentState::Value::Withdrawn);
    QVERIFY(store.clientId().isEmpty());
    QCOMPARE(tracker.trackPageView({.path = QStringLiteral("preferences")}).status, RequestStatus::Value::RequestBlockedByPrivacy);
}

void TrackerTest::trackerReadsClientIdFromStore() {
    InMemoryClientIdStore store;
    store.setClientId(QStringLiteral("0123456789abcdef"));

    Tracker tracker(TrackerConfig{}, nullptr, &store);
    QCOMPARE(tracker.clientId(), QStringLiteral("0123456789abcdef"));
}

void TrackerTest::trackerWritesClientIdToStore() {
    InMemoryClientIdStore store;

    Tracker tracker(TrackerConfig{}, nullptr, &store);
    QVERIFY(tracker.setClientId(QStringLiteral("0123456789abcdef")));

    QCOMPARE(store.clientId(), QStringLiteral("0123456789abcdef"));
    QCOMPARE(tracker.clientId(), QStringLiteral("0123456789abcdef"));
}

void TrackerTest::trackerRejectsInvalidClientId() {
    InMemoryClientIdStore store;
    store.setClientId(QStringLiteral("0123456789abcdef"));

    Tracker tracker(TrackerConfig{}, nullptr, &store);

    QVERIFY(!tracker.setClientId(QStringLiteral("not-a-client-id")));
    QCOMPARE(store.clientId(), QStringLiteral("0123456789abcdef"));
    QCOMPARE(tracker.clientId(), QStringLiteral("0123456789abcdef"));
}

void TrackerTest::trackerConstructsWithDeniedStoreClearsClientId() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.siteId = 1;

    InMemoryClientIdStore idStore;
    idStore.setClientId(QStringLiteral("abc123"));

    InMemoryConsentStore consentStore;
    consentStore.setConsentState(ConsentState::Value::Denied);

    Tracker tracker(config, &consentStore, &idStore);

    QVERIFY(idStore.clientId().isEmpty());
    QCOMPARE(tracker.trackPageView({.path = QStringLiteral("preferences")}).status, RequestStatus::Value::RequestBlockedByPrivacy);
}

void TrackerTest::trackerConstructsWithWithdrawnStoreClearsClientId() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.siteId = 1;

    InMemoryClientIdStore idStore;
    idStore.setClientId(QStringLiteral("abc123"));

    InMemoryConsentStore consentStore;
    consentStore.setConsentState(ConsentState::Value::Withdrawn);

    Tracker tracker(config, &consentStore, &idStore);

    QVERIFY(idStore.clientId().isEmpty());
    QCOMPARE(tracker.trackPageView({.path = QStringLiteral("preferences")}).status, RequestStatus::Value::RequestBlockedByPrivacy);
}

QTEST_MAIN(TrackerTest)
#include "tst_tracker.moc"
