import QtQuick
import QtQuick.Layouts

import DaemonApp.Theme

Item {
    id: root

    property var tableData: ({})
    // Wired by the delegate so cells can map clicks/drags to block raw offsets
    // and self-paint the document selection.
    property var editorController: null
    property var blockId: undefined

    readonly property int columnCount: (tableData && tableData.columns) ? tableData.columns : 0

    implicitHeight: grid.implicitHeight
    implicitWidth: grid.implicitWidth

    function alignmentFor(col) {
        const aligns = (tableData && tableData.alignments) ? tableData.alignments : []
        const a = col < aligns.length ? aligns[col] : 0
        if (a === 1)
            return Text.AlignRight
        if (a === 2)
            return Text.AlignHCenter
        return Text.AlignLeft
    }

    // Flatten header + body into one cell list so a single GridLayout/Repeater
    // lays them out; each entry carries its column index, header flag, and the
    // flattened rowIndex (0 = header, r+1 = body row r) used for offset mapping.
    readonly property var cells: {
        const out = []
        if (!tableData || !tableData.columns)
            return out
        const header = tableData.header || []
        for (let c = 0; c < tableData.columns; ++c)
            out.push({ markup: header[c] || "", col: c, header: true, rowIndex: 0 })
        const rows = tableData.rows || []
        for (let r = 0; r < rows.length; ++r) {
            const row = rows[r]
            for (let c = 0; c < tableData.columns; ++c)
                out.push({ markup: (row && row[c]) || "", col: c, header: false, rowIndex: r + 1 })
        }
        return out
    }

    // Resolve a scene point to { rowIndex, col, inCellVisual }. First try an exact
    // hit on a cell; if the point lands in inter-cell padding, the border, or just
    // above/below the table (cross-block drag entry), fall back to the nearest cell
    // by center distance and clamp into it. Returns null only when there are no cells.
    function cellHitAt(scenePoint) {
        let nearest = null
        let nearestDist = Number.MAX_VALUE
        const kids = grid.children
        for (let i = 0; i < kids.length; ++i) {
            const child = kids[i]
            if (!child || typeof child.hitTest !== "function")
                continue
            const hit = child.hitTest(scenePoint, false)
            if (hit)
                return hit
            const center = child.mapToItem(null, child.width / 2, child.height / 2)
            const dx = center.x - scenePoint.x
            const dy = center.y - scenePoint.y
            const d = dx * dx + dy * dy
            if (d < nearestDist) {
                nearestDist = d
                nearest = child
            }
        }
        return nearest ? nearest.hitTest(scenePoint, true) : null
    }

    GridLayout {
        id: grid
        anchors.left: parent.left
        anchors.right: parent.right
        columns: root.columnCount > 0 ? root.columnCount : 1
        columnSpacing: 0
        rowSpacing: 0

        Repeater {
            model: root.cells

            delegate: Rectangle {
                id: cell
                required property var modelData

                Layout.fillWidth: true
                Layout.fillHeight: true
                implicitHeight: cellText.implicitHeight + Theme.smallSpacing * 2
                color: cell.modelData.header ? Theme.surfaceRaised : Theme.surface
                border.width: Theme.hairline
                border.color: Theme.border

                // Document-selection highlight for this cell, recomputed whenever the
                // selection changes. Coordinates are offset by cellText's position so
                // the rects line up under the glyphs.
                readonly property var selectionRects: {
                    const _rev = root.editorController ? root.editorController.selectionRevision : 0
                    if (!root.editorController || root.blockId === undefined || !cellText.textDocument)
                        return []
                    const span = root.editorController.tableCellSelectionSpan(
                        Number(root.blockId), cell.modelData.rowIndex, cell.modelData.col)
                    if (!span || !span.has)
                        return []
                    const _w = cellText.width
                    const _h = cellText.contentHeight
                    const rects = root.editorController.activeTextController.lineRectsForRange(
                        cellText.textDocument, span.start, span.end)
                    const out = []
                    for (let i = 0; i < rects.length; ++i) {
                        const r = rects[i]
                        out.push({ x: cellText.x + r.x, y: cellText.y + r.y, width: r.width, height: r.height })
                    }
                    return out
                }

                // Hit-test this cell. When clamp is false, returns null unless the point
                // is inside the cell bounds; when clamp is true, always resolves by
                // clamping the point into the cell's text rect (used for nearest-cell
                // fallback). inCellVisual is the in-cell visual caret offset.
                function hitTest(scenePoint, clamp) {
                    const local = cell.mapFromItem(null, scenePoint.x, scenePoint.y)
                    const inside = local.x >= 0 && local.y >= 0 && local.x <= cell.width && local.y <= cell.height
                    if (!inside && !clamp)
                        return null
                    const t = cellText.mapFromItem(null, scenePoint.x, scenePoint.y)
                    const x = Math.max(0, Math.min(t.x, cellText.width))
                    const y = Math.max(0, Math.min(t.y, cellText.height))
                    // Only report a link for an exact (unclamped) hit so the
                    // nearest-cell fallback never reports a phantom link.
                    return {
                        rowIndex: cell.modelData.rowIndex,
                        col: cell.modelData.col,
                        inCellVisual: cellText.positionAt(x, y),
                        link: inside ? cellText.linkAt(t.x, t.y) : ""
                    }
                }

                // Selection highlight, declared before the text so it paints behind.
                Repeater {
                    model: cell.selectionRects
                    delegate: Rectangle {
                        required property var modelData
                        x: modelData.x
                        y: modelData.y
                        width: modelData.width
                        height: modelData.height
                        color: Theme.selection
                        opacity: 0.35
                    }
                }

                TextEdit {
                    id: cellText
                    anchors.fill: parent
                    anchors.margins: Theme.smallSpacing
                    // Read-only TextEdit (not Text) so the cell exposes positionAt
                    // hit-testing and a QTextDocument for selection geometry. Native
                    // selection is off; the document selection is painted above.
                    readOnly: true
                    selectByMouse: false
                    activeFocusOnPress: false
                    text: cell.modelData.markup
                    textFormat: TextEdit.RichText
                    wrapMode: TextEdit.Wrap
                    color: Theme.text
                    font.pixelSize: Theme.bodyFontSize
                    font.bold: cell.modelData.header
                    horizontalAlignment: root.alignmentFor(cell.modelData.col)
                    verticalAlignment: TextEdit.AlignVCenter
                    renderType: Text.QtRendering
                }
            }
        }
    }
}
