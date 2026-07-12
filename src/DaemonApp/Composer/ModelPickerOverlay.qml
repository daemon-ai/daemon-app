// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The composer's model picker - a richer upward overlay than the old ModelPill
// menu (a la Hermes' model-picker.tsx): a filter field, a provider-grouped model
// list, and a modes footer (reasoning effort / fast / verbose). It binds directly
// to the shared ComposerSessionController so the GUI and the daemon-fed catalog
// stay one source. Open it with open(); selection + mode writes go straight back
// to the controller.
QQC.Popup {
    id: root

    // The shared ComposerSessionController (modelCatalog + currentModelIndex +
    // mode props/setters). Required.
    property var session: null

    // Live filter query (label/id/provider substring, case-insensitive).
    property string query: ""

    // Phase E: the picker's choice set + selection path fork on the session's foreign backend.
    //   - native (no foreignBackend): the node catalog; selection sets the default model
    //     (session.selectModel by catalog index).
    //   - AgentNative: the agent-advertised model_selector choices; selection sends
    //     SetSessionModel{model: choice.id} over ACP (session.selectForeignModel).
    //   - NodeProvider: the node catalog (routed through the gateway); selection sends
    //     SetSessionModel{model} — NEVER a provider (session.selectForeignModel).
    // Each entry: { provider, id, label, index, foreign, current }. `index` is the catalog index
    // for the native/NodeProvider selectModel/highlight; `foreign` routes selection; `current`
    // marks the checked row.
    function pickerEntries() {
        if (!root.session)
            return [];
        const backend = root.session.foreignBackend;
        if (backend === "AgentNative") {
            const sel = root.session.foreignModelSelector;
            if (!sel || sel.hasSelector !== true)
                return [];
            const choices = sel.choices || [];
            const out = [];
            for (let i = 0; i < choices.length; ++i) {
                out.push({ provider: "", id: choices[i].id, label: choices[i].label,
                           index: -1, foreign: true, current: choices[i].id === sel.current });
            }
            return out;
        }
        const foreign = backend === "NodeProvider";
        const cat = root.session.modelCatalog || [];
        const res = [];
        for (let j = 0; j < cat.length; ++j) {
            const e = cat[j];
            res.push({ provider: e.provider, id: e.id, label: e.label, index: j,
                       foreign: foreign,
                       current: !foreign && j === root.session.currentModelIndex });
        }
        return res;
    }

    // Abbreviated, translatable labels for the reasoning-effort segments; the
    // stored value stays the canonical off/low/medium/high token.
    function effortSegLabel(v) {
        switch (v) {
        case "off": return qsTr("Off", "reasoning effort (abbreviated)");
        case "low": return qsTr("Low", "reasoning effort (abbreviated)");
        case "medium": return qsTr("Med", "reasoning effort (abbreviated)");
        case "high": return qsTr("Hig", "reasoning effort (abbreviated)");
        }
        return v;
    }

    // Open upward, right-aligned to the pill that anchors us.
    y: -implicitHeight - 6
    x: parent ? parent.width - width : 0
    width: 280
    padding: 1
    modal: false
    focus: true

    onAboutToShow: { root.query = ""; filterField.text = ""; filterField.forceActiveFocus(); }

    background: Rectangle {
        color: Theme.background
        border.color: Theme.border
        border.width: 1
        radius: Theme.radius
    }

    contentItem: ColumnLayout {
        spacing: 0

        // --- Filter -------------------------------------------------------
        Kit.TextField {
            id: filterField
            Layout.fillWidth: true
            Layout.margins: 6
            placeholderText: qsTr("Filter models\u2026")
            font.pixelSize: 12
            onTextChanged: root.query = text
            Keys.onDownPressed: list.incrementCurrentIndex()
            Keys.onUpPressed: list.decrementCurrentIndex()
            Keys.onReturnPressed: list.activateCurrent()
            Keys.onEscapePressed: root.close()
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        // --- Provider-grouped, filtered model list ------------------------
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 240)
            clip: true
            currentIndex: 0

            function _matches(entry) {
                if (root.query.length === 0)
                    return true;
                const q = root.query.toLowerCase();
                return entry.label.toLowerCase().indexOf(q) >= 0
                    || entry.id.toLowerCase().indexOf(q) >= 0
                    || entry.provider.toLowerCase().indexOf(q) >= 0;
            }

            // Build a flat row model: provider headers + visible model rows, carrying each entry's
            // selection route (foreign vs native) + current flag. Sourced from pickerEntries() so
            // the AgentNative selector / NodeProvider-and-native catalog forks stay in one place.
            property var rows: {
                const out = [];
                const entries = root.pickerEntries();
                let lastProvider = "";
                for (let i = 0; i < entries.length; ++i) {
                    const e = entries[i];
                    if (!_matches(e))
                        continue;
                    // Skip an empty provider header (AgentNative choices carry no provider group).
                    if (e.provider !== lastProvider) {
                        if (e.provider !== "")
                            out.push({ header: true, provider: e.provider, index: -1 });
                        lastProvider = e.provider;
                    }
                    out.push({ header: false, provider: e.provider, label: e.label, id: e.id,
                               index: e.index, foreign: e.foreign, current: e.current });
                }
                return out;
            }
            model: rows

            // Route a picked row: foreign sessions send SetSessionModel by model id; a native
            // session sets the default model by catalog index.
            function pick(r) {
                if (!r || r.header || !root.session)
                    return;
                if (r.foreign)
                    root.session.selectForeignModel(r.id);
                else
                    root.session.selectModel(r.index);
                root.close();
            }

            function activateCurrent() {
                if (currentIndex < 0 || currentIndex >= rows.length)
                    return;
                pick(rows[currentIndex]);
            }

            delegate: Item {
                required property int index
                required property var modelData
                width: ListView.view.width
                implicitHeight: modelData.header ? 22 : 30
                enabled: !modelData.header

                // Provider group header.
                QQC.Label {
                    visible: modelData.header
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    verticalAlignment: Text.AlignVCenter
                    text: modelData.provider !== undefined ? modelData.provider : ""
                    font.family: FontIcons.display
                    font.pixelSize: 10
                    font.capitalization: Font.AllUppercase
                    color: Theme.textMuted
                }

                // Model row.
                Rectangle {
                    visible: !modelData.header
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: 5
                    color: (list.currentIndex === index || rowArea.containsMouse)
                        ? Theme.hover : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 8
                        spacing: 8

                        QQC.Label {
                            Layout.fillWidth: true
                            text: modelData.label !== undefined ? modelData.label : ""
                            font.family: FontIcons.display
                            font.pixelSize: 12
                            color: modelData.current === true ? Theme.accent : Theme.text
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            visible: modelData.current === true
                            text: FontIcons.check
                            font.family: FontIcons.faSolid
                            font.pixelSize: 11
                            color: Theme.accent
                        }
                    }

                    MouseArea {
                        id: rowArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: list.currentIndex = index
                        onClicked: list.pick(modelData)

                        // Model choice named for its label.
                        Accessible.role: Accessible.ListItem
                        Accessible.name: modelData.label !== undefined ? modelData.label : ""
                        Accessible.selected: modelData.current === true
                        Accessible.onPressAction: list.pick(modelData)
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        // --- Modes footer (reasoning effort / fast / verbose) -------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 8
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                QQC.Label {
                    text: qsTr("Reasoning")
                    font.family: FontIcons.display
                    font.pixelSize: 11
                    color: Theme.textMuted
                }
                Item { Layout.fillWidth: true }
                Repeater {
                    model: ["off", "low", "medium", "high"]
                    delegate: Rectangle {
                        required property int index
                        required property string modelData
                        readonly property bool active:
                            root.session && root.session.reasoningEffort === modelData
                        implicitWidth: segLabel.implicitWidth + 14
                        implicitHeight: 20
                        radius: 4
                        color: active ? Theme.accent : Theme.surfaceRaised
                        border.color: active ? Theme.accent : Theme.border
                        border.width: 1
                        QQC.Label {
                            id: segLabel
                            anchors.centerIn: parent
                            text: root.effortSegLabel(modelData)
                            font.family: FontIcons.display
                            font.pixelSize: 10
                            color: parent.active ? Theme.background : Theme.text
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.session) root.session.setReasoningEffort(modelData)
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                TogglePill {
                    label: qsTr("Fast")
                    active: root.session ? root.session.fastMode : false
                    onToggled: if (root.session) root.session.setFastMode(!root.session.fastMode)
                }
                TogglePill {
                    label: qsTr("Verbose")
                    active: root.session ? root.session.verbose : false
                    onToggled: if (root.session) root.session.setVerbose(!root.session.verbose)
                }
                Item { Layout.fillWidth: true }
            }
        }
    }

    // A boolean toggle styled like the reasoning-effort segments above it
    // (accent fill when on), so the modes footer reads as one cohesive control.
    component TogglePill: Rectangle {
        id: pill
        property string label: ""
        property bool active: false
        signal toggled()

        implicitWidth: pillLabel.implicitWidth + 18
        implicitHeight: 20
        radius: 4
        color: active ? Theme.accent : Theme.surfaceRaised
        border.color: active ? Theme.accent : Theme.border
        border.width: 1

        QQC.Label {
            id: pillLabel
            anchors.centerIn: parent
            text: pill.label
            font.family: FontIcons.display
            font.pixelSize: 10
            color: pill.active ? Theme.background : Theme.text
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: pill.toggled()
        }
    }
}
