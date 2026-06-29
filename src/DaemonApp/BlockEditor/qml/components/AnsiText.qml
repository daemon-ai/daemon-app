// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick

import DaemonApp.Theme

// Renders terminal output carrying ANSI SGR colors. The C++ side (be::ansiToSpans,
// surfaced as editorController.ansiSpans) splits the text into styled spans; this
// turns them into a monospace RichText block, mapping the 16-color index to the
// theme ANSI palette and falling back to the body text color for the default.
Item {
    id: root

    property var spans: []

    implicitHeight: label.implicitHeight

    function esc(s) {
        return String(s)
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/ /g, "&nbsp;")
            .replace(/\n/g, "<br/>")
    }

    function colorFor(idx) {
        if (idx === undefined || idx < 0 || idx >= Theme.ansiPalette.length)
            return ""
        return Theme.ansiPalette[idx]
    }

    function buildHtml() {
        const list = root.spans || []
        let out = ""
        for (let i = 0; i < list.length; ++i) {
            const s = list[i]
            const fgIdx = s.inverse ? s.bg : s.fg
            const bgIdx = s.inverse ? s.fg : s.bg
            let style = ""
            const fg = colorFor(fgIdx)
            if (fg !== "")
                style += "color:" + fg + ";"
            const bg = colorFor(bgIdx)
            if (bg !== "")
                style += "background-color:" + bg + ";"
            if (s.bold)
                style += "font-weight:bold;"
            if (s.italic)
                style += "font-style:italic;"
            if (s.underline)
                style += "text-decoration:underline;"
            out += "<span style=\"" + style + "\">" + esc(s.text) + "</span>"
        }
        return out
    }

    Text {
        id: label
        width: parent.width
        textFormat: Text.RichText
        text: root.buildHtml()
        color: Theme.text
        font.family: FontIcons.mono
        font.pixelSize: Theme.bodyFontSize - 1
        wrapMode: Text.Wrap
    }
}
