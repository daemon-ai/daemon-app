import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Settings

ColumnLayout {
    id: root
    spacing: 14

    SectionLabel { text: qsTr("Approvals") }

    ComboRow {
        Layout.fillWidth: true
        label: qsTr("Approval policy")
        description: qsTr("When the agent must ask before running tools.")
        configKey: "safety/approvalPolicy"
        options: DaemonConfig.options("safety/approvalPolicy")
    }

    SectionLabel { text: qsTr("Sandbox") }

    ComboRow {
        Layout.fillWidth: true
        label: qsTr("Filesystem access")
        configKey: "safety/sandbox"
        options: DaemonConfig.options("safety/sandbox")
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Allow network access")
        checked: DaemonConfig.value("safety/allowNetwork", false)
        onToggled: function(on) { DaemonConfig.setValue("safety/allowNetwork", on); }
    }

    Item { Layout.fillHeight: true }
}
