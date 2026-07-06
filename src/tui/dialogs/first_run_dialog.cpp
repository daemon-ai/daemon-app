// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "first_run_dialog.h"

#include "agent_type_view.h"
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
                               daemonapp::daemon::AgentRepository* agents,
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

    // Agent type (agenttype phase, CON-16): the shared native-vs-foreign picker projection.
    // FirstRunModel gates the phase (offered only when the node reflected foreign agents).
    m_agentTypeLabel = new Tui::ZLabel(tr("Agent type (Enter to pick):"), this);
    m_agentTypeLabel->setVisible(false);
    layout->addWidget(m_agentTypeLabel);
    m_agentType = new AgentTypeView(agents, this);
    m_agentType->setObjectName(QStringLiteral("firstRunAgentType"));
    m_agentType->setVisible(false);
    layout->addWidget(m_agentType);

    // Agent name (inference phase): the profile id the node keys the agent by, prefilled from
    // the chosen provider's label until the user edits it - parity with the GUI wizard's
    // agentNameField. The name mints a NAMED agent instead of leaving the node's placeholder id.
    m_nameLabel = new Tui::ZLabel(tr("Agent name"), this);
    m_nameLabel->setVisible(false);
    layout->addWidget(m_nameLabel);
    m_agentName = new Tui::ZInputBox(QString(), this);
    m_agentName->setObjectName(QStringLiteral("firstRunAgentName"));
    m_agentName->setVisible(false);
    layout->addWidget(m_agentName);

    // Provider picker (inference phase): the node's ProviderCatalog, selection-only.
    m_providerLabel = new Tui::ZLabel(tr("Provider"), this);
    m_providerLabel->setVisible(false);
    layout->addWidget(m_providerLabel);
    m_providerList = new Tui::ZListView(this);
    m_providerList->setObjectName(QStringLiteral("firstRunProviderList"));
    m_providerList->setVisible(false);
    layout->addWidget(m_providerList);

    // Provider API key: shown only for a key-requiring provider (Daemon Cloud lists keyless).
    m_key = new Tui::ZInputBox(QString(), this);
    m_key->setObjectName(QStringLiteral("firstRunKey"));
    m_key->setEchoMode(Tui::ZInputBox::Password);
    m_key->setVisible(false);
    layout->addWidget(m_key);
    m_listModelsBtn = new Tui::ZButton(tr("List models"), this);
    m_listModelsBtn->setVisible(false);
    layout->addWidget(m_listModelsBtn);
    // The shared key gate's blocking reason (FIX 4) - the TUI render of the GUI picker's
    // key-validation message line; Finish stays blocked while it shows.
    m_keyGateMsg = new Tui::ZLabel(QString(), this);
    m_keyGateMsg->setObjectName(QStringLiteral("firstRunKeyGateMsg"));
    m_keyGateMsg->setVisible(false);
    layout->addWidget(m_keyGateMsg);

    // Model picker (inference phase): the selected provider's models; local providers end with a
    // "Discover More Models" row that opens the download flow.
    m_modelLabel = new Tui::ZLabel(tr("Model"), this);
    m_modelLabel->setVisible(false);
    layout->addWidget(m_modelLabel);
    m_modelList = new Tui::ZListView(this);
    m_modelList->setObjectName(QStringLiteral("firstRunModelList"));
    m_modelList->setVisible(false);
    layout->addWidget(m_modelList);

    // A1 (CON-10): the custom-endpoint fields — base URL + typed model (no wire op can list a
    // self-hosted server's models). Shown only for the synthetic "Custom endpoint…" row.
    m_baseUrlLabel = new Tui::ZLabel(tr("Base URL (e.g. https://…)"), this);
    m_baseUrlLabel->setVisible(false);
    layout->addWidget(m_baseUrlLabel);
    m_baseUrl = new Tui::ZInputBox(QString(), this);
    m_baseUrl->setObjectName(QStringLiteral("firstRunBaseUrl"));
    m_baseUrl->setVisible(false);
    layout->addWidget(m_baseUrl);
    m_customModelLabel = new Tui::ZLabel(tr("Model id"), this);
    m_customModelLabel->setVisible(false);
    layout->addWidget(m_customModelLabel);
    m_customModel = new Tui::ZInputBox(QString(), this);
    m_customModel->setObjectName(QStringLiteral("firstRunCustomModel"));
    m_customModel->setVisible(false);
    layout->addWidget(m_customModel);

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
    m_primary->setObjectName(QStringLiteral("firstRunPrimary"));
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
        if (m_model->phase() == QStringLiteral("agenttype")) {
            // Native -> the inference step; an INSTALLED foreign agent -> full commit (the
            // model's applyForeignChoice; name falls back to the agent name).
            const QString agent = m_agentType != nullptr ? m_agentType->selectedAgent() : QString();
            if (agent.isEmpty()) {
                m_model->chooseAgentType(QString());
            } else {
                m_model->applyForeignChoice(agent, agent);
            }
        } else if (m_model->phase() == QStringLiteral("inference")) {
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
    // The shared key gate (FIX 4) resolved or re-armed: update Finish + the reason line.
    connect(m_model, &firstrun::FirstRunModel::keyGateChanged, this,
            &FirstRunDialog::refreshInferenceControls);
    // Every key edit re-arms the shared gate with the fresh selection (the entered key must
    // re-prove via the next authenticated LIST), exactly like the GUI picker's onTextEdited.
    connect(m_key, &Tui::ZInputBox::textChanged, this,
            [this](const QString& text) { m_model->setInferenceSelection(m_providerId, text); });
    // The user took ownership of the agent name: provider changes stop re-seeding it. The
    // programmatic seed itself is guarded out; Finish enablement tracks the name either way.
    connect(m_agentName, &Tui::ZInputBox::textChanged, this, [this] {
        if (!m_seedingName) {
            m_agentNameEdited = true;
        }
        refreshInferenceControls();
    });
    // Custom-endpoint completeness tracks the base URL + typed model (A1).
    connect(m_baseUrl, &Tui::ZInputBox::textChanged, this,
            &FirstRunDialog::refreshInferenceControls);
    connect(m_customModel, &Tui::ZInputBox::textChanged, this,
            &FirstRunDialog::refreshInferenceControls);

    // Agent-type selection changes re-gate the primary button (uninstalled foreign rows are
    // not committable, parity with the GUI picker's unselectable rows).
    connect(m_agentType, &AgentTypeView::selectionChanged, this, &FirstRunDialog::syncToPhase);
    connect(m_agentType, &Tui::ZListView::enterPressed, this, [this](int) { syncToPhase(); });

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
    // Tall enough for the inference phase's agent-name row + key-gate reason line while still
    // fitting a classic 80x24 terminal.
    setGeometry(QRect(0, 0, 66, 23));
}

