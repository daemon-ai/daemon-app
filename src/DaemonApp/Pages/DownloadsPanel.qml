// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The shared download-queue view over ModelCatalog.downloads, embedded by the
// Discover dialog (full, with "Use this model"), the wizard/agent picker (compact
// active-only strip), and the Models hub Downloads tab (full, anchor-filled).
// Row states are the facade's verbatim tokens:
//   queued | downloading | paused  -> live progress + pause/resume/cancel
//   failed                         -> inline error + Retry (node resume re-spawns failed jobs)
//   done | failed | cancelled      -> Dismiss (client-side hide; the node keeps job history)
// The panel hides itself when it has nothing to render (no rows, or activeOnly
// with only terminal rows), so embedders don't need their own visibility guard.
Item {
    id: root

    // Single-line rows without controls; a tap emits openRequested() (the picker
    // strip reopens the discover dialog for the full management surface).
    property bool compact: false
    // Render only queued/downloading/paused rows (terminal rows are skipped).
    property bool activeOnly: false
    // Offer "Use this model" on done rows; emits useModelRequested(catalogId).
    property bool showUseAction: false
    // Height cap when embedded in a column (dialog / picker); overflow scrolls.
    // Anchor-fill consumers (the Models hub tab) override the height entirely.
    property real maxListHeight: compact ? 124 : 320

    // A done row's "Use this model" resolved to this installed catalog id.
    signal useModelRequested(string modelId)
    // A compact row was tapped (progress strips route this back into discovery).
    signal openRequested()

    function stateLabel(state) {
        if (state === "queued")
            return qsTr("Queued");
        if (state === "downloading")
            return qsTr("Downloading");
        if (state === "paused")
            return qsTr("Paused");
        if (state === "done")
            return qsTr("Done");
        if (state === "failed")
            return qsTr("Failed");
        if (state === "cancelled")
            return qsTr("Cancelled");
        return state;
    }

    visible: rowsColumn.implicitHeight > 0
    implicitHeight: Math.min(flick.contentHeight, maxListHeight)

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: rowsColumn.implicitHeight
        clip: true
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds

        QQC.ScrollBar.vertical: Kit.ScrollBar {}

        // Column (not ListView): invisible rows cost no height OR spacing, which
        // is what makes the activeOnly per-delegate guard collapse cleanly.
        Column {
            id: rowsColumn
            width: flick.width
            spacing: root.compact ? 4 : 8

            Repeater {
                model: ModelCatalog.downloads

                delegate: Rectangle {
                    id: row
                    required property var entry
                    readonly property bool active: entry.state === "queued"
                                                   || entry.state === "downloading"
                                                   || entry.state === "paused"

                    visible: !root.activeOnly || active
                    width: rowsColumn.width
                    height: body.implicitHeight + (root.compact ? 12 : 20)
                    radius: root.compact ? 6 : 8
                    color: root.compact && stripHover.hovered ? Theme.hover : Theme.surface
                    border.color: Theme.border

                    HoverHandler { id: stripHover; enabled: root.compact }
                    TapHandler { enabled: root.compact; onTapped: root.openRequested() }

                    ColumnLayout {
                        id: body
                        anchors.fill: parent
                        anchors.leftMargin: root.compact ? 8 : 10
                        anchors.rightMargin: root.compact ? 8 : 10
                        anchors.topMargin: root.compact ? 6 : 10
                        anchors.bottomMargin: root.compact ? 6 : 10
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                Layout.fillWidth: true
                                text: row.entry.name
                                font.family: FontIcons.display
                                font.pixelSize: root.compact ? 12 : 13
                                color: Theme.text
                                elide: Text.ElideRight
                            }

                            // Compact: the whole row is this one line.
                            Kit.ProgressBar {
                                visible: root.compact
                                Layout.preferredWidth: 84
                                value: row.entry.progress
                                fillColor: row.entry.state === "paused" ? Theme.textMuted
                                                                        : Theme.accent
                            }
                            Text {
                                visible: root.compact
                                text: Math.round(row.entry.progress * 100) + "%"
                                font.family: FontIcons.mono; font.pixelSize: 10
                                color: Theme.textMuted
                            }

                            // Full: state word + per-state actions.
                            Text {
                                visible: !root.compact
                                text: root.stateLabel(row.entry.state)
                                font.family: FontIcons.display; font.pixelSize: 11
                                color: row.entry.state === "failed" ? Theme.danger
                                     : row.entry.state === "done" ? Theme.accent
                                     : Theme.textMuted
                            }
                            Kit.IconButton {
                                visible: !root.compact && row.active
                                icon: row.entry.state === "paused" ? FontIcons.fa_play
                                                                   : FontIcons.fa_pause
                                iconPointSize: 11; implicitWidth: 28; implicitHeight: 24
                                tooltipText: row.entry.state === "paused" ? qsTr("Resume")
                                                                          : qsTr("Pause")
                                onClicked: row.entry.state === "paused"
                                           ? ModelCatalog.resumeDownload(row.entry.id)
                                           : ModelCatalog.pauseDownload(row.entry.id)
                            }
                            Kit.IconButton {
                                visible: !root.compact && row.active
                                icon: FontIcons.fa_xmark; iconColor: Theme.danger
                                iconPointSize: 12; implicitWidth: 28; implicitHeight: 24
                                tooltipText: qsTr("Cancel")
                                onClicked: ModelCatalog.cancelDownload(row.entry.id)
                            }
                            Kit.TextButton {
                                visible: !root.compact && row.entry.state === "failed"
                                text: qsTr("Retry")
                                // The node re-spawns a failed job on resume.
                                onClicked: ModelCatalog.resumeDownload(row.entry.id)
                            }
                            Kit.TextButton {
                                objectName: "useModelButton"
                                visible: !root.compact && root.showUseAction
                                         && row.entry.state === "done"
                                text: qsTr("Use this model")
                                accentFilled: true
                                onClicked: {
                                    // Download rows carry repo+file (entry.name is the file);
                                    // resolve the installed CATALOG id the pickers select by.
                                    var id = ModelCatalog.installedIdFor(row.entry.repo,
                                                                         row.entry.name);
                                    if (id.length > 0)
                                        root.useModelRequested(id);
                                }
                            }
                            Kit.TextButton {
                                visible: !root.compact && !row.active
                                text: qsTr("Dismiss")
                                onClicked: ModelCatalog.dismissDownload(row.entry.id)
                            }
                        }

                        RowLayout {
                            visible: !root.compact && row.active
                            Layout.fillWidth: true
                            spacing: 8

                            Kit.ProgressBar {
                                Layout.fillWidth: true
                                value: row.entry.progress
                                fillColor: row.entry.state === "paused" ? Theme.textMuted
                                                                        : Theme.accent
                            }
                            // Multi-file jobs (split GGUF sets) surface their per-file position;
                            // single-file jobs (filesTotal <= 1, or rows without the keys) hide it.
                            Text {
                                objectName: "filesProgressLabel"
                                visible: row.entry.filesTotal !== undefined
                                         && row.entry.filesTotal > 1
                                text: qsTr("file %1/%2").arg(row.entry.filesDone)
                                                        .arg(row.entry.filesTotal)
                                font.family: FontIcons.mono; font.pixelSize: 10
                                color: Theme.textMuted
                            }
                            Text {
                                text: row.entry.downloadedLabel + " / " + row.entry.sizeLabel
                                font.family: FontIcons.mono; font.pixelSize: 10
                                color: Theme.textMuted
                            }
                            Text {
                                text: Math.round(row.entry.progress * 100) + "%"
                                font.family: FontIcons.mono; font.pixelSize: 11
                                color: Theme.text
                            }
                        }

                        Text {
                            visible: !root.compact && row.entry.state === "failed"
                                     && row.entry.error.length > 0
                            Layout.fillWidth: true
                            text: row.entry.error
                            font.family: FontIcons.display; font.pixelSize: 11
                            color: Theme.danger
                            wrapMode: Text.WordWrap
                        }

                        // A4 (CON-12) gated-repo guidance: an auth-shaped failure (401/403 /
                        // gated) gets an actionable path — accept the license on the Hub, add a
                        // token, then Retry — instead of only the raw error string above.
                        RowLayout {
                            readonly property bool gatedFailure: {
                                if (root.compact || row.entry.state !== "failed"
                                    || !row.entry.error)
                                    return false;
                                var e = row.entry.error.toLowerCase();
                                return e.indexOf("401") !== -1 || e.indexOf("403") !== -1
                                       || e.indexOf("gated") !== -1
                                       || e.indexOf("unauthorized") !== -1
                                       || e.indexOf("forbidden") !== -1;
                            }
                            visible: gatedFailure
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("This repo is gated — accept its license on the Hub "
                                           + "(and add a token if required), then retry.")
                                font.family: FontIcons.display; font.pixelSize: 11
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                            }
                            Kit.TextButton {
                                text: qsTr("Open license page")
                                onClicked: Qt.openUrlExternally(
                                               "https://huggingface.co/" + row.entry.repo)
                            }
                        }
                    }
                }
            }
        }
    }
}
