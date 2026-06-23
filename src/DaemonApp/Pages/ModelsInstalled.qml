import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Installed tab: locally installed models with Activate (set default) / Remove.
Item {
    id: root
    anchors.fill: parent
    anchors.margins: 20

    Text {
        anchors.centerIn: parent
        visible: ModelCatalog.installed.count === 0
        text: qsTr("No models installed yet.")
        font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
    }

    QQC.ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ListView {
            model: ModelCatalog.installed
            spacing: 8
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                required property var entry
                width: ListView.view.width
                height: 64
                radius: 8
                color: Theme.surface
                border.color: entry.active ? Theme.accent : Theme.border
                border.width: entry.active ? 2 : 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        RowLayout {
                            spacing: 8
                            Text {
                                text: entry.name
                                font.family: FontIcons.display; font.pixelSize: 14
                                font.bold: true; color: Theme.text
                            }
                            Text {
                                visible: entry.active === true
                                text: qsTr("ACTIVE")
                                font.family: FontIcons.display; font.pixelSize: 10
                                font.bold: true; color: Theme.accent
                            }
                        }
                        Text {
                            text: entry.params + " · " + entry.sizeGiB + " GiB · " + entry.provider
                            font.family: FontIcons.mono; font.pixelSize: 11; color: Theme.textMuted
                        }
                    }

                    Kit.TextButton {
                        text: entry.active ? qsTr("Active") : qsTr("Activate")
                        enabled: !entry.active
                        onClicked: ModelCatalog.activate(entry.id)
                    }
                    Kit.IconButton {
                        icon: FontIcons.fa_trash; iconColor: Theme.danger
                        iconPointSize: 13; implicitWidth: 32; implicitHeight: 28
                        tooltipText: qsTr("Remove")
                        onClicked: ModelCatalog.remove(entry.id)
                    }
                }
            }
        }
    }
}
