// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// [wave2:app-engines] Origin engine chip for approval surfaces (C5 / ENG-6). Given a `sessionId`,
// it resolves the REQUESTING engine through the shared EngineIdentity facade (session -> profile ->
// engine, joined with the catalog protocol) and renders a compact chip naming the foreign agent
// (e.g. "gemini · ACP" / "claude · stream-json"). It renders NOTHING for native sessions, so it is
// no-noise and safe to drop unconditionally into any approval row.
//
// Self-contained by design: it does its own facade join, so the approvals card (owned by the
// app-approvals-safety stream this wave) can insert it with a SINGLE line and no extra wiring —
// see the insertion snippets in this stream's landing record (.stream-plan.md). It is intentionally
// NOT inserted here to avoid a cross-branch QML reference into the D3-redesigned card.
Kit.Chip {
    id: root

    // The approval's session (the wire HostRequest is session-scoped; engine is the session's
    // profile property). Empty => nothing renders.
    property string sessionId: ""

    // EngineIdentity.engineForSession is a plain Q_INVOKABLE (not reactive); bump a revision on the
    // facade's change signal and reference it so this re-resolves when the catalog/profiles change.
    property int rev: 0
    Connections {
        target: (typeof EngineIdentity !== "undefined" && EngineIdentity) ? EngineIdentity : null
        function onChanged() { root.rev++; }
    }

    readonly property var info: {
        var r = root.rev;
        return (r >= 0 && root.sessionId.length > 0 && typeof EngineIdentity !== "undefined"
                && EngineIdentity)
               ? EngineIdentity.engineForSession(root.sessionId)
               : ({ "engine": "", "label": "" });
    }

    visible: info.engine === "Foreign"
    text: info.label && info.label.length > 0 ? info.label : qsTr("Foreign")
    iconGlyph: FontIcons.fa_robot
    tone: "accent"
    tooltipText: qsTr("Requested by this foreign agent")
}
