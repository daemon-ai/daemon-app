// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// [integrations wire v38] The ONE schema-driven account form (Pidgin-style): the SAME dialog serves
// BOTH "add integration" and "edit account", driven entirely by the node's account_schema through
// the shared AccountManagementController. Nothing is hardcoded per protocol — each field renders by
// its `kind` (Password masks, Choice is a dropdown, Number gets a numeric hint) and the controller
// owns the write (add hands off to the auth flow; edit applies a merge-edit that omits untouched
// secrets).
Kit.Dialog {
    id: root

    // The shared controller (bound by the parent to an AccountManagementController instance).
    property var controller: null

    // Local edit buffer: key -> current value. `revision` drives reactivity.
    property var buffer: ({})
    property int revision: 0

    title: controller && controller.mode === "add" ? qsTr("Add integration")
                                                    : qsTr("Account settings")
    acceptText: controller && controller.mode === "add" ? qsTr("Continue") : qsTr("Save")
    width: 420

    // True when every required field carries a value (secrets on ADD are required; on EDIT a blank
    // secret means "keep the stored one", so it is not required).
    readonly property bool formComplete: {
        root.revision;
        if (!controller)
            return false;
        var rows = controller.fields();
        for (var i = 0; i < rows.length; ++i) {
            var f = rows[i];
            var masked = f.kind === "Password";
            var editing = controller.mode === "edit";
            var required = f.required && !(masked && editing);
            var v = root.buffer[f.key];
            if (required && (v === undefined || String(v).length === 0))
                return false;
        }
        return true;
    }
    acceptEnabled: formComplete

    function openAdd(family) {
        controller.beginAdd(family);
        seed();
        open();
    }
    function openEdit(transport) {
        controller.beginEdit(transport);
        seed();
        open();
    }
    function seed() {
        var b = {};
        var rows = controller ? controller.fields() : [];
        for (var i = 0; i < rows.length; ++i)
            b[rows[i].key] = rows[i].value !== undefined ? rows[i].value : "";
        root.buffer = b;
        root.revision++;
    }

    onAccepted: {
        if (!controller)
            return;
        var out = Object.assign({}, root.buffer);
        if (controller.mode === "add")
            controller.submitAdd(out);
        else
            controller.submitEdit(out);
    }
    onRejected: if (controller) controller.cancel()

    // Re-seed when the controller patches values in (TransportSettings landing on edit).
    Connections {
        target: root.controller
        function onFieldsChanged() {
            if (root.visible)
                root.seed();
        }
    }

    contentItem: ColumnLayout {
        spacing: 10

        Repeater {
            model: root.controller ? root.controller.fields : []
            delegate: ColumnLayout {
                id: fieldRow
                required property var entry
                Layout.fillWidth: true
                spacing: 3

                QQC.Label {
                    text: fieldRow.entry.required ? qsTr("%1 *").arg(fieldRow.entry.label)
                                                  : fieldRow.entry.label
                    font.family: FontIcons.display
                    font.pixelSize: 11
                    color: Theme.textMuted
                }

                // Choice -> dropdown; Password -> masked field; Number -> numeric hint; else text.
                Kit.Dropdown {
                    Layout.fillWidth: true
                    visible: fieldRow.entry.kind === "Choice"
                    model: fieldRow.entry.choices
                    onActivated: {
                        root.buffer[fieldRow.entry.key] = currentText;
                        root.revision++;
                    }
                    Component.onCompleted: {
                        if (fieldRow.entry.kind === "Choice") {
                            var cur = root.buffer[fieldRow.entry.key];
                            var idx = fieldRow.entry.choices ? fieldRow.entry.choices.indexOf(cur)
                                                             : -1;
                            currentIndex = idx >= 0 ? idx : 0;
                            root.buffer[fieldRow.entry.key] = currentText;
                            root.revision++;
                        }
                    }
                }

                Kit.TextField {
                    Layout.fillWidth: true
                    visible: fieldRow.entry.kind !== "Choice"
                    text: root.buffer[fieldRow.entry.key] !== undefined
                          ? root.buffer[fieldRow.entry.key] : ""
                    placeholderText: fieldRow.entry.placeholder ? fieldRow.entry.placeholder : ""
                    echoMode: fieldRow.entry.kind === "Password" ? TextInput.Password
                                                                 : TextInput.Normal
                    inputMethodHints: fieldRow.entry.kind === "Number" ? Qt.ImhDigitsOnly
                                                                       : Qt.ImhNone
                    onTextEdited: {
                        root.buffer[fieldRow.entry.key] = text;
                        root.revision++;
                    }
                }

                // Masked-edit hint: a blank secret keeps the stored one (never re-sent).
                QQC.Label {
                    visible: fieldRow.entry.kind === "Password" && root.controller
                             && root.controller.mode === "edit"
                    text: qsTr("Leave blank to keep the current value")
                    font.family: FontIcons.display
                    font.pixelSize: 10
                    color: Theme.textMuted
                }
            }
        }
    }
}
