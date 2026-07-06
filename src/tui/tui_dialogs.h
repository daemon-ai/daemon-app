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
namespace profiles {
class IProfileStore;
}

class AgentTypeView;

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

// The TUI "+ New agent" dialog (foreign engines; wire v29): a Name field + the SHARED
// AgentTypeView engine list — "daemon-core" (the native engine, default) plus every foreign catalog
// entry with installed markers (the same AgentRepository rows the GUI AgentTypePicker renders) —
// committing through the SAME IProfileStore create path the GUI dialog uses: native ->
// createProfile(name) (provider/model then configured via the Profile page), foreign ->
// createForeignProfile(name, agent) carrying engine=Foreign{agent} (a catalog NAME; recipes stay
// node-side), then setDefault. An uninstalled foreign row is not committable (parity with the
// GUI's unselectable rows).
class NewAgentDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    NewAgentDialog(profiles::IProfileStore* profiles, daemonapp::daemon::AgentRepository* agents,
                   Tui::ZWidget* parent);

signals:
    // A ProfileCreate was issued under `profileId` (the row appears when the repo re-lists).
    void created(const QString& profileId);

private:
    void commit();

    profiles::IProfileStore* m_profiles = nullptr;
    Tui::ZInputBox* m_name = nullptr;
    AgentTypeView* m_engines = nullptr; // the shared native+ACP picker projection
};
