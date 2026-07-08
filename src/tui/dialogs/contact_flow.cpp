// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "dialogs/contact_flow.h"

#include "palette_dialog.h"
#include "transports/icontacts_service.h"
#include "transports/itransport_registry.h"
#include "tui_dialogs.h"

#include <QVariant>
#include <QVariantMap>
#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZVBoxLayout.h>

namespace {

// GUI status-dot parity (ChannelsPage.qml dotColor), reduced to a glyph for the palette title.
QString presenceGlyph(const QString& presence) {
    if (presence == QLatin1String("available") || presence == QLatin1String("streaming")) {
        return QStringLiteral("●");
    }
    if (presence == QLatin1String("idle") || presence == QLatin1String("away")) {
        return QStringLiteral("◐");
    }
    return QStringLiteral("○"); // offline / invisible / dnd / unknown
}

} // namespace

ContactFlow::ContactFlow(transports::IContactsService* contacts,
                         transports::ITransportRegistry* registry, Tui::ZWidget* host)
    : QObject(host), m_contacts(contacts), m_registry(registry), m_host(host) {
    if (m_contacts == nullptr) {
        return;
    }
    connect(m_contacts, &transports::IContactsService::directoryResults, this,
            [this](const QString& transport, const QVariantList& results) {
                if (!m_findPending || transport != m_transport) {
                    return;
                }
                m_findPending = false;
                m_directory = results;
                showDirectoryPalette();
            });
    connect(m_contacts, &transports::IContactsService::profileReady, this,
            [this](const QString& transport, const QString& contactId, const QString& profile) {
                if (!m_profilePending || transport != m_transport || contactId != m_contactId) {
                    return;
                }
                m_profilePending = false;
                // A small read-only dialog showing the node-rendered profile string verbatim.
                auto* dialog = new Tui::ZDialog(m_host);
                dialog->setOptions(Tui::ZWindow::DeleteOnClose);
                dialog->setWindowTitle(tr("Contact profile"));
                dialog->setContentsMargins({2, 1, 2, 1});
                auto* layout = new Tui::ZVBoxLayout();
                dialog->setLayout(layout);
                layout->addWidget(new Tui::ZLabel(contactId, dialog));
                layout->addWidget(new Tui::ZLabel(
                    profile.isEmpty() ? tr("No profile details.") : profile, dialog));
                layout->addSpacing(1);
                auto* buttons = new Tui::ZHBoxLayout();
                layout->add(buttons);
                buttons->addStretch();
                auto* closeBtn = new Tui::ZButton(tr("Close"), dialog);
                buttons->addWidget(closeBtn);
                connect(closeBtn, &Tui::ZButton::clicked, dialog, &Tui::ZDialog::close);
                connect(dialog, &QObject::destroyed, this, &ContactFlow::finished);
                dialog->setGeometry(QRect(0, 0, 60, 9));
                closeBtn->setFocus();
            });
}

void ContactFlow::startAdd(const QString& transport) {
    if (m_contacts == nullptr) {
        return;
    }
    m_transport = transport;
    auto* dialog = new TextPromptDialog(tr("Add contact — contact id"), QString(),
                                        /*masked=*/false, m_host);
    connect(dialog, &TextPromptDialog::submitted, this, [this, transport](const QString& id) {
        const QString trimmed = id.trimmed();
        if (!trimmed.isEmpty()) {
            m_contacts->addContact(transport, trimmed);
            emit changed();
        }
        emit finished();
    });
    connect(dialog, &TextPromptDialog::canceled, this, &ContactFlow::finished);
}

void ContactFlow::startFind(const QString& transport) {
    if (m_contacts == nullptr) {
        return;
    }
    m_transport = transport;
    auto* dialog = new TextPromptDialog(tr("Find people — search query"), QString(),
                                        /*masked=*/false, m_host);
    connect(dialog, &TextPromptDialog::submitted, this, [this, transport](const QString& query) {
        m_findPending = true;
        m_contacts->searchDirectory(transport, query.trimmed());
        // The directoryResults handler opens the results palette; no finished() yet.
    });
    connect(dialog, &TextPromptDialog::canceled, this, &ContactFlow::finished);
}

void ContactFlow::startAlias(const QString& transport, const QString& contactId,
                             const QString& currentName) {
    if (m_contacts == nullptr) {
        return;
    }
    m_transport = transport;
    m_contactId = contactId;
    auto* dialog = new TextPromptDialog(tr("Alias for %1 (empty clears)").arg(contactId),
                                        currentName, /*masked=*/false, m_host);
    connect(dialog, &TextPromptDialog::submitted, this,
            [this, transport, contactId](const QString& alias) {
                m_contacts->setAlias(transport, contactId, alias.trimmed());
                emit changed();
                emit finished();
            });
    connect(dialog, &TextPromptDialog::canceled, this, &ContactFlow::finished);
}

