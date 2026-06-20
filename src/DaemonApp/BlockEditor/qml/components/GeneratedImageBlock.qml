import QtQuick

import DaemonApp.Theme

// Passive render for the image_generate tool result (Hermes inventory #6): an
// aspect-ratio frame that shows a diffusion-style shimmer while the tool is
// running, fades the image in once it resolves, and offers a lightbox (click)
// plus a download affordance. Driven by the ToolCall metadata passed through
// buildToolView (imageUrl, aspectRatio, status).
Item {
    id: root

    property var toolData: ({})
    property var editorController: null
    property var blockId: undefined

    readonly property string imageUrl: (toolData && toolData.imageUrl) ? String(toolData.imageUrl) : ""
    readonly property string status: (toolData && toolData.status) ? toolData.status : "ok"
    readonly property bool running: status === "running" || imageUrl.length === 0

    // Parse an "W:H" aspect ratio (default square). Guards against malformed
    // values so the frame never collapses to zero height.
    readonly property real aspectRatio: {
        const raw = (toolData && toolData.aspectRatio) ? String(toolData.aspectRatio) : ""
        const parts = raw.split(":")
        if (parts.length === 2) {
            const w = parseFloat(parts[0])
            const h = parseFloat(parts[1])
            if (w > 0 && h > 0)
                return w / h
        }
        return 1.0
    }

    // Clamp the frame to a readable size within the column.
    readonly property real frameWidth: Math.min(root.width, 360)
    implicitHeight: Math.max(1, frameWidth / aspectRatio)

    Rectangle {
        id: frame
        anchors.left: parent.left
        width: root.frameWidth
        height: root.implicitHeight
        radius: Theme.radiusSmall
        color: Theme.surfaceRaised
        border.width: Theme.hairline
        border.color: Theme.toolBorder
        clip: true

        Image {
            id: image
            anchors.fill: parent
            source: root.imageUrl
            asynchronous: true
            cache: true
            mipmap: true
            fillMode: Image.PreserveAspectCrop
            visible: status === Image.Ready
            opacity: status === Image.Ready ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 220 } }

            sourceSize.width: Math.round(frame.width * Screen.devicePixelRatio)
        }

        // Diffusion-style placeholder: a sweeping shimmer while the tool runs or
        // the image is still decoding.
        Rectangle {
            id: shimmer
            anchors.fill: parent
            visible: root.running || image.status !== Image.Ready
            color: Theme.toolHeader

            Rectangle {
                width: parent.width * 0.4
                height: parent.height
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Theme.transparent }
                    GradientStop { position: 0.5; color: Theme.hover }
                    GradientStop { position: 1.0; color: Theme.transparent }
                }
                visible: root.running

                XAnimator on x {
                    running: shimmer.visible && root.running
                    from: -parent.width * 0.4
                    to: parent.width
                    duration: 1100
                    loops: Animation.Infinite
                }
            }

            Text {
                anchors.centerIn: parent
                text: FontIcons.fa_wand_magic_sparkles
                font.family: FontIcons.faSolid
                font.pixelSize: Theme.bodyFontSize * 2
                color: Theme.mutedText
                opacity: 0.6
            }
        }

        // Download affordance (opens the source externally as a save stub).
        Rectangle {
            id: downloadButton
            visible: root.imageUrl.length > 0
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.smallSpacing
            width: 24
            height: 24
            radius: Theme.radiusSmall
            color: downloadHover.hovered ? Theme.pressed : Theme.toolHeader
            opacity: 0.9

            Text {
                anchors.centerIn: parent
                text: FontIcons.fa_download
                font.family: FontIcons.faSolid
                font.pixelSize: Theme.captionFontSize
                color: Theme.text
            }

            HoverHandler { id: downloadHover }
            TapHandler {
                onTapped: Qt.openUrlExternally(root.imageUrl)
            }
        }

        // Click the image (not the download button) to open the lightbox.
        MouseArea {
            anchors.fill: parent
            anchors.rightMargin: 32
            enabled: root.imageUrl.length > 0
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (root.editorController)
                    root.editorController.requestImagePreview(root.imageUrl, "")
            }
        }
    }
}
