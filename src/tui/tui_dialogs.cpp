// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "tui_dialogs.h"

#include "accounts/iaccounts_service.h"
#include "models/imodel_catalog.h"

#include <Tui/ZHBoxLayout.h>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWindow.h>

QuitDialog::QuitDialog(Tui::ZWidget* parent) : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(tr("Quit"));
    setContentsMargins({2, 1, 2, 1});

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    auto* message = new Tui::ZLabel(tr("Quit daemon-app?"), this);
    layout->addWidget(message);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();

    auto* quitButton = new Tui::ZButton(tr("Quit"), this);
    quitButton->setDefault(true); // Enter confirms
    buttons->addWidget(quitButton);

    auto* cancelButton = new Tui::ZButton(tr("Cancel"), this);
    buttons->addWidget(cancelButton);

    connect(quitButton, &Tui::ZButton::clicked, this, &QuitDialog::quitConfirmed);
    connect(cancelButton, &Tui::ZButton::clicked, this, &Tui::ZDialog::reject);

    // The user explicitly asked to quit, so focus Quit: Enter confirms, while Esc
    // (ZDialog::reject) and Tab->Cancel back out.
    quitButton->setFocus();
}

TextPromptDialog::TextPromptDialog(const QString& title, const QString& initial, bool masked,
                                   Tui::ZWidget* parent)
    : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(title);
    setContentsMargins({2, 1, 2, 1});

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    m_input = new Tui::ZInputBox(initial, this);
    if (masked) {
        m_input->setEchoMode(Tui::ZInputBox::Password);
    }
    layout->addWidget(m_input);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();

    auto* ok = new Tui::ZButton(tr("OK"), this);
    ok->setDefault(true);
    buttons->addWidget(ok);
    auto* cancel = new Tui::ZButton(tr("Cancel"), this);
    buttons->addWidget(cancel);

    connect(ok, &Tui::ZButton::clicked, this, [this] {
        emit submitted(m_input->text());
        close();
    });
    connect(cancel, &Tui::ZButton::clicked, this, &Tui::ZDialog::reject);
    connect(this, &Tui::ZDialog::rejected, this, &TextPromptDialog::canceled);

    m_input->setFocus();
    setGeometry(QRect(0, 0, qMax(40, static_cast<int>(initial.size()) + 16), 7));
}

ConfirmDialog::ConfirmDialog(const QString& title, const QString& message, Tui::ZWidget* parent)
    : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(title);
    setContentsMargins({2, 1, 2, 1});

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    auto* label = new Tui::ZLabel(message, this);
    layout->addWidget(label);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();

    auto* yes = new Tui::ZButton(tr("Yes"), this);
    buttons->addWidget(yes);
    auto* no = new Tui::ZButton(tr("Cancel"), this);
    no->setDefault(true); // safe default for a destructive action
    buttons->addWidget(no);

    connect(yes, &Tui::ZButton::clicked, this, [this] {
        emit confirmed();
        close();
    });
    connect(no, &Tui::ZButton::clicked, this, &Tui::ZDialog::reject);

    no->setFocus();
    setGeometry(QRect(0, 0, qMax(40, static_cast<int>(message.size()) + 8), 7));
}

