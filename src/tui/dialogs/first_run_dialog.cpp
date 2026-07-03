// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "first_run_dialog.h"

#include "models/iprovider_catalog.h"

#include <QAbstractItemModel>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZListView.h>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWindow.h>

FirstRunDialog::FirstRunDialog(firstrun::FirstRunModel* model,
                               connection::IConnectionService* connection,
                               settings::ISettingsStore* settings,
                               models::IProviderCatalog* providerCatalog,
                               const QString& defaultTarget, Tui::ZWidget* parent)
    : Tui::ZDialog(parent), m_model(model), m_connection(connection), m_settings(settings),
      m_providerCatalog(providerCatalog) {
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

    // Local sub-choice: App-managed (the app spawns + manages a node) vs Attach (connect to a
    // socket you run yourself). Toggles the persisted managedLocalDaemon preference; hidden for
    // remote and outside the connect phase.
    m_managedBtn = new Tui::ZButton(QString(), this);
    layout->addWidget(m_managedBtn);
    connect(m_managedBtn, &Tui::ZButton::clicked, this, [this] {
        if (m_settings != nullptr) {
            m_settings->setManagedLocalDaemon(!m_settings->managedLocalDaemon());
        }
        applyMode(m_mode); // refresh label + target visibility
    });

    m_target = new Tui::ZInputBox(defaultTarget, this);
    layout->addWidget(m_target);

    // Auth token: only relevant for a remote node; hidden in local mode.
    m_token = new Tui::ZInputBox(QString(), this);
    m_token->setEchoMode(Tui::ZInputBox::Password);
    layout->addWidget(m_token);

    // Provider picker (inference phase): the node's ProviderCatalog, selection-only.
    m_providerLabel = new Tui::ZLabel(tr("Provider"), this);
    m_providerLabel->setVisible(false);
    layout->addWidget(m_providerLabel);
    m_providerList = new Tui::ZListView(this);
    m_providerList->setVisible(false);
    layout->addWidget(m_providerList);

    // Provider API key: shown only for a key-requiring provider (Daemon Cloud lists keyless).
    m_key = new Tui::ZInputBox(QString(), this);
    m_key->setEchoMode(Tui::ZInputBox::Password);
    m_key->setVisible(false);
    layout->addWidget(m_key);
    m_listModelsBtn = new Tui::ZButton(tr("List models"), this);
    m_listModelsBtn->setVisible(false);
    layout->addWidget(m_listModelsBtn);

    // Model picker (inference phase): the selected provider's models; local providers end with a
    // "Discover More Models" row that opens the download flow.
    m_modelLabel = new Tui::ZLabel(tr("Model"), this);
    m_modelLabel->setVisible(false);
    layout->addWidget(m_modelLabel);
    m_modelList = new Tui::ZListView(this);
    m_modelList->setVisible(false);
    layout->addWidget(m_modelList);

    // SASL login: username + masked password, shown only in the auth phase.
    m_username = new Tui::ZInputBox(QString(), this);
    m_username->setVisible(false);
    layout->addWidget(m_username);
    m_password = new Tui::ZInputBox(QString(), this);
    m_password->setEchoMode(Tui::ZInputBox::Password);
    m_password->setVisible(false);
    layout->addWidget(m_password);
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
        } else if (m_model->phase() == QStringLiteral("auth")) {
            // Submit SASL credentials; the password is read from the field only and never logged.
            m_model->submitLogin(m_username->text(), m_password->text());
            m_password->setText(QString());
        } else if (m_connection != nullptr) {
            // App-managed local uses the default user-writable managed socket (no path shown) so
            // the launcher spawns a node; remote/attach use the target field. Persist the choice
            // (parity with the GUI picker writing AppSettings.setLastConnection before connecting).
            const bool managed = m_settings != nullptr && m_settings->managedLocalDaemon();
            const QString target =
                (m_mode == QStringLiteral("local") && managed && m_settings != nullptr)
                    ? m_settings->defaultManagedSocketPath()
                    : m_target->text();
            if (m_settings != nullptr) {
                m_settings->setLastConnection(m_mode, target);
            }
            m_connection->connectTo(m_mode, target, m_token->text());
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
    // An auth error (wrong password) updates in place while the phase stays "auth".
    connect(m_model, &firstrun::FirstRunModel::errorChanged, this, &FirstRunDialog::syncToPhase);

    // Provider -> model wiring (node-driven, selection-only).
    connect(m_providerList, &Tui::ZListView::enterPressed, this, &FirstRunDialog::selectProvider);
    connect(m_modelList, &Tui::ZListView::enterPressed, this, &FirstRunDialog::selectModel);
    connect(m_listModelsBtn, &Tui::ZButton::clicked, this, [this] {
        if (m_providerCatalog != nullptr && !m_providerId.isEmpty()) {
            m_providerCatalog->refreshModels(m_providerId, QString(), m_key->text());
        }
    });
    if (m_providerCatalog != nullptr) {
        connect(m_providerCatalog, &models::IProviderCatalog::providersChanged, this,
                &FirstRunDialog::rebuildProviderList);
        connect(m_providerCatalog, &models::IProviderCatalog::offeredModelsChanged, this,
                [this](const QString& providerId) {
                    if (providerId == m_providerId) {
                        rebuildModelList();
                    }
                });
    }

    applyMode(m_mode);
    rebuildProviderList();
    syncToPhase();
    m_target->setFocus();
    setGeometry(QRect(0, 0, 66, 20));
}

