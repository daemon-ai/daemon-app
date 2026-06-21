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
