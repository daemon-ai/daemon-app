// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "dialogs/room_flow.h"

#include "palette_dialog.h"
#include "transports/itransport_registry.h"
#include "tui_dialogs.h"

#include <QVariant>
#include <Tui/ZWidget.h>

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

RoomFlow::RoomFlow(transports::ITransportRegistry* registry, Tui::ZWidget* host)
    : QObject(host), m_registry(registry), m_host(host) {
    if (m_registry == nullptr) {
        return;
    }
    connect(m_registry, &transports::ITransportRegistry::joinDetailsReady, this,
            [this](const QString& transport, const QVariantMap& form) {
                // Only act on the flow we started (the seam is shared across surfaces/pages).
                if (transport != m_transport) {
                    return;
                }
                m_joinForm = QVariantMap{};
                m_joinForm[QStringLiteral("extras")] = QVariantMap{};
                m_joinSteps.clear();
                QVariantMap nameStep;
                nameStep[QStringLiteral("key")] = QStringLiteral("name");
                nameStep[QStringLiteral("label")] = tr("Channel name");
                nameStep[QStringLiteral("masked")] = false;
                m_joinSteps.append(nameStep);
                if (form.value(QStringLiteral("passwordSupported")).toBool()) {
                    QVariantMap step;
                    step[QStringLiteral("key")] = QStringLiteral("password");
                    step[QStringLiteral("label")] = tr("Password");
                    step[QStringLiteral("masked")] = true;
                    m_joinSteps.append(step);
                }
                if (form.value(QStringLiteral("nicknameSupported")).toBool()) {
                    QVariantMap step;
                    step[QStringLiteral("key")] = QStringLiteral("nickname");
                    step[QStringLiteral("label")] = tr("Nickname");
                    step[QStringLiteral("masked")] = false;
                    m_joinSteps.append(step);
                }
                for (const QVariant& fv : form.value(QStringLiteral("extras")).toList()) {
                    const QVariantMap f = fv.toMap();
                    QVariantMap step;
                    step[QStringLiteral("key")] =
                        QStringLiteral("extra:") + f.value(QStringLiteral("key")).toString();
                    step[QStringLiteral("label")] = f.value(QStringLiteral("label")).toString();
                    step[QStringLiteral("masked")] = false;
                    m_joinSteps.append(step);
                }
                runNextJoinStep();
            });
    connect(m_registry, &transports::ITransportRegistry::createDetailsReady, this,
            [this](const QString& transport, const QVariantMap& form) {
                if (transport != m_transport) {
                    return;
                }
                m_createExtras = QVariantMap{};
                m_createSteps.clear();
                for (const QVariant& fv : form.value(QStringLiteral("extras")).toList()) {
                    const QVariantMap f = fv.toMap();
                    QVariantMap step;
                    step[QStringLiteral("key")] = f.value(QStringLiteral("key")).toString();
                    step[QStringLiteral("label")] = f.value(QStringLiteral("label")).toString();
                    m_createSteps.append(step);
                }
                runNextCreateStep();
            });
    connect(
        m_registry, &transports::ITransportRegistry::membersChanged, this,
        [this](const QString& transport, const QString& conversation, const QVariantList& members) {
            if (!m_membersPending || transport != m_transport || conversation != m_conversation) {
                return;
            }
            m_membersPending = false;
            m_members = members;
            showMembersPalette();
        });
}

void RoomFlow::startJoin(const QString& transport) {
    if (m_registry == nullptr) {
        return;
    }
    m_transport = transport;
    m_conversation.clear();
    m_registry->conversationJoinDetails(transport);
}

void RoomFlow::startCreate(const QString& transport) {
    if (m_registry == nullptr) {
        return;
    }
    m_transport = transport;
    m_conversation.clear();
    m_registry->conversationCreateDetails(transport);
}

void RoomFlow::startInvite(const QString& transport, const QString& conversation) {
    if (m_registry == nullptr) {
        return;
    }
    m_transport = transport;
    m_conversation = conversation;
    auto* dialog = new TextPromptDialog(tr("Invite to room — contact id"), QString(),
                                        /*masked=*/false, m_host);
    connect(dialog, &TextPromptDialog::submitted, this,
            [this, transport, conversation](const QString& id) {
                const QString trimmed = id.trimmed();
                if (!trimmed.isEmpty()) {
                    m_registry->memberInvite(transport, conversation, trimmed);
                    emit changed();
                }
                emit finished();
            });
    connect(dialog, &TextPromptDialog::canceled, this, &RoomFlow::finished);
}

