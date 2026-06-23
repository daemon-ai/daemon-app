import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Cron manager: scheduled jobs with schedule/profile, enable toggle, run-now, and
// an edit dialog for create/update.
Item {
    id: root

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Scheduled jobs")
        icon: FontIcons.fa_clock
    }

    ColumnLayout {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            SectionLabel { text: qsTr("Jobs"); Layout.fillWidth: true }
            Kit.TextButton {
                text: qsTr("+ New job")
                accentFilled: true
                onClicked: dialog.openFor("")
            }
        }

        QQC.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ListView {
                model: Cron.jobs
                spacing: 8
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    required property var entry
                    width: ListView.view.width
                    height: 72
                    radius: 8
                    color: Theme.surface
                    border.color: Theme.border
                    opacity: entry.enabled ? 1.0 : 0.55

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: entry.name
                                font.family: FontIcons.display; font.pixelSize: 14
                                font.bold: true; color: Theme.text
                            }
                            Text {
                                text: entry.schedule + " · " + entry.profile
                                font.family: FontIcons.mono; font.pixelSize: 11; color: Theme.textMuted
                            }
                            Text {
                                text: qsTr("next ") + entry.nextRun + qsTr("  ·  last ") + entry.lastRun
                                font.family: FontIcons.display; font.pixelSize: 10
                                color: Theme.textMuted
                            }
                        }

                        Kit.TextButton { text: qsTr("Run now"); onClicked: Cron.runNow(entry.id) }
                        Kit.IconButton {
                            icon: FontIcons.fa_gear; iconPointSize: 12
                            implicitWidth: 30; implicitHeight: 26
                            tooltipText: qsTr("Edit")
                            onClicked: dialog.openFor(entry.id)
                        }
                        Kit.Switch {
                            checked: entry.enabled
                            onToggled: Cron.setEnabled(entry.id, checked)
                        }
                        Kit.IconButton {
                            icon: FontIcons.fa_trash; iconColor: Theme.danger
                            iconPointSize: 12; implicitWidth: 30; implicitHeight: 26
                            onClicked: Cron.remove(entry.id)
                        }
                    }
                }
            }
        }
    }

    CronJobDialog { id: dialog }
}
