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
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Value::RequiresConsent, ConsentState::Value::Unknown));
}

void PrivacyControllerTest::disabledAlwaysBlocks() {
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Value::Disabled, ConsentState::Value::Unknown));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Value::Disabled, ConsentState::Value::Granted));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Value::Disabled, ConsentState::Value::Denied));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Value::Disabled, ConsentState::Value::Withdrawn));
}

void PrivacyControllerTest::requiresConsentMatrix() {
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Value::RequiresConsent, ConsentState::Value::Unknown));
    QVERIFY(PrivacyController::isTrackingAllowed(PrivacyMode::Value::RequiresConsent, ConsentState::Value::Granted));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Value::RequiresConsent, ConsentState::Value::Denied));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Value::RequiresConsent, ConsentState::Value::Withdrawn));
}

void PrivacyControllerTest::exemptMatrix() {
    QVERIFY(PrivacyController::isTrackingAllowed(PrivacyMode::Value::ConsentExemptWithOptOut, ConsentState::Value::Unknown));
    QVERIFY(PrivacyController::isTrackingAllowed(PrivacyMode::Value::ConsentExemptWithOptOut, ConsentState::Value::Granted));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Value::ConsentExemptWithOptOut, ConsentState::Value::Denied));
    QVERIFY(!PrivacyController::isTrackingAllowed(PrivacyMode::Value::ConsentExemptWithOptOut, ConsentState::Value::Withdrawn));
}

void PrivacyControllerTest::fullExhaustiveMatrix() {
    struct Case {
        PrivacyMode::Value mode;
        ConsentState::Value state;
        bool expected;
    };

    const std::vector<Case> cases = {
        {PrivacyMode::Value::Disabled, ConsentState::Value::Unknown, false},
        {PrivacyMode::Value::Disabled, ConsentState::Value::Granted, false},
        {PrivacyMode::Value::Disabled, ConsentState::Value::Denied, false},
        {PrivacyMode::Value::Disabled, ConsentState::Value::Withdrawn, false},

        {PrivacyMode::Value::RequiresConsent, ConsentState::Value::Unknown, false},
        {PrivacyMode::Value::RequiresConsent, ConsentState::Value::Granted, true},
        {PrivacyMode::Value::RequiresConsent, ConsentState::Value::Denied, false},
        {PrivacyMode::Value::RequiresConsent, ConsentState::Value::Withdrawn, false},

        {PrivacyMode::Value::ConsentExemptWithOptOut, ConsentState::Value::Unknown, true},
        {PrivacyMode::Value::ConsentExemptWithOptOut, ConsentState::Value::Granted, true},
        {PrivacyMode::Value::ConsentExemptWithOptOut, ConsentState::Value::Denied, false},
        {PrivacyMode::Value::ConsentExemptWithOptOut, ConsentState::Value::Withdrawn, false},
    };

    for (const auto &c : cases) {
        QCOMPARE(PrivacyController::isTrackingAllowed(c.mode, c.state), c.expected);
    }
}

QTEST_MAIN(PrivacyControllerTest)
#include "tst_privacycontroller.moc"
