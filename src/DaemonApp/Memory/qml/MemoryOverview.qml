import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Overview sub-tab: stat cards + breakdown bars (shared fraction with the TUI
// gauges) + a live memory event feed driven by the IMemoryService watch stream.
Item {
    id: root

    MemoryStatsModel {
        id: stats
        service: typeof Memory !== "undefined" ? Memory : null
    }

    // Live event feed: subscribe to the watch stream while this view is alive.
    ListModel { id: feed }

    Connections {
        target: typeof Memory !== "undefined" ? Memory : null
        function onMemoryEvent(ev) {
            feed.insert(0, { kind: ev.kind, summary: ev.summary, memId: ev.memoryId });
            while (feed.count > 12)
                feed.remove(feed.count - 1);
        }
    }

    Component.onCompleted: {
        stats.refresh();
        if (typeof Memory !== "undefined" && Memory)
            Memory.startWatch();
    }
    Component.onDestruction: {
        if (typeof Memory !== "undefined" && Memory)
            Memory.stopWatch();
    }

    component StatCard: Rectangle {
        property string label: ""
        property int value: 0
        property string tone: Theme.text
        implicitWidth: 132
        implicitHeight: 64
        radius: 8
        color: Theme.surface
        border.color: Theme.border
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 2
            Text {
                text: value
                font.family: FontIcons.display
                font.pixelSize: 22
                font.weight: Font.DemiBold
                color: tone
            }
            Text {
                text: label
                font.family: FontIcons.display
                font.pixelSize: 11
                color: Theme.textMuted
            }
        }
    }

    // A labelled breakdown bar group bound to one of the stats QVariantLists.
    component Breakdown: ColumnLayout {
        property string title: ""
        property var rows: []
        Layout.fillWidth: true
        spacing: 4
        Text {
            text: title
            font.family: FontIcons.display
            font.pixelSize: 11
            font.weight: Font.DemiBold
            color: Theme.textMuted
        }
        Repeater {
            model: rows
            delegate: RowLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: 8
                Text {
                    text: modelData.key
                    font.family: FontIcons.display
                    font.pixelSize: 11
                    color: Theme.text
                    Layout.preferredWidth: 120
                    elide: Text.ElideRight
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 10
                    radius: 3
                    color: Theme.hover
                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.max(2, parent.width * modelData.fraction)
                        height: parent.height
                        radius: 3
                        color: Theme.accent
                    }
                }
                Text {
                    text: modelData.count
                    font.family: FontIcons.mono
                    font.pixelSize: 11
                    color: Theme.textMuted
                    Layout.preferredWidth: 28
                    horizontalAlignment: Text.AlignRight
                }
            }
        }
    }

    QQC.ScrollView {
        anchors.fill: parent
        anchors.margins: 16
        clip: true
        contentWidth: availableWidth
        QQC.ScrollBar.vertical: Kit.ScrollBar {}

        ColumnLayout {
            width: parent.width
            spacing: 18

            Flow {
                Layout.fillWidth: true
                spacing: 10
                StatCard { label: qsTr("Working"); value: stats.working; tone: Theme.accent }
                StatCard { label: qsTr("Episodic"); value: stats.episodic }
                StatCard { label: qsTr("Scratchpad"); value: stats.scratchpad }
                StatCard { label: qsTr("Total"); value: stats.total }
                StatCard { label: qsTr("Facts"); value: stats.facts }
                StatCard { label: qsTr("Conflicts"); value: stats.conflicts; tone: Theme.danger }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 24
                Breakdown { title: qsTr("By source"); rows: stats.bySource }
                Breakdown { title: qsTr("By veracity"); rows: stats.byVeracity }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 24
                Breakdown { title: qsTr("By lifecycle"); rows: stats.byDegradation }
                Breakdown { title: qsTr("By scope"); rows: stats.byScope }
            }
            Breakdown { title: qsTr("By session"); rows: stats.bySession }

            // --- Live stream ------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 8; height: 8; radius: 4; color: Theme.statusOk
                    }
                    Text {
                        text: qsTr("Live memory stream")
                        font.family: FontIcons.display
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: Theme.textMuted
                    }
                }
                Repeater {
                    model: feed
                    delegate: RowLayout {
                        required property string kind
                        required property string summary
                        Layout.fillWidth: true
                        spacing: 8
                        Rectangle {
                            radius: 3
                            implicitWidth: kindLabel.implicitWidth + 12
                            implicitHeight: 16
                            color: Theme.hover
                            Text {
                                id: kindLabel
                                anchors.centerIn: parent
                                text: kind
                                font.family: FontIcons.mono
                                font.pixelSize: 9
                                color: Theme.accent
                            }
                        }
                        Text {
                            text: summary
                            font.family: FontIcons.display
                            font.pixelSize: 11
                            color: Theme.text
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }
                Text {
                    visible: feed.count === 0
                    text: qsTr("Waiting for memory activity...")
                    font.family: FontIcons.display
                    font.pixelSize: 11
                    color: Theme.textMuted
                }
            }
        }
    }
}
