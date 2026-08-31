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

ApplicationWindow {
    id: window

    width: 480
    height: 640
    visible: true
    title: "MatomoQt QML Example"

    // The tracker is a singleton, configured once here.
    // Every QML file that imports MatomoQt can use MatomoTracker directly.
    Component.onCompleted: {
        MatomoTracker.endpoint = "http://127.0.0.1:8080/matomo.php"
        MatomoTracker.actionUrlBase = "app://matomoqt-qml-basic-example/"
        MatomoTracker.siteId = 1
        MatomoTracker.privacyMode = PrivacyMode.RequiresConsent
    }

    Connections {
        target: MatomoTracker

        function onLastRequestStatusChanged() {
            appendLog("[tracker] " + requestStatusName(MatomoTracker.lastRequestStatus) +
                (MatomoTracker.lastRequestMessage.length > 0 ? (" - " + MatomoTracker.lastRequestMessage) : ""));
        }

        function onDispatchFinished(status, httpStatus, message) {
            appendLog("[network] " + dispatchStatusName(status) +
                (httpStatus > 0 ? (" (HTTP " + httpStatus + ")") : "") +
                (message.length > 0 ? (" - " + message) : ""));
        }
    }

    function consentStateName(state) {
        switch (state) {
            case ConsentState.Granted: return "Granted";
            case ConsentState.Denied: return "Denied";
            case ConsentState.Withdrawn: return "Withdrawn";
            default: return "Unknown";
        }
    }

    function requestStatusName(status) {
        switch (status) {
            case RequestStatus.RequestAccepted: return "Accepted";
            case RequestStatus.RequestDisabled: return "Rejected: tracker disabled";
            case RequestStatus.RequestBlockedByPrivacy: return "Rejected: blocked by privacy/consent";
            case RequestStatus.RequestInvalidConfig: return "Rejected: invalid tracker configuration";
            case RequestStatus.RequestInvalidPayload: return "Rejected: invalid call payload";
            default: return "Unknown";
        }
    }

    function dispatchStatusName(status) {
        switch (status) {
            case DispatchStatus.Success: return "Delivered";
            case DispatchStatus.Timeout: return "Timed out";
            case DispatchStatus.NetworkError: return "Network error";
            case DispatchStatus.SslError: return "SSL error";
            case DispatchStatus.CircuitBreakerOpen: return "Circuit breaker open";
            default: return "Unknown";
        }
    }

    function appendLog(line) {
        logModel.append({ text: line });
        logView.positionViewAtEnd();
    }

    ListModel {
        id: logModel
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: "Consent state: %1".arg(consentStateName(MatomoTracker.consentState))
            font.bold: true
        }

        Label {
            text: "No tracking call is accepted before consent is granted."
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        GroupBox {
            title: "Consent"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent

                Button {
                    text: "Grant consent"
                    onClicked: {
                        MatomoTracker.grantConsent();
                        appendLog("[user] Consent granted.");
                    }
                }

                Button {
                    text: "Deny consent"
                    onClicked: {
                        MatomoTracker.denyConsent();
                        appendLog("[user] Consent denied.");
                    }
                }

                Button {
                    text: "Withdraw consent"
                    onClicked: {
                        MatomoTracker.withdrawConsent();
                        appendLog("[user] Consent withdrawn.");
                    }
                }
            }
        }

        GroupBox {
            title: "Tracking"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent

                Button {
                    text: "Track page view"
                    onClicked: {
                        const accepted = MatomoTracker.trackPageView("/settings", "Settings");
                        appendLog("[app] trackPageView(\"/settings\") -> %1".arg(accepted));
                    }
                }

                Button {
                    text: "Track event"
                    onClicked: {
                        const accepted = MatomoTracker.trackEvent("preferences", "click", "saveButton", 1);
                        appendLog("[app] trackEvent(\"preferences\", \"click\") -> %1".arg(accepted));
                    }
                }

                Button {
                    text: "Send ping"
                    onClicked: {
                        const accepted = MatomoTracker.sendPing();
                        appendLog("[app] sendPing() -> %1".arg(accepted));
                    }
                }
            }
        }

        Button {
            text: "Reset client ID"
            onClicked: {
                MatomoTracker.resetClientId();
                appendLog("[user] Client ID reset.");
            }
        }

        SettingsPanel {
            logCallback: appendLog
            Layout.fillWidth: true
        }

        Label {
            text: "Activity log"
            font.bold: true
        }

        ListView {
            id: logView

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: logModel
            delegate: Text {
                width: logView.width
                text: model.text
                wrapMode: Text.WordWrap
            }
        }
    }
}
