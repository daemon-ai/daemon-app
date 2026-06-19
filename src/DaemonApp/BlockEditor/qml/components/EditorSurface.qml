import QtQuick

import DaemonApp.Theme

Item {
    id: root

    default property alias content: contentLayer.data

    Item {
        id: contentLayer

        anchors.fill: parent
        anchors.margins: Theme.pageMargin
    }
}
