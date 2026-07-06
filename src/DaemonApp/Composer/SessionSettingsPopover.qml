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

    // [wave2:app-approvals-safety] D4: re-query the active session's remembered exec-approval
    // fingerprints when the popover opens (no-op on the mock / non-daemon facade).
    onOpened: if (SessionSettings.refreshFingerprints) SessionSettings.refreshFingerprints()

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
                    // Stored value is the canonical off/low/medium/high; show a
                    // translatable capitalized label to match the rest of the UI.
                    text: root.effortLabel(modelData)
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

        // --- Remembered approvals (D4) -----------------------------------
        // [wave2:app-approvals-safety] The per-session allow-list of exec-approval fingerprints the
        // user allowed permanently (FingerprintList/Revoke, wire v29). Provenance is limited to the
        // fingerprint hash (+ a label when the node ever fills it) — the node stores only the hash,
        // so no "what/when" is fabricated here.
        Text {
            text: qsTr("Remembered approvals")
            font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
        }
        Text {
            Layout.fillWidth: true
            visible: !SessionSettings.rememberedFingerprints
                     || SessionSettings.rememberedFingerprints.count === 0
            text: qsTr("No remembered approvals for this session.")
            font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
            wrapMode: Text.WordWrap
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: SessionSettings.rememberedFingerprints
                delegate: RowLayout {
                    required property var model
                    Layout.fillWidth: true
                    spacing: 6
                    Kit.Chip {
                        iconGlyph: FontIcons.fa_lock
                        tone: "muted"
                        text: model.label && String(model.label).length > 0
                              ? model.label : model.shortFingerprint
                        tooltipText: model.fingerprint
                    }
                    Item { Layout.fillWidth: true }
                    Kit.TextButton {
                        text: qsTr("Revoke")
                        onClicked: SessionSettings.revokeFingerprint(model.fingerprint)
                    }
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
