// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

// The TUI projection of the GUI AuthFlowSheet (interactive auth: begin -> browser hand-off ->
// await-redirect -> complete), bound to the SAME shared auth::AuthFlowController. The reduced
// terminal form (stated degradation): no embedded browser hand-off — the authorization URL is
// shown in a selectable input for copying, and completion arrives either through the loopback
// redirect sink (when it could bind) or by PASTING the redirect URL into the callback box.
//
// AuthFlowLauncher is the flow driver (the AddAccountFlow precedent): family pick (palette over
// AuthFlow.providers()) -> one TextPromptDialog per schema param -> controller.start() -> the
// AuthFlowDialog panel. Lives in its own dialogs/ TU so the wizard-auth workstream owns one
// focused file.

#include <QObject>
#include <QString>
#include <QVariantList>
#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>

class PaletteDialog;
namespace auth {
class AuthFlowController;
}

// The await/complete panel: status line (phase-driven), the copyable authorization URL, the
// manual-callback paste box, and Cancel/Close. Opens on start(); closes itself on success.
class AuthFlowDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    AuthFlowDialog(auth::AuthFlowController* flow, Tui::ZWidget* parent);

private:
    void syncToPhase();

    auth::AuthFlowController* m_flow = nullptr;
    Tui::ZLabel* m_status = nullptr;
    Tui::ZLabel* m_urlLabel = nullptr;
    Tui::ZInputBox* m_url = nullptr; // selectable/copyable authorization URL
    Tui::ZLabel* m_pasteLabel = nullptr;
    Tui::ZInputBox* m_callback = nullptr; // manual redirect paste (the no-loopback path)
    Tui::ZButton* m_complete = nullptr;
    Tui::ZButton* m_cancel = nullptr;
};

// Drives family pick -> param prompts -> controller.start(), then opens the AuthFlowDialog.
class AuthFlowLauncher : public QObject {
    Q_OBJECT

public:
    AuthFlowLauncher(auth::AuthFlowController* flow, Tui::ZWidget* host);

    // Start at the family pick (or straight at the single family when only one is offered).
    void open();

signals:
    // The flow's dialogs went away (completed, failed terminal, or canceled).
    void finished();

private:
    void promptParams(const QVariantMap& provider);
    void promptNextParam(const QString& family, const QVariantList& fields, int index,
                         const QVariantMap& collected);
    void launch(const QString& family, const QVariantMap& params);

    auth::AuthFlowController* m_flow = nullptr;
    Tui::ZWidget* m_host = nullptr;
    PaletteDialog* m_familyPick = nullptr;
};
