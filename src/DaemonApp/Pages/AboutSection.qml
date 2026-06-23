import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// About / updates. Reads the shared StatusBarModel for the app version and the
// connection seam for the current target. Updates are a declared platform
// capability (no-op today), shown as a disabled action.
ColumnLayout {
    id: root
    spacing: 16

    SectionLabel { text: qsTr("Application") }

    SettingRow {
        label: qsTr("Version")
        Text {
            text: Status ? Status.appVersion : "v0.1.0"
            font.family: FontIcons.mono; font.pixelSize: 13; color: Theme.textMuted
        }
    }
    SettingRow {
        label: qsTr("Connection")
        Text {
            text: Connection ? (Connection.mode + " - " + Connection.state) : qsTr("unknown")
            font.family: FontIcons.mono; font.pixelSize: 13; color: Theme.textMuted
        }
    }

    SectionLabel { text: qsTr("Updates") }

    Kit.TextButton {
        text: qsTr("Check for updates")
        enabled: false
    }
    Text {
        text: qsTr("Automatic updates are not configured in this build.")
        font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    Item { Layout.fillHeight: true }
}