void RoomFlow::openMembers(const QString& transport, const QString& conversation) {
    if (m_registry == nullptr) {
        return;
    }
    m_transport = transport;
    m_conversation = conversation;
    m_membersPending = true;
    m_registry->conversationMembers(transport, conversation);
}

void RoomFlow::runNextJoinStep() {
    if (m_joinSteps.isEmpty()) {
        m_registry->joinRoom(m_transport, m_joinForm);
        emit changed();
        emit finished();
        return;
    }
    const QVariantMap step = m_joinSteps.takeFirst().toMap();
    const QString key = step.value(QStringLiteral("key")).toString();
    auto* dialog = new TextPromptDialog(step.value(QStringLiteral("label")).toString(), QString(),
                                        step.value(QStringLiteral("masked")).toBool(), m_host);
    connect(dialog, &TextPromptDialog::submitted, this, [this, key](const QString& text) {
        if (key.startsWith(QLatin1String("extra:"))) {
            QVariantMap extras = m_joinForm.value(QStringLiteral("extras")).toMap();
            extras[key.mid(int(qstrlen("extra:")))] = text;
            m_joinForm[QStringLiteral("extras")] = extras;
        } else {
            m_joinForm[key] = text;
        }
        runNextJoinStep();
    });
    connect(dialog, &TextPromptDialog::canceled, this, &RoomFlow::finished);
}

void RoomFlow::runNextCreateStep() {
    if (m_createSteps.isEmpty()) {
        QVariantMap form;
        form[QStringLiteral("maxParticipants")] = 0;
        form[QStringLiteral("participants")] = QVariantList{};
        form[QStringLiteral("extras")] = m_createExtras;
        m_registry->createRoom(m_transport, form);
        emit changed();
        emit finished();
        return;
    }
    const QVariantMap step = m_createSteps.takeFirst().toMap();
    const QString key = step.value(QStringLiteral("key")).toString();
    auto* dialog = new TextPromptDialog(step.value(QStringLiteral("label")).toString(), QString(),
                                        /*masked=*/false, m_host);
    connect(dialog, &TextPromptDialog::submitted, this, [this, key](const QString& text) {
        m_createExtras[key] = text;
        runNextCreateStep();
    });
    connect(dialog, &TextPromptDialog::canceled, this, &RoomFlow::finished);
}

void RoomFlow::showMembersPalette() {
    if (m_membersPick == nullptr) {
        m_membersPick = new PaletteDialog(tr("Members"), m_host);
        connect(m_membersPick, &PaletteDialog::canceled, this, &RoomFlow::finished);
    }
    disconnect(m_membersPick, &PaletteDialog::activated, this, nullptr);
    connect(m_membersPick, &PaletteDialog::activated, this, [this](const QString& contactId) {
        for (const QVariant& mv : m_members) {
            const QVariantMap m = mv.toMap();
            if (m.value(QStringLiteral("contactId")).toString() == contactId) {
                showMemberActions(m);
                return;
            }
        }
    });
    QVector<PaletteDialog::Item> items;
    items.reserve(m_members.size());
    for (const QVariant& mv : m_members) {
        const QVariantMap m = mv.toMap();
        const QString id = m.value(QStringLiteral("contactId")).toString();
        const QString name = m.value(QStringLiteral("displayName")).toString();
        const QString role = m.value(QStringLiteral("role")).toString();
        const QString title = QStringLiteral("%1 %2").arg(
            presenceGlyph(m.value(QStringLiteral("presence")).toString()),
            name.isEmpty() ? id : name);
        const QString hint = role.isEmpty() ? id : QStringLiteral("%1 · %2").arg(role, id);
        items.push_back({id, title, hint});
    }
    m_membersPick->setItems(items);
    m_membersPick->openCentered();
}

