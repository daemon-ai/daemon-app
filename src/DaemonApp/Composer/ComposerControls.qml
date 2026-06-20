import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The composer's right-hand control cluster - the QML port of Hermes'
// ComposerControls (apps/desktop/src/app/chat/composer/controls.tsx): a model
// pill, an optional steer button (mid-turn), and a high-contrast primary action
// that reads as send / queue / stop depending on busy + payload state.
//
// Voice dictation and the voice-conversation pill are intentionally omitted -
// there is no audio backend wired yet.
RowLayout {
    id: root

    property bool busy: false
    property bool hasPayload: false
    // Steering is only meaningful mid-turn with a text-only draft.
    property bool canSteer: false
    property bool composerEnabled: true

    signal send()
    signal queue()
    signal stop()
    signal steer()

    spacing: Theme.spacingSmall

    ModelPill {
        id: modelPill
        Layout.alignment: Qt.AlignVCenter
        enabled: root.composerEnabled
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

        readonly property string mode: root.busy ? (root.hasPayload ? "queue" : "stop") : "send"
        readonly property bool actionEnabled: root.composerEnabled && (root.mode !== "send" || root.hasPayload)

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