namespace {
// The synthetic custom-endpoint provider row id (A1 / CON-10) — client-side only, never a
// catalog descriptor; mirrors AgentInferencePicker.qml's kCustomId.
const QString kCustomProviderId = QStringLiteral("__custom__");
} // namespace

QVariantMap FirstRunDialog::currentProviderDescriptor() const {
    if (m_providerCatalog == nullptr || m_providerId.isEmpty() || customSelected()) {
        return {};
    }
    return m_providerCatalog->descriptorFor(m_providerId);
}

bool FirstRunDialog::customSelected() const {
    return m_providerId == kCustomProviderId;
}

void FirstRunDialog::rebuildProviderList() {
    if (m_providerCatalog == nullptr || m_providerList == nullptr) {
        return;
    }
    m_providerRows = m_providerCatalog->providers();
    // Vision (CON-10): a "Custom endpoint…" row at the bottom of the provider list.
    QVariantMap customRow;
    customRow[QStringLiteral("id")] = kCustomProviderId;
    customRow[QStringLiteral("name")] = tr("Custom endpoint…");
    customRow[QStringLiteral("kind")] = QStringLiteral("custom");
    m_providerRows.append(customRow);
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
    // Re-arm the shared key gate with the fresh selection (FIX 4) and re-seed the agent name
    // from the provider's label, both before the refresh so its resolution lands on this state.
    m_model->setInferenceSelection(m_providerId, m_key != nullptr ? m_key->text() : QString());
    seedAgentName();
    if (m_providerCatalog != nullptr && !customSelected()) {
        // Transient key while listing (no profile yet); Daemon Cloud/local ignore it. The
        // custom endpoint lists nothing: no wire op can enumerate an arbitrary server's models.
        m_providerCatalog->refreshModels(m_providerId, QString(), m_key->text());
    }
    rebuildModelList();
    refreshInferenceControls();
}

