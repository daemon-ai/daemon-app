#pragma once

#include <Tui/ZColor.h>
#include <Tui/ZCommon.h>
#include <Tui/ZPalette.h>

#include <QString>

#include "theme/theme_palette.h"

// A Tui palette built from the GUI's theme tokens (the shared theme::ThemePalette
// that also backs Theme.qml): each GUI hex token becomes a named terminal palette
// entry for the given theme, so stock widgets (lists, inputs, the quit dialog)
// recolor with the rest of the app. Only the roles the shell surfaces are
// overridden; the rest fall back to the built-in classic theme.
Tui::ZPalette daemonPalette(theme::ThemeName name);

// Semantic transcript palette + glyph helpers. The TUI's custom transcript view
// paints styled spans directly (it cannot use ZPalette roles per-span), so the
// colors the GUI's Theme.qml exposes as tokens are mirrored here as ZColors and
// the FontAwesome-equivalent unicode glyphs as plain strings. Centralizing them
// keeps the block "delegates" (transcript_render.cpp) declarative.
namespace tpal {

// Active theme for the semantic accessors below. The TUI sets this once at
// startup (from the persisted ui/theme) and again whenever the user cycles
// themes; every accessor then resolves its color from the shared theme table for
// the active theme. Glyphs and the ANSI table are theme-independent.
void setActiveTheme(theme::ThemeName name);
theme::ThemeName activeTheme();

// Surface + text.
Tui::ZColor fg();      // primary text (#cdd6f4)
Tui::ZColor muted();   // dim text / borders (#7f849c)
Tui::ZColor faint();   // very dim (reasoning prose) (#9399b2)
Tui::ZColor bg();      // base surface (#1e1e2e)
Tui::ZColor codeBg();  // recessed code/card surface
Tui::ZColor accent();  // header accent / focus (#89b4fa)

// Status (tool / reasoning lifecycle).
Tui::ZColor statusOk();      // settled ok (green)
Tui::ZColor statusError();   // failed (red)
Tui::ZColor statusRunning(); // in flight (peach/yellow)
Tui::ZColor warn();          // approval gate (peach)

// Unified-diff line tones.
Tui::ZColor diffAdd();
Tui::ZColor diffDel();
Tui::ZColor diffHunk();

// Tool tone -> accent color (terminal/web/edit/code/image/agent/tool...).
Tui::ZColor toneColor(const QString &tone);

// 16-color ANSI palette entry (0-15); -1 (or out of range) yields the default
// foreground. Used to resolve be::ansiToSpans fg/bg indices to real colors.
Tui::ZColor ansi(int index);
Tui::ZColor ansiBg(int index); // like ansi(), but -1 maps to the card background

// Lifecycle status ("running"/"ok"/"error") -> glyph.
QString statusGlyph(const QString &status);
// Tool tone -> glyph (mirrors the GUI's per-tone FontAwesome icon).
QString toneGlyph(const QString &tone);

// Shared structural glyphs.
QString barGlyph();       // card left rule (left half block)
QString reasoningGlyph(); // reasoning header marker

// --- Chrome (conversation list / status bar / completion popup) ---------------
// Colors reused by the custom-painted chrome widgets that bring the TUI shell to
// visual parity with the GUI (ConversationListView, StatusBarView, ComposerChrome,
// CompletionView). They mirror the same Theme.qml tokens the GUI chrome uses.

Tui::ZColor selectionBg();         // active-row wash when the list is focused
Tui::ZColor selectionInactiveBg(); // weaker wash when the list is not focused
Tui::ZColor surfaceAlt();          // recessed popup/footer surface (#181825)
Tui::ZColor activeFieldBg();       // focused text-field surface (accent-tinted)

// gatewayTone ("danger"/"warning"/"default") -> red/peach/green (ready = green).
Tui::ZColor gatewayToneColor(const QString &tone);

Tui::ZColor gaugeFill();  // context-gauge filled cells
Tui::ZColor gaugeTrack(); // context-gauge empty track

// Chrome glyphs (all width-1, geometric - safe across terminal fonts).
QString gatewayGlyph(bool alert); // signal (\u25c9) vs alert (\u26a0)
QString agentsGlyph();            // agents cluster marker
QString contextGlyph();           // context-window marker
QString sessionGlyph();           // session-timer marker
QString versionGlyph();           // app-version marker ('#')
QString sendGlyph();              // send affordance (\u25b2)
QString stopGlyph();              // stop affordance (\u25a0)
QString steerGlyph();             // steer affordance (\u2726)
QString warnGlyph();              // error/alert (\u26a0)
// domain agent-kind icon key ("sitemap"/"server"/"robot") -> distinct glyph.
QString agentKindGlyph(const QString &kindKey);
QString tagDot();              // tag chip bullet (\u25cf)
QString spinnerFrame(int tick); // ◐◓◑◒ braille-style turn spinner

} // namespace tpal
