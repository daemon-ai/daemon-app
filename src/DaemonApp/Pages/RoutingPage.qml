import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Routing matrix: intent rules mapped to a target model (+ fallback), each with
// an enable toggle. The target/fallback dropdowns are bound to Routing.targets().
Item {
    id: root

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Routing")
        icon: FontIcons.fa_sitemap
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
            SectionLabel { text: qsTr("Intent → model rules"); Layout.fillWidth: true }
            Kit.TextButton {
                text: qsTr("+ Add rule")
                accentFilled: true
                onClicked: Routing.addRule(qsTr("New rule"))
            }
        }

        QQC.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ListView {
                model: Routing.rules
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

                        Kit.TextField {
                            text: entry.intent
                            underline: true
                            font.pixelSize: 13
                            Layout.preferredWidth: 200
                            // Commit the renamed intent on Enter / focus loss.
                            onEditingFinished: if (text !== entry.intent)
                                Routing.setIntent(entry.id, text)
                        }

                        Text {
                            text: "\u2192"
                            font.family: FontIcons.display; font.pixelSize: 13; color: Theme.iconMuted
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Kit.Dropdown {
                                Layout.fillWidth: true
                                model: Routing.targets()
                                Component.onCompleted: currentIndex =
                                    Math.max(0, Routing.targets().indexOf(entry.target))
                                onActivated: Routing.setTarget(entry.id,
                                                               Routing.targets()[currentIndex])
                            }
                            RowLayout {
                                spacing: 6
                                Text {
                                    text: qsTr("fallback")
                                    font.family: FontIcons.display; font.pixelSize: 10
                                    color: Theme.textMuted
                                }
                                Kit.Dropdown {
                                    Layout.fillWidth: true
                                    model: Routing.targets()
                                    Component.onCompleted: currentIndex =
                                        Math.max(0, Routing.targets().indexOf(entry.fallback))
                                    onActivated: Routing.setFallback(
                                        entry.id, Routing.targets()[currentIndex])
                                }
                            }
                        }

                        Kit.Switch {
                            checked: entry.enabled
                            onToggled: Routing.setEnabled(entry.id, checked)
                        }
                        Kit.IconButton {
                            icon: FontIcons.fa_trash; iconColor: Theme.danger
                            iconPointSize: 12; implicitWidth: 30; implicitHeight: 26
                            onClicked: Routing.remove(entry.id)
                        }
                    }
                }
            }
        }
    }
}
