// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// A transient, self-dismissing message bar. Call show(text) to surface a brief notice near the
// bottom of the host; it fades out after `duration` ms. Used for best-effort-failure feedback
// where there is no blocking dialog (e.g. a clipboard path that no-ops in an insecure browser
// context, or an oversized attachment upload). Non-modal and click-through-free (it does not steal
// focus). Instantiate inside the surface that raises it and leave positioning to the default
// bottom-center anchor, or override x/y.
QQC.Popup {
    id: root

    // How long the toast stays up before auto-dismissing.
    property int duration: 4000
    // The message currently shown (set via show()).
    property string text: ""

    // Surface `message` and (re)start the dismiss timer.
    function show(message) {
        root.text = message;
        root.open();
        dismissTimer.restart();
    }

    modal: false
    focus: false
    closePolicy: QQC.Popup.NoAutoClose
    padding: 0

    // Default placement: bottom-center of the parent, clear of the very edge.
    parent: QQC.Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? parent.height - height - 24 : 0
    width: Math.min(420, (parent ? parent.width : 420) - 32)

    enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 120 } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 200 } }

    background: Rectangle {
        radius: Theme.radius
        color: Theme.text
        opacity: 0.95
    }

    contentItem: QQC.Label {
        text: root.text
        color: Theme.background
        font.family: FontIcons.display
        font.pixelSize: 13
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        padding: Theme.spacing
    }

    Timer {
        id: dismissTimer
        interval: root.duration
        repeat: false
        onTriggered: root.close()
    }
}
