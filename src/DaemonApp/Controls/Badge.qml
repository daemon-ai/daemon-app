// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// A compact count-pill badge: a small rounded chip carrying a number, used to
// surface a pending count on a nav affordance (e.g. the sidebar Approvals
// entry / the settings cog). Hidden while `count` is 0 so a zero never shows.
// `tone` tints the fill ("danger" | "accent"); the label uses the theme's
// on-fill text color. Everything binds to Theme tokens (no hardcoded colors).
Rectangle {
    id: root

    property int count: 0
    property string tone: "danger"
    // Clamp the rendered label so a large count stays pill-sized ("99+").
    property int max: 99

    readonly property color fill: tone === "accent" ? Theme.accent : Theme.danger

    visible: count > 0
    implicitHeight: 16
    implicitWidth: Math.max(16, label.implicitWidth + 8)
    radius: height / 2
    color: fill

    QQC.Label {
        id: label
        anchors.centerIn: parent
        text: root.count > root.max ? (root.max + "+") : root.count
        font.family: FontIcons.display
        font.pixelSize: 10
        font.bold: true
        color: Theme.background
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
}
