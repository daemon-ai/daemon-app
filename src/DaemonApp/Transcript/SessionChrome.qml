import QtQuick

import DaemonApp.Theme

// Streaming-state session chrome that overlays the transcript bottom,
// driven entirely by the turn driver (TurnSimulator or a future gateway). Pure
// overlay: no input handlers, so the ListView underneath still scrolls.
//   N27 response loading + elapsed timer
//   N28 "still thinking" stall
//   N31 assistant error alert
Item {
    id: root

    property bool active: false
    property string turnState: "idle"
    property int elapsedMs: 0
    property string errorText: ""

    readonly property bool showLoading: active && turnState !== "error"
    readonly property bool showError: turnState === "error" && errorText.length > 0
    readonly property int elapsedSeconds: Math.floor(elapsedMs / 1000)

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spacing
        spacing: Theme.smallSpacing

        // N31 - assistant error alert.
        Rectangle {
            id: errorBar
            visible: root.showError
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width, errorRow.implicitWidth + Theme.spacing * 2)
            implicitHeight: errorRow.implicitHeight + Theme.smallSpacing * 2
            radius: Theme.radiusSmall
            color: Theme.toolSurface
            border.width: Theme.hairline
            border.color: Theme.danger

            Row {
                id: errorRow
                anchors.centerIn: parent
                spacing: Theme.smallSpacing

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: FontIcons.fa_circle_exclamation
                    font.family: FontIcons.faSolid
                    font.pixelSize: Theme.captionFontSize
                    color: Theme.danger
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.errorText
                    color: Theme.text
                    font.pixelSize: Theme.captionFontSize
                    wrapMode: Text.Wrap
                }
            }
        }

        // N27 / N28 - loading + elapsed timer, with the stall variant.
        Rectangle {
            id: loadingPill
            visible: root.showLoading
            anchors.horizontalCenter: parent.horizontalCenter
            width: loadingRow.implicitWidth + Theme.spacing * 2
            implicitHeight: loadingRow.implicitHeight + Theme.smallSpacing * 2
            radius: height / 2
            color: Theme.toolSurface
            border.width: Theme.hairline
            border.color: Theme.toolBorder
            opacity: 0.96

            Row {
                id: loadingRow
                anchors.centerIn: parent
                spacing: Theme.smallSpacing

                Text {
                    id: spinner
                    anchors.verticalCenter: parent.verticalCenter
                    text: FontIcons.fa_spinner
                    font.family: FontIcons.faSolid
                    font.pixelSize: Theme.captionFontSize
                    color: Theme.statusRunning

                    RotationAnimation on rotation {
                        running: loadingPill.visible
                        from: 0; to: 360
                        duration: 900
                        loops: Animation.Infinite
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: {
                        if (root.turnState === "stalled")
                            return qsTr("Still thinking…")
                        return root.elapsedSeconds >= 1
                            ? qsTr("Thinking… %1s").arg(root.elapsedSeconds)
                            : qsTr("Thinking…")
                    }
                    color: Theme.mutedText
                    font.pixelSize: Theme.captionFontSize
                }
            }
        }
    }
}
