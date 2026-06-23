import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Fleet tree: the flattened orchestrator/worker hierarchy with indentation by
// depth, a status badge, and pause/resume.
Item {
    id: root

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Fleet")
        icon: FontIcons.fa_sitemap
    }

    QQC.ScrollView {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        clip: true
        contentWidth: availableWidth

        ListView {
            model: FleetTree.nodes
            spacing: 6
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                id: fleetRow
                required property var entry
                // Click the row to drill into the node's detail.
                property bool expanded: false
                width: ListView.view.width
                height: fbody.implicitHeight + 20
                radius: 8
                color: Theme.surface
                border.color: Theme.border

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: fleetRow.expanded = !fleetRow.expanded
                }

                ColumnLayout {
                    id: fbody
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 12 + entry.depth * 24
                    anchors.rightMargin: 12
                    anchors.topMargin: 10
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Text {
                            text: entry.kind === "orchestrator" ? FontIcons.fa_sitemap
                                                                : FontIcons.fa_robot
                            font.family: FontIcons.faSolid; font.pixelSize: 13
                            color: Theme.iconMuted
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                text: entry.name
                                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
                                elide: Text.ElideRight; Layout.fillWidth: true
                            }
                            Text {
                                text: entry.model
                                font.family: FontIcons.mono; font.pixelSize: 10; color: Theme.textMuted
                            }
                        }

                        Text {
                            text: entry.status
                            font.family: FontIcons.display; font.pixelSize: 11
                            color: entry.status === "running" ? Theme.accent : Theme.textMuted
                        }
                        Kit.IconButton {
                            icon: entry.status === "paused" ? FontIcons.fa_play : FontIcons.fa_pause
                            iconPointSize: 11; implicitWidth: 30; implicitHeight: 26
                            tooltipText: entry.status === "paused" ? qsTr("Resume") : qsTr("Pause")
                            onClicked: entry.status === "paused" ? FleetTree.resume(entry.id)
                                                                : FleetTree.pause(entry.id)
                        }
                    }

                    // --- Drill-in detail panel ----------------------------------
                    ColumnLayout {
                        visible: fleetRow.expanded
                        Layout.fillWidth: true
                        spacing: 3
                        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
                        Text {
                            text: qsTr("%1 · %2 · depth %3").arg(entry.id).arg(entry.kind)
                                  .arg(entry.depth)
                            font.family: FontIcons.mono; font.pixelSize: 11; color: Theme.textMuted
                        }
                    }
                }
            }
        }
    }
}
