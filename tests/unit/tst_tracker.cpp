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

using namespace MatomoQt;

class TrackerTest : public QObject {
        Q_OBJECT

    private slots:
        void defaultConfigIsPrivacySafe();
        void configValidityRequiresEndpointAndSiteId();
        void payloadValidityUsesRequiredFields();
        void trackerDoesNotAcceptWithoutConsentByDefault();
        void trackerAcceptsCallsAfterConsent();
        void trackerSupportsDisabledAndOptOutModes();
};

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
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));

    QVERIFY(!config.isValid());

    config.siteId = 1;
    QVERIFY(config.isValid());
}

void TrackerTest::payloadValidityUsesRequiredFields() {
    QVERIFY(!PageView{}.isValid());
    QVERIFY(PageView{.path = QStringLiteral("preferences")}.isValid());

    QVERIFY(!Event{}.isValid());
    QVERIFY(!Event{.category = QStringLiteral("preferences")}.isValid());
    const Event validEvent{.category = QStringLiteral("preferences"), .action = QStringLiteral("click")};
    QVERIFY(validEvent.isValid());

    QVERIFY(!CustomDimension{}.isValid());
    const CustomDimension validDimension{.id = 1, .value = QStringLiteral("enabled")};
    QVERIFY(validDimension.isValid());
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
