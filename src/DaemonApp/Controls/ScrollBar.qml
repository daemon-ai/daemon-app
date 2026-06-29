// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// Slim, themed ScrollBar that fades the handle to the muted token and thickens
// on hover - Needs a full CustomVerticalScrollBar.
QQC.ScrollBar {
    id: root

    padding: 2

    contentItem: Rectangle {
        implicitWidth: root.hovered ? 8 : 6
        implicitHeight: root.hovered ? 8 : 6
        radius: width / 2
        color: Theme.textMuted
        opacity: root.pressed ? 0.8 : root.active ? 0.5 : 0.0

        Behavior on opacity { NumberAnimation { duration: 150 } }
        Behavior on implicitWidth { NumberAnimation { duration: 100 } }
    }
}
