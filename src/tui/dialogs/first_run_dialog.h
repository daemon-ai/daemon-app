// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "connection/iconnection_service.h"
#include "firstrun/first_run_model.h"
#include "settings/isettings_store.h"

#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>

namespace models {
class IProviderCatalog;
}

// The TUI first-run gate: a lighter "Setup Required" modal mirroring the GUI's
// FirstRunGate. A target field + Connect drives the shared connection seam; the
// FirstRunModel advances connect -> connecting -> inference, and Finish completes
// setup. Reuses the same shared FirstRunModel the GUI binds. Lives in its own
// dialogs/ TU (split out of tui_dialogs.cpp) so the first-run workstream owns
// one focused file; the small generic dialogs stay in tui_dialogs.{h,cpp}.
class FirstRunDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    FirstRunDialog(firstrun::FirstRunModel* model, connection::IConnectionService* connection,
                   settings::ISettingsStore* settings, models::IProviderCatalog* providerCatalog,
                   const QString& defaultTarget, Tui::ZWidget* parent);

signals:
    // The user picked the local "Discover More Models" row; the host opens the download flow.
    void modelDiscoverRequested();

private:
    void syncToPhase();
    // Apply the selected transport mode to the editable fields: swap the target
    // placeholder/seed and show the token field only for "remote" (parity with the
    // GUI ConnectionPicker's mode cards).
    void applyMode(const QString& mode);
    // The inference step's commit: persist a working profile for the chosen provider + model
    // (ProviderSelector + base URL) + profile-scoped key + make default, then finish.
    void commitInference();
    // Node-driven provider->model wiring (inference phase).
    void rebuildProviderList();
    void selectProvider(int row);
    void rebuildModelList();
    void selectModel(int row);
    void refreshInferenceControls();
    [[nodiscard]] QVariantMap currentProviderDescriptor() const;

    firstrun::FirstRunModel* m_model = nullptr;
    connection::IConnectionService* m_connection = nullptr;
    settings::ISettingsStore* m_settings = nullptr;
    models::IProviderCatalog* m_providerCatalog = nullptr;
    Tui::ZLabel* m_status = nullptr;
    Tui::ZInputBox* m_target = nullptr;
    Tui::ZInputBox* m_token = nullptr;
    Tui::ZInputBox* m_key = nullptr;      // provider API key (inference phase)
    Tui::ZInputBox* m_username = nullptr; // SASL username (auth phase)
    Tui::ZInputBox* m_password = nullptr; // SASL password (auth phase; masked)
    Tui::ZLabel* m_providerLabel = nullptr;
    Tui::ZListView* m_providerList = nullptr; // provider picker (inference phase)
    Tui::ZLabel* m_modelLabel = nullptr;
    Tui::ZListView* m_modelList = nullptr;   // per-provider model picker (inference phase)
    Tui::ZButton* m_listModelsBtn = nullptr; // re-list a key-requiring provider's models
    Tui::ZButton* m_localBtn = nullptr;
    Tui::ZButton* m_remoteBtn = nullptr;
    Tui::ZButton* m_managedBtn = nullptr; // local: App-managed (spawn) vs Attach (existing socket)
    Tui::ZButton* m_testBtn = nullptr;
    Tui::ZLabel* m_testResult = nullptr;
    Tui::ZButton* m_primary = nullptr; // Connect / Finish
    QString m_mode = QStringLiteral("local");
    QString m_providerId;        // selected ProviderCatalog descriptor id
    QString m_selectedModel;     // selected model id (selection-only)
    QVariantList m_providerRows; // cached providers() rows for row->id mapping
    QVariantList m_modelRows;    // cached offeredModels() rows for row->id mapping
};
