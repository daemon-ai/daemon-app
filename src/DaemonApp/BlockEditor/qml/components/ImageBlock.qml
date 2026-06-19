import QtQuick

import DaemonApp.Theme

Item {
    id: root

    property var imageData: ({})
    property var editorController: null
    property var blockId: undefined

    readonly property string source: (imageData && imageData.source) ? imageData.source : ""
    readonly property string linkUrl: (imageData && imageData.link) ? imageData.link : ""
    readonly property string altText: (imageData && imageData.alt) ? imageData.alt : ""

    // Author-specified Pandoc dimensions. A percent width is relative to the
    // column; a percent height has no well-defined basis here so it is ignored.
    readonly property real explicitWidth: {
        if (!imageData || !imageData.widthValue)
            return 0
        return imageData.widthIsPercent ? root.width * imageData.widthValue / 100 : imageData.widthValue
    }
    readonly property real explicitHeight: {
        if (!imageData || !imageData.heightValue || imageData.heightIsPercent)
            return 0
        return imageData.heightValue
    }

    // Final on-screen size: honor explicit dimensions, always clamp to the column
    // width, and preserve the source aspect ratio when a dimension is missing.
    readonly property size fitted: {
        const col = root.width
        const iw = image.implicitWidth
        const ih = image.implicitHeight
        const ratio = iw > 0 ? ih / iw : 0
        let w
        let h
        if (root.explicitWidth > 0 && root.explicitHeight > 0) {
            w = Math.min(root.explicitWidth, col)
            h = root.explicitHeight * (w / root.explicitWidth)
        } else if (root.explicitWidth > 0) {
            w = Math.min(root.explicitWidth, col)
            h = w * ratio
        } else if (root.explicitHeight > 0 && ratio > 0) {
            h = root.explicitHeight
            w = h / ratio
            if (w > col) {
                w = col
                h = w * ratio
            }
        } else {
            w = Math.min(col, iw)
            h = w * ratio
        }
        return Qt.size(w, h)
    }

    implicitHeight: {
        if (image.status === Image.Error)
            return errorBox.implicitHeight
        if (image.status === Image.Ready && image.implicitWidth > 0)
            return Math.max(root.fitted.height, 1)
        return 28
    }

    Image {
        id: image
        anchors.left: parent.left
        source: root.source
        asynchronous: true
        cache: true
        mipmap: true
        fillMode: Image.PreserveAspectFit
        visible: status === Image.Ready

        // Cap the decode to the displayed width (in device pixels) to bound VRAM;
        // depends only on the column + author width, never on the decoded size.
        sourceSize.width: {
            const target = root.explicitWidth > 0 ? Math.min(root.explicitWidth, root.width) : root.width
            return target > 0 ? Math.round(target * Screen.devicePixelRatio) : 0
        }

        width: root.fitted.width
        height: root.fitted.height
    }

    // Loading placeholder.
    Rectangle {
        anchors.left: parent.left
        width: Math.min(root.width, 240)
        height: 28
        visible: image.status === Image.Loading || image.status === Image.Null
        radius: Theme.radiusSmall
        color: Theme.surfaceRaised
        border.width: Theme.hairline
        border.color: Theme.border

        Text {
            anchors.centerIn: parent
            text: root.altText.length > 0 ? root.altText : qsTr("Loading image…")
            color: Theme.mutedText
            font.pixelSize: Theme.captionFontSize
            elide: Text.ElideRight
        }
    }

    // Error placeholder.
    Rectangle {
        id: errorBox
        anchors.left: parent.left
        width: Math.min(root.width, 320)
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
            text: (root.altText.length > 0 ? root.altText + " — " : "") + qsTr("image unavailable")
            color: Theme.mutedText
            font.pixelSize: Theme.captionFontSize
            wrapMode: Text.Wrap
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        cursorShape: root.linkUrl !== "" ? Qt.PointingHandCursor : Qt.IBeamCursor

        // A linked image opens its target on a single click; a plain image (or a
        // double-click on a linked one) opens the block for raw editing.
        onClicked: {
            if (root.linkUrl !== "") {
                Qt.openUrlExternally(root.linkUrl)
                return
            }
            root.activateForEditing()
        }
        onDoubleClicked: root.activateForEditing()
    }

    function activateForEditing() {
        if (root.editorController && root.blockId !== undefined)
            root.editorController.activateBlock(Number(root.blockId))
    }
}
