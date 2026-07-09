// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Custom OpenAI-compatible providers management (the "generalized Daemon Cloud" surface): list,
// add, edit, and remove user-defined providers. Node-authoritative — every mutation goes through
// the shared ProviderCatalog CRUD (createCustom/updateCustom/removeCustom) and the node re-forces
// provenance + selector and re-validates the base URL. A created provider then appears as a normal
// row in the picker (provider_catalog merges the durable custom rows).
//
// This is the editor's read-your-writes view (ProviderCatalog.customProviders()), distinct from the
// merged picker rows. Config-seeded rows render read-only (not user-removable).
ColumnLayout {
    id: section
    spacing: 8

    // Re-read explicitly (a plain Q_INVOKABLE, populated asynchronously) on customProvidersChanged.
    property var rows: (typeof ProviderCatalog !== "undefined" && ProviderCatalog)
                       ? ProviderCatalog.customProviders() : []
    function refresh() {
        rows = (typeof ProviderCatalog !== "undefined" && ProviderCatalog)
               ? ProviderCatalog.customProviders() : [];
    }
    Component.onCompleted: refresh()
    Connections {
        target: ProviderCatalog
        function onCustomProvidersChanged() { section.refresh(); }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        SectionLabel { text: qsTr("Custom providers") }
        Item { Layout.fillWidth: true }
        Kit.TextButton {
            text: qsTr("Add custom provider")
            onClicked: editor.openForNew()
        }
    }

    Text {
        visible: section.rows.length === 0
        Layout.fillWidth: true
        text: qsTr("No custom providers yet. Add an OpenAI-compatible endpoint to select it like "
                   + "any other provider.")
        wrapMode: Text.WordWrap
        font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
    }

    Repeater {
        model: section.rows
        delegate: Rectangle {
            required property var modelData
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            radius: 8
            color: Theme.surface
            border.color: Theme.border

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: modelData.displayName && modelData.displayName.length > 0
                              ? modelData.displayName : modelData.id
                        font.family: FontIcons.display; font.pixelSize: 14
                        font.bold: true; color: Theme.text; elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Text {
                        text: modelData.baseUrl
                        font.family: FontIcons.display; font.pixelSize: 11
                        color: Theme.textMuted; elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Text {
                    visible: modelData.source === "config"
                    text: qsTr("From config")
                    font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                }
                Kit.TextButton {
                    text: qsTr("Edit")
                    onClicked: editor.openForEdit(modelData)
                }
                Kit.TextButton {
                    // Config-seeded providers are owned by node config and are not user-removable.
                    visible: modelData.source !== "config"
                    text: qsTr("Remove")
                    onClicked: {
                        if (typeof ProviderCatalog !== "undefined" && ProviderCatalog)
                            ProviderCatalog.removeCustom(modelData.id);
                    }
                }
            }
        }
    }

    // The add/edit editor. The node validates + persists; on accept we submit the field map and
    // let customProvidersChanged refresh the list.
    Kit.Dialog {
        id: editor
        title: editor.isEdit ? qsTr("Edit custom provider") : qsTr("Add custom provider")
        acceptText: qsTr("Save")
        // id + base URL are the minimum a usable provider needs.
        acceptEnabled: idField.text.trim().length > 0 && baseUrlField.text.trim().length > 0

        property bool isEdit: false

        function openForNew() {
            isEdit = false;
            idField.text = "";
            nameField.text = "";
            baseUrlField.text = "";
            credField.text = "";
            requiresKeyBox.checked = false;
            open();
        }
        function openForEdit(row) {
            isEdit = true;
            idField.text = row.id ? row.id : "";
            nameField.text = row.displayName ? row.displayName : "";
            baseUrlField.text = row.baseUrl ? row.baseUrl : "";
            credField.text = row.credentialRef ? row.credentialRef : "";
            requiresKeyBox.checked = row.requiresKey === true;
            open();
        }

        onAccepted: {
            if (typeof ProviderCatalog === "undefined" || !ProviderCatalog)
                return;
            var fields = {
                "id": idField.text.trim(),
                "displayName": nameField.text.trim(),
                "baseUrl": baseUrlField.text.trim(),
                "requiresKey": requiresKeyBox.checked,
                "credentialRef": credField.text.trim()
            };
            if (editor.isEdit)
                ProviderCatalog.updateCustom(fields);
            else
                ProviderCatalog.createCustom(fields);
        }

        contentItem: ColumnLayout {
            spacing: 8
            implicitWidth: 360

            SectionLabel { text: qsTr("Id") }
            Kit.TextField {
                id: idField
                Layout.fillWidth: true
                // The id is the stable key; editing an existing provider keeps it fixed.
                enabled: !editor.isEdit
                placeholderText: qsTr("custom/my-gateway")
            }
            SectionLabel { text: qsTr("Display name") }
            Kit.TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: qsTr("My Gateway")
            }
            SectionLabel { text: qsTr("Base URL") }
            Kit.TextField {
                id: baseUrlField
                Layout.fillWidth: true
                placeholderText: qsTr("https://my-gateway/v1/")
            }
            Kit.CheckBox {
                id: requiresKeyBox
                text: qsTr("Requires an API key")
            }
            SectionLabel { text: qsTr("Default credential (optional)") }
            Kit.TextField {
                id: credField
                Layout.fillWidth: true
                placeholderText: qsTr("credential ref")
            }
        }
    }
}
