// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "connection/iconnection_service.h"
#include "firstrun/first_run_model.h"
#include "settings/isettings_store.h"

#include <QString>
#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>

namespace models {
class IProviderCatalog;
}
namespace daemonapp::daemon {
class AgentRepository;
}
class AgentTypeView;
class AgentSetupView;

// The TUI first-run gate: a lighter "Setup Required" modal mirroring the GUI's FirstRunGate. A
// target field + Connect drives the shared connection seam; the FirstRunModel advances
// connect -> connecting -> [agenttype ->] inference, and Finish completes setup. Reuses the same
// shared FirstRunModel the GUI binds.
//
// Phase D/G: the hand-rolled provider/model/key lists are gone — the inference (and the foreign
// backend) are rendered by the shared AgentSetupView over the wizard's AgentSetupModel
// (model->agentSetup()), the TUI twin of AgentSetupForm's inner steps. The agent-type phase shows
// the engine picker (AgentTypeView) + the backend rows (AgentSetupView, backend section) folded in;
// the inference phase shows the inference section. Continue advances only when the model needs
// inference; a foreign AgentNative agent finishes from the agent-type phase. Finish commits the
// shared-model selection (FirstRunModel::commitSetup), foreign-aware.
class FirstRunDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    FirstRunDialog(firstrun::FirstRunModel* model, connection::IConnectionService* connection,
                   settings::ISettingsStore* settings, models::IProviderCatalog* providerCatalog,
                   daemonapp::daemon::AgentRepository* agents, const QString& defaultTarget,
                   Tui::ZWidget* parent);

signals:
    // The user picked the local "Discover More Models" row; the host opens the download flow.
    void modelDiscoverRequested();

private:
    void syncToPhase();
    // Apply the selected transport mode to the editable fields (target placeholder + token
    // visibility), parity with the GUI ConnectionPicker's mode cards.
    void applyMode(const QString& mode);
    // Push the engine picker's current selection into the shared model so the backend rows +
    // needsInference reflect it (the TUI AgentTypeView is selection-only; it does not drive the
    // model itself).
    void applyEngineFromPicker();
    // Prefill the agent name: from the chosen provider's catalog label (native inference) or the
    // selected foreign agent (foreign), lowercased, until the user edits the field.
    void seedAgentName();
    // The name the Finish commit uses (a foreign profile is named in the agent-type pane, a native
    // one on the inference step — both write the single m_agentName field, so this just trims it).
    [[nodiscard]] QString finishName() const;
    // Whether the shared model currently needs the inference step (Core or NodeProvider).
    [[nodiscard]] bool needsInference() const;

    firstrun::FirstRunModel* m_model = nullptr;
    connection::IConnectionService* m_connection = nullptr;
    settings::ISettingsStore* m_settings = nullptr;
    models::IProviderCatalog* m_providerCatalog = nullptr;
    Tui::ZLabel* m_status = nullptr;
    Tui::ZInputBox* m_target = nullptr;
    Tui::ZInputBox* m_token = nullptr;
    Tui::ZInputBox* m_username = nullptr;    // SASL username (auth phase)
    Tui::ZInputBox* m_password = nullptr;    // SASL password (auth phase; masked)
    Tui::ZLabel* m_nameLabel = nullptr;      // "Agent name"
    Tui::ZInputBox* m_agentName = nullptr;   // agent name = the profile id the node keys it by
    Tui::ZLabel* m_agentTypeLabel = nullptr; // "Agent type" (agenttype phase, CON-16)
    AgentTypeView* m_agentType = nullptr;    // the shared native+ACP picker projection
    AgentSetupView* m_setupView = nullptr;   // shared backend + inference steps over AgentSetup
    Tui::ZButton* m_localBtn = nullptr;
    Tui::ZButton* m_remoteBtn = nullptr;
    Tui::ZButton* m_managedBtn = nullptr; // local: App-managed (spawn) vs Attach (existing socket)
    Tui::ZButton* m_testBtn = nullptr;
    Tui::ZLabel* m_testResult = nullptr;
    Tui::ZButton* m_primary = nullptr; // Connect / Continue / Finish
    QString m_mode = QStringLiteral("local");
    // Once the user edits the agent name it is theirs; selection changes stop re-seeding (the
    // guard suppresses the programmatic seed's own textChanged).
    bool m_agentNameEdited = false;
    bool m_seedingName = false;
};
