// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Settings
import DaemonApp.Controls as Kit

ColumnLayout {
    id: root
    spacing: 14

    SectionLabel { text: qsTr("Diagnostics") }

    ComboRow {
        Layout.fillWidth: true
        label: qsTr("Log level")
        configKey: "advanced/logLevel"
        options: DaemonConfig.options("advanced/logLevel")
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Send anonymous telemetry")
        // Telemetry consent is now node-owned via the Feedback seam (the source
        // of truth the app-feedback dialog also reads), not the old
        // advanced/telemetry config key. Visible behavior is unchanged.
        checked: (typeof Feedback !== "undefined" && Feedback) ? Feedback.telemetryEnabled : false
        onToggled: function(on) {
            if (typeof Feedback !== "undefined" && Feedback)
                Feedback.setTelemetryEnabled(on);
        }
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Enable experimental tools")
        checked: DaemonConfig.value("advanced/experimentalTools", false)
        onToggled: function(on) { DaemonConfig.setValue("advanced/experimentalTools", on); }
    }

    SectionLabel { text: qsTr("Onboarding") }

    Kit.TextButton {
        text: qsTr("Re-run first-run setup")
        onClicked: FirstRun.restart()
    }

    SectionLabel { text: qsTr("Local data") }

    // Full device wipe (App.clearLocalData): connection prefs, tokens, the first-run flag, every
    // UI preference, the SQLite cache, and the images cache. Distinct from "Re-run first-run setup"
    // above, which only clears the setupComplete flag.
    Kit.TextButton {
        text: qsTr("Clear local data…")
        onClicked: clearDataDialog.open()
    }

    // Non-layout holder: the confirm dialog reparents to the window Overlay at open(), so it must
    // not be a direct ColumnLayout child (an explicit width on a layout-managed item is flagged).
    Item {
        Kit.Dialog {
            id: clearDataDialog
            // Explicit width so the wrapping Label content cannot feed implicitWidth back into the
            // dialog (binding loop) - mirrors SettingsMenu's resetDialog.
            width: 380
            title: qsTr("Clear local data?")
            acceptText: qsTr("Clear")
            destructive: true
            onAccepted: App.clearLocalData()

            contentItem: QQC.Label {
                text: qsTr("This erases everything stored on this device: your connection settings "
                           + "and saved sign-in, appearance preferences, and the local cache of "
                           + "sessions and files. It does NOT delete anything on the node itself.\n\n"
                           + "You'll return to first-run setup.")
                wrapMode: Text.WordWrap
                color: Theme.text
                font.family: FontIcons.display
                font.pixelSize: 13
            }
        }
    }

    Item { Layout.fillHeight: true }
}
