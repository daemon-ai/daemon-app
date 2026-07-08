// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "first_run_dialog.h"

#include "agent_setup_view.h"
#include "agent_type_view.h"
#include "models/iprovider_catalog.h"
#include "setup/agent_setup_model.h"

#include <QVariantMap>
#include <Tui/ZHBoxLayout.h>
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

    // Transport mode "cards": Embedded (disabled, coming soon) / Local (Unix socket) / Remote.
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

    // Local sub-choice: App-managed (spawn) vs Attach (an existing socket).
    m_managedBtn = new Tui::ZButton(QString(), this);
    layout->addWidget(m_managedBtn);
    connect(m_managedBtn, &Tui::ZButton::clicked, this, [this] {
        if (m_settings != nullptr) {
            m_settings->setManagedLocalDaemon(!m_settings->managedLocalDaemon());
        }
        applyMode(m_mode);
    });

    m_target = new Tui::ZInputBox(defaultTarget, this);
    layout->addWidget(m_target);

    m_token = new Tui::ZInputBox(QString(), this);
    m_token->setEchoMode(Tui::ZInputBox::Password);
    layout->addWidget(m_token);

    // Agent type (agenttype phase, CON-16): the shared native-vs-foreign engine picker projection.
    m_agentTypeLabel = new Tui::ZLabel(tr("Agent type (Enter to pick):"), this);
    m_agentTypeLabel->setVisible(false);
    layout->addWidget(m_agentTypeLabel);
    m_agentType = new AgentTypeView(agents, this);
    m_agentType->setObjectName(QStringLiteral("firstRunAgentType"));
    m_agentType->setVisible(false);
    layout->addWidget(m_agentType);

    // Agent name: the profile id the node keys the agent by, prefilled from the chosen provider's
    // label (native) or the selected foreign agent, until the user edits it.
    m_nameLabel = new Tui::ZLabel(tr("Agent name"), this);
    m_nameLabel->setVisible(false);
    layout->addWidget(m_nameLabel);
    m_agentName = new Tui::ZInputBox(QString(), this);
    m_agentName->setObjectName(QStringLiteral("firstRunAgentName"));
    m_agentName->setVisible(false);
    layout->addWidget(m_agentName);

    // Backend + inference: the shared AgentSetupView over the wizard's AgentSetupModel (the TUI
    // twin of ForeignBackendPicker + AgentInferencePicker). Section visibility is host-controlled
    // per phase below.
    m_setupView = new AgentSetupView(m_model != nullptr ? m_model->agentSetup() : nullptr,
                                     m_providerCatalog, this);
    m_setupView->setVisible(false);
    layout->addWidget(m_setupView);

    // SASL login: username + masked password (auth phase).
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
            // Inference needed (native, or a foreign NodeProvider backend) -> Continue; a foreign
            // AgentNative agent finishes here (commit the shared-model selection).
            if (needsInference()) {
                m_model->continueFromAgentType();
            } else {
                m_model->commitSetup(finishName());
            }
        } else if (m_model->phase() == QStringLiteral("inference")) {
            m_model->commitSetup(finishName());
        } else if (m_model->phase() == QStringLiteral("auth")) {
            m_model->submitLogin(m_username->text(), m_password->text());
            m_password->setText(QString());
        } else if (m_connection != nullptr) {
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
    connect(m_model, &firstrun::FirstRunModel::inferenceReadyChanged, this,
            &FirstRunDialog::syncToPhase);
    connect(m_model, &firstrun::FirstRunModel::errorChanged, this, &FirstRunDialog::syncToPhase);
    // The shared key gate resolved or re-armed, or the selection changed: re-gate the primary.
    connect(m_model, &firstrun::FirstRunModel::keyGateChanged, this, &FirstRunDialog::syncToPhase);
    connect(m_setupView, &AgentSetupView::selectionChanged, this, [this] {
        seedAgentName();
        syncToPhase();
    });
    connect(m_setupView, &AgentSetupView::modelDiscoverRequested, this,
            &FirstRunDialog::modelDiscoverRequested);

    // The user took ownership of the agent name: selection changes stop re-seeding it.
    connect(m_agentName, &Tui::ZInputBox::textChanged, this, [this] {
        if (!m_seedingName) {
            m_agentNameEdited = true;
        }
        syncToPhase();
    });

    // Engine selection: drive the shared model (the TUI AgentTypeView is selection-only) so the
    // backend rows + needsInference reflect it, then re-seed + re-gate.
    connect(m_agentType, &AgentTypeView::selectionChanged, this, [this] {
        applyEngineFromPicker();
        seedAgentName();
        syncToPhase();
    });
    connect(m_agentType, &Tui::ZListView::enterPressed, this, [this](int) {
        applyEngineFromPicker();
        seedAgentName();
        syncToPhase();
    });

    applyMode(m_mode);
    seedAgentName(); // initial prefill from the default provider selection
    syncToPhase();
    m_target->setFocus();
    // Tall enough for the inference phase's agent-name row + key-gate reason line.
    setGeometry(QRect(0, 0, 66, 25));
}

bool FirstRunDialog::needsInference() const {
    setup::AgentSetupModel* setup = m_model != nullptr ? m_model->agentSetup() : nullptr;
    return setup == nullptr || setup->needsInference();
}

QString FirstRunDialog::finishName() const {
    return m_agentName != nullptr ? m_agentName->text().trimmed() : QString();
}

void FirstRunDialog::applyEngineFromPicker() {
    setup::AgentSetupModel* setup = m_model != nullptr ? m_model->agentSetup() : nullptr;
    if (setup == nullptr || m_agentType == nullptr) {
        return;
    }
    const QString agent = m_agentType->selectedAgent();
    setup->chooseEngine(agent.isEmpty() ? QStringLiteral("Core") : QStringLiteral("Foreign"),
                        agent);
}

