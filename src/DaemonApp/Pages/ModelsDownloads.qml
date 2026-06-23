import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Downloads tab: live progress over ModelCatalog.downloads with pause/resume/cancel.
Item {
    id: root
    anchors.fill: parent
    anchors.margins: 20

    Text {
        anchors.centerIn: parent
        visible: ModelCatalog.downloads.count === 0
        text: qsTr("No active downloads.\nStart one from the Discover tab.")
        horizontalAlignment: Text.AlignHCenter
        font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
    }

    QQC.ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ListView {
            model: ModelCatalog.downloads
            spacing: 8
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                required property var entry
                width: ListView.view.width
                height: 72
                radius: 8
                color: Theme.surface
                border.color: Theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text {
                            text: entry.name
                            font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
                            Layout.fillWidth: true; elide: Text.ElideRight
                        }
                        Text {
                            text: Math.round(entry.progress * 100) + "%"
                            font.family: FontIcons.mono; font.pixelSize: 11; color: Theme.textMuted
                        }
                        Kit.IconButton {
                            icon: entry.state === "paused" ? FontIcons.fa_play : FontIcons.fa_pause
                            iconPointSize: 11; implicitWidth: 28; implicitHeight: 24
                            onClicked: entry.state === "paused"
                                       ? ModelCatalog.resumeDownload(entry.id)
                                       : ModelCatalog.pauseDownload(entry.id)
                        }
                        Kit.IconButton {
                            icon: FontIcons.fa_xmark; iconColor: Theme.danger
                            iconPointSize: 12; implicitWidth: 28; implicitHeight: 24
                            onClicked: ModelCatalog.cancelDownload(entry.id)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 6; radius: 3; color: Theme.hover
                        Rectangle {
                            width: parent.width * Math.max(0, Math.min(1, entry.progress))
                            height: parent.height; radius: 3
                            color: entry.state === "paused" ? Theme.textMuted : Theme.accent
                        }
                    }
                }
            }
        }
    }
}