QVariantMap FirstRunDialog::currentProviderDescriptor() const {
    if (m_providerCatalog == nullptr || m_providerId.isEmpty()) {
        return {};
    }
    return m_providerCatalog->descriptorFor(m_providerId);
}

void FirstRunDialog::rebuildProviderList() {
    if (m_providerCatalog == nullptr || m_providerList == nullptr) {
        return;
    }
    m_providerRows = m_providerCatalog->providers();
    QStringList names;
    for (const QVariant& v : m_providerRows) {
        names << v.toMap().value(QStringLiteral("name")).toString();
    }
    m_providerList->setItems(names);
    // Default the selection to Daemon Cloud (the new-profile default) on first populate.
    if (m_providerId.isEmpty() && !m_providerRows.isEmpty()) {
        int pick = 0;
        for (int i = 0; i < m_providerRows.size(); ++i) {
            const QVariantMap m = m_providerRows.at(i).toMap();
            if (m.value(QStringLiteral("id")).toString() == QStringLiteral("daemon_cloud") ||
                m.value(QStringLiteral("kind")).toString() == QStringLiteral("daemon_cloud")) {
                pick = i;
                break;
            }
        }
        selectProvider(pick);
    }
}

void FirstRunDialog::selectProvider(int row) {
    if (row < 0 || row >= m_providerRows.size()) {
        return;
    }
    if (m_providerList->model() != nullptr) {
        m_providerList->setCurrentIndex(m_providerList->model()->index(row, 0));
    }
    m_providerId = m_providerRows.at(row).toMap().value(QStringLiteral("id")).toString();
    m_selectedModel.clear();
    if (m_providerCatalog != nullptr) {
        // Transient key while listing (no profile yet); Daemon Cloud/local ignore it.
        m_providerCatalog->refreshModels(m_providerId, QString(), m_key->text());
    }
    rebuildModelList();
    refreshInferenceControls();
}

void FirstRunDialog::rebuildModelList() {
    if (m_providerCatalog == nullptr || m_modelList == nullptr) {
        return;
    }
    m_modelRows = m_providerCatalog->offeredModels(m_providerId);
    QStringList names;
    for (const QVariant& v : m_modelRows) {
        const QVariantMap m = v.toMap();
        const bool discover =
            m.value(QStringLiteral("kind")).toString() == QStringLiteral("discover");
        names << (discover ? QStringLiteral("+ ") : QString()) +
                     m.value(QStringLiteral("name")).toString();
    }
    m_modelList->setItems(names);
    refreshInferenceControls();
}

void FirstRunDialog::selectModel(int row) {
    if (row < 0 || row >= m_modelRows.size()) {
        return;
    }
    if (m_modelList->model() != nullptr) {
        m_modelList->setCurrentIndex(m_modelList->model()->index(row, 0));
    }
    const QVariantMap m = m_modelRows.at(row).toMap();
    if (m.value(QStringLiteral("kind")).toString() == QStringLiteral("discover")) {
        // Local "Discover More Models": hand off to the host's download flow.
        emit modelDiscoverRequested();
        return;
    }
    m_selectedModel = m.value(QStringLiteral("id")).toString();
    refreshInferenceControls();
}

