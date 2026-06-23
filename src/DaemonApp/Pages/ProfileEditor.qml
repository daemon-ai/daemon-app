import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The profile editor + Skills/tools curator. Loads a working copy of the selected
// profile, lets the user edit fields and toggle skills/tools, and writes back via
// IProfileStore.updateProfile on Save. The toggle rows are fully controlled (they
// reflect the working arrays) to avoid binding breaks.
Item {
    id: root

    property string profileId: ""
    // Working copy fields.
    property string wModel: ""
    property var wSkills: []
    property var wTools: []

    signal deleted()

    onProfileIdChanged: load()
    Component.onCompleted: load()

    function load() {
        const p = profileId.length > 0 ? Profiles.profile(profileId) : ({});
        nameField.text = p.name !== undefined ? p.name : "";
        wModel = p.model !== undefined ? p.model : "";
        modelCombo.syncFromModel();
        descField.text = p.description !== undefined ? p.description : "";
        promptArea.text = p.systemPrompt !== undefined ? p.systemPrompt : "";
        wSkills = p.skills !== undefined ? p.skills.slice() : [];
        wTools = p.tools !== undefined ? p.tools.slice() : [];
    }

    function _toggle(arr, id) {
        const i = arr.indexOf(id);
        const copy = arr.slice();
        if (i >= 0) copy.splice(i, 1); else copy.push(id);
        return copy;
    }

    function save() {
        Profiles.updateProfile(profileId, {
            "name": nameField.text,
            "model": root.wModel,
            "description": descField.text,
            "systemPrompt": promptArea.text,
            "skills": wSkills,
            "tools": wTools,
        });
    }

    // Empty state.
    Text {
        anchors.centerIn: parent
        visible: root.profileId.length === 0
        text: qsTr("Select or create a profile.")
        font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
    }

    QQC.ScrollView {
        anchors.fill: parent
        visible: root.profileId.length > 0
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: root.width - 48
            x: 24
            y: 20
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                SectionLabel { text: qsTr("Profile"); Layout.fillWidth: true }
                Kit.TextButton {
                    text: Profiles.defaultProfileId === root.profileId
                          ? qsTr("Default") : qsTr("Make default")
                    enabled: Profiles.defaultProfileId !== root.profileId
                    onClicked: Profiles.setDefault(root.profileId)
                }
                Kit.IconButton {
                    icon: FontIcons.fa_trash; iconColor: Theme.danger
                    iconPointSize: 13; implicitWidth: 32; implicitHeight: 28
                    tooltipText: qsTr("Delete profile")
                    onClicked: { Profiles.remove(root.profileId); root.deleted(); }
                }
            }

            Kit.TextField { id: nameField; Layout.fillWidth: true; placeholderText: qsTr("Name") }
            // Model is chosen from the installed models (shared catalog), not free text.
            Kit.Dropdown {
                id: modelCombo
                Layout.fillWidth: true
                model: ModelCatalog.installedIds()
                // Reflect root.wModel into the current index (called on (re)load).
                function syncFromModel() {
                    currentIndex = Math.max(0, ModelCatalog.installedIds().indexOf(root.wModel));
                }
                onActivated: root.wModel = ModelCatalog.installedIds()[currentIndex]
            }
            Kit.TextField {
                id: descField; Layout.fillWidth: true
                placeholderText: qsTr("Short description")
            }

            SectionLabel { text: qsTr("System prompt") }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 110
                radius: Theme.radius
                color: "transparent"
                border.color: Theme.border

                QQC.ScrollView {
                    anchors.fill: parent
                    anchors.margins: 6
                    clip: true
                    QQC.TextArea {
                        id: promptArea
                        wrapMode: TextEdit.Wrap
                        font.family: FontIcons.display
                        font.pixelSize: 13
                        color: Theme.text
                        background: null
                    }
                }
            }

            // --- Skills curator -------------------------------------------
            SectionLabel { text: qsTr("Skills") }
            Repeater {
                model: Profiles.availableSkills()
                delegate: CuratorRow {
                    required property var modelData
                    Layout.fillWidth: true
                    label: modelData.name
                    description: modelData.description
                    checked: root.wSkills.indexOf(modelData.id) >= 0
                    onToggled: root.wSkills = root._toggle(root.wSkills, modelData.id)
                }
            }

            // --- Tools curator --------------------------------------------
            SectionLabel { text: qsTr("Tools") }
            Repeater {
                model: Profiles.availableTools()
                delegate: CuratorRow {
                    required property var modelData
                    Layout.fillWidth: true
                    label: modelData.name
                    description: modelData.description
                    checked: root.wTools.indexOf(modelData.id) >= 0
                    onToggled: root.wTools = root._toggle(root.wTools, modelData.id)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 6
                Layout.bottomMargin: 20
                Item { Layout.fillWidth: true }
                Kit.TextButton { text: qsTr("Revert"); onClicked: root.load() }
                Kit.TextButton { text: qsTr("Save"); accentFilled: true; onClicked: root.save() }
            }
        }
    }
}
