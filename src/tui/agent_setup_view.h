// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <Tui/ZButton.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>
#include <Tui/ZWidget.h>

namespace models {
class IProviderCatalog;
}
namespace setup {
class AgentSetupModel;
}

// The TUI projection of the shared setup pipeline's BACKEND + INFERENCE steps (Phase D), rendered
// over the same setup::AgentSetupModel the GUI AgentSetupForm binds — the TUI twin of
// ForeignBackendPicker + AgentInferencePicker. The engine step is the existing AgentTypeView; the
// hosting surface composes the two, so a native profile, a foreign AgentNative profile, and a
// foreign NodeProvider profile all flow through one model with no TUI-only domain logic.
//
//   Backend (foreign only, when needsBackendChoice):
//     (o) Agent's own backend      -> chooseBackend(AgentNative) [+ optional model hint]
//     ( ) Node provider (gateway)  -> chooseBackend(NodeProvider) [inline Enable when gateway off]
//   Inference (Core OR NodeProvider, when needsInference): the node's ProviderCatalog provider
//     list -> API key (key-requiring vendors) -> model list, plus the synthetic custom-endpoint
//     (base URL + typed model) — every change reported to AgentSetup.setInference /
//     setInferenceSelection, and the key gate's blocking reason rendered from keyGateMessage.
//
// The host gates its commit on AgentSetup.commitReady (+ inferenceComplete()/keyGatePassed here for
// the wizard's inference-step Finish). Section visibility is host-controlled (the wizard shows the
// backend in its agent-type phase and the inference in its inference phase; the create/edit dialogs
// show whichever the model says is needed) via setBackendAllowed()/setInferenceAllowed().
class AgentSetupView : public Tui::ZWidget {
    Q_OBJECT

public:
    AgentSetupView(setup::AgentSetupModel* setup, models::IProviderCatalog* catalog,
                   Tui::ZWidget* parent);

    // Host section gates (combined with the model's needsBackendChoice / needsInference): the
    // wizard hides the inference in the agent-type phase and the backend in the inference phase.
    void setBackendAllowed(bool allowed);
    void setInferenceAllowed(bool allowed);

    // Whether the inference selection is complete (provider + concrete model, key where required;
    // or a custom endpoint's base URL + typed model) — the wizard's Finish gate half, alongside
    // AgentSetup.keyGatePassed. Trivially true when the model does not need inference.
    [[nodiscard]] bool inferenceComplete() const;
    // The selected ProviderCatalog descriptor id (for the host's provider-label name seed).
    [[nodiscard]] QString selectedProviderId() const { return m_providerId; }
    // Reseed the provider list + re-sync from the model (call when the hosting surface opens).
    void refresh();

signals:
    // Any selection changed (the host re-gates Finish / re-seeds the agent name).
    void selectionChanged();
    // The user picked the local "Discover More Models" row; the host opens the download flow.
    void modelDiscoverRequested();

private:
    void rebuildProviderList();
    void selectProvider(int row);
    void rebuildModelList();
    void selectModel(int row);
    void syncFromModel(); // reflect AgentSetup state (backend selection, visibility)
    void refreshInferenceControls();
    void pushInference(); // AgentSetup.setInference(provider, model, key, base)
    [[nodiscard]] QVariantMap currentProviderDescriptor() const;
    [[nodiscard]] bool customSelected() const;

    setup::AgentSetupModel* m_setup = nullptr;
    models::IProviderCatalog* m_catalog = nullptr;

    // Backend section.
    Tui::ZLabel* m_backendLabel = nullptr;
    Tui::ZButton* m_agentNativeBtn = nullptr;
    Tui::ZButton* m_nodeProviderBtn = nullptr;
    Tui::ZLabel* m_gatewayLabel = nullptr;
    Tui::ZButton* m_gatewayEnableBtn = nullptr;
    Tui::ZLabel* m_modelHintLabel = nullptr;
    Tui::ZInputBox* m_modelHint = nullptr;

    // Inference section.
    Tui::ZLabel* m_providerLabel = nullptr;
    Tui::ZListView* m_providerList = nullptr;
    Tui::ZInputBox* m_key = nullptr;
    Tui::ZButton* m_listModelsBtn = nullptr;
    Tui::ZLabel* m_keyGateMsg = nullptr;
    Tui::ZLabel* m_modelLabel = nullptr;
    Tui::ZListView* m_modelList = nullptr;
    Tui::ZLabel* m_baseUrlLabel = nullptr;
    Tui::ZInputBox* m_baseUrl = nullptr;
    Tui::ZLabel* m_customModelLabel = nullptr;
    Tui::ZInputBox* m_customModel = nullptr;
    // Persist the one-off custom endpoint as a named, reusable custom provider (node-backed): the
    // TUI twin of the GUI "Save as a reusable provider" button. It then appears in the provider
    // list like any other provider.
    Tui::ZButton* m_saveCustomBtn = nullptr;

    QString m_providerId;
    QString m_selectedModel;
    QVariantList m_providerRows;
    QVariantList m_modelRows;
    bool m_backendAllowed = true;
    bool m_inferenceAllowed = true;
    bool m_syncing = false; // guard: model-driven updates must not re-drive the model
};