void FirstRunDialog::seedAgentName() {
    if (m_agentName == nullptr || m_agentNameEdited) {
        return;
    }
    setup::AgentSetupModel* setup = m_model != nullptr ? m_model->agentSetup() : nullptr;
    QString seed;
    if (setup != nullptr && setup->engineKind() == QStringLiteral("Foreign")) {
        // Foreign: seed from the selected agent name (verbatim, like the GUI acpNameField).
        seed = m_agentType != nullptr ? m_agentType->selectedAgent() : QString();
    } else {
        // Native: seed from the selected provider's catalog label, lowercased.
        const QString pid = m_setupView != nullptr ? m_setupView->selectedProviderId() : QString();
        QString label;
        if (m_providerCatalog != nullptr && !pid.isEmpty()) {
            label = m_providerCatalog->descriptorFor(pid).value(QStringLiteral("name")).toString();
        }
        if (label.isEmpty()) {
            label = pid;
        }
        seed = label.toLower();
    }
    if (seed.isEmpty()) {
        return;
    }
    m_seedingName = true;
    m_agentName->setText(seed);
    m_seedingName = false;
}

void FirstRunDialog::applyMode(const QString& mode) {
    m_mode = mode;
    const bool remote = mode == QStringLiteral("remote");
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
    setup::AgentSetupModel* setup = m_model != nullptr ? m_model->agentSetup() : nullptr;
    const bool foreign = setup != nullptr && setup->engineKind() == QStringLiteral("Foreign");

    // The agent-type picker lives in its own phase, fronting the inference step.
    if (m_agentTypeLabel != nullptr) {
        m_agentTypeLabel->setVisible(agenttype);
    }
    if (m_agentType != nullptr) {
        const bool wasVisible = m_agentType->isVisible();
        m_agentType->setVisible(agenttype);
        if (agenttype && !wasVisible) {
            m_agentType->refresh(); // fresh catalog + discovery badges on step entry
            applyEngineFromPicker();
        }
    }

    // The shared setup view: backend section in the agent-type phase, inference in the inference
    // phase (each additionally gated by the model's needsBackendChoice / needsInference).
    if (m_setupView != nullptr) {
        m_setupView->setVisible(agenttype || inference);
        m_setupView->setBackendAllowed(agenttype);
        m_setupView->setInferenceAllowed(inference);
    }

    // Agent name: a foreign profile is named in the agent-type pane; a native one on the inference
    // step (the single field carries the value across the phase change either way).
    const bool nameVisible = (agenttype && foreign) || (inference && !foreign);
    if (m_nameLabel != nullptr) {
        m_nameLabel->setVisible(nameVisible);
    }
    if (m_agentName != nullptr) {
        m_agentName->setVisible(nameVisible);
    }

    if (m_managedBtn != nullptr) {
        const bool managed = m_settings != nullptr && m_settings->managedLocalDaemon();
        m_managedBtn->setVisible(p == QStringLiteral("connect") &&
                                 m_mode == QStringLiteral("local"));
        m_managedBtn->setText(managed ? tr("Local: App-managed (press to Attach instead)")
                                      : tr("Local: Attach to socket (press for App-managed)"));
    }
    if (m_username != nullptr) {
        m_username->setVisible(p == QStringLiteral("auth"));
    }
    if (m_password != nullptr) {
        m_password->setVisible(p == QStringLiteral("auth"));
    }

    if (p == QStringLiteral("connecting")) {
        m_status->setText(tr("Connecting..."));
        m_primary->setText(tr("Connect"));
        m_primary->setEnabled(false);
        m_target->setEnabled(false);
    } else if (p == QStringLiteral("auth")) {
        m_status->setText(m_model->error().isEmpty() ? tr("Sign in to continue.")
                                                     : m_model->error());
        m_primary->setText(tr("Sign in"));
        m_primary->setEnabled(true);
        m_target->setEnabled(false);
        if (m_token != nullptr) {
            m_token->setVisible(false);
        }
        m_username->setFocus();
    } else if (agenttype) {
        const bool native = m_agentType == nullptr || m_agentType->selectedAgent().isEmpty();
        const bool gatewayReq = setup != nullptr && setup->gatewayRequired();
        m_status->setText(tr("Choose the kind of agent: native picks a model next; a foreign "
                             "agent uses its own backend or the node's gateway."));
        m_primary->setText(needsInference() ? tr("Continue") : tr("Finish"));
        // Native continues into inference; a foreign selection needs an installed agent, a name,
        // and (for NodeProvider) the gateway enabled before it can proceed.
        m_primary->setEnabled(native || (m_agentType != nullptr && m_agentType->selectionValid() &&
                                         !finishName().isEmpty() && !gatewayReq));
        m_target->setEnabled(false);
        if (m_token != nullptr) {
            m_token->setVisible(false);
        }
    } else if (inference) {
        m_status->setText(m_model->error().isEmpty()
                              ? tr("Pick a provider and a model, then Finish.")
                              : m_model->error());
        m_primary->setText(tr("Finish"));
        m_target->setEnabled(false);
        if (m_token != nullptr) {
            m_token->setVisible(false);
        }
        const bool ready = m_setupView != nullptr && m_setupView->inferenceComplete() &&
                           m_model->keyGatePassed() && !finishName().isEmpty();
        m_primary->setEnabled(ready);
    } else { // connect
        m_status->setText(m_model->error().isEmpty() ? QString() : m_model->error());
        m_primary->setText(tr("Connect"));
        m_primary->setEnabled(true);
        m_target->setEnabled(true);
    }
}
