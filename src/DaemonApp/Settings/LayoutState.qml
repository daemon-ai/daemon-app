// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

pragma Singleton
import QtQuick
import DaemonApp.Theme

// Window size-class layer driving the adaptive shell. The root window writes
// windowWidth; the rest of the UI binds to isCompact / isExpanded to switch
// layout structure.
//
// Responsiveness applies only on touch devices (Theme.touch): desktop is always
// expanded and never affected by window width. On touch, a folded phone falls
// below the breakpoint and uses the compact single-pane shell, while a wide or
// unfolded device past the breakpoint uses the expanded multi-pane shell.
QtObject {
    id: root

    // Current window width in logical px; written by the shell (Main.qml).
    property real windowWidth: 0

    // Expanded needs room for the three-pane SplitView (160 + 220 + 320 mins).
    readonly property int expandedMin: 720

    readonly property bool isCompact: Theme.touch && windowWidth < expandedMin
    readonly property bool isExpanded: !isCompact
}
