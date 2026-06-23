import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Settings -> Connection. Shows the live gateway state and hosts the shared
// ConnectionPicker so reconfiguring the node uses the exact same control as the
// first-run gate.
ColumnLayout {
    id: root
    spacing: 16

    SectionLabel { text: qsTr("Status") }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        Rectangle {
            width: 10; height: 10; radius: 5
            color: Connection && Connection.ready ? Theme.accent
                 : (Connection && Connection.state === "offline" ? Theme.danger : Theme.warning)
        }
        Text {
            text: Connection ? (Connection.state + "  -  " + Connection.mode
                                + (Connection.target.length ? "  (" + Connection.target + ")" : ""))
                             : qsTr("unknown")
            font.family: FontIcons.mono; font.pixelSize: 13; color: Theme.textMuted
            Layout.fillWidth: true
        }
        // Drop the active connection (data stays readable from cache). Hidden once
        // already offline so the action only appears when there's a live link.
        Kit.TextButton {
            text: qsTr("Disconnect")
            visible: Connection && Connection.state !== "offline"
            onClicked: Connection.disconnect()
        }
    }

    ConnectionPicker {
        Layout.fillWidth: true
        showConnect: true
    }
}
