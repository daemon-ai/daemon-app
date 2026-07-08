// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

// The small generic TUI dialogs (quit / text prompt / confirm / new agent).
// The first-run gate lives in its own TU: dialogs/first_run_dialog.h.

#include <QStringList>
#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>

namespace daemonapp::daemon {
class AgentRepository;
}
namespace models {
class IProviderCatalog;
}
namespace setup {
class AgentSetupModel;
}

class AgentTypeView;
class AgentSetupView;

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

// The TUI "+ New agent" dialog (Phase D): the full ONE setup pipeline over the shared
// AgentSetupModel — a Name field + the SHARED AgentTypeView engine list (native + the foreign
// catalog with verification badges), the SHARED AgentSetupView (foreign backend choice + the
// provider/model/key inference sub-form), committing through AgentSetup.commit (never a bespoke
// create path). Native, foreign AgentNative, and foreign NodeProvider all flow through one form;
// Create is gated on AgentSetup.commitReady, and an uninstalled foreign row is not selectable
// (parity with the GUI). The TUI twin of the GUI AgentSetupForm dialog.
class NewAgentDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    NewAgentDialog(setup::AgentSetupModel* setup, daemonapp::daemon::AgentRepository* agents,
                   models::IProviderCatalog* providerCatalog, Tui::ZWidget* parent);

signals:
    // A ProfileCreate resolved under `profileId` (the row appears when the repo re-lists).
    void created(const QString& profileId);
    // The user picked the local "Discover More Models" row; the host opens the download flow.
    void modelDiscoverRequested();

private:
    void applyEngineFromPicker();
    void commit();
    void refreshCommit();

    setup::AgentSetupModel* m_setup = nullptr;
    Tui::ZInputBox* m_name = nullptr;
    Tui::ZLabel* m_error = nullptr;
    AgentTypeView* m_engines = nullptr;    // the shared native + foreign engine picker projection
    AgentSetupView* m_setupView = nullptr; // shared backend + inference steps over AgentSetup
    Tui::ZButton* m_create = nullptr;
    // The shared AgentSetupModel's committed()/failed() reach every bound surface; only react to
    // the outcome of a commit THIS dialog started.
    bool m_committing = false;
};
