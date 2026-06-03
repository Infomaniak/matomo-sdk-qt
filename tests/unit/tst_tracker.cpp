#include <MatomoQt/ClientIdStore.h>
#include <MatomoQt/ConsentStore.h>
#include <MatomoQt/CustomDimension.h>
#include <MatomoQt/Event.h>
#include <MatomoQt/PageView.h>
#include <MatomoQt/PrivacyMode.h>
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
        static void trackerRejectsInvalidConfigBeforePayload();
        static void trackerRejectsInvalidPayloadAfterConsent();
        static void trackerDoesNotEmitSignalsForUnchangedValues();
        static void trackerEmitsConfigChangedForChangedConfig();

        static void trackerSupportsDisabledAndOptOutModes();
};
} // namespace

void TrackerTest::defaultConfigIsPrivacySafe() {
    const TrackerConfig config;

    QVERIFY(!config.isValid());
    QCOMPARE(config.siteId, 0);
    QCOMPARE(config.privacyMode, PrivacyMode::RequiresConsent);

    const Tracker tracker;
    QCOMPARE(tracker.consentState(), ConsentState::Unknown);
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
    const PageView pageViewWithInvalidDimension{.path = QStringLiteral("preferences"), .customDimensions = {{.id = 0}}};
    QVERIFY(!pageViewWithInvalidDimension.isValid());
    const PageView pageViewWithValidDimension{.path = QStringLiteral("preferences"), .customDimensions = {{.id = 1}}};
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
            .customDimensions = {{.id = 0}},
    };
    QVERIFY(!eventWithInvalidDimension.isValid());
    const Event validEvent{.category = QStringLiteral("preferences"), .action = QStringLiteral("click")};
    QVERIFY(validEvent.isValid());
    const Event eventWithValidOptionalFields{
            .category = QStringLiteral("preferences"),
            .action = QStringLiteral("click"),
            .value = 0.0,
            .customDimensions = {{.id = 1}},
    };
    QVERIFY(eventWithValidOptionalFields.isValid());

    QVERIFY(!CustomDimension{}.isValid());
    const CustomDimension validDimension{.id = 1, .value = QStringLiteral("enabled")};
    QVERIFY(validDimension.isValid());

    QVERIFY(!RequestResult{}.accepted());
    QCOMPARE(RequestResult{}.status, RequestResult::Status::InvalidConfig);
}

void TrackerTest::trackerDoesNotAcceptWithoutConsentByDefault() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.siteId = 1;

    Tracker tracker(config);
    const auto result = tracker.trackPageView({.path = QStringLiteral("preferences")});

    QVERIFY(!result.accepted());
    QCOMPARE(result.status, RequestResult::Status::BlockedByPrivacy);
}

void TrackerTest::trackerAcceptsCallsAfterConsent() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.siteId = 1;

    Tracker tracker(config);
    QSignalSpy consentSpy(&tracker, &Tracker::consentStateChanged);
    tracker.setConsentState(ConsentState::Granted);

    QCOMPARE(consentSpy.count(), 1);
    QCOMPARE(tracker.trackPageView({.path = QStringLiteral("preferences")}).status, RequestResult::Status::Accepted);
    QCOMPARE(tracker.trackEvent({.category = QStringLiteral("preferences"), .action = QStringLiteral("click")}).status,
             RequestResult::Status::Accepted);
    QCOMPARE(tracker.sendPing().status, RequestResult::Status::Accepted);
}

void TrackerTest::trackerRejectsInvalidConfigBeforePayload() {
    Tracker tracker;
    tracker.setConsentState(ConsentState::Granted);

    QCOMPARE(tracker.trackPageView({}).status, RequestResult::Status::InvalidConfig);
    QCOMPARE(tracker.trackEvent({}).status, RequestResult::Status::InvalidConfig);
    QCOMPARE(tracker.sendPing().status, RequestResult::Status::InvalidConfig);
}

void TrackerTest::trackerRejectsInvalidPayloadAfterConsent() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.siteId = 1;

    Tracker tracker(config);
    tracker.setConsentState(ConsentState::Granted);

    QCOMPARE(tracker.trackPageView({}).status, RequestResult::Status::InvalidPayload);
    QCOMPARE(tracker.trackEvent({}).status, RequestResult::Status::InvalidPayload);
    QCOMPARE(tracker.trackPageView({.path = QStringLiteral("preferences"), .customDimensions = {{.id = 0}}}).status,
             RequestResult::Status::InvalidPayload);
    QCOMPARE(tracker.trackEvent({.category = QStringLiteral("preferences"),
                                 .action = QStringLiteral("click"),
                                 .customDimensions = {{.id = 0}}})
                     .status,
             RequestResult::Status::InvalidPayload);
    QCOMPARE(tracker.trackEvent({.category = QStringLiteral("preferences"),
                                 .action = QStringLiteral("click"),
                                 .value = std::numeric_limits<double>::quiet_NaN()})
                     .status,
             RequestResult::Status::InvalidPayload);
}

void TrackerTest::trackerDoesNotEmitSignalsForUnchangedValues() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.siteId = 1;

    Tracker tracker(config);
    tracker.setConsentState(ConsentState::Granted);

    QSignalSpy configSpy(&tracker, &Tracker::configChanged);
    QSignalSpy consentSpy(&tracker, &Tracker::consentStateChanged);
    QSignalSpy enabledSpy(&tracker, &Tracker::enabledChanged);

    tracker.setConfig(config);
    tracker.setConsentState(ConsentState::Granted);
    tracker.setEnabled(true);

    QCOMPARE(configSpy.count(), 0);
    QCOMPARE(consentSpy.count(), 0);
    QCOMPARE(enabledSpy.count(), 0);
}

void TrackerTest::trackerEmitsConfigChangedForChangedConfig() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.siteId = 1;

    Tracker tracker(config);
    QSignalSpy configSpy(&tracker, &Tracker::configChanged);

    config.privacyMode = PrivacyMode::ConsentExemptWithOptOut;
    tracker.setConfig(config);

    QCOMPARE(configSpy.count(), 1);
    QCOMPARE(tracker.config().privacyMode, PrivacyMode::ConsentExemptWithOptOut);
}

void TrackerTest::trackerSupportsDisabledAndOptOutModes() {
    TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.siteId = 1;
    config.privacyMode = PrivacyMode::Disabled;

    Tracker tracker(config);
    QCOMPARE(tracker.sendPing().status, RequestResult::Status::BlockedByPrivacy);

    config.privacyMode = PrivacyMode::ConsentExemptWithOptOut;
    tracker.setConfig(config);
    QCOMPARE(tracker.sendPing().status, RequestResult::Status::Accepted);

    tracker.setConsentState(ConsentState::Denied);
    QCOMPARE(tracker.sendPing().status, RequestResult::Status::BlockedByPrivacy);

    tracker.setEnabled(false);
    QCOMPARE(tracker.sendPing().status, RequestResult::Status::Disabled);
}

QTEST_MAIN(TrackerTest)
#include "tst_tracker.moc"
