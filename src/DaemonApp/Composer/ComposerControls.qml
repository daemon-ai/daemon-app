// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The composer's right-hand control cluster - the QML port of Hermes'
// ComposerControls (apps/desktop/src/app/chat/composer/controls.tsx): a model
// pill, an optional steer button (mid-turn), and a high-contrast primary action
// that reads as send / queue / stop depending on busy + payload state.
//
// Voice dictation and the voice-session pill are intentionally omitted -
// there is no audio backend wired yet.
RowLayout {
    id: root

    // The primary-action rule (send/queue/stop + enablement) now lives in the
    // shared ComposerSessionController; this cluster just renders it.
    property string primaryAction: "send"
    property bool primaryActionEnabled: false
    // Steering is only meaningful mid-turn with a text-only draft.
    property bool canSteer: false
    property bool composerEnabled: true
    // The shared ComposerSessionController (drives the model picker + modes).
    property var session: null
    // Model selector list + selection, owned by the controller.
    property var modelList: []
    property int currentModelIndex: 0

    signal send()
    signal queue()
    signal stop()
    signal steer()
    signal modelSelected(int index)

    // Open the model picker overlay (forwarded from the composer for /model + palette).
    function openModelPicker() { modelPill.openOverlay(); }

    // Whether the bound session runs on a foreign ACP engine (gates rewind affordances - C4).
    readonly property bool foreignSession: engineChip.info.engine === "Acp"

    // Facade reads (profileFor/approvalModeFor) are invokables, so QML cannot track them; bump a
    // revision on the facades' change signals and reference it from the chip bindings.
    property int settingsRev: 0
    Connections {
        target: SessionSettings
        function onChanged() { root.settingsRev++; }
    }
    Connections {
        target: Profiles
        function onChanged() { root.settingsRev++; }
    }
    Connections {
        target: DaemonNet
        function onChanged() { root.settingsRev++; }
    }

    // The session's engine identity (C3/ENG-7): the per-session profile override, falling back to
    // the node's active default profile, joined against the profile store's engine/acpAgent.
    // Reads settingsRev first so bindings calling this re-evaluate on facade change.
    function sessionEngineInfo() {
        var rev = root.settingsRev;
        if (rev < 0 || !root.session || !root.session.sessionId)
            return ({ engine: "", agent: "" });
        var pid = SessionSettings.profileFor(root.session.sessionId);
        if (!pid || pid.length === 0)
            pid = Profiles.defaultProfileId;
        var p = Profiles.profile(pid);
        if (!p || p.engine === undefined)
            return ({ engine: "", agent: "" });
        return ({ engine: p.engine, agent: p.acpAgent || "" });
    }

    spacing: Theme.spacingSmall

    // Engine identity chip (C3/ENG-7): Native core vs the foreign ACP agent driving this session.
    Kit.Chip {
        id: engineChip
        Layout.alignment: Qt.AlignVCenter
        readonly property var info: root.sessionEngineInfo()
        visible: info.engine !== ""
        text: info.engine === "Acp" ? (info.agent || qsTr("Foreign")) : qsTr("Native")
        iconGlyph: info.engine === "Acp" ? FontIcons.fa_robot : FontIcons.fa_microchip
        tone: info.engine === "Acp" ? "accent" : "muted"
        tooltipText: qsTr("Engine")
    }

    // Approval-policy chip (E1/TOOL-7): the session's HITL mode at a glance; tap to change (opens
    // the session-settings popover, which mirrors the same value). Reflects the client's last-set
    // value — v28 wires no per-session mode getter.
    Kit.Chip {
        id: policyChip
        Layout.alignment: Qt.AlignVCenter
        readonly property string mode: {
            var rev = root.settingsRev; // re-evaluate on facade change
            return rev >= 0 && root.session && root.session.sessionId
                   ? SessionSettings.approvalModeFor(root.session.sessionId) : "";
        }
        visible: mode !== ""
        text: mode === "accept_edits" ? qsTr("Edits")
              : mode === "auto_allow" ? qsTr("Auto")
              : mode === "deny" ? qsTr("Deny")
              : qsTr("Ask")
        iconGlyph: mode === "deny" ? FontIcons.fa_lock : FontIcons.fa_shield_halved
        tone: mode === "auto_allow" ? "accent" : mode === "deny" ? "danger" : "muted"
        interactive: true
        tooltipText: qsTr("Approval policy (reflects last set value)")
        onClicked: {
            if (root.session)
                SessionSettings.sessionId = root.session.sessionId;
            sessionSettingsPopover.open();
        }
    }

    // Route-pin chip (B6/EIO-12): shows when an external chat origin is pinned to this session
    // ("⇄ #ops-room"); tap opens the routing manager scoped to the pin table.
    Kit.Chip {
        id: routeChip
        Layout.alignment: Qt.AlignVCenter
        readonly property var pins: {
            var rev = root.settingsRev; // re-evaluate on DaemonNet change
            return rev >= 0 && root.session && root.session.sessionId
                   ? DaemonNet.pinsForSession(root.session.sessionId) : [];
        }
        visible: pins.length > 0
        text: pins.length > 1 ? qsTr("⇄ %1 +%2").arg(pins[0].label).arg(pins.length - 1)
                              : pins.length === 1 ? qsTr("⇄ %1").arg(pins[0].label) : ""
        tone: "accent"
        interactive: true
        tooltipText: pins.length > 0
                     ? qsTr("Pinned from %1 — open the routing manager").arg(pins[0].transport)
                     : ""
        onClicked: Nav.open("routing")
    }

    // Checkpoints / rewind timeline for the active session.
    Kit.IconButton {
        id: checkpointsBtn
        Layout.alignment: Qt.AlignVCenter
        implicitWidth: 30
        implicitHeight: 30
        icon: FontIcons.fa_clock
        iconColor: Theme.iconMuted
        iconPointSize: 13
        tooltipText: qsTr("Checkpoints / rewind")
        onClicked: {
            // Bind the timeline to the focused chat so rewind is per-session.
            if (root.session)
                Checkpoints.sessionId = root.session.sessionId;
            checkpointsPopover.open();
        }

        CheckpointsPopover {
            id: checkpointsPopover
            foreignSession: root.foreignSession
        }
    }

    // Per-session settings (profile / effort / modes override).
    Kit.IconButton {
        id: sessionSettingsBtn
        Layout.alignment: Qt.AlignVCenter
        implicitWidth: 30
        implicitHeight: 30
        icon: FontIcons.fa_sliders
        iconColor: Theme.iconMuted
        iconPointSize: 13
        tooltipText: qsTr("Session settings")
        onClicked: {
            // Bind the overrides to the focused chat so they are per-session.
            if (root.session)
                SessionSettings.sessionId = root.session.sessionId;
            sessionSettingsPopover.open();
        }

        SessionSettingsPopover { id: sessionSettingsPopover }
    }

    ModelPill {
        id: modelPill
        Layout.alignment: Qt.AlignVCenter
        enabled: root.composerEnabled
        session: root.session
        models: root.modelList
        currentIndex: root.currentModelIndex
        onSelected: function(index) { root.modelSelected(index); }
    }

    // Mid-turn steer affordance: takes the slot a mic would otherwise occupy.
    Kit.IconButton {
        visible: root.canSteer
        Layout.alignment: Qt.AlignVCenter
        implicitWidth: 30
        implicitHeight: 30
        icon: FontIcons.fa_wand_magic_sparkles
        iconColor: Theme.iconMuted
        iconPointSize: 14
        tooltipText: qsTr("Steer the running turn (Ctrl+Enter)")
        enabled: root.composerEnabled
        onClicked: root.steer()
    }

    // Primary action: a solid high-contrast circle (black-on-white in light
    // themes, accent-tinted otherwise), mirroring PRIMARY_ICON_BTN.
    Item {
        id: primary
        Layout.alignment: Qt.AlignVCenter | Qt.AlignBottom
        implicitWidth: 32
        implicitHeight: 32

        readonly property string mode: root.primaryAction
        readonly property bool actionEnabled: root.primaryActionEnabled

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: {
                if (!primary.actionEnabled)
                    return Qt.rgba(Theme.text.r, Theme.text.g, Theme.text.b, 0.30);
                if (primaryArea.pressed)
                    return Qt.darker(Theme.text, 1.1);
                return Theme.text;
            }

            Behavior on color { ColorAnimation { duration: Theme.motionFast } }

            // Send (arrow-up) and queue (layers) use the glyph font; stop is a
            // small filled square drawn as a rectangle for a crisp shape.
            Text {
                anchors.centerIn: parent
                visible: primary.mode !== "stop"
                text: primary.mode === "queue" ? FontIcons.fa_layer_group : FontIcons.fa_arrow_up
                font.family: FontIcons.faSolid
                font.pixelSize: primary.mode === "queue" ? 14 : 15
                renderType: Text.NativeRendering
                color: Theme.background
            }

            Rectangle {
                anchors.centerIn: parent
                visible: primary.mode === "stop"
                width: 11
                height: 11
                radius: 2
                color: Theme.background
            }
        }

        MouseArea {
            id: primaryArea
            anchors.fill: parent
            hoverEnabled: true
            enabled: primary.actionEnabled
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (primary.mode === "queue")
                    root.queue();
                else if (primary.mode === "stop")
                    root.stop();
                else
                    root.send();
            }
        }
    }
}