void FirstRunDialog::seedAgentName() {
    // Prefill from the chosen provider's catalog label, lowercased (fallback: the descriptor id)
    // - the same seed the GUI wizard derives from providerLabel - until the user edits the name.
    if (m_agentName == nullptr || m_agentNameEdited) {
        return;
    }
    QString label = currentProviderDescriptor().value(QStringLiteral("name")).toString();
    if (label.isEmpty()) {
        label = customSelected() ? tr("custom") : m_providerId;
    }
    m_seedingName = true;
    m_agentName->setText(label.toLower());
    m_seedingName = false;
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
    const bool inference = m_model->phase() == QStringLiteral("inference");
    const bool custom = customSelected();
    const QVariantMap desc = currentProviderDescriptor();
    const bool requiresKey = desc.value(QStringLiteral("requiresKey")).toBool();
    if (m_key != nullptr) {
        // The custom endpoint offers an OPTIONAL key (stored profile-scoped like any other).
        m_key->setVisible(inference && (requiresKey || custom));
    }
    if (m_listModelsBtn != nullptr) {
        m_listModelsBtn->setVisible(inference && requiresKey);
    }
    // The custom endpoint swaps the selection-only model list for base-URL + typed-model inputs
    // (A1: nothing to list; the server names its own models).
    if (m_modelLabel != nullptr) {
        m_modelLabel->setVisible(inference && !custom);
    }
    if (m_modelList != nullptr) {
        m_modelList->setVisible(inference && !custom);
    }
    if (m_baseUrlLabel != nullptr) {
        m_baseUrlLabel->setVisible(inference && custom);
    }
    if (m_baseUrl != nullptr) {
        m_baseUrl->setVisible(inference && custom);
    }
    if (m_customModelLabel != nullptr) {
        m_customModelLabel->setVisible(inference && custom);
    }
    if (m_customModel != nullptr) {
        m_customModel->setVisible(inference && custom);
    }
    // The shared key gate's blocking reason (empty until an authenticated LIST fails), rendered
    // like the GUI picker's message line.
    if (m_keyGateMsg != nullptr) {
        const QString reason = m_model->keyGateMessage();
        m_keyGateMsg->setText(reason);
        m_keyGateMsg->setVisible(inference && requiresKey && !reason.isEmpty());
    }
    if (m_primary != nullptr && inference) {
        // Finish parity with the GUI wizard: a non-empty agent name, a provider + concrete
        // model, an entered key where required, AND the shared key gate (FIX 4) - the key must
        // be PROVEN by an authenticated listing, not merely typed. The custom endpoint needs
        // its base URL + a typed model instead (the key stays optional; no gate can validate
        // it without a wire op).
        const bool nameOk = m_agentName != nullptr && !m_agentName->text().trimmed().isEmpty();
        if (custom) {
            const bool baseOk = m_baseUrl != nullptr && !m_baseUrl->text().trimmed().isEmpty();
            const bool modelOk =
                m_customModel != nullptr && !m_customModel->text().trimmed().isEmpty();
            m_primary->setEnabled(baseOk && modelOk && nameOk);
        } else {
            const bool keyOk = !requiresKey || !m_key->text().isEmpty();
            m_primary->setEnabled(!m_providerId.isEmpty() && !m_selectedModel.isEmpty() && keyOk &&
                                  nameOk && m_model->keyGatePassed());
        }
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
                    ? tr("https://node.example:8080", "example remote connection target")
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
    const bool agenttype = p == QStringLiteral("agenttype");
    // The agent-type picker lives in its own phase (CON-16), fronting the inference step.
    if (m_agentTypeLabel != nullptr) {
        m_agentTypeLabel->setVisible(agenttype);
    }
    if (m_agentType != nullptr) {
        const bool wasVisible = m_agentType->isVisible();
        m_agentType->setVisible(agenttype);
        if (agenttype && !wasVisible) {
            m_agentType->refresh(); // fresh catalog + discovery badges on step entry
        }
    }
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
    // The agent name + provider/model pickers live in the inference phase only.
    if (m_nameLabel != nullptr) {
        m_nameLabel->setVisible(inference);
    }
    if (m_agentName != nullptr) {
        m_agentName->setVisible(inference);
    }
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
    if (m_keyGateMsg != nullptr && !inference) {
        m_keyGateMsg->setVisible(false); // refreshInferenceControls re-shows it when blocking
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
    } else if (p == QStringLiteral("agenttype")) {
        const bool native = m_agentType == nullptr || m_agentType->selectedAgent().isEmpty();
        m_status->setText(tr("Choose the kind of agent: native picks a model next; a foreign "
                             "ACP agent brings its own."));
        m_primary->setText(native ? tr("Continue") : tr("Finish"));
        m_primary->setEnabled(m_agentType == nullptr || m_agentType->selectionValid());
        m_target->setEnabled(false);
        if (m_token != nullptr) {
            m_token->setVisible(false);
        }
        if (m_key != nullptr) {
            m_key->setVisible(false);
        }
        if (m_listModelsBtn != nullptr) {
            m_listModelsBtn->setVisible(false);
        }
        if (m_agentType != nullptr) {
            m_agentType->setFocus();
        }
    } else if (p == QStringLiteral("inference")) {
        // The A7 finish probe's actionable failure ("Couldn't reach your model…") replaces the
        // instruction line until the user fixes the provider and re-commits.
        m_status->setText(m_model->error().isEmpty()
                              ? tr("Pick a provider and a model, then Finish.")
                              : m_model->error());
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
    // the descriptor) + a profile-scoped key + make it default under the chosen agent NAME, then
    // finish. No hardcoded provider: the id/model/key come from the node-driven selection.
    // FirstRunModel owns the persistence so the GUI + TUI share one path: a name that is empty or
    // equal to the seeded placeholder id configures the placeholder in place; anything else mints
    // a named agent (same semantics as the GUI wizard). The custom endpoint (A1) rides the same
    // path with the typed model + the user base URL.
    const QString key = m_key != nullptr ? m_key->text() : QString();
    const QString name = m_agentName != nullptr ? m_agentName->text().trimmed() : QString();
    const bool custom = customSelected();
    const QString model =
        custom && m_customModel != nullptr ? m_customModel->text().trimmed() : m_selectedModel;
    const QString baseUrl =
        custom && m_baseUrl != nullptr ? m_baseUrl->text().trimmed() : QString();
    m_model->applyInferenceChoice(m_providerId, model, key, name, baseUrl);
    if (m_key != nullptr) {
        m_key->setText(QString());
    }
}
