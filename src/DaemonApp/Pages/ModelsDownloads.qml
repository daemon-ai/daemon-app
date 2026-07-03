// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import DaemonApp.Theme

// Downloads tab: the shared DownloadsPanel (full variant) over ModelCatalog.downloads —
// live progress, pause/resume/cancel, inline errors with Retry, Dismiss for finished rows.
Item {
    id: root
    anchors.fill: parent
    anchors.margins: 20

    Text {
        anchors.centerIn: parent
        visible: ModelCatalog.downloads.count === 0
        text: qsTr("No active downloads.\nStart one from the Discover tab.")
        horizontalAlignment: Text.AlignHCenter
        font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
    }

    DownloadsPanel {
        anchors.fill: parent
    }
}
