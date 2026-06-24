import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// One entry in the footer status bar
// STATUSBAR_ACTION_CLASS / text item. Renders [icon] [label] [muted detail] at
// 11px, full bar height. `action` items get hover/active fills and a pointing
// cursor; `text` items are inert read-outs (timers, context usage).
Item {
    id: root

    // Glyph string (FontIcons.fa_*). Empty => no icon.
    property string glyph: ""
    property string iconFamily: FontIcons.faSolid
    // Primary label; empty => icon-only item (centered, fixed width).
    property string label: ""
    // Trailing muted detail (e.g. "ready", elapsed time, usage bar).
    property string detail: ""
    // "action" (interactive) | "text" (inert read-out).
    property string variant: "action"
    // Toggled-on appearance (accent fill), e.g. YOLO/Terminal/Command Center.
    property bool active: false
    // "default" | "warning" | "danger" | "primary" - colors the icon+label.
    property string tone: "default"
    // Tooltip shown on hover (used for icon-only items).
    property string tooltipText: ""
    // Spin the icon (running/loading spinner).
    property bool spinning: false

    readonly property bool iconOnly: label === ""
    readonly property bool interactive: variant === "action" && enabled

    // Captures keyboard modifiers for callers that need alternate click behavior.
    signal clicked(var modifiers)

    implicitHeight: parent ? parent.height : Theme.statusBarHeight
    implicitWidth: iconOnly ? (Theme.touch ? Theme.tapTargetMin : 28) : row.implicitWidth + 12

    function toneColor(base) {
        switch (root.tone) {
        case "warning": return Theme.warning;
        case "danger": return Theme.danger;
        case "primary": return Theme.accent;
        default: return base;
        }
    }

    readonly property bool hovered: mouseArea.containsMouse

    Rectangle {
        anchors.fill: parent
        radius: 0
        color: root.active ? Qt.alpha(Theme.accent, Theme.isDarkMode ? 0.42 : 0.30)
             : (root.interactive && root.hovered) ? Theme.statusBarHover
             : "transparent"
    }

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 4

        Kit.Glyph {
            visible: root.glyph !== ""
            glyph: root.glyph
            family: root.iconFamily
            font.pointSize: 11 + Theme.pointSizeOffset
            color: root.active ? Theme.text
                 : (root.interactive && root.hovered) ? Theme.text
                 : root.toneColor(Theme.statusBarText)
            opacity: root.enabled ? 1.0 : 0.45
            Layout.alignment: Qt.AlignVCenter

            // Continuous rotation for the loading spinner.
            RotationAnimation on rotation {
                running: root.spinning
                loops: Animation.Infinite
                from: 0
                to: 360
                duration: 900
            }
        }

        QQC.Label {
            visible: root.label !== ""
            text: root.label
            font.family: FontIcons.display
            font.pixelSize: 11
            elide: Text.ElideRight
            color: root.active ? Theme.text
                 : (root.interactive && root.hovered) ? Theme.text
                 : root.toneColor(Theme.statusBarText)
            opacity: root.enabled ? 1.0 : 0.45
            Layout.alignment: Qt.AlignVCenter
        }

        QQC.Label {
            visible: root.detail !== ""
            text: root.detail
            font.family: FontIcons.display
            font.pixelSize: 11
            elide: Text.ElideRight
            color: Qt.alpha(root.toneColor(Theme.statusBarText), 0.8)
            opacity: root.enabled ? 1.0 : 0.45
            Layout.alignment: Qt.AlignVCenter
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: root.interactive
        hoverEnabled: root.interactive
        cursorShape: Qt.PointingHandCursor
        onClicked: function(mouse) { root.clicked(mouse.modifiers); }
    }

    // Themed tooltip (avoids the bland Basic-styled attached ToolTip). Shown
    // above the item since the status bar sits at the bottom of the window.
    Kit.Tooltip {
        text: root.tooltipText
        visible: root.tooltipText.length > 0 && root.hovered
        x: Math.round((root.width - implicitWidth) / 2)
        y: -implicitHeight - 6
    }
}
