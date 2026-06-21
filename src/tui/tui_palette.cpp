#include "tui_palette.h"

#include <Tui/ZColor.h>

#include <QColor>

namespace {

// The active theme for the tpal::* accessors. Defaults to Dark so the TUI keeps
// its historical look until main.cpp applies the persisted ui/theme.
theme::ThemeName g_active = theme::ThemeName::Dark;

Tui::ZColor toZ(const QColor &c) { return Tui::ZColor(c.red(), c.green(), c.blue()); }

// Resolve a shared theme token to a ZColor for a specific theme / the active one.
Tui::ZColor tokColor(theme::ThemeName t, theme::Token token)
{
    return toZ(theme::ThemePalette::color(t, token));
}
Tui::ZColor tok(theme::Token token) { return tokColor(g_active, token); }

} // namespace

Tui::ZPalette daemonPalette(theme::ThemeName name)
{
    using theme::Token;

    // Stock-widget chrome roles, sourced from the shared theme tokens for `name`
    // so the quit dialog, lists, and inputs recolor with the custom-painted views.
    const Tui::ZColor bg = tokColor(name, Token::Background);
    const Tui::ZColor bgAlt = tokColor(name, Token::SurfaceRaised); // recessed surface
    const Tui::ZColor fg = tokColor(name, Token::Text);
    const Tui::ZColor dim = tokColor(name, Token::TextMuted);
    const Tui::ZColor border = tokColor(name, Token::Border);
    const Tui::ZColor sel = tokColor(name, Token::RowActive); // neutral selection wash
    const Tui::ZColor accent = tokColor(name, Token::Accent);
    const Tui::ZColor accentFg = tokColor(name, Token::Background); // text on accent
    // A focused text field lifts to the faint accent-tinted "active block" surface
    // so focus is visible even when the field is empty.
    const Tui::ZColor activeField = tokColor(name, Token::ActiveBlockBackground);

    Tui::ZPalette p = Tui::ZPalette::classic();
    p.setColors({
        {"root.bg", bgAlt},

        // Window + dialog frame/body (the quit dialog is a ZDialog == window).
        {"window.default.bg", bg},
        {"window.default.frame.focused.fg", accent},
        {"window.default.frame.focused.bg", bg},
        {"window.default.frame.unfocused.fg", border},
        {"window.default.frame.unfocused.bg", bg},

        {"window.default.text.fg", fg},
        {"window.default.text.bg", bg},
        {"window.default.text.selected.fg", accentFg},
        {"window.default.text.selected.bg", accent},

        // Buttons (quit dialog Quit/Cancel).
        {"window.default.control.bg", bg},
        {"window.default.control.fg", fg},
        {"window.default.control.focused.bg", bg},
        {"window.default.control.focused.fg", accent},
        {"window.default.control.shortcut.fg", accent},

        // List views (sidebar + conversation list).
        {"window.default.dataview.bg", bg},
        {"window.default.dataview.fg", fg},
        {"window.default.dataview.selected.bg", sel},
        {"window.default.dataview.selected.fg", fg},
        {"window.default.dataview.selected.focused.bg", accent},
        {"window.default.dataview.selected.focused.fg", accentFg},

        // Transcript (read-only ZTextEdit).
        {"window.default.textedit.bg", bg},
        {"window.default.textedit.fg", fg},
        {"window.default.textedit.focused.bg", bg},
        {"window.default.textedit.focused.fg", fg},
        {"window.default.textedit.selected.bg", accent},
        {"window.default.textedit.selected.fg", accentFg},
        {"window.default.textedit.linenumber.bg", bgAlt},
        {"window.default.textedit.linenumber.fg", dim},

        // Text inputs (composer). Unfocused sits on the recessed surface; focused
        // lifts to the accent-tinted active surface while keeping text readable, so
        // the focused field is obvious at a glance.
        {"window.default.lineedit.bg", bgAlt},
        {"window.default.lineedit.fg", fg},
        {"window.default.lineedit.focused.bg", activeField},
        {"window.default.lineedit.focused.fg", fg},
    });
    return p;
}

