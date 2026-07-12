// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// Themed ToolTip for cases where a tooltip is instantiated directly. IconButton
// uses the attached ToolTip; this is the reusable styled variant.
QQC.ToolTip {
    id: root

    delay: 500
    font.family: FontIcons.display
    font.pixelSize: 12

    contentItem: Text {
        text: root.text
        font: root.font
        color: Theme.background

        // The tooltip text is supplementary chrome for the control it describes;
        // screen readers announce that control directly, so keep the tip out of
        // the accessibility tree. (Attached here on the Item-derived contentItem;
        // the ToolTip root is a Popup and cannot carry Accessible.*)
        Accessible.ignored: true
    }

    background: Rectangle {
        radius: Theme.radius
        color: Theme.text
    }
}