void FirstRunDialog::refreshInferenceControls() {
    const QVariantMap desc = currentProviderDescriptor();
    const bool requiresKey = desc.value(QStringLiteral("requiresKey")).toBool();
    if (m_key != nullptr) {
        m_key->setVisible(m_model->phase() == QStringLiteral("inference") && requiresKey);
    }
    if (m_listModelsBtn != nullptr) {
        m_listModelsBtn->setVisible(m_model->phase() == QStringLiteral("inference") && requiresKey);
    }
    if (m_primary != nullptr && m_model->phase() == QStringLiteral("inference")) {
        const bool keyOk = !requiresKey || !m_key->text().isEmpty();
        m_primary->setEnabled(!m_providerId.isEmpty() && !m_selectedModel.isEmpty() && keyOk);
    }
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
    const bool managed = m_settings != nullptr && m_settings->managedLocalDaemon();
    if (m_managedBtn != nullptr) {
        m_managedBtn->setVisible(!remote);
        m_managedBtn->setText(managed ? tr("Local: App-managed (press to Attach instead)")
                                      : tr("Local: Attach to socket (press for App-managed)"));
    }
    if (m_target != nullptr) {
        // App-managed local hides the socket field (the app picks a user-writable socket); remote
        // and attach show it. Seed a representative value when empty (never the root-owned
        // /run/daemon.sock the managed daemon could not bind).
        m_target->setVisible(remote || !managed);
        if (m_target->text().isEmpty()) {
            m_target->setText(
                remote
                    ? QStringLiteral("https://node.example:8080")
                    : (m_settings != nullptr ? m_settings->resolvedConnectionTarget() : QString()));
        }
    }
    if (m_testResult != nullptr) {
        m_testResult->setText(QString());
    }
}

void FirstRunDialog::syncToPhase() {
    const QString p = m_model->phase();
    const bool inference = p == QStringLiteral("inference");
    // The local App-managed/Attach toggle only applies while choosing a transport (connect phase).
    if (m_managedBtn != nullptr) {
        const bool managed = m_settings != nullptr && m_settings->managedLocalDaemon();
        m_managedBtn->setVisible(p == QStringLiteral("connect") &&
                                 m_mode == QStringLiteral("local"));
        m_managedBtn->setText(managed ? tr("Local: App-managed (press to Attach instead)")
                                      : tr("Local: Attach to socket (press for App-managed)"));
    }
    // Default: the SASL login fields are hidden outside the auth phase.
    if (m_username != nullptr) {
        m_username->setVisible(p == QStringLiteral("auth"));
    }
    if (m_password != nullptr) {
        m_password->setVisible(p == QStringLiteral("auth"));
    }
    // Provider/model pickers live in the inference phase only.
    if (m_providerLabel != nullptr) {
        m_providerLabel->setVisible(inference);
    }
    if (m_providerList != nullptr) {
        m_providerList->setVisible(inference);
    }
    if (m_modelLabel != nullptr) {
        m_modelLabel->setVisible(inference);
    }
    if (m_modelList != nullptr) {
        m_modelList->setVisible(inference);
    }
    if (p == QStringLiteral("connecting")) {
        m_status->setText(tr("Connecting..."));
        m_primary->setText(tr("Connect"));
        m_primary->setEnabled(false);
        m_target->setEnabled(false);
    } else if (p == QStringLiteral("auth")) {
        // The node requires credentials: show username/password, hide the transport fields.
        m_status->setText(m_model->error().isEmpty() ? tr("Sign in to continue.")
                                                     : m_model->error());
        m_primary->setText(tr("Sign in"));
        m_primary->setEnabled(true);
        m_target->setEnabled(false);
        if (m_token != nullptr) {
            m_token->setVisible(false);
        }
        if (m_key != nullptr) {
            m_key->setVisible(false);
        }
        m_username->setFocus();
    } else if (p == QStringLiteral("inference")) {
        m_status->setText(tr("Pick a provider and a model, then Finish."));
        m_primary->setText(tr("Finish"));
        m_target->setEnabled(false);
        if (m_token != nullptr) {
            m_token->setVisible(false);
        }
        // The key field + Finish enablement track the selected provider/model.
        refreshInferenceControls();
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
    // Persist a working profile for the chosen provider + model (ProviderSelector + base URL from
    // the descriptor) + a profile-scoped key + make it default, then finish. No hardcoded provider:
    // the id/model/key come from the node-driven selection. FirstRunModel owns the persistence so
    // the GUI + TUI share one path.
    const QString key = m_key != nullptr ? m_key->text() : QString();
    m_model->applyInferenceChoice(m_providerId, m_selectedModel, key);
    if (m_key != nullptr) {
        m_key->setText(QString());
    }
}
