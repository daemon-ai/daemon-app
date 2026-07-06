// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "dialogs/agents_dialog.h"

#include "daemon/repositories.h"

#include <QRect>
#include <QRegularExpression>
#include <QVariantMap>
#include <Tui/ZButton.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWindow.h>

namespace {
// Match the GUI badge wording so the two surfaces read the same.
QString rowLabel(const daemonapp::daemon::DecodedAgentEntry& e) {
    QString label = e.name;
    label += e.protocol == QStringLiteral("StreamJson") ? QStringLiteral("  [stream-json]")
                                                        : QStringLiteral("  [ACP]");
    label += e.source == QStringLiteral("Manual") ? QObject::tr("  [manual]")
                                                  : QObject::tr("  [builtin]");
    label += e.installed ? QObject::tr("  [installed]") : QObject::tr("  [not installed]");
    return label;
}
} // namespace

AgentsDialog::AgentsDialog(daemonapp::daemon::AgentRepository* agents, Tui::ZWidget* parent)
    : Tui::ZDialog(parent), m_agents(agents) {
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(tr("Foreign agents"));
    setContentsMargins({2, 1, 2, 1});

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    layout->addWidget(new Tui::ZLabel(tr("Registered + discovered agents:"), this));
    m_list = new Tui::ZListView(this);
    layout->addWidget(m_list);

    m_status = new Tui::ZLabel(QString(), this);
    layout->addWidget(m_status);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    auto* reg = new Tui::ZButton(tr("Register…"), this);
    reg->setDefault(true);
    buttons->addWidget(reg);
    auto* remove = new Tui::ZButton(tr("Remove"), this);
    buttons->addWidget(remove);
    auto* rescan = new Tui::ZButton(tr("Re-scan"), this);
    buttons->addWidget(rescan);
    buttons->addStretch();
    auto* close = new Tui::ZButton(tr("Close"), this);
    buttons->addWidget(close);

    connect(reg, &Tui::ZButton::clicked, this, &AgentsDialog::openRegisterForm);
    connect(remove, &Tui::ZButton::clicked, this, &AgentsDialog::removeSelected);
    connect(rescan, &Tui::ZButton::clicked, this, [this] {
        if (m_agents != nullptr) {
            m_agents->discover();
        }
    });
    connect(close, &Tui::ZButton::clicked, this, &Tui::ZDialog::reject);

    if (m_agents != nullptr) {
        connect(m_agents, &daemonapp::daemon::AgentRepository::catalogRefreshed, this,
                &AgentsDialog::rebuild);
        // Register/remove outcomes (C6): surface the node's honest cause / a confirmation.
        connect(m_agents, &daemonapp::daemon::AgentRepository::operationFailed, this,
                [this](const QString& message) {
                    if (m_status != nullptr) {
                        m_status->setText(message);
                    }
                });
        connect(m_agents, &daemonapp::daemon::AgentRepository::agentRegistered, this, [this] {
            if (m_status != nullptr) {
                m_status->setText(tr("Agent registered."));
            }
        });
        connect(m_agents, &daemonapp::daemon::AgentRepository::agentRemoved, this,
                [this](const QString& name) {
                    if (m_status != nullptr) {
                        m_status->setText(tr("Removed “%1”.").arg(name));
                    }
                });
        m_agents->refreshCatalog();
        m_agents->discover();
    }
    rebuild();
    setGeometry(QRect(0, 0, 62, 18));
}

void AgentsDialog::rebuild() {
    const int keep =
        (m_list != nullptr && m_list->currentIndex().isValid()) ? m_list->currentIndex().row() : 0;
    m_names.clear();
    m_manual.clear();
    QStringList items;
    if (m_agents != nullptr) {
        for (const daemonapp::daemon::DecodedAgentEntry& e : m_agents->entries()) {
            m_names << e.name;
            m_manual << (e.source == QStringLiteral("Manual"));
            items << rowLabel(e);
        }
    }
    if (items.isEmpty()) {
        items << tr("(no foreign agents in the catalog)");
    }
    if (m_list != nullptr) {
        m_list->setItems(items);
        if (m_list->model() != nullptr) {
            const int row = qBound(0, keep, static_cast<int>(items.size()) - 1);
            m_list->setCurrentIndex(m_list->model()->index(row, 0));
        }
    }
}

void AgentsDialog::openRegisterForm() {
    auto* form = new RegisterAgentDialog(m_agents, this);
    form->setVisible(true);
}

void AgentsDialog::removeSelected() {
    if (m_agents == nullptr || m_list == nullptr || !m_list->currentIndex().isValid()) {
        return;
    }
    const int row = m_list->currentIndex().row();
    if (row < 0 || row >= m_names.size()) {
        return;
    }
    // Only manual registrations are removable (builtins are node-owned).
    if (!m_manual.at(row)) {
        if (m_status != nullptr) {
            m_status->setText(tr("Only manually registered agents can be removed."));
        }
        return;
    }
    const QString name = m_names.at(row);
    // Confirm the fail-closed consequence before AgentRemove commits (ENG-3).
    auto* confirm = new Tui::ZDialog(this);
    confirm->setOptions(Tui::ZWindow::DeleteOnClose);
    confirm->setWindowTitle(tr("Remove agent"));
    confirm->setContentsMargins({2, 1, 2, 1});
    auto* clayout = new Tui::ZVBoxLayout();
    confirm->setLayout(clayout);
    clayout->addWidget(new Tui::ZLabel(
        tr("Remove “%1”? Profiles that use it will fail to start\nuntil it is re-registered.")
            .arg(name),
        confirm));
    auto* cbuttons = new Tui::ZHBoxLayout();
    clayout->add(cbuttons);
    cbuttons->addStretch();
    auto* yes = new Tui::ZButton(tr("Remove"), confirm);
    yes->setDefault(true);
    cbuttons->addWidget(yes);
    auto* no = new Tui::ZButton(tr("Cancel"), confirm);
    cbuttons->addWidget(no);
    connect(yes, &Tui::ZButton::clicked, confirm, [this, confirm, name] {
        if (m_agents != nullptr) {
            m_agents->removeAgent(name);
        }
        confirm->reject();
    });
    connect(no, &Tui::ZButton::clicked, confirm, &Tui::ZDialog::reject);
    confirm->setGeometry(QRect(0, 0, 52, 8));
    confirm->setVisible(true);
}

