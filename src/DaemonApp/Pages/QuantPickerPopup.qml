// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Quant picker (Phase 2, local model track): step 2 of discovery. openFor(repo) loads the repo's
// loadable GGUF files (ModelFiles) grouped by quant + size, pre-highlights the hardware-aware
// ModelRecommend pick ("Recommended / fits ~N"), and offers a one-click "Download recommended"
// plus per-file Download in the advanced list. Binds live to ModelCatalog.files / .recommendation().
Kit.Dialog {
    id: root

    property string repo: ""
    property var rec: ({})
    property bool advanced: false

    title: qsTr("Choose a quantization")
    width: 520
    footer: null

    function openFor(repoId) {
        root.repo = repoId;
        root.rec = ({});
        root.advanced = false;
        ModelCatalog.repoFiles(repoId);
        open();
    }

    function _refreshRec() {
        if (ModelCatalog.filesRepo === root.repo || true)
            root.rec = ModelCatalog.recommendation();
    }

    Connections {
        target: ModelCatalog
        function onRecommendChanged(r) {
            if (r === root.repo)
                root.rec = ModelCatalog.recommendation();
        }
        function onFilesChanged(r) {
            if (r === root.repo)
                root.rec = ModelCatalog.recommendation();
        }
        function onDownloadStarted() {} // no-op; the Downloads tab tracks progress
    }

    contentItem: ColumnLayout {
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: root.repo
            font.family: FontIcons.mono; font.pixelSize: 12; color: Theme.textMuted
            elide: Text.ElideMiddle
        }

        // Recommended banner: the ModelRecommend pick, downloadable in one click.
        Rectangle {
            Layout.fillWidth: true
            visible: root.rec && root.rec.quant !== undefined && root.rec.quant !== ""
            radius: 8
            color: Theme.hover
            border.color: Theme.accent
            implicitHeight: recCol.implicitHeight + 20

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12

                ColumnLayout {
                    id: recCol
                    Layout.fillWidth: true
                    spacing: 2
                    RowLayout {
                        spacing: 8
                        Text {
                            text: qsTr("Recommended: %1").arg(root.rec.quant !== undefined ? root.rec.quant : "")
                            font.family: FontIcons.display; font.pixelSize: 13; font.bold: true
                            color: Theme.text
                        }
                        Text {
                            text: (root.rec.fits ? qsTr("fits") : qsTr("tight")) +
                                  (root.rec.sizeLabel ? " \u00b7 " + root.rec.sizeLabel : "")
                            font.family: FontIcons.mono; font.pixelSize: 10
                            color: root.rec.fits ? Theme.accent : Theme.danger
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: !!root.rec.reason
                        text: root.rec.reason ? root.rec.reason : ""
                        font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                        wrapMode: Text.WordWrap
                    }
                }
                Kit.TextButton {
                    text: qsTr("Download recommended")
                    accentFilled: true
                    enabled: !!root.rec.file
                    onClicked: {
                        ModelCatalog.downloadFile(root.repo, root.rec.file);
                        root.close();
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Available files")
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.text
                Layout.fillWidth: true
            }
            Kit.TextButton {
                text: root.advanced ? qsTr("Hide advanced") : qsTr("Advanced")
                onClicked: root.advanced = !root.advanced
            }
        }

        QQC.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 240
            clip: true
            contentWidth: availableWidth

            ListView {
                model: ModelCatalog.files
                spacing: 6
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    required property var entry
                    // In non-advanced mode show only the recommended row + sensible quants.
                    visible: root.advanced || entry.recommended ||
                             (entry.quant !== undefined && entry.quant !== "")
                    width: ListView.view.width
                    height: visible ? 52 : 0
                    radius: 6
                    color: Theme.surface
                    border.color: entry.recommended ? Theme.accent : Theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            RowLayout {
                                spacing: 6
                                Text {
                                    text: (entry.quant !== undefined && entry.quant !== "")
                                          ? entry.quant : entry.path
                                    font.family: FontIcons.display; font.pixelSize: 12
                                    font.bold: true; color: Theme.text
                                }
                                Text {
                                    visible: entry.recommended === true
                                    text: qsTr("RECOMMENDED")
                                    font.family: FontIcons.display; font.pixelSize: 9; font.bold: true
                                    color: Theme.accent
                                }
                                // Vision-projector (mmproj) companion: downloadable, but it is
                                // loaded ALONGSIDE text weights — never pick it as the chat model.
                                Text {
                                    visible: entry.isMmproj === true
                                    text: qsTr("PROJECTOR")
                                    font.family: FontIcons.display; font.pixelSize: 9; font.bold: true
                                    color: Theme.textMuted
                                }
                            }
                            Text {
                                text: entry.path + "  \u00b7  " + entry.sizeLabel
                                font.family: FontIcons.mono; font.pixelSize: 10
                                color: Theme.textMuted; elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                        }
                        Kit.TextButton {
                            text: qsTr("Download")
                            onClicked: {
                                ModelCatalog.downloadFile(root.repo, entry.path);
                                root.close();
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            Item { Layout.fillWidth: true }
            Kit.TextButton { text: qsTr("Close"); onClicked: root.close() }
        }
    }
}
