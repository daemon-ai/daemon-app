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
class AcpRepository;
}
namespace profiles {
class IProfileStore;
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

// The TUI "+ New agent" stub (foreign engines; wire v23): a Name field + a minimal ENGINE list —
// "daemon-core" (the native engine, default) plus every ACP catalog name (no badges) — committing
// through the SAME IProfileStore create path the GUI dialog uses: native -> createProfile(name)
// (provider/model then configured via the Profile page), foreign -> createAcpProfile(name, agent)
// carrying engine=Acp{agent} (a catalog NAME; recipes stay node-side), then setDefault.
class NewAgentDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    NewAgentDialog(profiles::IProfileStore* profiles, daemonapp::daemon::AcpRepository* acp,
                   Tui::ZWidget* parent);

signals:
    // A ProfileCreate was issued under `profileId` (the row appears when the repo re-lists).
    void created(const QString& profileId);

private:
    void rebuildEngines();
    void commit();

    profiles::IProfileStore* m_profiles = nullptr;
    daemonapp::daemon::AcpRepository* m_acp = nullptr;
    Tui::ZInputBox* m_name = nullptr;
    Tui::ZListView* m_engines = nullptr; // row 0 = daemon-core; rows 1.. = m_agents[row-1]
    QStringList m_agents;                // ACP catalog names backing rows 1..
};