void RoomFlow::showMemberActions(const QVariantMap& member) {
    const QString contactId = member.value(QStringLiteral("contactId")).toString();
    if (m_actionPick == nullptr) {
        m_actionPick = new PaletteDialog(tr("Member action"), m_host);
        connect(m_actionPick, &PaletteDialog::canceled, this, &RoomFlow::finished);
    }
    disconnect(m_actionPick, &PaletteDialog::activated, this, nullptr);
    const QString transport = m_transport;
    const QString conv = m_conversation;
    connect(
        m_actionPick, &PaletteDialog::activated, this,
        [this, member, contactId, transport, conv](const QString& action) {
            if (action == QLatin1String("invite")) {
                startInvite(transport, conv);
            } else if (action == QLatin1String("role")) {
                showRolePalette(member);
            } else if (action == QLatin1String("kick")) {
                auto* confirm = new ConfirmDialog(
                    tr("Kick member"), tr("Remove %1 from this room?").arg(contactId), m_host);
                connect(confirm, &ConfirmDialog::confirmed, this,
                        [this, transport, conv, contactId] {
                            m_registry->memberKick(transport, conv, contactId);
                            emit changed();
                            emit finished();
                        });
            } else if (action == QLatin1String("ban")) {
                auto* confirm = new ConfirmDialog(
                    tr("Ban member"),
                    tr("Ban %1 from this room? They cannot re-join until unbanned.").arg(contactId),
                    m_host);
                connect(confirm, &ConfirmDialog::confirmed, this,
                        [this, transport, conv, contactId] {
                            m_registry->memberBan(transport, conv, contactId);
                            emit changed();
                            emit finished();
                        });
            }
        });
    // [acct-mgmt] Rows present only when the verb is supported (per-verb membership_ops, wire
    // v33; legacy `rooms` fallback) — GUI member-row button parity: hidden verbs = hidden rows.
    QVector<PaletteDialog::Item> items;
    if (canMembershipOp(QStringLiteral("invite"))) {
        items.push_back({QStringLiteral("invite"), tr("Invite another…"), QString()});
    }
    if (canMembershipOp(QStringLiteral("setRole"))) {
        items.push_back({QStringLiteral("role"), tr("Change role…"), QString()});
    }
    if (canMembershipOp(QStringLiteral("remove"))) {
        items.push_back({QStringLiteral("kick"), tr("Kick"), QString()});
    }
    if (canMembershipOp(QStringLiteral("ban"))) {
        items.push_back({QStringLiteral("ban"), tr("Ban"), QString()});
    }
    m_actionPick->setItems(items);
    m_actionPick->openCentered();
}

bool RoomFlow::canMembershipOp(const QString& verb) const {
    if (m_registry == nullptr) {
        return false;
    }
    // Resolve the flow's transport to its adapter family (the instances row carries it).
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
        if (a.contains(QStringLiteral("membershipOps"))) {
            return a.value(QStringLiteral("membershipOps")).toMap().value(verb).toBool();
        }
        return a.value(QStringLiteral("capabilities"))
            .toMap()
            .value(QStringLiteral("rooms"))
            .toBool();
    }
    // No adapter row (offline / picker not yet fetched): do not hide the action palette entirely —
    // the node still validates every verb server-side.
    return true;
}

void RoomFlow::showRolePalette(const QVariantMap& member) {
    const QString contactId = member.value(QStringLiteral("contactId")).toString();
    if (m_rolePick == nullptr) {
        m_rolePick = new PaletteDialog(tr("Change role"), m_host);
        connect(m_rolePick, &PaletteDialog::canceled, this, &RoomFlow::finished);
    }
    disconnect(m_rolePick, &PaletteDialog::activated, this, nullptr);
    const QString transport = m_transport;
    const QString conv = m_conversation;
    connect(m_rolePick, &PaletteDialog::activated, this,
            [this, transport, conv, contactId](const QString& role) {
                m_registry->memberSetRole(transport, conv, contactId, role);
                emit changed();
                emit finished();
            });
    QVector<PaletteDialog::Item> items;
    for (const QString& role :
         {QStringLiteral("None"), QStringLiteral("Voice"), QStringLiteral("HalfOp"),
          QStringLiteral("Op"), QStringLiteral("Founder")}) {
        items.push_back({role, role, QString()});
    }
    m_rolePick->setItems(items);
    m_rolePick->openCentered();
}
