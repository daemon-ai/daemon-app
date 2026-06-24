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

    spacing: Theme.spacingSmall

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

        CheckpointsPopover { id: checkpointsPopover }
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
