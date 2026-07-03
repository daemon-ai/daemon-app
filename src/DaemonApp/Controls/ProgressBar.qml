// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import DaemonApp.Theme

// A thin themed determinate progress bar (value 0..1): muted track, accent fill.
// Set `fillColor` to render a non-default state (e.g. a paused download in the
// muted token). Replaces the hand-rolled nested-Rectangle bars.
Item {
    id: root

    // Completed fraction, clamped to 0..1 when rendered.
    property real value: 0
    property color fillColor: Theme.accent

    implicitWidth: 120
    implicitHeight: 6

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: Theme.hover

        Rectangle {
            width: parent.width * Math.max(0, Math.min(1, root.value))
            height: parent.height
            radius: parent.radius
            color: root.fillColor
        }
    }
}
