// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "tui_dialogs.h"

#include "agent_setup_view.h"
#include "agent_type_view.h"
#include "daemon/repositories.h"
#include "setup/agent_setup_model.h"

#include <QAbstractItemModel>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZListView.h>
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

NewAgentDialog::NewAgentDialog(setup::AgentSetupModel* setup,
                               daemonapp::daemon::AgentRepository* agents,
                               models::IProviderCatalog* providerCatalog, Tui::ZWidget* parent)
    : Tui::ZDialog(parent), m_setup(setup) {
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(tr("New agent"));
    setContentsMargins({2, 1, 2, 1});

    // Fresh Core setup each open (the shared model is reused across surfaces).
    if (m_setup != nullptr) {
        m_setup->reset();
    }

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    layout->addWidget(new Tui::ZLabel(tr("Name:"), this));
    m_name = new Tui::ZInputBox(QString(), this);
    layout->addWidget(m_name);

    layout->addWidget(new Tui::ZLabel(tr("Engine (Enter to pick):"), this));
    // The shared native+foreign picker projection (verification badges; same rows as the GUI).
    m_engines = new AgentTypeView(agents, this);
    layout->addWidget(m_engines);

    // The shared backend + inference steps over the same AgentSetupModel (auto-visible: backend
    // when foreign, inference when Core or NodeProvider).
    m_setupView = new AgentSetupView(m_setup, providerCatalog, this);
    layout->addWidget(m_setupView);

    m_error = new Tui::ZLabel(QString(), this);
    m_error->setVisible(false);
    layout->addWidget(m_error);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    m_create = new Tui::ZButton(tr("Create"), this);
    m_create->setDefault(true);
    buttons->addWidget(m_create);
    auto* cancel = new Tui::ZButton(tr("Cancel"), this);
    buttons->addWidget(cancel);

    connect(m_create, &Tui::ZButton::clicked, this, &NewAgentDialog::commit);
    connect(cancel, &Tui::ZButton::clicked, this, &Tui::ZDialog::reject);

    // Engine selection drives the shared model (the TUI picker is selection-only), then re-gates
    // Create.
    connect(m_engines, &AgentTypeView::selectionChanged, this, [this] {
        applyEngineFromPicker();
        refreshCommit();
    });
    connect(m_engines, &Tui::ZListView::enterPressed, this, [this](int) {
        applyEngineFromPicker();
        refreshCommit();
    });
    connect(m_setupView, &AgentSetupView::selectionChanged, this, &NewAgentDialog::refreshCommit);
    connect(m_setupView, &AgentSetupView::modelDiscoverRequested, this,
            &NewAgentDialog::modelDiscoverRequested);
    connect(m_name, &Tui::ZInputBox::textChanged, this,
            [this](const QString&) { refreshCommit(); });
    if (m_setup != nullptr) {
        connect(m_setup, &setup::AgentSetupModel::stateChanged, this,
                &NewAgentDialog::refreshCommit);
        // A commit resolved / failed: surface the new agent or the node's honest reason. Guard on
        // m_committing so another surface's commit on the shared model cannot close this dialog.
        connect(m_setup, &setup::AgentSetupModel::committed, this, [this](const QString& id) {
            if (!m_committing) {
                return;
            }
            m_committing = false;
            emit created(id);
            close();
        });
        connect(m_setup, &setup::AgentSetupModel::failed, this, [this](const QString& message) {
            if (!m_committing) {
                return;
            }
            m_committing = false;
            if (m_error != nullptr) {
                m_error->setText(message);
                m_error->setVisible(true);
            }
            if (m_create != nullptr) {
                m_create->setEnabled(true);
            }
        });
    }

    m_engines->refresh(); // fresh catalog + discovery badges each open
    applyEngineFromPicker();
    refreshCommit();
    m_name->setFocus();
    setGeometry(QRect(0, 0, 60, 22));
}

void NewAgentDialog::applyEngineFromPicker() {
    if (m_setup == nullptr || m_engines == nullptr) {
        return;
    }
    const QString agent = m_engines->selectedAgent();
    m_setup->chooseEngine(agent.isEmpty() ? QStringLiteral("Core") : QStringLiteral("Foreign"),
                          agent);
}

void NewAgentDialog::refreshCommit() {
    if (m_create == nullptr) {
        return;
    }
    const QString name = m_name != nullptr ? m_name->text().trimmed() : QString();
    const bool ready = m_setup != nullptr && m_setup->commitReady() && !name.isEmpty() &&
                       (m_engines == nullptr || m_engines->selectionValid());
    m_create->setEnabled(ready);
}

void NewAgentDialog::commit() {
    const QString name = m_name != nullptr ? m_name->text().trimmed() : QString();
    if (name.isEmpty() || m_setup == nullptr || !m_setup->commitReady()) {
        return;
    }
    // Parity with the GUI accept gate: an uninstalled foreign row is not committable.
    if (m_engines != nullptr && !m_engines->selectionValid()) {
        return;
    }
    if (m_error != nullptr) {
        m_error->setVisible(false);
    }
    if (m_create != nullptr) {
        m_create->setEnabled(false); // in-flight; committed()/failed() re-enables / closes
    }
    // Commit the shared-model selection (engine + backend + inference) — the one create path.
    m_committing = true;
    m_setup->commit(name);
}
