import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Memories sub-tab: a filter toolbar over a selectable memory list, with a detail
// drawer on the right. Backed by MemoryListModel (shared with the TUI list view).
Item {
    id: root

    property int currentRow: -1

    MemoryListModel {
        id: memModel
        service: typeof Memory !== "undefined" ? Memory : null
        onCountChanged: if (root.currentRow >= memModel.count) root.currentRow = -1
    }

    readonly property var tierOptions: ["All tiers", "working", "episodic", "scratchpad"]
    readonly property var veracityOptions: ["All trust", "stated", "inferred", "tool", "imported", "unknown"]
    readonly property var sortOptions: ["recent", "importance", "recall"]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // --- Filter toolbar -------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Kit.TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search memories...")
                onTextEdited: searchDebounce.restart()
                Timer {
                    id: searchDebounce
                    interval: 180
                    onTriggered: memModel.search(searchField.text)
                }
            }
            Kit.Dropdown {
                id: tierPick
                model: root.tierOptions
                Layout.preferredWidth: 130
                onActivated: memModel.setFilter("tier", currentIndex <= 0 ? "" : root.tierOptions[currentIndex])
            }
            Kit.Dropdown {
                id: veracityPick
                model: root.veracityOptions
                Layout.preferredWidth: 130
                onActivated: memModel.setFilter("veracity", currentIndex <= 0 ? "" : root.veracityOptions[currentIndex])
            }
            Kit.Dropdown {
                id: sortPick
                model: root.sortOptions
                Layout.preferredWidth: 120
                onActivated: memModel.sort = root.sortOptions[currentIndex]
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // --- List ---------------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: Theme.surface
                border.color: Theme.border

                ListView {
                    id: list
                    anchors.fill: parent
                    anchors.margins: 6
                    clip: true
                    model: memModel
                    spacing: 4
                    boundsBehavior: Flickable.StopAtBounds
                    QQC.ScrollBar.vertical: Kit.ScrollBar {}

                    delegate: Rectangle {
                        id: memRow
                        required property int index
                        required property string memId
                        required property string content
                        required property string tier
                        required property string veracity
                        required property string source
                        required property real importance
                        required property string status
                        required property string degradation
                        required property bool contaminated
                        width: ListView.view.width
                        height: rowBody.implicitHeight + 16
                        radius: 6
                        color: root.currentRow === index ? Theme.rowActive
                                                          : (rowHover.hovered ? Theme.rowHover : "transparent")

                        ColumnLayout {
                            id: rowBody
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 8
                            spacing: 4

                            Text {
                                text: memRow.content
                                font.family: FontIcons.display
                                font.pixelSize: 13
                                color: Theme.text
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            RowLayout {
                                spacing: 6
                                MemoryBadge { text: memRow.tier; tone: Theme.accent }
                                MemoryBadge {
                                    text: memRow.veracity
                                    tone: memRow.contaminated ? Theme.warning : Theme.statusOk
                                }
                                MemoryBadge { text: memRow.source; tone: Theme.textMuted }
                                MemoryBadge {
                                    visible: memRow.degradation.length > 0
                                    text: memRow.degradation
                                    tone: Theme.textMuted
                                }
                                MemoryBadge {
                                    visible: memRow.status !== "active"
                                    text: memRow.status
                                    tone: Theme.danger
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: "imp " + memRow.importance.toFixed(2)
                                    font.family: FontIcons.mono
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                }
                            }
                        }

                        HoverHandler { id: rowHover }
                        TapHandler { onTapped: root.currentRow = memRow.index }
                    }
                }
            }

            // --- Detail drawer ------------------------------------------------
            MemoryDetail {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                visible: root.currentRow >= 0
                entry: root.currentRow >= 0 ? memModel.entryAt(root.currentRow) : ({})
            }
        }
    }

    // Small inline pill used for tier / veracity / source labels.
    component MemoryBadge: Rectangle {
        property string text: ""
        property color tone: Theme.textMuted
        visible: text.length > 0
        radius: 3
        implicitWidth: badgeText.implicitWidth + 10
        implicitHeight: 16
        color: Theme.hover
        Text {
            id: badgeText
            anchors.centerIn: parent
            text: parent.text
            font.family: FontIcons.mono
            font.pixelSize: 9
            color: parent.tone
        }
    }
}