void ContactFlow::openProfile(const QString& transport, const QString& contactId) {
    if (m_contacts == nullptr) {
        return;
    }
    m_transport = transport;
    m_contactId = contactId;
    m_profilePending = true;
    m_contacts->getProfile(transport, contactId);
}

void ContactFlow::showDirectoryPalette() {
    if (m_directoryPick == nullptr) {
        m_directoryPick = new PaletteDialog(tr("Directory"), m_host);
        connect(m_directoryPick, &PaletteDialog::canceled, this, &ContactFlow::finished);
    }
    disconnect(m_directoryPick, &PaletteDialog::activated, this, nullptr);
    connect(m_directoryPick, &PaletteDialog::activated, this, [this](const QString& id) {
        for (const QVariant& pv : m_directory) {
            const QVariantMap p = pv.toMap();
            if (p.value(QStringLiteral("id")).toString() == id) {
                showPersonActions(p);
                return;
            }
        }
    });
    QVector<PaletteDialog::Item> items;
    items.reserve(m_directory.size());
    for (const QVariant& pv : m_directory) {
        const QVariantMap p = pv.toMap();
        const QString id = p.value(QStringLiteral("id")).toString();
        const QString name = p.value(QStringLiteral("displayName")).toString();
        const QString title = QStringLiteral("%1 %2").arg(
            presenceGlyph(p.value(QStringLiteral("presence")).toString()),
            name.isEmpty() ? id : name);
        items.push_back({id, title, id});
    }
    m_directoryPick->setItems(items);
    m_directoryPick->openCentered();
}

void ContactFlow::showPersonActions(const QVariantMap& person) {
    const QString id = person.value(QStringLiteral("id")).toString();
    if (m_actionPick == nullptr) {
        m_actionPick = new PaletteDialog(tr("Directory action"), m_host);
        connect(m_actionPick, &PaletteDialog::canceled, this, &ContactFlow::finished);
    }
    disconnect(m_actionPick, &PaletteDialog::activated, this, nullptr);
    const QString transport = m_transport;
    connect(m_actionPick, &PaletteDialog::activated, this,
            [this, transport, id](const QString& action) {
                if (action == QLatin1String("add")) {
                    m_contacts->addContact(transport, id);
                    emit changed();
                } else if (action == QLatin1String("dm") && m_registry != nullptr) {
                    // The conv-create seam with the person as participant (no new wire op).
                    QVariantMap form;
                    form[QStringLiteral("participants")] = QVariantList{id};
                    m_registry->createRoom(transport, form);
                    emit changed();
                }
                emit finished();
            });
    // Rows present only when the verb is supported (GUI per-result Add/DM parity).
    QVector<PaletteDialog::Item> items;
    if (canRosterOp(QStringLiteral("add"))) {
        items.push_back({QStringLiteral("add"), tr("Add to contacts"), QString()});
    }
    if (canConversationOp(QStringLiteral("create"))) {
        items.push_back({QStringLiteral("dm"), tr("Message (DM)"), QString()});
    }
    m_actionPick->setItems(items);
    m_actionPick->openCentered();
}

bool ContactFlow::canRosterOp(const QString& verb) const {
    if (m_registry == nullptr) {
        return true; // no adapter info to gate on; the node still validates server-side
    }
    QString family;
    for (const QVariant& v : m_registry->instances()) {
        const QVariantMap row = v.toMap();
        if (row.value(QStringLiteral("transport")).toString() == m_transport) {
            family = row.value(QStringLiteral("family")).toString();
            break;
        }
    }
    for (const QVariant& v : m_registry->availableAdapters()) {
        const QVariantMap a = v.toMap();
        if (a.value(QStringLiteral("family")).toString() != family) {
            continue;
        }
        if (a.contains(QStringLiteral("rosterOps"))) {
            return a.value(QStringLiteral("rosterOps")).toMap().value(verb).toBool();
        }
        return false; // no legacy coarse fallback for contacts
    }
    return true;
}

bool ContactFlow::canConversationOp(const QString& verb) const {
    if (m_registry == nullptr) {
        return true;
    }
    QString family;
    for (const QVariant& v : m_registry->instances()) {
        const QVariantMap row = v.toMap();
        if (row.value(QStringLiteral("transport")).toString() == m_transport) {
            family = row.value(QStringLiteral("family")).toString();
            break;
        }
    }
    for (const QVariant& v : m_registry->availableAdapters()) {
        const QVariantMap a = v.toMap();
        if (a.value(QStringLiteral("family")).toString() != family) {
            continue;
        }
        if (a.contains(QStringLiteral("conversationOps"))) {
            return a.value(QStringLiteral("conversationOps")).toMap().value(verb).toBool();
        }
        return a.value(QStringLiteral("capabilities"))
            .toMap()
            .value(QStringLiteral("rooms"))
            .toBool();
    }
    return true;
}
