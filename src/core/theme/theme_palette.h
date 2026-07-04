// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QColor>
#include <QObject>
#include <qqmlintegration.h>
#include <QString>
#include <QStringList>

// Shared, front-end-agnostic palette: the single source of truth for the four
// named themes' color tokens (Light / Dark / Sepia / Midnight). The GUI's
// Theme.qml reads these via the ThemeTokens singleton (keeping its derived
// aliases + metrics in QML); the TUI's tui_palette reads them as ZColors. Hex
// values are copied verbatim from the original Theme.qml ternaries, so the GUI's
// tst_theme regression (canonical per-theme hexes) stays green.
//
// Only the *base* tokens (those defined by literal per-theme hexes) live here.
// Pure aliases (statusRunning == accent, surface == background, ...) and the
// layout metrics remain QML-only - the TUI does not need them.
namespace theme {

enum class ThemeName { Light, Dark, Sepia, Midnight };

// Base color tokens. Names mirror the Theme.qml property names 1:1 so the QML
// string lookup (ThemeTokens.colorFor(theme, "background")) is mechanical.
enum class Token {
    Background,
    ListBackground,
    Sidebar,
    Border,
    Splitter,
    Text,
    TextMuted,
    CountText,
    Hover,
    Pressed,
    RowHover,
    RowActive,
    RowActiveInactive,
    Accent,
    SidebarText,
    ListText,
    ListSnippet,
    ListSeparator,
    SearchBackground,
    SearchBorder,
    SearchFocusBorder,
    SearchText,
    SearchSelection,
    CodeBackground,
    SurfaceRaised,
    ActiveBlockBackground,
    ActiveBlockBorder,
    ToolHeader,
    StatusOk,
    DiffAddBackground,
    DiffDelBackground,
    DiffAddText,
    DiffDelText,
    BubbleUser,
    BubbleUserBorder,
    RoleAvatarAssistant,
    RoleAvatarUserIcon,
    IconColor,
    IconMuted,
    ChipLight,
    ChipDark,
    ChipSepia,
    ChipMidnight,
    ChipBorder,
    Scrim,
    ScrimModal,
    ScrimControl,
    ScrimControlHover,
    ScrimText,
    ScrimTextMuted,
    StatusBarBackground,
    StatusBarText,
    StatusBarHover,
    Warning,
    Danger,
};

// Static core, callable from non-QML C++ (the TUI) without a QML engine.
class ThemePalette {
public:
    [[nodiscard]] static bool isKnown(const QString& name);
    [[nodiscard]] static QStringList allNames();
    // Unknown names resolve to Light (matches the GUI/UiSettings default).
    [[nodiscard]] static ThemeName fromString(const QString& name);
    [[nodiscard]] static QString toString(ThemeName name);

    [[nodiscard]] static QColor color(ThemeName theme, Token token);
    // String-keyed overload for QML; an unknown token yields an invalid QColor.
    [[nodiscard]] static QColor color(const QString& theme, const QString& token);
    [[nodiscard]] static bool hasToken(const QString& token);
};

} // namespace theme

// QML singleton (DaemonApp.ThemeCore): a thin reactive facade over the static
// core so Theme.qml can bind `ThemeTokens.colorFor(theme, "<token>")` and stay
// live on theme switches (the binding depends on the QML-side `theme` string).
class ThemeTokens : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit ThemeTokens(QObject* parent = nullptr);

    Q_INVOKABLE [[nodiscard]] QColor colorFor(const QString& theme, const QString& token) const;
    Q_INVOKABLE [[nodiscard]] bool isKnownTheme(const QString& name) const;
};