namespace tpal {

using Tui::ZColor;

void setActiveTheme(theme::ThemeName name) { g_active = name; }
theme::ThemeName activeTheme() { return g_active; }

ZColor fg() { return tok(theme::Token::Text); }
ZColor muted() { return tok(theme::Token::TextMuted); }
ZColor faint() { return tok(theme::Token::TextMuted); }
ZColor bg() { return tok(theme::Token::Background); }
ZColor codeBg() { return tok(theme::Token::CodeBackground); }
ZColor accent() { return tok(theme::Token::Accent); }

ZColor statusOk() { return tok(theme::Token::StatusOk); }
ZColor statusError() { return tok(theme::Token::Danger); }
ZColor statusRunning() { return tok(theme::Token::Accent); }
ZColor warn() { return tok(theme::Token::Warning); }

ZColor diffAdd() { return tok(theme::Token::DiffAddText); }
ZColor diffDel() { return tok(theme::Token::DiffDelText); }
ZColor diffHunk() { return tok(theme::Token::Accent); }

ZColor toneColor(const QString &tone)
{
    const QString t = tone.trimmed().toLower();
    if (t == QStringLiteral("terminal") || t == QStringLiteral("pty")) {
        return ZColor(166, 227, 161); // green
    }
    if (t == QStringLiteral("web") || t == QStringLiteral("search")) {
        return ZColor(137, 220, 235); // sky
    }
    if (t == QStringLiteral("edit") || t == QStringLiteral("file")) {
        return ZColor(250, 179, 135); // peach
    }
    if (t == QStringLiteral("code")) {
        return ZColor(203, 166, 247); // mauve
    }
    if (t == QStringLiteral("image")) {
        return ZColor(245, 194, 231); // pink
    }
    if (t == QStringLiteral("agent")) {
        return ZColor(180, 190, 254); // lavender
    }
    return accent(); // default tone tracks the theme accent
}

ZColor ansi(int index)
{
    // Classic xterm 16-color table; bright variants for 8-15. -1 == default fg.
    switch (index) {
    case 0: return ZColor(0, 0, 0);
    case 1: return ZColor(205, 49, 49);
    case 2: return ZColor(13, 188, 121);
    case 3: return ZColor(229, 229, 16);
    case 4: return ZColor(36, 114, 200);
    case 5: return ZColor(188, 63, 188);
    case 6: return ZColor(17, 168, 205);
    case 7: return ZColor(229, 229, 229);
    case 8: return ZColor(102, 102, 102);
    case 9: return ZColor(241, 76, 76);
    case 10: return ZColor(35, 209, 139);
    case 11: return ZColor(245, 245, 67);
    case 12: return ZColor(59, 142, 234);
    case 13: return ZColor(214, 112, 214);
    case 14: return ZColor(41, 184, 219);
    case 15: return ZColor(255, 255, 255);
    default: return fg();
    }
}

ZColor ansiBg(int index)
{
    if (index < 0 || index > 15) {
        return codeBg();
    }
    return ansi(index);
}

QString statusGlyph(const QString &status)
{
    const QString s = status.trimmed().toLower();
    if (s == QStringLiteral("running")) {
        return QStringLiteral("\u25d0"); // ◐
    }
    if (s == QStringLiteral("error")) {
        return QStringLiteral("\u2717"); // ✗
    }
    return QStringLiteral("\u2713"); // ✓
}

QString toneGlyph(const QString &tone)
{
    const QString t = tone.trimmed().toLower();
    if (t == QStringLiteral("terminal") || t == QStringLiteral("pty")) {
        return QStringLiteral("\u276f"); // ❯
    }
    if (t == QStringLiteral("web") || t == QStringLiteral("search")) {
        return QStringLiteral("\u2295"); // ⊕
    }
    if (t == QStringLiteral("edit") || t == QStringLiteral("file")) {
        return QStringLiteral("\u270e"); // ✎
    }
    if (t == QStringLiteral("code")) {
        return QStringLiteral("\u03bb"); // λ
    }
    if (t == QStringLiteral("image")) {
        return QStringLiteral("\u25a3"); // ▣
    }
    if (t == QStringLiteral("agent")) {
        return QStringLiteral("\u2726"); // ✦
    }
    return QStringLiteral("\u2699"); // ⚙
}

QString barGlyph() { return QStringLiteral("\u258c"); }      // ▌
QString reasoningGlyph() { return QStringLiteral("\u2732"); } // ✲

// --- Chrome -------------------------------------------------------------------

ZColor selectionBg() { return tok(theme::Token::RowActive); } // focused row wash
ZColor selectionInactiveBg() { return tok(theme::Token::RowActiveInactive); } // unfocused
ZColor surfaceAlt() { return tok(theme::Token::SurfaceRaised); } // recessed surface
ZColor activeFieldBg() { return tok(theme::Token::ActiveBlockBackground); } // focused field

ZColor gatewayToneColor(const QString &tone)
{
    const QString t = tone.trimmed().toLower();
    if (t == QStringLiteral("danger")) {
        return statusError();
    }
    if (t == QStringLiteral("warning")) {
        return warn();
    }
    return statusOk(); // "default" == ready
}

ZColor gaugeFill() { return accent(); }
ZColor gaugeTrack() { return muted(); }

QString gatewayGlyph(bool alert)
{
    return alert ? QStringLiteral("\u26a0")   // ⚠
                 : QStringLiteral("\u25c9");   // ◉ (signal/connected)
}

QString agentsGlyph() { return QStringLiteral("\u2726"); }   // ✦
QString contextGlyph() { return QStringLiteral("\u25a4"); }  // ▤
QString sessionGlyph() { return QStringLiteral("\u25f7"); }  // ◷ clock-ish
QString versionGlyph() { return QStringLiteral("#"); }
QString sendGlyph() { return QStringLiteral("\u25b2"); }     // ▲
QString stopGlyph() { return QStringLiteral("\u25a0"); }     // ■
QString steerGlyph() { return QStringLiteral("\u2726"); }    // ✦
QString warnGlyph() { return QStringLiteral("\u26a0"); }     // ⚠

QString agentKindGlyph(const QString &kindKey)
{
    const QString k = kindKey.trimmed().toLower();
    if (k == QStringLiteral("sitemap")) {
        return QStringLiteral("\u2261"); // ≡ (orchestrator / hierarchy)
    }
    if (k == QStringLiteral("server")) {
        return QStringLiteral("\u25a6"); // ▦ (server)
    }
    if (k == QStringLiteral("robot")) {
        return QStringLiteral("\u25c9"); // ◉ (agent)
    }
    return QStringLiteral("\u2022"); // • fallback
}

QString tagDot() { return QStringLiteral("\u25cf"); } // ●

QString spinnerFrame(int tick)
{
    static const char16_t frames[] = {0x25d0, 0x25d3, 0x25d1, 0x25d2}; // ◐◓◑◒
    const int i = ((tick % 4) + 4) % 4;
    return QString(QChar(frames[i]));
}

} // namespace tpal
