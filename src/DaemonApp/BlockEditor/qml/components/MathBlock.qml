// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

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

    // Logical (device-independent) size from the same MicroTeX measurer inline
    // math uses, so a block renders at the identical scale and crisply, instead
    // of inheriting the supersampled texture's implicit (oversized) pixel size.
    // Touches the same palette signals as `source` so it re-evaluates on a
    // theme/font-size change.
    readonly property size logical: {
        if (!editorController || latex.length === 0)
            return Qt.size(0, 0)
        void editorController.bodyTextColor
        void editorController.bodyFontSize
        return editorController.mathLogicalSize(latex, true)
    }

    implicitHeight: {
        if (image.status === Image.Error)
            return errorBox.implicitHeight
        if (root.logical.width > 0)
            return Math.max(Math.min(root.width, root.logical.width) * (root.logical.height / root.logical.width), 1)
        return 28
    }

    Image {
        id: image
        // Whole-pixel x; sub-pixel centering softens the rasterized glyphs.
        x: Math.round((parent.width - width) / 2)
        source: root.source
        asynchronous: true
        cache: true
        fillMode: Image.PreserveAspectFit
        visible: status === Image.Ready
        // Clamp the formula to the column while preserving its aspect ratio. Size
        // from the logical measurer (matches inline math); fall back to the
        // texture's implicit size only until the measurer resolves.
        width: root.logical.width > 0 ? Math.min(root.width, root.logical.width) : Math.min(root.width, implicitWidth)
        height: root.logical.width > 0
                ? width * (root.logical.height / root.logical.width)
                : (implicitWidth > 0 ? width * (implicitHeight / implicitWidth) : implicitHeight)
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
