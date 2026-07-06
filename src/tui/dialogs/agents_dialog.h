// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QList>
#include <QString>
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>

namespace daemonapp::daemon {
class AgentRepository;
}

// [wave2:app-engines] TUI foreign-agent management (C1 / ENG-2/ENG-3), the parity counterpart of
// the GUI Settings -> Agents section. Bound to the SAME AgentRepository the GUI binds, so the two
// front ends cannot drift. Lists the catalog (name + protocol + installed + source markers), and
// offers Register (opens RegisterAgentDialog -> AgentRegister), Remove on a manual row (AgentRemove
// with a fail-closed consequence confirm), and Re-scan (AgentDiscover). Register/remove outcomes —
// including the node's honest failure cause (binary not found / handshake failed) — show in the
// status line (C6).
class AgentsDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    AgentsDialog(daemonapp::daemon::AgentRepository* agents, Tui::ZWidget* parent);

private:
    void rebuild();
    void openRegisterForm();
    void removeSelected();

    daemonapp::daemon::AgentRepository* m_agents = nullptr;
    Tui::ZListView* m_list = nullptr;
    Tui::ZLabel* m_status = nullptr;
    QStringList m_names;  // catalog names backing the rows
    QList<bool> m_manual; // parallel to m_names: is this a manual registration?
};

// The register form (Name / Protocol / stdio program+args+env OR tcp:// endpoint). Emits the
// AgentRegister via the repository; the node re-probes and never trusts a caller-supplied installed
// flag (ENG-3). A compact projection of the GUI RegisterAgentDialog.
class RegisterAgentDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    RegisterAgentDialog(daemonapp::daemon::AgentRepository* agents, Tui::ZWidget* parent);

private:
    void commit();

    daemonapp::daemon::AgentRepository* m_agents = nullptr;
    Tui::ZInputBox* m_name = nullptr;
    Tui::ZListView* m_protocol = nullptr;
    Tui::ZInputBox* m_program = nullptr;
    Tui::ZInputBox* m_args = nullptr;
    Tui::ZInputBox* m_env = nullptr; // TUI degradation: single-line "KEY=VAL,KEY=VAL"
    Tui::ZInputBox* m_endpoint = nullptr;
};
