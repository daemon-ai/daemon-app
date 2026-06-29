// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Create / edit a routing pin (origin -> session [+ agent]). openFor(null) adds a new pin;
// openFor(entry) edits an existing route row (preselects). Writes back via the controller's pin().
// Mirrors CronJobDialog's shape (Kit.Dialog + Kit.Dropdowns + a footer of Kit.TextButtons).
Kit.Dialog {
    id: root

    // The RoutingManagerController (set by the page).
    property var controller: null
    // The origin key being edited ("" = adding a new pin).
    property string editKey: ""

    title: editKey.length > 0 ? qsTr("Edit route") : qsTr("Add route")
    width: 460
    footer: null

    // Parallel id arrays for the label-keyed dropdowns.
    property var _originIds: []
    property var _sessionIds: []

    function _rebuildLists() {
        const ol = [];
        _originIds = [];
        for (let i = 0; i < controller.bindable.count; ++i) {
            const e = controller.bindable.at(i);
            _originIds.push(e.id);
            ol.push(e.label);
        }
        originCombo.model = ol;

        const sl = [];
        _sessionIds = [];
        for (let j = 0; j < controller.sessions.count; ++j) {
            const s = controller.sessions.at(j);
            _sessionIds.push(s.id);
            sl.push(s.title + "  \u00b7  " + s.id);
        }
        sessionCombo.model = sl;

        agentCombo.model = ["(inherit)"].concat(Profiles.profileNames());
    }

    function openFor(entry) {
        if (controller === null)
            return;
        _rebuildLists();
        if (entry && entry.id !== undefined) {
            editKey = entry.id;
            originCombo.currentIndex = Math.max(0, _originIds.indexOf(entry.id));
            sessionCombo.currentIndex = Math.max(0, _sessionIds.indexOf(entry.session));
            const pi = Profiles.profileNames().indexOf(entry.profile);
            agentCombo.currentIndex = pi >= 0 ? pi + 1 : 0;
        } else {
            editKey = "";
            originCombo.currentIndex = 0;
            sessionCombo.currentIndex = 0;
            agentCombo.currentIndex = 0;
        }
        _updatePreview();
        open();
    }

    function _originKey() {
        return (originCombo.currentIndex >= 0 && originCombo.currentIndex < _originIds.length)
            ? _originIds[originCombo.currentIndex] : "";
    }
    function _sessionId() {
        return (sessionCombo.currentIndex >= 0 && sessionCombo.currentIndex < _sessionIds.length)
            ? _sessionIds[sessionCombo.currentIndex] : "";
    }
    function _agent() {
        return agentCombo.currentIndex <= 0 ? "" : agentCombo.currentText;
    }

    property string previewText: ""
    function _updatePreview() {
        const key = _originKey();
        if (controller === null || key === "") {
            previewText = "";
            return;
        }
        const r = controller.explain(key);
        previewText = r.found
            ? qsTr("Currently: \u2192 %1 as %2 \u00b7 decided by %3")
                  .arg(r.session).arg(r.profile === "" ? qsTr("(inherit)") : r.profile).arg(r.decidedBy)
            : qsTr("Currently: unrouted (deterministic)");
    }

    function save() {
        const o = _originKey();
        const s = _sessionId();
        if (o === "" || s === "")
            return;
        controller.pin(o, s, _agent());
        close();
    }

    contentItem: ColumnLayout {
        spacing: 10

        SectionLabel { text: qsTr("Origin (where it comes from)") }
        Kit.Dropdown {
            id: originCombo
            Layout.fillWidth: true
            onActivated: root._updatePreview()
        }

        SectionLabel { text: qsTr("Session (the conversation)") }
        Kit.Dropdown {
            id: sessionCombo
            Layout.fillWidth: true
        }

        SectionLabel { text: qsTr("Agent") }
        Kit.Dropdown {
            id: agentCombo
            Layout.fillWidth: true
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: 2
            visible: root.previewText.length > 0
            text: root.previewText
            font.family: FontIcons.mono
            font.pixelSize: 10
            color: Theme.textMuted
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 6
            Item { Layout.fillWidth: true }
            Kit.TextButton { text: qsTr("Cancel"); onClicked: root.reject() }
            Kit.TextButton {
                text: qsTr("Save route")
                accentFilled: true
                enabled: root._originKey() !== "" && root._sessionId() !== ""
                onClicked: root.save()
            }
        }
    }
}
