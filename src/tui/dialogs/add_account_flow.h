// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

// The TUI Add-account wizard ('a' on the Accounts page): the terminal analog of
// the GUI AddAccountWizard.qml over the SAME IAccountsService seam. Steps:
// provider pick (palette over availableProviders()) -> auth-method pick when
// the provider advertises more than one kind (the GUI's Authentication
// dropdown) -> masked API-key entry (+ base URL for needsBaseUrl providers)
// committing through addApiKey, or an OAuth kick (beginOAuth) whose pending ->
// connected resolution lands on the shared accounts model exactly like the GUI
// status badges; an async oauthFailed reason is surfaced in a message dialog.
// Lives in its own dialogs/ TU (the first_run_dialog precedent) so the
// accounts workstream owns one focused file.

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <Tui/ZWidget.h>

class PaletteDialog;
namespace accounts {
class IAccountsService;
}

class AddAccountFlow : public QObject {
    Q_OBJECT

public:
    AddAccountFlow(accounts::IAccountsService* service, Tui::ZWidget* host);

    // Start (or restart) the flow at the provider pick. Esc at any step aborts
    // without touching the seam (the GUI wizard's Cancel / CloseOnEscape).
    void open();

signals:
    // A seam mutation was driven (key stored / OAuth handshake kicked); the
    // host re-renders the Accounts page.
    void changed();
    // The flow left its dialogs (committed, kicked or canceled); the host can
    // restore focus to the page.
    void finished();

private:
    // Step 2: pick the auth method when the provider advertises several kinds;
    // single-kind providers skip straight to their only method.
    void pickMethod(const QVariantMap& provider);
    void dispatch(const QVariantMap& provider, const QString& method);
    // Step 3 (API key): masked key entry, then a base-URL prompt for
    // needsBaseUrl providers. The label stays empty: the seam defaults it to
    // the provider name, exactly like leaving the GUI's optional Label blank.
    void promptApiKey(const QVariantMap& provider);
    void promptBaseUrl(const QVariantMap& provider, const QString& key);

    [[nodiscard]] QVariantMap providerById(const QString& id) const;

    accounts::IAccountsService* m_service = nullptr;
    Tui::ZWidget* m_host = nullptr;
    PaletteDialog* m_providerPick = nullptr;
    PaletteDialog* m_methodPick = nullptr;
};
