// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Discover tab (Phase 2): repo-centric search over ModelCatalog.discover. Each row is a Hugging
// Face repo (name, author, params, downloads); selecting one opens the QuantPickerPopup to pick a
// quant + download. The two-step repo -> files flow replaces the old flat model rows.
ColumnLayout {
    id: root
    anchors.fill: parent
    anchors.margins: 20
    spacing: 12

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

    // Operation failures (HF network errors, gated repos) surfaced inline: without this a failed
    // search just looks like an empty, dead page.
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
        text: qsTr("Search for a model repo to browse its quantizations.")
        font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
    }

    QQC.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        contentWidth: availableWidth

        ListView {
            model: ModelCatalog.discover
            spacing: 8
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                required property var entry
                width: ListView.view.width
                height: 78
                radius: 8
                color: hover.hovered ? Theme.hover : Theme.surface
                border.color: Theme.border

                HoverHandler { id: hover }
                TapHandler { onTapped: quantPicker.openFor(entry.repo) }

                // Discoverable model row named for the model.
                Accessible.role: Accessible.ListItem
                Accessible.name: entry.name
                Accessible.onPressAction: quantPicker.openFor(entry.repo)

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        RowLayout {
                            spacing: 8
                            Text {
                                text: entry.name
                                font.family: FontIcons.display; font.pixelSize: 14
                                font.bold: true; color: Theme.text
                                elide: Text.ElideRight; Layout.maximumWidth: 360
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
                            text: entry.author ? qsTr("by %1").arg(entry.author) : ""
                            font.family: FontIcons.display; font.pixelSize: 12
                            color: Theme.textMuted; elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: "\u2193 " + Math.round(entry.downloads) + "   \u2661 " + Math.round(entry.likes)
                            font.family: FontIcons.mono; font.pixelSize: 11
                            color: Theme.textMuted
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

    // Paged search (the node reports has_more): append the next page on demand.
    Kit.TextButton {
        visible: ModelCatalog.searchHasMore
        text: qsTr("Load more")
        Layout.alignment: Qt.AlignHCenter
        onClicked: ModelCatalog.searchMore()
    }

    QuantPickerPopup {
        id: quantPicker
    }
}
