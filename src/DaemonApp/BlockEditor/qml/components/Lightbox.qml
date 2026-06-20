import QtQuick
import QtQuick.Controls

import DaemonApp.Theme

// Shared modal image lightbox (Hermes inventory #14 ZoomableImage): a dimmed
// full-surface overlay that shows one image with wheel zoom and drag pan; click
// the backdrop or press Escape to dismiss. Driven by show(url, alt); host wires
// it to editorController.imagePreviewRequested.
Popup {
    id: root

    property string source: ""
    property string alt: ""

    function show(url, altText) {
        root.source = url
        root.alt = altText ? altText : ""
        viewport.resetView()
        root.open()
    }

    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 0

    // Fill the whole overlay surface.
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0

    background: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.82)
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.55)
    }

    contentItem: Item {
        id: viewport

        function resetView() {
            previewImage.scale = 1.0
            previewImage.x = 0
            previewImage.y = 0
        }

        // Backdrop click closes (the image swallows its own clicks).
        TapHandler {
            onTapped: root.close()
        }

        Image {
            id: previewImage
            anchors.centerIn: parent
            source: root.source
            asynchronous: true
            cache: true
            mipmap: true
            fillMode: Image.PreserveAspectFit
            width: Math.min(viewport.width * 0.92, implicitWidth)
            height: Math.min(viewport.height * 0.92, implicitHeight)
            transformOrigin: Item.Center

            // Swallow taps so clicking the image doesn't close the lightbox.
            TapHandler { onTapped: {} }
            DragHandler { }
            WheelHandler {
                onWheel: event => {
                    const step = event.angleDelta.y > 0 ? 1.1 : 1 / 1.1
                    previewImage.scale = Math.max(0.2, Math.min(8.0, previewImage.scale * step))
                }
            }
        }

        // Close affordance.
        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.spacing
            width: 32
            height: 32
            radius: Theme.radiusSmall
            color: closeHover.hovered ? Qt.rgba(1, 1, 1, 0.2) : Qt.rgba(1, 1, 1, 0.1)

            Text {
                anchors.centerIn: parent
                text: FontIcons.fa_xmark
                font.family: FontIcons.faSolid
                font.pixelSize: Theme.bodyFontSize
                color: "#ffffff"
            }

            HoverHandler { id: closeHover }
            TapHandler { onTapped: root.close() }
        }

        Text {
            visible: root.alt.length > 0
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: Theme.spacingLarge
            text: root.alt
            color: "#e6e6e6"
            font.pixelSize: Theme.captionFontSize
        }
    }
}
