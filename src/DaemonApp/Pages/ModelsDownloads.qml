// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme

// Downloads tab: the shared DownloadsPanel (full variant) over ModelCatalog.downloads —
// live progress, pause/resume/cancel, inline errors with Retry, Dismiss for finished rows —
// plus the local re-quantization jobs (A5 / CON-13), which mirror the download rows.
Item {
    id: root
    anchors.fill: parent
    anchors.margins: 20

    readonly property var quantJobs: (typeof ModelCatalog !== "undefined" && ModelCatalog)
                                     ? ModelCatalog.quantizeJobs : null

    Text {
        anchors.centerIn: parent
        visible: ModelCatalog.downloads.count === 0
                 && (!root.quantJobs || root.quantJobs.count === 0)
        text: qsTr("No active downloads.\nStart one from the Discover tab.")
        horizontalAlignment: Text.AlignHCenter
        font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        DownloadsPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // A5: quantize jobs ({source → target quant}; queued/running/done/failed). The node
        // catalogs the produced model on completion, so it appears under Installed.
        SectionLabel {
            visible: root.quantJobs && root.quantJobs.count > 0
            text: qsTr("Quantize jobs")
        }
        ListView {
            visible: root.quantJobs && root.quantJobs.count > 0
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 160)
            clip: true
            spacing: 6
            model: root.quantJobs
            delegate: Rectangle {
                required property var entry
                width: ListView.view ? ListView.view.width : 0
                implicitHeight: quantCol.implicitHeight + 16
                radius: 6
                color: Theme.surface
                border.color: entry.state === "failed" ? Theme.danger : Theme.border
                ColumnLayout {
                    id: quantCol
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 2
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Quantizing %1").arg(entry.name)
                            font.family: FontIcons.display; font.pixelSize: 12
                            color: Theme.text; elide: Text.ElideMiddle
                        }
                        Text {
                            text: entry.state === "downloading" ? qsTr("Working…")
                                  : entry.state === "done" ? qsTr("Done")
                                  : entry.state === "failed" ? qsTr("Failed")
                                  : qsTr("Queued")
                            font.family: FontIcons.mono; font.pixelSize: 10
                            color: entry.state === "failed" ? Theme.danger : Theme.textMuted
                        }
                    }
                    Text {
                        visible: entry.state === "failed" && !!entry.error
                        Layout.fillWidth: true
                        text: entry.error ? entry.error : ""
                        font.family: FontIcons.display; font.pixelSize: 11
                        color: Theme.danger; wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}
