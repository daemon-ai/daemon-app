import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Accounts manager: the connected accounts list over IAccountsService, plus an
// "Add account" button that opens the generic wizard. Status badges reflect
// connected / pending (OAuth in flight) / error.
Item {
    id: root

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Accounts")
        icon: FontIcons.fa_user
    }

    ColumnLayout {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            SectionLabel { text: qsTr("Connected accounts"); Layout.fillWidth: true }
            Kit.TextButton {
                text: qsTr("Add account")
                accentFilled: true
                onClicked: wizard.openFresh()
            }
        }

        Text {
            visible: Accounts.accounts.count === 0
            text: qsTr("No accounts yet. Add one to enable remote providers.")
            font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
        }

        QQC.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ListView {
                model: Accounts.accounts
                spacing: 8
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    required property var entry
                    width: ListView.view.width
                    height: 60
                    radius: 8
                    color: Theme.surface
                    border.color: Theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Kit.TextField {
                                text: entry.label
                                underline: true
                                font.pixelSize: 14
                                font.bold: true
                                Layout.fillWidth: true
                                // Commit the renamed account on Enter / focus loss.
                                onEditingFinished: if (text !== entry.label)
                                    Accounts.rename(entry.id, text)
                            }
                            Text {
                                text: (entry.kind === "oauth" ? qsTr("OAuth") : qsTr("API key"))
                                      + " · " + entry.detail
                                font.family: FontIcons.mono; font.pixelSize: 11
                                color: Theme.textMuted
                            }
                        }

                        Text {
                            text: entry.status === "connected" ? qsTr("Connected")
                                : entry.status === "pending" ? qsTr("Connecting…")
                                : qsTr("Error")
                            font.family: FontIcons.display; font.pixelSize: 11
                            color: entry.status === "connected" ? Theme.accent
                                 : entry.status === "pending" ? Theme.textMuted
                                 : Theme.danger
                        }
                        Kit.IconButton {
                            icon: FontIcons.fa_rotate; iconColor: Theme.iconMuted
                            iconPointSize: 13; implicitWidth: 32; implicitHeight: 28
                            tooltipText: qsTr("Reconnect / re-authenticate")
                            enabled: entry.status !== "pending"
                            onClicked: Accounts.reauth(entry.id)
                        }
                        Kit.IconButton {
                            icon: FontIcons.fa_trash; iconColor: Theme.danger
                            iconPointSize: 13; implicitWidth: 32; implicitHeight: 28
                            tooltipText: qsTr("Remove account")
                            onClicked: Accounts.remove(entry.id)
                        }
                    }
                }
            }
        }
    }

    AddAccountWizard { id: wizard }
}
