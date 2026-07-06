// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "tui_dialogs.h"

#include "agent_type_view.h"
#include "daemon/repositories.h"
#include "profiles/iprofile_store.h"

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

NewAgentDialog::NewAgentDialog(profiles::IProfileStore* profiles,
                               daemonapp::daemon::AgentRepository* agents, Tui::ZWidget* parent)
    : Tui::ZDialog(parent), m_profiles(profiles) {
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(tr("New agent"));
    setContentsMargins({2, 1, 2, 1});

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    layout->addWidget(new Tui::ZLabel(tr("Name:"), this));
    m_name = new Tui::ZInputBox(QString(), this);
    layout->addWidget(m_name);

    layout->addWidget(new Tui::ZLabel(tr("Engine (Enter to pick):"), this));
    // The shared native+foreign picker projection (installed markers; same rows as the GUI).
    m_engines = new AgentTypeView(agents, this);
    layout->addWidget(m_engines);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* create = new Tui::ZButton(tr("Create"), this);
    create->setDefault(true);
    buttons->addWidget(create);
    auto* cancel = new Tui::ZButton(tr("Cancel"), this);
    buttons->addWidget(cancel);

    connect(create, &Tui::ZButton::clicked, this, &NewAgentDialog::commit);
    connect(cancel, &Tui::ZButton::clicked, this, &Tui::ZDialog::reject);

    m_engines->refresh(); // fresh catalog + discovery badges each open
    m_name->setFocus();
    setGeometry(QRect(0, 0, 52, 14));
}

void NewAgentDialog::commit() {
    const QString name = m_name != nullptr ? m_name->text().trimmed() : QString();
    if (name.isEmpty() || m_profiles == nullptr) {
        return;
    }
    // Parity with the GUI accept gate: an uninstalled foreign row is not committable.
    if (m_engines != nullptr && !m_engines->selectionValid()) {
        return;
    }
    const QString agent = m_engines != nullptr ? m_engines->selectedAgent() : QString();
    // Empty agent = the native engine (plain create; provider/model configured on the Profile
    // page); otherwise a foreign agent (the named ProfileCreate carries engine=Foreign{agent}).
    const QString id = agent.isEmpty() ? m_profiles->createProfile(name)
                                       : m_profiles->createForeignProfile(name, agent);
    if (id.isEmpty()) {
        return;
    }
    m_profiles->setDefault(id);
    emit created(id);
    close();
}
