#include "tui_palette.h"

#include <Tui/ZColor.h>

Tui::ZPalette daemonDarkPalette()
{
    using Tui::ZColor;

    // Tokens lifted from the GUI Dark/Midnight theme (Theme.qml), expressed as
    // RGB. Terminals that support truecolor render these directly; others snap to
    // the nearest palette entry.
    const ZColor bg(30, 30, 46);       // base surface (#1e1e2e)
    const ZColor bgAlt(24, 24, 37);    // recessed surface (#181825)
    const ZColor fg(205, 214, 244);    // primary text (#cdd6f4)
    const ZColor dim(127, 132, 156);   // muted text / borders (#7f849c)
    const ZColor sel(69, 71, 90);      // selection wash (#45475a)
    const ZColor accent(137, 180, 250); // accent (#89b4fa)
    const ZColor accentFg(17, 17, 27);  // text on accent (#11111b)

    Tui::ZPalette p = Tui::ZPalette::classic();
    p.setColors({
        {"root.bg", bgAlt},

        {"window.default.bg", bg},
        {"window.default.frame.focused.fg", accent},
        {"window.default.frame.unfocused.fg", dim},

        {"window.default.text.fg", fg},
        {"window.default.text.bg", bg},
        {"window.default.text.selected.fg", accentFg},
        {"window.default.text.selected.bg", accent},

        {"window.default.control.bg", bg},
        {"window.default.control.fg", fg},
        {"window.default.control.focused.bg", bg},
        {"window.default.control.focused.fg", accent},

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

        // Composer (ZInputBox).
        {"window.default.lineedit.bg", bgAlt},
        {"window.default.lineedit.fg", fg},
        {"window.default.lineedit.focused.bg", bgAlt},
        {"window.default.lineedit.focused.fg", accent},
    });
    return p;
}

namespace tpal {

using Tui::ZColor;

ZColor fg() { return ZColor(205, 214, 244); }
ZColor muted() { return ZColor(127, 132, 156); }
ZColor faint() { return ZColor(147, 153, 178); }
ZColor bg() { return ZColor(30, 30, 46); }
ZColor codeBg() { return ZColor(43, 43, 58); }
ZColor accent() { return ZColor(137, 180, 250); }

ZColor statusOk() { return ZColor(166, 227, 161); }
ZColor statusError() { return ZColor(243, 139, 168); }
ZColor statusRunning() { return ZColor(249, 226, 175); }
ZColor warn() { return ZColor(250, 179, 135); }

ZColor diffAdd() { return ZColor(166, 227, 161); }
ZColor diffDel() { return ZColor(243, 139, 168); }
ZColor diffHunk() { return ZColor(137, 180, 250); }

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
    return ZColor(137, 180, 250); // default accent (blue)
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

ZColor selectionBg() { return ZColor(69, 71, 90); }  // #45475a
ZColor surfaceAlt() { return ZColor(24, 24, 37); }   // #181825

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
