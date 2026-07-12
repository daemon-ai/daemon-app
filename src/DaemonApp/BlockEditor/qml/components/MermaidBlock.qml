// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick

import DaemonApp.Theme

Item {
    id: root

    property var mermaidData: ({})
    property var editorController: null
    property var blockId: undefined

    readonly property string source: (mermaidData && mermaidData.source) ? mermaidData.source : ""

    implicitHeight: controller.hasError ? errorBox.implicitHeight
                                        : Math.max(diagram.implicitHeight, 24)

    DiagramController {
        id: controller
        source: root.source
        contentWidth: root.width
        fontPixelSize: Theme.bodyFontSize - 2
        surfaceColor: Theme.surface
        borderColor: Theme.border
        textColor: Theme.text
        edgeColor: Theme.mutedText
        clusterColor: Theme.surfaceRaised
    }

    DiagramItem {
        id: diagram
        anchors.left: parent.left
        anchors.right: parent.right
        height: implicitHeight
        visible: !controller.hasError
        controller: controller

        HoverHandler {
            id: hover
            onPointChanged: diagram.setHoverAt(hover.point.position)
        }

        WheelHandler {
            acceptedModifiers: Qt.ControlModifier
            onWheel: event => {
                const factor = event.angleDelta.y > 0 ? 1.1 : 1.0 / 1.1
                diagram.zoomAt(factor, point.position)
            }
        }

        // Touch pinch-to-zoom about the gesture centroid; feeds the same
        // incremental zoomAt() the wheel uses.
        PinchHandler {
            target: null
            property real prevScale: 1.0
            onActiveChanged: if (active) prevScale = 1.0
            onActiveScaleChanged: {
                const factor = activeScale / prevScale
                prevScale = activeScale
                diagram.zoomAt(factor, centroid.position)
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton

            // Inherently graphical: pan/zoom/select happen through this surface,
            // which is exposed as a named graphic rather than a control.
            Accessible.role: Accessible.Graphic
            Accessible.name: qsTr("Diagram")

            // Only steal drags from the list when zoomed in (panning); a plain
            // click selects the node under the cursor.
            preventStealing: diagram.zoom > 1.0
            property real lastX: 0
            property real lastY: 0
            property bool panning: false

            onPressed: mouse => {
                lastX = mouse.x
                lastY = mouse.y
                panning = false
            }
            onPositionChanged: mouse => {
                if (diagram.zoom <= 1.0)
                    return
                panning = true
                diagram.panX += mouse.x - lastX
                diagram.panY += mouse.y - lastY
                lastX = mouse.x
                lastY = mouse.y
            }
            onReleased: mouse => {
                if (panning)
                    return
                const id = diagram.hitTest(Qt.point(mouse.x, mouse.y))
                if (id === "") {
                    // Clicking empty diagram space opens the block for raw editing.
                    if (root.editorController && root.blockId !== undefined)
                        root.editorController.activateBlock(Number(root.blockId))
                    return
                }
                diagram.selectedId = (id === diagram.selectedId) ? "" : id
            }
            onDoubleClicked: diagram.resetView()
        }
    }

    Column {
        id: errorBox
        anchors.left: parent.left
        anchors.right: parent.right
        visible: controller.hasError
        spacing: Theme.smallSpacing

        Text {
            width: parent.width
            text: qsTr("Diagram error: %1").arg(controller.errorText)
            color: Theme.mutedText
            font.pixelSize: Theme.captionFontSize
            wrapMode: Text.Wrap
        }

        Rectangle {
            width: parent.width
            implicitHeight: rawText.implicitHeight + Theme.smallSpacing * 2
            color: Theme.surfaceRaised
            border.width: Theme.hairline
            border.color: Theme.border
            radius: Theme.radiusSmall

            Text {
                id: rawText
                anchors.fill: parent
                anchors.margins: Theme.smallSpacing
                text: root.source
                color: Theme.text
                font.family: FontIcons.mono
                font.pixelSize: Theme.captionFontSize
                wrapMode: Text.Wrap
            }
        }
    }
}
