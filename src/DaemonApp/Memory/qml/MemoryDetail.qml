// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Detail drawer for a single memory: content, trust strip, lifecycle, weights and
// provenance. `entry` is the QVariantMap from MemoryListModel.entryAt(row).
Rectangle {
    id: root

    property var entry: ({})

    function field(key, fallback) {
        return (root.entry && root.entry[key] !== undefined) ? root.entry[key] : (fallback === undefined ? "" : fallback);
    }
    function num(key) {
        const v = field(key, 0);
        return (typeof v === "number") ? v.toFixed(2) : String(v);
    }

    radius: 8
    color: Theme.surface
    border.color: Theme.border

    component KV: RowLayout {
        property string k: ""
        property string v: ""
        property color tone: Theme.text
        Layout.fillWidth: true
        spacing: 8
        Text {
            text: k
            font.family: FontIcons.display
            font.pixelSize: 11
            color: Theme.textMuted
            Layout.preferredWidth: 96
        }
        Text {
            text: v
            font.family: FontIcons.mono
            font.pixelSize: 11
            color: tone
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    component Section: Text {
        font.family: FontIcons.display
        font.pixelSize: 11
        font.weight: Font.DemiBold
        color: Theme.accent
        topPadding: 6
    }

    QQC.ScrollView {
        anchors.fill: parent
        anchors.margins: 12
        clip: true
        contentWidth: availableWidth
        QQC.ScrollBar.vertical: Kit.ScrollBar {}

        ColumnLayout {
            width: parent.width
            spacing: 6

            Text {
                text: root.field("content", "")
                font.family: FontIcons.display
                font.pixelSize: 13
                color: Theme.text
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            Section { text: qsTr("Trust") }
            KV {
                k: qsTr("veracity")
                v: root.field("veracity")
                tone: root.field("contaminated", false) ? Theme.warning : Theme.statusOk
            }
            KV { k: qsTr("trust tier"); v: root.field("trustTier") }
            KV { k: qsTr("trust weight"); v: root.num("trustWeight") }

            Section { text: qsTr("Lifecycle") }
            KV { k: qsTr("tier"); v: root.field("tier") + " · L" + root.field("tierLevel", 1) }
            KV {
                k: qsTr("degradation")
                v: root.field("degradation", "-")
            }
            KV {
                k: qsTr("status")
                v: root.field("status")
                tone: root.field("status") === "active" ? Theme.text : Theme.danger
            }
            KV { k: qsTr("valid until"); v: root.field("validUntil", "-") }
            KV { k: qsTr("superseded by"); v: root.field("supersededBy", "-") }

            Section { text: qsTr("Weights") }
            KV { k: qsTr("importance"); v: root.num("importance") }
            KV { k: qsTr("degradation w"); v: root.num("degradationWeight") }
            KV { k: qsTr("effective w"); v: root.num("effectiveWeight"); tone: Theme.accent }

            Section { text: qsTr("Provenance") }
            KV { k: qsTr("id"); v: root.field("id") }
            KV { k: qsTr("source"); v: root.field("source") }
            KV { k: qsTr("session"); v: root.field("sessionId") }
            KV { k: qsTr("scope"); v: root.field("scope") }
            KV { k: qsTr("recalls"); v: String(root.field("recallCount", 0)) }
            KV { k: qsTr("last recalled"); v: root.field("lastRecalled", "-") }
        }
    }
}
