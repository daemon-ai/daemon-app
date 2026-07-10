// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

// [waveB:app-v31] The TUI projection of the GUI AuthFlowSheet (interactive auth: begin ->
// challenge/response state machine -> complete), bound to the SAME shared auth::AuthFlowController
// — full parity with the GUI through the shared view-model. It renders every AuthChallenge kind:
//   - Redirect: the authorization URL in a selectable input for copying; completion arrives via the
//     loopback redirect sink (when it could bind) or by PASTING the redirect URL into the callback.
//   - Form: collected in-flow via one TextPromptDialog per field, then submitFields().
//   - Qr: the module matrix rendered as unicode half-blocks (QrCodeWidget over the shared
//     qr::QrMatrix — the SAME matrix the GUI Kit.QrCode paints), plus the copyable payload text;
//     the controller auto-polls until completion. If the payload cannot encode (too long), the
//     widget hides and the copyable payload text stands in (stated degradation).
//   - Message: the informational text; the controller auto-polls until completion.
//
// AuthFlowLauncher is the flow driver (the AddAccountFlow precedent): family pick (palette over
// AuthFlow.providers()) -> one TextPromptDialog per BEGIN schema param -> controller.start() -> the
// AuthFlowDialog panel. Lives in its own dialogs/ TU so the wizard-auth workstream owns one
// focused file.

#include <QObject>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZEvent.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZWidget.h>

class PaletteDialog;
namespace auth {
class AuthFlowController;
}

// [integrations wire v38] The TUI QR surface: paints a qr::QrMatrix as unicode half-blocks (two
// modules per character cell) with a scanner-safe black-on-white palette — the terminal counterpart
// of the GUI Kit.QrCode, rendering from the SAME shared matrix. An un-encodable payload leaves it
// empty (zero-sized) so the dialog falls back to the copyable payload text.
class QrCodeWidget : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit QrCodeWidget(Tui::ZWidget* parent = nullptr);

    // Encode `payload` and cache the half-block rows; resizes to the rendered grid. An empty result
    // (invalid payload) reports a zero size hint so the dialog can hide it.
    void setPayload(const QString& payload);
    [[nodiscard]] bool hasCode() const { return !m_rows.isEmpty(); }

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;

private:
    QStringList m_rows;
};

// The challenge panel: a phase/kind-driven status line, a copyable value box (the Redirect URL or
// the Qr payload), the Redirect manual-callback paste box, and Cancel/Close. Opens on start();
// closes itself on success. Form fields are collected via TextPromptDialog prompts.
class AuthFlowDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    AuthFlowDialog(auth::AuthFlowController* flow, Tui::ZWidget* parent);

private:
    void syncToPhase();
    // Collect a Form challenge's fields one prompt at a time (reusing TextPromptDialog), then
    // submitFields() the assembled map.
    void promptFormFields(const QVariantList& fields, int index, const QVariantMap& collected);

    auth::AuthFlowController* m_flow = nullptr;
    Tui::ZWidget* m_host = nullptr; // parent for the in-flow field prompts
    Tui::ZLabel* m_status = nullptr;
    Tui::ZLabel* m_valueLabel = nullptr;
    Tui::ZInputBox* m_value = nullptr; // selectable/copyable Redirect URL or Qr payload
    QrCodeWidget* m_qr = nullptr;      // half-block QR render (Qr challenge)
    Tui::ZLabel* m_pasteLabel = nullptr;
    Tui::ZInputBox* m_callback = nullptr; // manual redirect paste (the no-loopback path)
    Tui::ZButton* m_complete = nullptr;
    Tui::ZButton* m_cancel = nullptr;
    bool m_formActive = false; // a Form field-prompt chain is in progress (debounces re-entry)
};

// Drives family pick -> param prompts -> controller.start(), then opens the AuthFlowDialog.
class AuthFlowLauncher : public QObject {
    Q_OBJECT

public:
    AuthFlowLauncher(auth::AuthFlowController* flow, Tui::ZWidget* host);

    // Start at the family pick (or straight at the single family when only one is offered).
    void open();
    // [waveB:app-v30] CON-15: open pre-narrowed to one family (a provider row's node-advertised
    // sign_in.family) — skips the family pick, goes straight to that family's schema params. Falls
    // back to the family pick when the family is not among the node's offered providers.
    void openForFamily(const QString& family);

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
