// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QString>

// Free helpers shared across the RootWidget translation units (split out of the
// former root_widget.cpp anonymous namespace so each TU can call them). Kept in a
// named namespace rather than a per-TU anonymous one so there is a single
// definition (root_widget_detail.cpp) instead of one copy per including TU.
namespace rwdetail {

// Best-effort desktop notification from the TUI: notify-send when present
// (reliable on Linux desktops), plus an OSC 9 escape as a terminal-native
// fallback. The BEL urgency hint is rung separately by the caller.
void emitDesktopNotification(const QString& title, const QString& body);

// A short tab title for a session: the first non-empty content line (heading
// markers stripped, capped), falling back to a generic label.
QString titleForContent(const QString& markdown);

// The static markdown shown for a (non-transcript) page tab.
QString pageMarkdown(int kind);

} // namespace rwdetail
