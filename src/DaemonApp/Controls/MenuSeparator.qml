// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// Themed MenuSeparator - a single token-colored hairline with small vertical
// breathing room, matching the dividers used in the kit's popups.
QQC.MenuSeparator {
    id: root

    leftPadding: 6
    rightPadding: 6
    topPadding: 4
    bottomPadding: 4

    contentItem: Rectangle {
        implicitHeight: 1
        color: Theme.border
    }

    // A non-interactive visual divider: keep it out of the accessibility tree.
    Accessible.ignored: true
}
