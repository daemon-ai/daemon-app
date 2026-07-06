// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Safety posture is owned and enforced by the node, not by the app. This section
// therefore carries NO editable controls that would imply otherwise:
//   - Approval policy has a real, node-enforced home (the per-session approval
//     mode in the composer's session settings -> SetSessionMode ->
//     SessionOverlay.approval_mode); we point at it rather than duplicate it with
//     a cosmetic global copy.
//   - Filesystem sandbox and network egress are enforced by the node with no
//     app-facing wire setter, so they are shown read-only (disabled + annotated),
//     never written anywhere. This keeps the app honest: nothing here gates node
//     behavior.
ColumnLayout {
    id: root
    spacing: 14

    SectionLabel { text: qsTr("Approvals") }

    Text {
        Layout.fillWidth: true
        text: qsTr("Approval policy is set per session from the composer's session settings, where it is enforced by the node.")
        color: Theme.textMuted
        font.family: FontIcons.display
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }

    // [wave2:app-delegation] F7/DEL-7: read-only node delegation guardrail ceilings.
    DelegationLimitsSection { Layout.fillWidth: true }

    SectionLabel { text: qsTr("Sandbox") }

    SettingRow {
        Layout.fillWidth: true
        enabled: false
        label: qsTr("Filesystem access")
        description: qsTr("Enforced by the node — not configurable from the app.")

        Kit.Dropdown {
            implicitWidth: 170
            enabled: false
            model: [qsTr("Managed by the node")]
        }
    }

    SettingRow {
        Layout.fillWidth: true
        enabled: false
        label: qsTr("Allow network access")
        description: qsTr("Enforced by the node — not configurable from the app.")

        Kit.Switch {
            enabled: false
            checkable: false
        }
    }

    // [wave2:app-approvals-safety] D4: remembered exec-approval fingerprints are per-session (the
    // wire ops are session-scoped), so they are managed from the composer's session settings, not
    // globally here. Point at their real home rather than duplicate a session-less copy.
    SectionLabel { text: qsTr("Remembered approvals") }

    Text {
        Layout.fillWidth: true
        text: qsTr("Commands you allowed permanently are remembered per session. Review and revoke them from the composer's session settings.")
        color: Theme.textMuted
        font.family: FontIcons.display
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }

    Item { Layout.fillHeight: true }
}
