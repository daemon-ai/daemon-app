// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// In-place model discovery: the ModelsDiscover flow (repo search -> QuantPickerPopup -> download)
// hosted in a window-overlay dialog, so the "+ Discover more models" affordance works from the
// first-run wizard (which sits above the shell at z:200 — a Nav.open there lands invisibly
// underneath) and from the "+ New agent" dialog alike. Auto-runs a search on open (an empty query
// asks the node for trending repos). Started downloads surface as a live queue (DownloadsPanel)
// right in the dialog — progress, pause/resume/cancel/retry, and a "Use this model" action that
// hands the installed catalog id back to the hosting picker; operation failures render inline so
// the flow never looks silently dead.
Kit.Dialog {
    id: root

    title: qsTr("Discover models")
    width: 560
    footer: null

    // A done download's "Use this model" resolved to the installed catalog id; the hosting
    // picker selects it (the dialog closes itself).
    signal useModelRequested(string modelId)

    function openDialog() {
        open();
        // Refresh on every open: re-runs the last query, or lists trending repos when empty.
        ModelCatalog.search(query.text);
    }

    contentItem: ColumnLayout {
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Kit.TextField {
                id: query
                Layout.fillWidth: true
                placeholderText: qsTr("Search Hugging Face repos (e.g. SmolLM2)")
                onAccepted: ModelCatalog.search(text)
            }
            Kit.TextButton {
                text: qsTr("Search")
                accentFilled: true
                onClicked: ModelCatalog.search(query.text)
            }
        }

        Text {
            Layout.fillWidth: true
            visible: ModelCatalog.lastError.length > 0
            text: ModelCatalog.lastError
            font.family: FontIcons.display; font.pixelSize: 12; color: Theme.danger
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            visible: ModelCatalog.discover.count === 0
            text: qsTr("Searching…")
            font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
        }

        QQC.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 300
            clip: true
            contentWidth: availableWidth

            ListView {
                model: ModelCatalog.discover
                spacing: 8
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    required property var entry
                    width: ListView.view.width
                    height: 64
                    radius: 8
                    color: rowHover.hovered ? Theme.hover : Theme.surface
                    border.color: Theme.border

                    HoverHandler { id: rowHover }
                    TapHandler { onTapped: quantPicker.openFor(entry.repo) }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            RowLayout {
                                spacing: 8
                                Text {
                                    text: entry.name
                                    font.family: FontIcons.display; font.pixelSize: 13
                                    font.bold: true; color: Theme.text
                                    elide: Text.ElideRight; Layout.maximumWidth: 330
                                }
                                Rectangle {
                                    visible: !!entry.params
                                    radius: 3; color: Theme.hover
                                    implicitWidth: prm.implicitWidth + 10
                                    implicitHeight: prm.implicitHeight + 4
                                    Text {
                                        id: prm; anchors.centerIn: parent; text: entry.params
                                        font.family: FontIcons.display; font.pixelSize: 10
                                        color: Theme.textMuted
                                    }
                                }
                            }
                            Text {
                                text: (entry.author ? qsTr("by %1").arg(entry.author) + "   " : "")
                                      + "\u2193 " + Math.round(entry.downloads)
                                font.family: FontIcons.mono; font.pixelSize: 10
                                color: Theme.textMuted; elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        Kit.TextButton {
                            text: qsTr("Quantizations")
                            accentFilled: true
                            onClicked: quantPicker.openFor(entry.repo)
                        }
                    }
                }
            }
        }

        // The live download queue for everything started from this dialog (multiple quant picks
        // stack). Self-hides while empty; "Use this model" resolves the installed catalog id and
        // hands it to the hosting picker.
        SectionLabel {
            visible: downloadsPanel.visible
            text: qsTr("Downloads")
        }
        DownloadsPanel {
            id: downloadsPanel
            Layout.fillWidth: true
            showUseAction: true
            maxListHeight: 180
            onUseModelRequested: function(modelId) {
                root.useModelRequested(modelId);
                root.close();
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Kit.TextButton {
                visible: ModelCatalog.searchHasMore
                text: qsTr("Load more")
                onClicked: ModelCatalog.searchMore()
            }
            Item { Layout.fillWidth: true }
            Kit.TextButton { text: qsTr("Close"); onClicked: root.close() }
        }

        // Step 2 of discovery (quant + download), stacked above this dialog in the same overlay.
        QuantPickerPopup {
            id: quantPicker
        }
    }
}
