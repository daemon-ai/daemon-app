import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Settings

// Footer status bar: a thin full-width chrome
// strip with a left cluster (Command Center / Gateway / Agents / Cron) and a
// right cluster (running timer / context usage / session timer / YOLO / terminal
// / version). Items are data-driven via plain QML state with placeholder/live
// defaults; the seams below map 1:1 to the daemon backend for a later wiring
// pass (gateway health, agent counts, token usage, session timing, version).
Rectangle {
    id: root

    // --- Backend-bound state (placeholder/live for now) ---------------------
    // "ready" | "needs setup" | "checking" | "connecting" | "offline".
    property string gatewayState: "ready"
    property int agentsRunning: 0
    property int agentsFailed: 0
    // Turn (busy) timer.
    property bool busy: false
    property double turnStartedAt: 0
    // Session timer: starts at launch.
    property double sessionStartedAt: 0
    // Context window usage (tokens).
    property int contextUsed: 12500
    property int contextMax: 128000
    // Local toggles (drive active styling; routes/overlays land later).
    property bool yoloActive: false
    property bool terminalActive: false
    property bool commandCenterOpen: false
    property bool agentsOpen: false
    property bool cronOpen: false
    // Terminal item only shows in the chat view (always true for now).
    property bool chatOpen: true
    property string appVersion: "v0.1.0"

    // Ticks once per second so the elapsed-time labels stay live.
    property double nowMs: Date.now()

    // Slightly taller, finger-friendly strip on a touch phone (compact only
    // occurs on touch); the dense desktop height everywhere else.
    implicitHeight: LayoutState.isCompact ? Theme.tapTargetMin : Theme.statusBarHeight
    color: Theme.statusBarBackground

    Component.onCompleted: sessionStartedAt = Date.now()

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: root.nowMs = Date.now()
    }

    // --- Derived helpers ----------------------------------------------------
    readonly property bool gatewayOffline: gatewayState === "offline"
    readonly property bool gatewayDegraded: gatewayState === "needs setup"
        || gatewayState === "connecting" || gatewayState === "checking"

    function agentsDetail() {
        if (agentsFailed > 0)
            return agentsFailed === 1 ? qsTr("1 failed") : qsTr("%1 failed").arg(agentsFailed);
        if (agentsRunning > 0)
            return agentsRunning === 1 ? qsTr("1 running") : qsTr("%1 running").arg(agentsRunning);
        return "";
    }

    function formatElapsed(startMs) {
        if (!startMs)
            return "0:00";
        var total = Math.max(0, Math.floor((root.nowMs - startMs) / 1000));
        var h = Math.floor(total / 3600);
        var m = Math.floor((total % 3600) / 60);
        var s = total % 60;
        var ss = s < 10 ? "0" + s : "" + s;
        if (h > 0) {
            var mm = m < 10 ? "0" + m : "" + m;
            return h + ":" + mm + ":" + ss;
        }
        return m + ":" + ss;
    }

    function abbrev(n) {
        if (n >= 1000000)
            return (n / 1000000).toFixed(1).replace(/\.0$/, "") + "M";
        if (n >= 1000)
            return (n / 1000).toFixed(1).replace(/\.0$/, "") + "k";
        return "" + n;
    }

    readonly property int contextPercent: contextMax > 0
        ? Math.round((contextUsed / contextMax) * 100) : 0

    function contextBar() {
        var filled = Math.max(0, Math.min(10, Math.round(root.contextPercent / 10)));
        var bar = "";
        for (var i = 0; i < 10; ++i)
            bar += (i < filled ? "\u2588" : "\u2591");
        return "[" + bar + "] " + root.contextPercent + "%";
    }

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
            glyph: root.gatewayOffline || root.gatewayDegraded
                ? FontIcons.fa_circle_exclamation : FontIcons.fa_signal
            label: qsTr("Gateway")
            detail: root.gatewayState
            tone: root.gatewayOffline ? "danger" : root.gatewayDegraded ? "warning" : "default"
            active: gatewayMenu.opened
            onClicked: gatewayMenu.opened ? gatewayMenu.close() : gatewayMenu.open()

            GatewayMenu {
                id: gatewayMenu
                gatewayState: root.gatewayState
                y: -height - 6
                x: 0
            }
        }
        StatusBarItem {
            glyph: FontIcons.fa_wand_magic_sparkles
            label: qsTr("Agents")
            detail: root.agentsDetail()
            tone: root.agentsFailed > 0 ? "danger" : "default"
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
            visible: root.busy && root.turnStartedAt > 0
            variant: "text"
            glyph: FontIcons.fa_spinner
            spinning: true
            label: qsTr("Running")
            detail: root.formatElapsed(root.turnStartedAt)
        }
        StatusBarItem {
            visible: root.contextMax > 0 || root.contextUsed > 0
            variant: "text"
            label: root.contextMax > 0
                ? root.abbrev(root.contextUsed) + "/" + root.abbrev(root.contextMax)
                : root.abbrev(root.contextUsed) + qsTr(" tok")
            detail: root.contextMax > 0 ? root.contextBar() : ""
        }
        StatusBarItem {
            variant: "text"
            label: qsTr("Session")
            detail: root.formatElapsed(root.sessionStartedAt)
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
            label: root.appVersion
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
            glyph: root.gatewayOffline || root.gatewayDegraded
                ? FontIcons.fa_circle_exclamation : FontIcons.fa_signal
            label: qsTr("Gateway")
            detail: root.gatewayState
            tone: root.gatewayOffline ? "danger" : root.gatewayDegraded ? "warning" : "default"
            active: gatewayMenuCompact.opened
            onClicked: gatewayMenuCompact.opened ? gatewayMenuCompact.close() : gatewayMenuCompact.open()

            GatewayMenu {
                id: gatewayMenuCompact
                gatewayState: root.gatewayState
                y: -height - 6
                x: 0
            }
        }

        StatusBarItem {
            visible: root.busy && root.turnStartedAt > 0
            variant: "text"
            glyph: FontIcons.fa_spinner
            spinning: true
            label: root.formatElapsed(root.turnStartedAt)
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
                        detail: root.agentsDetail()
                        tone: root.agentsFailed > 0 ? "danger" : "default"
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
                        visible: root.contextMax > 0 || root.contextUsed > 0
                        variant: "text"
                        label: root.contextMax > 0
                            ? root.abbrev(root.contextUsed) + "/" + root.abbrev(root.contextMax)
                            : root.abbrev(root.contextUsed) + qsTr(" tok")
                        detail: root.contextMax > 0 ? root.contextBar() : ""
                    }
                    StatusBarItem {
                        Layout.fillWidth: true
                        Layout.preferredHeight: overflowSheet.rowHeight
                        variant: "text"
                        label: qsTr("Session")
                        detail: root.formatElapsed(root.sessionStartedAt)
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
                        label: root.appVersion
                    }
                }
            }
        }
    }
}
