import QtQuick

import DaemonApp.Theme

Item {
    id: root

    property var mathData: ({})
    property var editorController: null
    property var blockId: undefined

    readonly property string latex: (mathData && mathData.source) ? mathData.source : ""

    // The provider URL carries the live theme palette, so touch the palette-
    // affecting controller properties here to re-render on a theme/size change.
    readonly property string source: {
        if (!editorController || latex.length === 0)
            return ""
        void editorController.bodyTextColor
        void editorController.bodyFontSize
        return editorController.mathImageUrl(latex, true)
    }

    implicitHeight: {
        if (image.status === Image.Error)
            return errorBox.implicitHeight
        if (image.status === Image.Ready && image.implicitHeight > 0)
            return Math.max(Math.min(root.width, image.implicitWidth) * (image.implicitHeight / image.implicitWidth), 1)
        return 28
    }

    Image {
        id: image
        anchors.horizontalCenter: parent.horizontalCenter
        source: root.source
        asynchronous: true
        cache: true
        fillMode: Image.PreserveAspectFit
        visible: status === Image.Ready
        // Clamp the formula to the column while preserving its aspect ratio.
        width: Math.min(root.width, implicitWidth)
        height: implicitWidth > 0 ? width * (implicitHeight / implicitWidth) : implicitHeight
    }

    Rectangle {
        id: errorBox
        anchors.left: parent.left
        anchors.right: parent.right
        implicitHeight: errorText.implicitHeight + Theme.smallSpacing * 2
        visible: image.status === Image.Error
        radius: Theme.radiusSmall
        color: Theme.surfaceRaised
        border.width: Theme.hairline
        border.color: Theme.border

        Text {
            id: errorText
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: Theme.smallSpacing
            text: qsTr("Math error: ") + root.latex
            color: Theme.mutedText
            font.family: FontIcons.mono
            font.pixelSize: Theme.captionFontSize
            wrapMode: Text.Wrap
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.IBeamCursor
        onClicked: root.activateForEditing()
        onDoubleClicked: root.activateForEditing()
    }

    function activateForEditing() {
        if (root.editorController && root.blockId !== undefined)
            root.editorController.activateBlock(Number(root.blockId))
    }
}
