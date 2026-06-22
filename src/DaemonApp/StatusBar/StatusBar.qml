import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Settings
import DaemonApp.StatusModel

// Footer status bar: a thin full-width chrome
// strip with a left cluster (Command Center / Gateway / Agents / Cron) and a
// right cluster (running timer / context usage / session timer / YOLO / terminal
// / version). Items are data-driven via plain QML state with placeholder/live
// defaults; the seams below map 1:1 to the daemon backend for a later wiring
// pass (gateway health, agent counts, token usage, session timing, version).
Rectangle {
    id: root

    // --- Backend-bound state + formatting -----------------------------------
    // The daemon-bound status state and all derived/formatted strings now live in
    // the shared C++ StatusBarModel (DaemonApp.StatusModel), so the GUI footer and
    // the TUI footer share one formatting path and one 1s live tick. Placeholder
    // defaults are unchanged, so pixels/behavior here are identical.
    StatusBarModel {
        id: model
    }

    // --- Local toggles (route + visual state; stay in QML) ------------------
    property bool yoloActive: false
    property bool terminalActive: false
    property bool commandCenterOpen: false
    property bool agentsOpen: false
    property bool cronOpen: false
    // Terminal item only shows in the chat view (always true for now).
    property bool chatOpen: true

    // Slightly taller, finger-friendly strip on a touch phone (compact only
    // occurs on touch); the dense desktop height everywhere else.
    implicitHeight: LayoutState.isCompact ? Theme.tapTargetMin : Theme.statusBarHeight
    color: Theme.statusBarBackground

    // Top hairline separating the bar from the content above.
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: Theme.hairline
        color: Theme.border
    }

    // --- Left cluster -------------------------------------------------------
    RowLayout {
        id: leftCluster
        visible: !LayoutState.isCompact
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        spacing: 2

        StatusBarItem {
            glyph: FontIcons.fa_terminal
            tooltipText: qsTr("Command Center")
            active: root.commandCenterOpen
            onClicked: root.commandCenterOpen = !root.commandCenterOpen
        }
        StatusBarItem {
            id: gatewayItem
            glyph: model.gatewayOffline || model.gatewayDegraded
                ? FontIcons.fa_circle_exclamation : FontIcons.fa_signal
            label: qsTr("Gateway")
            detail: model.gatewayState
            tone: model.gatewayTone
            active: gatewayMenu.opened
            onClicked: gatewayMenu.opened ? gatewayMenu.close() : gatewayMenu.open()

            GatewayMenu {
                id: gatewayMenu
                statusModel: model
                y: -height - 6
                x: 0
            }
        }
        StatusBarItem {
            glyph: FontIcons.fa_wand_magic_sparkles
            label: qsTr("Agents")
            detail: model.agentsDetail
            tone: model.agentsFailed > 0 ? "danger" : "default"
            active: root.agentsOpen
            onClicked: root.agentsOpen = !root.agentsOpen
        }
        StatusBarItem {
            glyph: FontIcons.fa_clock
            label: qsTr("Cron")
            active: root.cronOpen
            onClicked: root.cronOpen = !root.cronOpen
        }
    }

    // --- Right cluster ------------------------------------------------------
    RowLayout {
        id: rightCluster
        visible: !LayoutState.isCompact
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        spacing: 2

        StatusBarItem {
            visible: model.busy && model.turnStartedAt > 0
            variant: "text"
            glyph: FontIcons.fa_spinner
            spinning: true
            label: qsTr("Running")
            detail: model.turnElapsed
        }
        StatusBarItem {
            visible: model.contextMax > 0 || model.contextUsed > 0
            variant: "text"
            label: model.contextLabel
            detail: model.contextMax > 0 ? model.contextBar : ""
        }
        StatusBarItem {
            variant: "text"
            label: qsTr("Session")
            detail: model.sessionElapsed
        }
        StatusBarItem {
            glyph: FontIcons.fa_bolt
            tooltipText: qsTr("YOLO (Shift-click for global)")
            active: root.yoloActive
            onClicked: function(modifiers) { root.yoloActive = !root.yoloActive; }
        }
        StatusBarItem {
            visible: root.chatOpen
            glyph: FontIcons.fa_terminal
            tooltipText: qsTr("Terminal")
            active: root.terminalActive
            onClicked: root.terminalActive = !root.terminalActive
        }
        StatusBarItem {
            glyph: FontIcons.fa_hashtag
            label: model.appVersion
            tooltipText: qsTr("Check for updates")
        }
    }

    // --- Compact strip ------------------------------------------------------
    // On a phone the dense two-cluster bar does not fit, so we show only the
    // gateway state (plus the running timer when a turn is active) and fold the
    // rest behind a tap-to-expand sheet.
    RowLayout {
        id: compactCluster
        visible: LayoutState.isCompact
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        spacing: 2

        StatusBarItem {
            glyph: model.gatewayOffline || model.gatewayDegraded
                ? FontIcons.fa_circle_exclamation : FontIcons.fa_signal
            label: qsTr("Gateway")
            detail: model.gatewayState
            tone: model.gatewayTone
            active: gatewayMenuCompact.opened
            onClicked: gatewayMenuCompact.opened ? gatewayMenuCompact.close() : gatewayMenuCompact.open()

            GatewayMenu {
                id: gatewayMenuCompact
                statusModel: model
                y: -height - 6
                x: 0
            }
        }

        StatusBarItem {
            visible: model.busy && model.turnStartedAt > 0
            variant: "text"
            glyph: FontIcons.fa_spinner
            spinning: true
            label: model.turnElapsed
        }

        Item { Layout.fillWidth: true }

        StatusBarItem {
            glyph: FontIcons.fa_ellipsis_h
            tooltipText: qsTr("More")
            active: overflowSheet.opened
            onClicked: overflowSheet.opened ? overflowSheet.close() : overflowSheet.open()

            // Tap-to-expand sheet listing the full status set, opening upward
            // from the strip.
            QQC.Popup {
                id: overflowSheet
                y: -implicitHeight - 6
                // Right-align to the overflow button.
                x: parent.width - width
                padding: 4
                modal: false

                readonly property int rowHeight: Theme.touch ? Theme.tapTargetMin : 30

                background: Rectangle {
                    color: Theme.statusBarBackground
                    border.width: 1
                    border.color: Theme.border
                    radius: Theme.radius
                }

                contentItem: ColumnLayout {
                    spacing: 0

                    StatusBarItem {
                        Layout.fillWidth: true
                        Layout.preferredHeight: overflowSheet.rowHeight
                        glyph: FontIcons.fa_terminal
                        label: qsTr("Command Center")
                        active: root.commandCenterOpen
                        onClicked: { root.commandCenterOpen = !root.commandCenterOpen; overflowSheet.close(); }
                    }
                    StatusBarItem {
                        Layout.fillWidth: true
                        Layout.preferredHeight: overflowSheet.rowHeight
                        glyph: FontIcons.fa_wand_magic_sparkles
                        label: qsTr("Agents")
                        detail: model.agentsDetail
                        tone: model.agentsFailed > 0 ? "danger" : "default"
                        active: root.agentsOpen
                        onClicked: { root.agentsOpen = !root.agentsOpen; overflowSheet.close(); }
                    }
                    StatusBarItem {
                        Layout.fillWidth: true
                        Layout.preferredHeight: overflowSheet.rowHeight
                        glyph: FontIcons.fa_clock
                        label: qsTr("Cron")
                        active: root.cronOpen
                        onClicked: { root.cronOpen = !root.cronOpen; overflowSheet.close(); }
                    }
                    StatusBarItem {
                        Layout.fillWidth: true
                        Layout.preferredHeight: overflowSheet.rowHeight
                        visible: model.contextMax > 0 || model.contextUsed > 0
                        variant: "text"
                        label: model.contextLabel
                        detail: model.contextMax > 0 ? model.contextBar : ""
                    }
                    StatusBarItem {
                        Layout.fillWidth: true
                        Layout.preferredHeight: overflowSheet.rowHeight
                        variant: "text"
                        label: qsTr("Session")
                        detail: model.sessionElapsed
                    }
                    StatusBarItem {
                        Layout.fillWidth: true
                        Layout.preferredHeight: overflowSheet.rowHeight
                        glyph: FontIcons.fa_bolt
                        label: qsTr("YOLO")
                        active: root.yoloActive
                        onClicked: function(modifiers) { root.yoloActive = !root.yoloActive; overflowSheet.close(); }
                    }
                    StatusBarItem {
                        Layout.fillWidth: true
                        Layout.preferredHeight: overflowSheet.rowHeight
                        visible: root.chatOpen
                        glyph: FontIcons.fa_terminal
                        label: qsTr("Terminal")
                        active: root.terminalActive
                        onClicked: { root.terminalActive = !root.terminalActive; overflowSheet.close(); }
                    }
                    StatusBarItem {
                        Layout.fillWidth: true
                        Layout.preferredHeight: overflowSheet.rowHeight
                        glyph: FontIcons.fa_hashtag
                        label: model.appVersion
                    }
                }
            }
        }
    }
}
