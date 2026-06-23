import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Checkpoints / rewind timeline popover (opens upward from the composer): the
// session's checkpoints with a Restore action. Backed by the Checkpoints facade
// (mock); restoring rewinds to that point.
QQC.Popup {
    id: root

    implicitWidth: 340
    padding: 12
    // Open upward, right-aligned to the trigger button so the panel grows leftward
    // into the window (the buttons sit at the window's right edge).
    y: -implicitHeight - 8
    x: parent ? parent.width - width : 0

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Checkpoints")
                font.family: FontIcons.display; font.pixelSize: 14; font.bold: true
                color: Theme.text; Layout.fillWidth: true
            }
            Kit.TextButton {
                text: qsTr("Save here")
                onClicked: Checkpoints.createCheckpoint(qsTr("Manual checkpoint"))
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 260)
            clip: true
            model: Checkpoints.checkpoints
            spacing: 6
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                required property var entry
                width: ListView.view.width
                height: 48
                radius: 6
                color: entry.current ? Theme.hover : "transparent"
                border.color: entry.current ? Theme.accent : Theme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 8
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            text: entry.label
                            font.family: FontIcons.display; font.pixelSize: 12; color: Theme.text
                            elide: Text.ElideRight; Layout.fillWidth: true
                        }
                        Text {
                            text: entry.time + " · " + entry.tokens + " tok"
                            font.family: FontIcons.mono; font.pixelSize: 10; color: Theme.textMuted
                        }
                    }

                    Text {
                        visible: entry.current === true
                        text: qsTr("current")
                        font.family: FontIcons.display; font.pixelSize: 10; color: Theme.accent
                    }
                    Kit.TextButton {
                        visible: entry.current !== true
                        text: qsTr("Restore")
                        onClicked: Checkpoints.restore(entry.id)
                    }
                }
            }
        }
    }
}
