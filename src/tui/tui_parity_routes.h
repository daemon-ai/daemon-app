// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QSet>
#include <QString>

// PARITY POLICY (enforced by tests/tui/tui_parity_tests.cpp): the node decides,
// the apps render - and there are TWO renderers. Every TabModel::Kind and every
// CommandRegistry command id (and every first-run phase) ships in BOTH the QML
// GUI and the TUI, or carries an explicit accepted-divergence entry in the
// parity test. These two functions are the TUI's hand-maintained coverage
// declaration - a small TU beside the dispatch code, so a route added to a
// dispatcher without updating it is obvious in review, and a new kind/command
// landing without a TUI route fails the test.
namespace tuiroutes {

// The TabModel kinds the TUI renders. Dispatch sites to keep in sync:
// RootWidget::activateTab (root_widget_tabs.cpp: Transcript/File) and
// TuiPageHub::pageMarkdownForKind (tui_page_hub.cpp -> pages/hub_*.cpp).
[[nodiscard]] QSet<int> routedKinds();

// The CommandRegistry ids the TUI routes. Dispatch sites to keep in sync:
// RootWidget::handleComposerCommand (root_widget_wiring.cpp: slash + palette
// funnel), TuiPageHub::openManagerPage (tui_page_hub.cpp: manager-page ids)
// and the palette-only branches in TuiOverlayHost::openCommandPalette
// (tui_overlay_host.cpp: "search" / "files").
[[nodiscard]] QSet<QString> handledCommands();

} // namespace tuiroutes
