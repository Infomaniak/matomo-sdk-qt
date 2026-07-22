#include <MatomoQt/ConsentState.h>
#include <MatomoQt/PrivacyController.h>
#include <MatomoQt/PrivacyMode.h>

#include <QtTest/QtTest>

using namespace MatomoQt;

namespace {
class PrivacyControllerTest : public QObject {
        Q_OBJECT

    private slots:
        static void defaultRequiresConsentBlocks();
        static void disabledAlwaysBlocks();
        static void requiresConsentMatrix();
        static void exemptMatrix();
        static void fullExhaustiveMatrix();
};
} // namespace

void PrivacyControllerTest::defaultRequiresConsentBlocks() {
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::RequiresConsent, ConsentState::Unknown));
}

void PrivacyControllerTest::disabledAlwaysBlocks() {
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Disabled, ConsentState::Unknown));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Disabled, ConsentState::Granted));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Disabled, ConsentState::Denied));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Disabled, ConsentState::Withdrawn));
}

void PrivacyControllerTest::requiresConsentMatrix() {
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::RequiresConsent, ConsentState::Unknown));
    QVERIFY(PrivacyController::isTrackingAllowed(PrivacyMode::RequiresConsent, ConsentState::Granted));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::RequiresConsent, ConsentState::Denied));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::RequiresConsent, ConsentState::Withdrawn));
}

void PrivacyControllerTest::exemptMatrix() {
    QVERIFY(PrivacyController::isTrackingAllowed(PrivacyMode::ConsentExemptWithOptOut, ConsentState::Unknown));
    QVERIFY(PrivacyController::isTrackingAllowed(PrivacyMode::ConsentExemptWithOptOut, ConsentState::Granted));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::ConsentExemptWithOptOut, ConsentState::Denied));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::ConsentExemptWithOptOut, ConsentState::Withdrawn));
}

void PrivacyControllerTest::fullExhaustiveMatrix() {
    struct Case {
        PrivacyMode mode;
        ConsentState state;
        bool expected;
    };

    const std::vector<Case> cases = {
        {PrivacyMode::Disabled, ConsentState::Unknown, false},
        {PrivacyMode::Disabled, ConsentState::Granted, false},
        {PrivacyMode::Disabled, ConsentState::Denied, false},
        {PrivacyMode::Disabled, ConsentState::Withdrawn, false},

        {PrivacyMode::RequiresConsent, ConsentState::Unknown, false},
        {PrivacyMode::RequiresConsent, ConsentState::Granted, true},
        {PrivacyMode::RequiresConsent, ConsentState::Denied, false},
        {PrivacyMode::RequiresConsent, ConsentState::Withdrawn, false},

        {PrivacyMode::ConsentExemptWithOptOut, ConsentState::Unknown, true},
        {PrivacyMode::ConsentExemptWithOptOut, ConsentState::Granted, true},
        {PrivacyMode::ConsentExemptWithOptOut, ConsentState::Denied, false},
        {PrivacyMode::ConsentExemptWithOptOut, ConsentState::Withdrawn, false},
    };

    for (const auto &c : cases) {
        QCOMPARE(PrivacyController::isTrackingAllowed(c.mode, c.state), c.expected);
    }
}

QTEST_MAIN(PrivacyControllerTest)
#include "tst_privacycontroller.moc"