FirstRunDialog::FirstRunDialog(firstrun::FirstRunModel* model,
                               connection::IConnectionService* connection,
                               settings::ISettingsStore* settings,
                               accounts::IAccountsService* accounts,
                               models::IModelCatalog* modelCatalog, const QString& defaultTarget,
                               Tui::ZWidget* parent)
    : Tui::ZDialog(parent), m_model(model), m_connection(connection), m_settings(settings),
      m_accounts(accounts), m_modelCatalog(modelCatalog) {
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(tr("Setup Required"));
    setContentsMargins({2, 1, 2, 1});

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    auto* intro =
        new Tui::ZLabel(tr("Connect to a daemon node to get started. Choose a transport:"), this);
    layout->addWidget(intro);

    // Transport mode "cards": the TUI analog of ConnectionPicker's cards. Embedded
    // is shown disabled (coming soon); Local (Unix socket) / Remote (network).
    auto* modes = new Tui::ZHBoxLayout();
    layout->add(modes);
    auto* embeddedBtn = new Tui::ZButton(tr("Embedded"), this);
    embeddedBtn->setEnabled(false); // in-process node: coming soon
    modes->addWidget(embeddedBtn);
    m_localBtn = new Tui::ZButton(tr("Local"), this);
    modes->addWidget(m_localBtn);
    m_remoteBtn = new Tui::ZButton(tr("Remote"), this);
    modes->addWidget(m_remoteBtn);
    layout->addSpacing(1);

    m_target = new Tui::ZInputBox(defaultTarget, this);
    layout->addWidget(m_target);

    // Auth token: only relevant for a remote node; hidden in local mode.
    m_token = new Tui::ZInputBox(QString(), this);
    m_token->setEchoMode(Tui::ZInputBox::Password);
    layout->addWidget(m_token);

    // Provider API key: only shown in the inference phase (onboarding CON-4). Masked.
    m_key = new Tui::ZInputBox(QString(), this);
    m_key->setEchoMode(Tui::ZInputBox::Password);
    m_key->setVisible(false);
    layout->addWidget(m_key);
    layout->addSpacing(1);

    m_status = new Tui::ZLabel(QString(), this);
    layout->addWidget(m_status);
    m_testResult = new Tui::ZLabel(QString(), this);
    layout->addWidget(m_testResult);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    m_testBtn = new Tui::ZButton(tr("Test"), this);
    buttons->addWidget(m_testBtn);
    buttons->addStretch();

    m_primary = new Tui::ZButton(tr("Connect"), this);
    m_primary->setDefault(true);
    buttons->addWidget(m_primary);

    auto* skip = new Tui::ZButton(tr("Skip"), this);
    buttons->addWidget(skip);

    connect(m_localBtn, &Tui::ZButton::clicked, this,
            [this] { applyMode(QStringLiteral("local")); });
    connect(m_remoteBtn, &Tui::ZButton::clicked, this,
            [this] { applyMode(QStringLiteral("remote")); });

    connect(m_testBtn, &Tui::ZButton::clicked, this, [this] {
        if (m_connection != nullptr && !m_target->text().isEmpty()) {
            m_testResult->setText(tr("Testing..."));
            m_connection->testConnection(m_mode, m_target->text(), m_token->text());
        }
    });
    if (m_connection != nullptr) {
        connect(m_connection, &connection::IConnectionService::testResult, this,
                [this](bool ok, const QString& message) {
                    m_testResult->setText((ok ? tr("OK — ") : tr("Failed — ")) + message);
                });
    }

    connect(m_primary, &Tui::ZButton::clicked, this, [this] {
        if (m_model->phase() == QStringLiteral("inference")) {
            commitInference();
        } else if (m_connection != nullptr) {
            // Persist the choice so the next boot reuses it (parity with the GUI
            // picker writing AppSettings.setLastConnection before connecting).
            if (m_settings != nullptr) {
                m_settings->setLastConnection(m_mode, m_target->text());
            }
            m_connection->connectTo(m_mode, m_target->text(), m_token->text());
        }
    });
    connect(skip, &Tui::ZButton::clicked, this, [this] { m_model->skip(); });

    // The model owns the flow; reflect each phase and close once done.
    connect(m_model, &firstrun::FirstRunModel::phaseChanged, this, [this] {
        if (m_model->phase() == QStringLiteral("done")) {
            close();
        } else {
            syncToPhase();
        }
    });
    // Readiness gates the inference Finish button; re-sync when it flips.
    connect(m_model, &firstrun::FirstRunModel::inferenceReadyChanged, this,
            &FirstRunDialog::syncToPhase);

    applyMode(m_mode);
    syncToPhase();
    m_target->setFocus();
    setGeometry(QRect(0, 0, 66, 16));
}

void FirstRunDialog::applyMode(const QString& mode) {
    m_mode = mode;
    const bool remote = mode == QStringLiteral("remote");
    // Mark the active card by (de)emphasising via the default flag, and seed the
    // target with a representative placeholder if the field is empty.
    if (m_localBtn != nullptr) {
        m_localBtn->setText(remote ? tr("Local") : QStringLiteral("[%1]").arg(tr("Local")));
    }
    if (m_remoteBtn != nullptr) {
        m_remoteBtn->setText(remote ? QStringLiteral("[%1]").arg(tr("Remote")) : tr("Remote"));
    }
    if (m_token != nullptr) {
        m_token->setVisible(remote);
    }
    if (m_target != nullptr && m_target->text().isEmpty()) {
        m_target->setText(remote ? QStringLiteral("https://node.example:8080")
                                 : QStringLiteral("/run/daemon/daemon.sock"));
    }
    if (m_testResult != nullptr) {
        m_testResult->setText(QString());
    }
}

void FirstRunDialog::syncToPhase() {
    const QString p = m_model->phase();
    if (p == QStringLiteral("connecting")) {
        m_status->setText(tr("Connecting..."));
        m_primary->setText(tr("Connect"));
        m_primary->setEnabled(false);
        m_target->setEnabled(false);
    } else if (p == QStringLiteral("inference")) {
        m_status->setText(tr("Connected. Paste a provider API key (optional), then Finish."));
        m_primary->setText(tr("Finish"));
        // CON-7: Finish is enabled once a usable model is reachable.
        m_primary->setEnabled(m_model->inferenceReady());
        m_target->setEnabled(false);
        if (m_token != nullptr) {
            m_token->setVisible(false);
        }
        if (m_key != nullptr) {
            m_key->setVisible(true);
        }
    } else { // connect
        m_status->setText(m_model->error().isEmpty() ? QString() : m_model->error());
        m_primary->setText(tr("Connect"));
        m_primary->setEnabled(true);
        m_target->setEnabled(true);
        if (m_key != nullptr) {
            m_key->setVisible(false);
        }
    }
}

void FirstRunDialog::commitInference() {
    // Store the typed provider key (if any) on the active profile, then pick the first discovered
    // model so a usable model is current; completeInference is gated on inferenceReady.
    if (m_accounts != nullptr && m_key != nullptr && !m_key->text().isEmpty()) {
        m_accounts->addApiKey(QStringLiteral("anthropic"), QString(), m_key->text(), QString());
        m_key->setText(QString());
    }
    if (m_modelCatalog != nullptr) {
        const QStringList ids = m_modelCatalog->installedIds();
        if (!ids.isEmpty()) {
            m_modelCatalog->activate(ids.first());
        }
    }
    m_model->completeInference();
}
