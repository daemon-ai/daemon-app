// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick

import DaemonApp.Theme

// Renders web/search tool results: a list of { title, url, snippet } hits. Each
// title links out; the url is muted; the snippet wraps. Mirrors Hermes' search
// result rendering at a Tier-1 fidelity (no raw-JSON drilldown yet).
Item {
    id: root

    property var hits: []

    implicitHeight: col.implicitHeight

    Column {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.smallSpacing

        Repeater {
            model: root.hits
            delegate: Column {
                required property var modelData
                width: col.width
                spacing: 1

                Text {
                    width: parent.width
                    text: modelData.title ? modelData.title
                                          : (modelData.url ? modelData.url : qsTr("result"))
                    color: Theme.link
                    font.pixelSize: Theme.bodyFontSize - 1
                    font.underline: linkHover.hovered
                    elide: Text.ElideRight

                    HoverHandler { id: linkHover }
                    TapHandler {
                        onTapped: {
                            if (modelData.url)
                                Qt.openUrlExternally(modelData.url)
                        }
                    }
                }

                Text {
                    width: parent.width
                    visible: !!modelData.url
                    text: modelData.url ? modelData.url : ""
                    color: Theme.mutedText
                    font.pixelSize: Theme.captionFontSize
                    elide: Text.ElideRight
                }

                Text {
                    width: parent.width
                    visible: !!modelData.snippet
                    text: modelData.snippet ? modelData.snippet : ""
                    color: Theme.text
                    font.pixelSize: Theme.captionFontSize
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
