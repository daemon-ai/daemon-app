// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "dialogs/add_account_flow.h"

#include "accounts/iaccounts_service.h"
#include "palette_dialog.h"
#include "tui_dialogs.h"

#include <QRect>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVector>
#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWindow.h>

namespace {

// The localized method label the GUI's Authentication dropdown shows.
QString methodLabel(const QString& kind) {
    return kind == QLatin1String("oauth") ? AddAccountFlow::tr("Sign in (OAuth)")
                                          : AddAccountFlow::tr("API key");
}

// A minimal message dialog (label + OK), used to surface an async OAuth
// failure reason: the seam adds no error row for a failed kick, so it would
// otherwise be silent in the TUI. DeleteOnClose; Esc / OK dismiss.
void showMessage(const QString& title, const QString& message, Tui::ZWidget* host) {
    auto* dialog = new Tui::ZDialog(host);
    dialog->setOptions(Tui::ZWindow::DeleteOnClose);
    dialog->setWindowTitle(title);
    dialog->setContentsMargins({2, 1, 2, 1});

    auto* layout = new Tui::ZVBoxLayout();
    dialog->setLayout(layout);
    layout->addWidget(new Tui::ZLabel(message, dialog));
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* ok = new Tui::ZButton(AddAccountFlow::tr("OK"), dialog);
    ok->setDefault(true);
    buttons->addWidget(ok);
    QObject::connect(ok, &Tui::ZButton::clicked, dialog, &Tui::ZDialog::reject);

    ok->setFocus();
    dialog->setGeometry(QRect(0, 0, qMax(40, static_cast<int>(message.size()) + 8), 7));
}

} // namespace

AddAccountFlow::AddAccountFlow(accounts::IAccountsService* service, Tui::ZWidget* host)
    : QObject(host), m_service(service), m_host(host) {
    if (m_service == nullptr) {
        return;
    }
    connect(m_service, &accounts::IAccountsService::oauthFailed, this,
            [this](const QString& provider, const QString& reason) {
                Q_UNUSED(provider)
                showMessage(tr("Sign-in failed"), reason, m_host);
            });
}

void AddAccountFlow::open() {
    if (m_service == nullptr) {
        return;
    }
    if (m_providerPick == nullptr) {
        m_providerPick = new PaletteDialog(tr("Add account — pick a provider"), m_host);
        connect(m_providerPick, &PaletteDialog::activated, this,
                [this](const QString& id) { pickMethod(providerById(id)); });
        connect(m_providerPick, &PaletteDialog::canceled, this, &AddAccountFlow::finished);
    }
    const QVariantList providers = m_service->availableProviders();
    QVector<PaletteDialog::Item> items;
    items.reserve(providers.size());
    for (const QVariant& v : providers) {
        const QVariantMap p = v.toMap();
        QStringList kindLabels;
        for (const QString& kind : p.value(QStringLiteral("kinds")).toStringList()) {
            kindLabels << methodLabel(kind);
        }
        items.push_back({p.value(QStringLiteral("id")).toString(),
                         p.value(QStringLiteral("name")).toString(),
                         kindLabels.join(QStringLiteral(" · "))});
    }
    m_providerPick->setItems(items);
    m_providerPick->openCentered();
}

QVariantMap AddAccountFlow::providerById(const QString& id) const {
    for (const QVariant& v : m_service->availableProviders()) {
        const QVariantMap p = v.toMap();
        if (p.value(QStringLiteral("id")).toString() == id) {
            return p;
        }
    }
    return {};
}

void AddAccountFlow::pickMethod(const QVariantMap& provider) {
    // Mirrors the GUI's _syncMethod(): a provider without advertised kinds
    // defaults to an API-key credential.
    QStringList kinds = provider.value(QStringLiteral("kinds")).toStringList();
    if (kinds.isEmpty()) {
        kinds << QStringLiteral("apikey");
    }
    if (kinds.size() == 1) {
        dispatch(provider, kinds.first());
        return;
    }
    if (m_methodPick == nullptr) {
        m_methodPick = new PaletteDialog(tr("Authentication"), m_host);
        connect(m_methodPick, &PaletteDialog::canceled, this, &AddAccountFlow::finished);
    }
    // Rewire each open to capture the current provider (the model-download
    // pattern: cheap, avoids stale lambdas).
    disconnect(m_methodPick, &PaletteDialog::activated, this, nullptr);
    connect(m_methodPick, &PaletteDialog::activated, this,
            [this, provider](const QString& method) { dispatch(provider, method); });

    QVector<PaletteDialog::Item> items;
    items.reserve(kinds.size());
    for (const QString& kind : kinds) {
        items.push_back({kind, methodLabel(kind), QString()});
    }
    m_methodPick->setItems(items);
    m_methodPick->openCentered();
}

void AddAccountFlow::dispatch(const QVariantMap& provider, const QString& method) {
    if (method == QLatin1String("oauth")) {
        // The GUI's Connect: kick the handshake and let the shared accounts
        // model surface it — the pending row ("Waiting for browser…") flips to
        // connected on oauthCompleted, the same states the GUI badges render.
        m_service->beginOAuth(provider.value(QStringLiteral("id")).toString());
        emit changed();
        emit finished();
        return;
    }
    promptApiKey(provider);
}

void AddAccountFlow::promptApiKey(const QVariantMap& provider) {
    const QString name = provider.value(QStringLiteral("name")).toString();
    auto* dialog =
        new TextPromptDialog(tr("%1 — API key").arg(name), QString(), /*masked=*/true, m_host);
    connect(dialog, &TextPromptDialog::submitted, this, [this, provider](const QString& key) {
        if (provider.value(QStringLiteral("needsBaseUrl")).toBool()) {
            promptBaseUrl(provider, key);
            return;
        }
        m_service->addApiKey(provider.value(QStringLiteral("id")).toString(), QString(), key);
        emit changed();
        emit finished();
    });
    connect(dialog, &TextPromptDialog::canceled, this, &AddAccountFlow::finished);
}

void AddAccountFlow::promptBaseUrl(const QVariantMap& provider, const QString& key) {
    auto* dialog = new TextPromptDialog(tr("Base URL (e.g. https://…)"), QString(),
                                        /*masked=*/false, m_host);
    connect(dialog, &TextPromptDialog::submitted, this,
            [this, provider, key](const QString& baseUrl) {
                m_service->addApiKey(provider.value(QStringLiteral("id")).toString(), QString(),
                                     key, baseUrl);
                emit changed();
                emit finished();
            });
    connect(dialog, &TextPromptDialog::canceled, this, &AddAccountFlow::finished);
}
