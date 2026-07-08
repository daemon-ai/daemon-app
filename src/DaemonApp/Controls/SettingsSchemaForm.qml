// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// [acct-mgmt] Renders a node-described AccountSettingsSchema — a list of {key, label, required}
// fields — as labeled Kit.TextField rows and exposes the filled {key: value} map. Node-first: the
// form is entirely data (the join/create dialogs and later phases reuse this); nothing is
// hardcoded per family. `complete` is true when every required field is non-empty, so dialogs can
// gate their primary action on it.
ColumnLayout {
    id: root

    // The schema fields: a list of maps {key (string), label (string), required (bool)}.
    property var fields: []

    // Bump on every edit so the derived bindings below re-evaluate.
    property int revision: 0
    // key -> current value (mutated in place on edit; revision drives reactivity).
    property var buffer: ({})

    // The filled {key: value} map — read by the consuming dialog on accept.
    readonly property var values: {
        root.revision; // track edits
        return Object.assign({}, root.buffer);
    }
    // True when every required field carries a non-empty value.
    readonly property bool complete: {
        root.revision; // track edits
        for (var i = 0; i < root.fields.length; ++i) {
            var f = root.fields[i];
            if (f.required && (!root.buffer[f.key] || String(root.buffer[f.key]).length === 0))
                return false;
        }
        return true;
    }

    spacing: 8

    Repeater {
        model: root.fields
        delegate: ColumnLayout {
            required property var modelData
            Layout.fillWidth: true
            spacing: 3

            Text {
                text: modelData.required
                      ? qsTr("%1 *").arg(modelData.label)
                      : modelData.label
                font.family: FontIcons.display
                font.pixelSize: 11
                color: Theme.textMuted
            }
            Kit.TextField {
                Layout.fillWidth: true
                onTextChanged: {
                    root.buffer[modelData.key] = text;
                    root.revision++;
                }
            }
        }
    }
}
