// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <qqmlintegration.h>
#include <QString>

// Shared, front-end-agnostic mapping from domain enums to *semantic* display
// tokens. This is the single source of truth for decisions like "which icon does
// an orchestrator node use" or "which tone does a running agent get". Each front
// end resolves the returned key/tone to its own concrete glyph/color:
//
//   - the QML GUI maps icon keys -> FontIcons glyphs and tones -> Theme colors;
//   - the TUI maps them -> Unicode glyphs and ZColor.
//
// The class is registered as a QML singleton (DaemonApp.Presentation), but the
// real logic lives in static methods so the TUI can call it without a QML engine.
// It is deliberately Quick-free (links only Qt6::Qml + the domain enums).
class DisplayPresenter : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit DisplayPresenter(QObject* parent = nullptr);

    // Semantic lifecycle tone for an agent node, decoupled from any palette.
    enum class StateTone {
        Neutral,  // unknown / no terminal outcome shown
        Running,  // attached, in progress
        Finished, // reached a terminal outcome
    };
    Q_ENUM(StateTone)

    // domain::NodeType -> scope icon key ("comments", "archive", or "" for rows
    // that carry no scope icon). Accepts int so QML can pass the role value.
    Q_INVOKABLE [[nodiscard]] QString scopeIconKey(int nodeType) const;

    // domain::UnitKind -> node icon key ("sitemap", "server", "robot").
    Q_INVOKABLE [[nodiscard]] QString agentKindIconKey(int kind) const;

    // domain::UnitState -> semantic tone.
    Q_INVOKABLE [[nodiscard]] StateTone agentStateTone(int state) const;

    // Static cores: callable from non-QML C++ (the TUI) without an instance.
    [[nodiscard]] static QString scopeIconKeyFor(int nodeType);
    [[nodiscard]] static QString agentKindIconKeyFor(int kind);
    [[nodiscard]] static StateTone agentStateToneFor(int state);
};
