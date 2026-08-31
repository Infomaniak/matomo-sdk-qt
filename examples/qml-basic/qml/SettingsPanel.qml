// Infomaniak - matomo-sdk-qt
// Copyright (C) 2026 Infomaniak Network SA
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MatomoQt

GroupBox {
    title: "Settings"
    property var logCallback

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: "Privacy mode: %1".arg(privacyModeName(MatomoTracker.privacyMode))
            font.bold: true
        }

        ComboBox {
            id: privacyModeCombo
            Layout.fillWidth: true
            model: [
                { label: "Disabled", value: PrivacyMode.Disabled },
                { label: "Requires consent", value: PrivacyMode.RequiresConsent },
                { label: "Consent exempt with opt-out", value: PrivacyMode.ConsentExemptWithOptOut }
            ]
            textRole: "label"
            valueRole: "value"
            currentIndex: MatomoTracker.privacyMode
            onActivated: {
                MatomoTracker.privacyMode = currentValue
                const actual = MatomoTracker.privacyMode
                if (actual !== currentValue) {
                    currentIndex = actual
                    logCallback("[settings] Privacy mode change rejected (tracker initialized); staying at %1".arg(privacyModeName(actual)))
                } else {
                    logCallback("[settings] Privacy mode changed to %1".arg(currentText))
                }
            }
        }

        Button {
            text: "Track settings page view"
            Layout.fillWidth: true
            onClicked: {
                const accepted = MatomoTracker.trackPageView("/settings", "Settings")
                logCallback("[settings] trackPageView(\"/settings\") -> %1".arg(accepted))
            }
        }

        Button {
            text: "Track theme toggle event"
            Layout.fillWidth: true
            onClicked: {
                const accepted = MatomoTracker.trackEvent("settings", "toggle", "theme", 1)
                logCallback("[settings] trackEvent(\"settings\", \"toggle\", \"theme\") -> %1".arg(accepted))
            }
        }
    }

    function privacyModeName(mode) {
        switch (mode) {
            case PrivacyMode.Disabled: return "Disabled"
            case PrivacyMode.RequiresConsent: return "Requires consent"
            case PrivacyMode.ConsentExemptWithOptOut: return "Consent exempt with opt-out"
            default: return "Unknown"
        }
    }
}
