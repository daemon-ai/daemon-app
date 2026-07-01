// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Per-session settings popover (opens upward from the composer gear): profile
// override, reasoning effort, and fast/verbose modes for the active session.
// Backed by the SessionSettings facade (mock); the profile list comes from Profiles.
QQC.Popup {
    id: root

    implicitWidth: 300
    padding: 12
    // Open upward, right-aligned to the trigger button so the panel grows leftward
    // into the window (the buttons sit at the window's right edge).
    y: -implicitHeight - 8
    x: parent ? parent.width - width : 0

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        spacing: 12

        Text {
            text: qsTr("Session settings")
            font.family: FontIcons.display; font.pixelSize: 14; font.bold: true; color: Theme.text
        }

        // --- Profile override --------------------------------------------
        Text {
            text: qsTr("Profile")
            font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Repeater {
                model: Profiles.profiles
                delegate: Rectangle {
                    required property var entry
                    Layout.fillWidth: true
                    implicitHeight: 30
                    radius: 5
                    // Store the profile id (not the display name) so the session->turn binding is a
                    // strict pass-through; the row still shows the human name.
                    color: entry.id === SessionSettings.profile ? Theme.hover : "transparent"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        Text {
                            text: entry.name
                            font.family: FontIcons.display; font.pixelSize: 12; color: Theme.text
                            Layout.fillWidth: true
                        }
                        Text {
                            visible: entry.id === SessionSettings.profile
                            text: FontIcons.check
                            font.family: FontIcons.faSolid; font.pixelSize: 10; color: Theme.accent
                        }
                    }
                    TapHandler { onTapped: SessionSettings.setProfile(entry.id) }
                }
            }
        }

        // --- Reasoning effort --------------------------------------------
        Text {
            text: qsTr("Reasoning effort")
            font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Repeater {
                model: SessionSettings.effortOptions()
                delegate: Kit.TextButton {
                    required property var modelData
                    Layout.fillWidth: true
                    // Stored value is the canonical off/low/medium/high; show it
                    // capitalized to match the rest of the UI.
                    text: modelData.charAt(0).toUpperCase() + modelData.slice(1)
                    accentFilled: SessionSettings.effort === modelData
                    onClicked: SessionSettings.setEffort(modelData)
                }
            }
        }

        // --- Approval mode (CHA-4) ---------------------------------------
        // Per-session HITL policy. The setter sends SetSessionMode in daemon mode (the node parks
        // or auto-resolves tool approvals accordingly); default Ask.
        Text {
            text: qsTr("Approval mode")
            font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Repeater {
                model: SessionSettings.approvalModeOptions()
                delegate: Kit.TextButton {
                    required property var modelData
                    Layout.fillWidth: true
                    // Stored values are the canonical ask/accept_edits/auto_allow/deny; show a
                    // friendlier label.
                    text: modelData === "accept_edits" ? qsTr("Edits")
                          : modelData === "auto_allow" ? qsTr("Auto")
                          : modelData === "deny" ? qsTr("Deny")
                          : qsTr("Ask")
                    accentFilled: SessionSettings.approvalMode === modelData
                    onClicked: SessionSettings.setApprovalMode(modelData)
                }
            }
        }

        // --- Modes -------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Fast mode")
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.text
                Layout.fillWidth: true
            }
            Kit.Switch {
                checked: SessionSettings.fast
                onToggled: SessionSettings.setFast(checked)
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Verbose")
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.text
                Layout.fillWidth: true
            }
            Kit.Switch {
                checked: SessionSettings.verbose
                onToggled: SessionSettings.setVerbose(checked)
            }
        }
    }
}
