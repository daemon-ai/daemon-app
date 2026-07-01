// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "connection/iconnection_service.h"
#include "firstrun/first_run_model.h"
#include "settings/isettings_store.h"

#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>

namespace models {
class IProviderCatalog;
}

// A small modal "Quit daemon-app?" confirmation. ZDialog auto-centers, handles
// Esc -> reject(), and routes Enter to the default button; we add Quit/Cancel and
// surface the confirmed choice via quitConfirmed().
class QuitDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    explicit QuitDialog(Tui::ZWidget* parent);

signals:
    void quitConfirmed();
};

// A small modal text-input dialog (ZInputBox + OK/Cancel), used for renaming a
// session. Seeded with the current title; Enter / OK emits accepted(text).
class TextPromptDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    // `masked` switches the input to password echo (sudo/secret prompts).
    TextPromptDialog(const QString& title, const QString& initial, bool masked,
                     Tui::ZWidget* parent);

signals:
    void submitted(const QString& text);
    // Emitted when the user cancels (Esc / Cancel), so a host gate can abort.
    void canceled();

private:
    Tui::ZInputBox* m_input = nullptr;
};

// A minimal modal Yes/No confirmation (destructive-confirm). Esc/No reject; the
// default focus is the cancelling choice. Surfaces the affirmative via confirmed().
class ConfirmDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    ConfirmDialog(const QString& title, const QString& message, Tui::ZWidget* parent);

signals:
    void confirmed();
};

// The TUI first-run gate: a lighter "Setup Required" modal mirroring the GUI's
// FirstRunGate. A target field + Connect drives the shared connection seam; the
// FirstRunModel advances connect -> connecting -> inference, and Finish completes
// setup. Reuses the same shared FirstRunModel the GUI binds.
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
