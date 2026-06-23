import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Discover tab: search + size filter over ModelCatalog.discover, each row a model
// card with a Download / Installed action.
ColumnLayout {
    id: root
    anchors.fill: parent
    anchors.margins: 20
    spacing: 12

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Kit.TextField {
            id: query
            Layout.fillWidth: true
            placeholderText: qsTr("Search models (name or provider)")
            onTextEdited: ModelCatalog.search(text, sizeFilter.currentText === qsTr("Any size")
                                              ? "" : sizeFilter.currentText)
        }
        Kit.Dropdown {
            id: sizeFilter
            implicitWidth: 130
            model: [qsTr("Any size"), "<=8B", ">8B"]
            onActivated: ModelCatalog.search(query.text,
                                             currentText === qsTr("Any size") ? "" : currentText)
        }
    }

    QQC.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        contentWidth: availableWidth

        ListView {
            model: ModelCatalog.discover
            spacing: 8
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                required property var entry
                width: ListView.view.width
                height: 78
                radius: 8
                color: Theme.surface
                border.color: Theme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        RowLayout {
                            spacing: 8
                            Text {
                                text: entry.name
                                font.family: FontIcons.display; font.pixelSize: 14
                                font.bold: true; color: Theme.text
                            }
                            Rectangle {
                                radius: 3; color: Theme.hover
                                implicitWidth: prov.implicitWidth + 10
                                implicitHeight: prov.implicitHeight + 4
                                Text {
                                    id: prov; anchors.centerIn: parent; text: entry.provider
                                    font.family: FontIcons.display; font.pixelSize: 10
                                    color: Theme.textMuted
                                }
                            }
                        }
                        Text {
                            text: entry.blurb
                            font.family: FontIcons.display; font.pixelSize: 12
                            color: Theme.textMuted; elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: entry.params + " · " + entry.sizeGiB + " GiB · " + entry.quant
                            font.family: FontIcons.mono; font.pixelSize: 11
                            color: Theme.textMuted
                        }
                    }

                    Kit.TextButton {
                        text: entry.installed ? qsTr("Installed") : qsTr("Download")
                        enabled: !entry.installed
                        accentFilled: !entry.installed
                        onClicked: ModelCatalog.download(entry.id)
                    }
                }
            }
        }
    }
}
