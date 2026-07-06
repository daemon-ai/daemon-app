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

    // C4: the bound session runs on a foreign engine — reasoning effort + fast/verbose are
    // inference tunables the foreign agent manages, so they are hidden (with an honest note).
    // Profile override + approval mode still apply (foreign approvals ride the same inbox).
    property bool foreignSession: false

    // Translatable label for a reasoning-effort segment; the stored value stays
    // the canonical off/low/medium/high token.
    function effortLabel(v) {
        switch (v) {
        case "off": return qsTr("Off");
        case "low": return qsTr("Low");
        case "medium": return qsTr("Medium");
        case "high": return qsTr("High");
        }
        return v;
    }

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

        // --- Reasoning effort (native engines only) ----------------------
        Text {
            visible: !root.foreignSession
            text: qsTr("Reasoning effort")
            font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
        }
        RowLayout {
            visible: !root.foreignSession
            Layout.fillWidth: true
            spacing: 6
            Repeater {
                model: SessionSettings.effortOptions()
                delegate: Kit.TextButton {
                    required property var modelData
                    Layout.fillWidth: true
                    // Stored value is the canonical off/low/medium/high; show a
                    // translatable capitalized label to match the rest of the UI.
                    text: root.effortLabel(modelData)
                    accentFilled: SessionSettings.effort === modelData
                    onClicked: SessionSettings.setEffort(modelData)
                }
            }
        }

        // C4 honesty: say WHY reasoning/modes are absent for a foreign engine.
        Text {
            visible: root.foreignSession
            Layout.fillWidth: true
            text: qsTr("Reasoning effort and modes are managed by the foreign agent.")
            font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
            wrapMode: Text.WordWrap
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

        // --- Modes (native engines only) ---------------------------------
        RowLayout {
            visible: !root.foreignSession
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
            visible: !root.foreignSession
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
