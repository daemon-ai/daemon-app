// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme

// The SHARED native-vs-foreign engine picker (ENG-2 / CON-16), now rendered over the shared
// AgentSetupModel's `engineChoices` (Phase C/D): one list — "daemon-core" (the native engine,
// row 0, default) plus every foreign catalog entry — where each foreign row carries the
// node-derived `verification` verdict ("Verified" | "Unverified" | "NotInstalled") rendered
// VERBATIM as its badge, and `disabled` (== verification is NotInstalled) marks the rows the node
// says cannot be selected. Selecting a row drives `AgentSetup.chooseEngine(kind, agent)`, so the
// engine step is a thin renderer + intent sender: the model is the single source of engine truth
// the GUI form and the first-run wizard both bind, and they cannot drift.
//
// The picker mirrors AgentSetup's current selection into `engineAgent` / `nativeEngine` /
// `engineAgentInstalled` for existing consumers, and re-emits `selected` on user taps. refresh()
// kicks a fresh node-side discovery scan (AgentDiscover) so just-installed agents surface live
// badges; the rows themselves flow from AgentSetup.engineChoices (recomposed from the catalog).
// Absent facade (pure-QML harness) => native only, sourced from Agents as a fallback.
ColumnLayout {
    id: picker
    spacing: 6

    // The selected engine: "" = daemon-core (native); otherwise the foreign catalog agent name.
    property string engineAgent: ""
    // Whether the selected foreign agent is installed (the consumer's accept-gate half).
    property bool engineAgentInstalled: false
    // The engine rows the list renders ({kind, name, agent, protocol, installed, verification,
    // disabled}); AgentSetup.engineChoices already prepends the Native row (kind "Core").
    property var engineRows: picker._composeRows()

    readonly property bool nativeEngine: engineAgent.length === 0

    // Fired on every user selection change (consumers re-seed names / re-gate commit).
    signal selected(string agent, bool installed)

    readonly property bool _hasSetup: (typeof AgentSetup !== "undefined" && AgentSetup)

    // AgentSetup.engineChoices already carries the Native row + verification badges; without the
    // facade (harness) synthesize a native-only list so the picker still renders.
    function _composeRows() {
        if (picker._hasSetup)
            return AgentSetup.engineChoices;
        var native = { kind: "Core", name: qsTr("Native"), agent: "", protocol: "",
                       installed: true, verification: "", disabled: false };
        var rows = [native];
        var cat = (typeof Agents !== "undefined" && Agents) ? Agents.agents() : [];
        for (var i = 0; i < cat.length; ++i)
            rows.push({ kind: "Foreign", name: cat[i].name, agent: cat[i].name,
                        protocol: cat[i].protocol, installed: cat[i].installed === true,
                        verification: "", disabled: cat[i].installed !== true });
        return rows;
    }

    // Reflect AgentSetup's current engine into the mirror properties (single source of truth): the
    // selected agent + whether it is installed (per the row's verification/disabled verdict).
    function _syncFromModel() {
        picker.engineRows = picker._composeRows();
        var agent = picker._hasSetup && AgentSetup.engineKind === "Foreign"
                    ? AgentSetup.foreignAgent : "";
        picker.engineAgent = agent;
        var installed = agent.length === 0; // native is always available
        for (var i = 0; i < picker.engineRows.length; ++i)
            if (picker.engineRows[i].agent === agent && agent.length > 0)
                installed = picker.engineRows[i].installed === true;
        picker.engineAgentInstalled = installed;
    }

    function selectEngine(kind, agent, installed) {
        if (picker._hasSetup)
            AgentSetup.chooseEngine(kind, agent); // the model is the source of truth
        picker.engineAgent = kind === "Foreign" ? agent : "";
        picker.engineAgentInstalled = kind === "Foreign" ? (installed === true) : false;
        picker.selected(picker.engineAgent, picker.engineAgentInstalled);
    }

    // Fresh catalog + a node-side discovery scan; call when the hosting surface opens. The
    // engineChoices rows update reactively (engineChoicesChanged / catalogRefreshed).
    function refresh() {
        if (typeof Agents !== "undefined" && Agents) {
            Agents.refreshCatalog();
            Agents.discover(); // fresh installed/verification badges beside the static catalog
        }
        picker._syncFromModel();
    }

    Component.onCompleted: picker._syncFromModel()
    Connections {
        target: picker._hasSetup ? AgentSetup : null
        function onEngineChoicesChanged() { picker._syncFromModel(); }
        function onStateChanged() { picker._syncFromModel(); }
    }
    Connections {
        target: (!picker._hasSetup && typeof Agents !== "undefined" && Agents) ? Agents : null
        function onCatalogRefreshed() { picker._syncFromModel(); }
    }

    ListView {
        id: engineList
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(contentHeight, 132)
        clip: true
        model: picker.engineRows.length
        delegate: Rectangle {
            id: engineRow
            required property int index
            readonly property var row: picker.engineRows[index]
            readonly property bool isNative: row && row.kind === "Core"
            readonly property bool isInstalled: row && row.installed === true
            readonly property bool isDisabled: row && row.disabled === true
            readonly property bool selected: row
                                             ? (isNative ? picker.nativeEngine
                                                         : picker.engineAgent === row.agent)
                                             : false
            width: ListView.view ? ListView.view.width : 0
            height: 30
            radius: 6
            color: engineRow.selected ? Theme.hover : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 8
                Text {
                    // Foreign rows suffix the protocol: "· ACP <version>" or "· stream-json".
                    text: engineRow.isNative
                          ? qsTr("daemon-core (native)")
                          : engineRow.row.name
                            + (engineRow.row.protocol === "StreamJson"
                               ? qsTr("  ·  stream-json")
                               : engineRow.row.protocol === "Acp"
                                 ? qsTr("  ·  ACP") : "")
                    font.family: FontIcons.display
                    font.pixelSize: 12
                    // A disabled (NotInstalled) foreign row mutes so it reads as unselectable.
                    color: (engineRow.isNative || !engineRow.isDisabled) ? Theme.text
                                                                         : Theme.textMuted
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                // Verification badge for foreign rows — the node-derived verdict, rendered VERBATIM
                // (never re-derived from installed/protocol/version). The native engine needs none.
                Rectangle {
                    visible: !engineRow.isNative && engineRow.row.verification
                             && engineRow.row.verification.length > 0
                    radius: 4
                    color: "transparent"
                    border.width: 1
                    border.color: engineRow.isDisabled ? Theme.border : Theme.stateRunning
                    implicitWidth: badgeText.implicitWidth + 12
                    implicitHeight: 18
                    Text {
                        id: badgeText
                        anchors.centerIn: parent
                        text: engineRow.row ? engineRow.row.verification : ""
                        font.family: FontIcons.display
                        font.pixelSize: 10
                        color: engineRow.isDisabled ? Theme.textMuted : Theme.stateRunning
                    }
                }
            }
            // Engine choice named for the agent (native core or the foreign engine).
            Accessible.role: Accessible.RadioButton
            Accessible.name: engineRow.isNative ? qsTr("daemon-core (native)") : engineRow.row.name
            Accessible.checkable: true
            Accessible.checked: engineRow.selected
            Accessible.onPressAction: {
                if (engineRow.isNative)
                    picker.selectEngine("Core", "", false);
                else if (!engineRow.isDisabled)
                    picker.selectEngine("Foreign", engineRow.row.agent, engineRow.isInstalled);
            }

            TapHandler {
                onTapped: {
                    // A disabled (NotInstalled) foreign row stays visible but unselectable (the
                    // badge explains why); row 0 returns to the native engine.
                    if (engineRow.isNative)
                        picker.selectEngine("Core", "", false);
                    else if (!engineRow.isDisabled)
                        picker.selectEngine("Foreign", engineRow.row.agent, engineRow.isInstalled);
                }
            }
        }
    }
}
