// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <Tui/ZListView.h>

namespace daemonapp::daemon {
class AgentRepository;
}

// The TUI projection of the shared AgentTypePicker (ENG-2 / CON-16): one ZListView listing
// "daemon-core (native)" (row 0, default) plus every foreign catalog entry with an installed marker
// ("[installed]" / "[not installed]") and a protocol suffix, consumed by BOTH the TUI "+ New agent"
// dialog and the first-run wizard's agent-type step — the same AgentRepository rows the GUI
// component renders, so the two front ends cannot drift. refresh() re-reads the catalog AND kicks a
// fresh node-side discovery scan (AgentDiscover), mirroring the GUI picker's refresh().
//
// Selection semantics match the GUI: an uninstalled foreign agent stays visible but is NOT a
// valid selection — selectedAgent() returns empty (native) for it and selectionValid() is false.
class AgentTypeView : public Tui::ZListView {
    Q_OBJECT

public:
    AgentTypeView(daemonapp::daemon::AgentRepository* repo, Tui::ZWidget* parent);

    // Re-read the catalog rows + kick refreshCatalog()+discover() for fresh installed badges.
    void refresh();

    // The selected engine: "" = daemon-core (native); otherwise the ACP catalog agent name.
    [[nodiscard]] QString selectedAgent() const;
    // Whether the selected foreign agent is installed (true for the native row).
    [[nodiscard]] bool selectedInstalled() const;
    // Whether the current row is committable (native, or an INSTALLED foreign agent).
    [[nodiscard]] bool selectionValid() const { return selectedInstalled(); }

signals:
    // The row selection changed (consumers re-gate their commit affordance).
    void selectionChanged();

private:
    void rebuild();

    daemonapp::daemon::AgentRepository* m_repo = nullptr;
    QStringList m_agents;         // catalog names backing rows 1..
    QList<bool> m_agentInstalled; // parallel to m_agents
};