// --- RegisterAgentDialog -----------------------------------------------------------------------

RegisterAgentDialog::RegisterAgentDialog(daemonapp::daemon::AgentRepository* agents,
                                         Tui::ZWidget* parent)
    : Tui::ZDialog(parent), m_agents(agents) {
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(tr("Register foreign agent"));
    setContentsMargins({2, 1, 2, 1});

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    layout->addWidget(new Tui::ZLabel(tr("Name:"), this));
    m_name = new Tui::ZInputBox(QString(), this);
    layout->addWidget(m_name);

    layout->addWidget(new Tui::ZLabel(tr("Protocol (Enter to pick):"), this));
    m_protocol = new Tui::ZListView(this);
    m_protocol->setItems({tr("ACP (Agent Client Protocol)"), tr("Claude Code (stream-json)")});
    if (m_protocol->model() != nullptr) {
        m_protocol->setCurrentIndex(m_protocol->model()->index(0, 0));
    }
    layout->addWidget(m_protocol);

    layout->addWidget(new Tui::ZLabel(tr("Program (blank = use TCP endpoint below):"), this));
    m_program = new Tui::ZInputBox(QString(), this);
    layout->addWidget(m_program);

    layout->addWidget(new Tui::ZLabel(tr("Arguments (space-separated):"), this));
    m_args = new Tui::ZInputBox(QString(), this);
    layout->addWidget(m_args);

    // TUI degradation (stated): a single-line "KEY=VAL,KEY=VAL" instead of the GUI's multi-line
    // editor — env is niche for the common program+args case; the GUI covers the full form.
    layout->addWidget(new Tui::ZLabel(tr("Environment (KEY=VAL,KEY=VAL):"), this));
    m_env = new Tui::ZInputBox(QString(), this);
    layout->addWidget(m_env);

    layout->addWidget(new Tui::ZLabel(tr("OR endpoint (tcp://host:port):"), this));
    m_endpoint = new Tui::ZInputBox(QString(), this);
    layout->addWidget(m_endpoint);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* reg = new Tui::ZButton(tr("Register"), this);
    reg->setDefault(true);
    buttons->addWidget(reg);
    auto* cancel = new Tui::ZButton(tr("Cancel"), this);
    buttons->addWidget(cancel);
    connect(reg, &Tui::ZButton::clicked, this, &RegisterAgentDialog::commit);
    connect(cancel, &Tui::ZButton::clicked, this, &Tui::ZDialog::reject);

    m_name->setFocus();
    setGeometry(QRect(0, 0, 58, 20));
}

void RegisterAgentDialog::commit() {
    if (m_agents == nullptr || m_name == nullptr) {
        return;
    }
    const QString name = m_name->text().trimmed();
    const QString program = m_program != nullptr ? m_program->text().trimmed() : QString();
    const QString endpoint = m_endpoint != nullptr ? m_endpoint->text().trimmed() : QString();
    if (name.isEmpty() || (program.isEmpty() && endpoint.isEmpty())) {
        return; // the repository also validates + the node re-probes
    }
    QVariantMap form;
    form[QStringLiteral("name")] = name;
    const bool streamJson = m_protocol != nullptr && m_protocol->currentIndex().isValid() &&
                            m_protocol->currentIndex().row() == 1;
    form[QStringLiteral("protocol")] =
        streamJson ? QStringLiteral("StreamJson") : QStringLiteral("Acp");
    if (program.isEmpty() && !endpoint.isEmpty()) {
        form[QStringLiteral("endpoint")] = endpoint;
    } else {
        form[QStringLiteral("program")] = program;
        const QString args = m_args != nullptr ? m_args->text().trimmed() : QString();
        form[QStringLiteral("args")] =
            args.isEmpty() ? QStringList{} : args.split(QRegularExpression(QStringLiteral("\\s+")));
        QVariantMap env;
        if (m_env != nullptr) {
            const QStringList pairs =
                m_env->text().split(QChar::fromLatin1(','), Qt::SkipEmptyParts);
            for (const QString& raw : pairs) {
                const QString pair = raw.trimmed();
                const qsizetype eq = pair.indexOf(QChar::fromLatin1('='));
                if (eq <= 0) {
                    continue;
                }
                env[pair.left(eq).trimmed()] = pair.mid(eq + 1).trimmed();
            }
        }
        form[QStringLiteral("env")] = env;
    }
    m_agents->registerAgent(form);
    reject(); // the parent AgentsDialog surfaces the outcome via the repository signals
}
