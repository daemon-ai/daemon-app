// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "transports/mock_transport_registry.h"

#include <QVariantList>
#include <QVariantMap>

namespace transports {

namespace {

constexpr auto kMockTransport = "matrix/@you:matrix.org";

QVariantMap adapterRow(const QString& family, const QString& displayName, bool rooms, bool dms,
                       bool presence, bool interactiveAuth) {
    QVariantMap caps;
    caps.insert(QStringLiteral("rooms"), rooms);
    caps.insert(QStringLiteral("directMessages"), dms);
    caps.insert(QStringLiteral("presence"), presence);
    caps.insert(QStringLiteral("interactiveAuth"), interactiveAuth);

    QVariantMap row;
    row.insert(QStringLiteral("family"), family);
    row.insert(QStringLiteral("displayName"), displayName);
    row.insert(QStringLiteral("capabilities"), caps);
    row.insert(QStringLiteral("schema"), QVariantList{});
    return row;
}

// [acct-mgmt] Per-verb ops maps (wire v33) for the canned families, so offline dev exercises the
// real gating paths (keys follow the daemon registry's camelCase convention).
QVariantMap conversationOps(bool create, bool joinChannel, bool leave, bool del) {
    QVariantMap ops;
    ops.insert(QStringLiteral("create"), create);
    ops.insert(QStringLiteral("joinChannel"), joinChannel);
    ops.insert(QStringLiteral("leave"), leave);
    ops.insert(QStringLiteral("delete"), del);
    ops.insert(QStringLiteral("send"), true);
    ops.insert(QStringLiteral("setTopic"), false);
    ops.insert(QStringLiteral("setTitle"), false);
    ops.insert(QStringLiteral("setDescription"), false);
    return ops;
}

QVariantMap membershipOps(bool invite, bool remove, bool ban, bool setRole) {
    QVariantMap ops;
    ops.insert(QStringLiteral("invite"), invite);
    ops.insert(QStringLiteral("remove"), remove);
    ops.insert(QStringLiteral("ban"), ban);
    ops.insert(QStringLiteral("setRole"), setRole);
    return ops;
}

QVariantMap roomRow(const QString& transport, const QString& id, const QString& title,
                    const QString& kind) {
    QVariantMap m;
    m.insert(QStringLiteral("transport"), transport);
    m.insert(QStringLiteral("id"), id);
    m.insert(QStringLiteral("title"), title);
    m.insert(QStringLiteral("topic"), QString());
    m.insert(QStringLiteral("kind"), kind);
    return m;
}

QVariantMap memberRow(const QString& id, const QString& displayName, const QString& presence,
                      const QString& role) {
    QVariantMap m;
    m.insert(QStringLiteral("contactId"), id);
    m.insert(QStringLiteral("displayName"), displayName);
    m.insert(QStringLiteral("presence"), presence);
    m.insert(QStringLiteral("permission"), QStringLiteral("allow"));
    m.insert(QStringLiteral("alias"), QString());
    m.insert(QStringLiteral("nickname"), QString());
    m.insert(QStringLiteral("typing"), QStringLiteral("none"));
    m.insert(QStringLiteral("role"), role);
    m.insert(QStringLiteral("session"), QString());
    return m;
}

} // namespace

MockTransportRegistry::MockTransportRegistry(QObject* parent) : ITransportRegistry(parent) {
    const QString t = QLatin1String(kMockTransport);
    m_rooms[t] = QVariantList{
        roomRow(t, QStringLiteral("!daemon-dev:matrix.org"), QStringLiteral("daemon-dev"),
                QStringLiteral("channel")),
        roomRow(t, QStringLiteral("!design:matrix.org"), QStringLiteral("design"),
                QStringLiteral("channel")),
    };
    m_members[QStringLiteral("!daemon-dev:matrix.org")] = QList<QVariantMap>{
        memberRow(QStringLiteral("@bob:matrix.org"), QStringLiteral("Bob"),
                  QStringLiteral("available"), QStringLiteral("Op")),
        memberRow(QStringLiteral("@carol:matrix.org"), QStringLiteral("Carol"),
                  QStringLiteral("offline"), QStringLiteral("None")),
    };
}

QVariantList MockTransportRegistry::availableAdapters() const {
    // [acct-mgmt] Matrix advertises the full room/member verb set; the internal Rooms loopback a
    // deliberately narrower one (no ban / no role changes, no room delete) so the per-verb gating
    // visibly differs between the two canned families in offline dev.
    QVariantMap matrix =
        adapterRow(QStringLiteral("matrix"), QStringLiteral("Matrix"), true, true, true, true);
    matrix.insert(QStringLiteral("conversationOps"), conversationOps(true, true, true, true));
    matrix.insert(QStringLiteral("membershipOps"), membershipOps(true, true, true, true));
    QVariantMap rooms = adapterRow(QStringLiteral("room"), QStringLiteral("Rooms (internal)"), true,
                                   true, false, false);
    rooms.insert(QStringLiteral("conversationOps"), conversationOps(true, true, true, false));
    rooms.insert(QStringLiteral("membershipOps"), membershipOps(true, true, false, false));
    return QVariantList{matrix, rooms};
}

QVariantList MockTransportRegistry::instances() const {
    QVariantMap row;
    row.insert(QStringLiteral("transport"), QLatin1String(kMockTransport));
    row.insert(QStringLiteral("family"), QStringLiteral("matrix"));
    row.insert(QStringLiteral("displayName"), QStringLiteral("@you:matrix.org"));
    row.insert(QStringLiteral("boundProfile"), QString());
    row.insert(QStringLiteral("connectionReason"), QString());
    row.insert(QStringLiteral("connectionMessage"), QString());
    row.insert(QStringLiteral("fatal"), false);
    return QVariantList{row};
}

QVariantList MockTransportRegistry::conversations(const QString& transport) const {
    return m_rooms.value(transport);
}

void MockTransportRegistry::refreshConversations(const QString& transport) {
    emit conversationsChanged(transport);
}

QVariantList MockTransportRegistry::membersFor(const QString& conversation) const {
    QVariantList out;
    for (const QVariantMap& m : m_members.value(conversation)) {
        out.append(m);
    }
    return out;
}

void MockTransportRegistry::conversationJoinDetails(const QString& transport) {
    QVariantMap form;
    form.insert(QStringLiteral("nameMaxLength"), 64);
    form.insert(QStringLiteral("nicknameSupported"), true);
    form.insert(QStringLiteral("nicknameMaxLength"), 32);
    form.insert(QStringLiteral("passwordSupported"), true);
    form.insert(QStringLiteral("passwordMaxLength"), 0);
    QVariantMap field;
    field.insert(QStringLiteral("key"), QStringLiteral("floor_policy"));
    field.insert(QStringLiteral("label"), QStringLiteral("Floor policy"));
    field.insert(QStringLiteral("required"), false);
    form.insert(QStringLiteral("extras"), QVariantList{field});
    emit joinDetailsReady(transport, form);
}

void MockTransportRegistry::conversationCreateDetails(const QString& transport) {
    QVariantMap form;
    form.insert(QStringLiteral("maxParticipants"), 0);
    QVariantMap field;
    field.insert(QStringLiteral("key"), QStringLiteral("room_name"));
    field.insert(QStringLiteral("label"), QStringLiteral("Room name"));
    field.insert(QStringLiteral("required"), true);
    form.insert(QStringLiteral("extras"), QVariantList{field});
    emit createDetailsReady(transport, form);
}

void MockTransportRegistry::joinRoom(const QString& transport, const QVariantMap& form) {
    const QString name = form.value(QStringLiteral("name")).toString();
    if (!name.isEmpty()) {
        const QString id = QStringLiteral("!%1:matrix.org").arg(name);
        m_rooms[transport].append(roomRow(transport, id, name, QStringLiteral("channel")));
    }
    emit conversationsChanged(transport);
}

void MockTransportRegistry::createRoom(const QString& transport, const QVariantMap& form) {
    const QVariantMap extras = form.value(QStringLiteral("extras")).toMap();
    const QString name = extras.value(QStringLiteral("room_name")).toString();
    if (!name.isEmpty()) {
        const QString id = QStringLiteral("!%1:matrix.org").arg(name);
        m_rooms[transport].append(roomRow(transport, id, name, QStringLiteral("channel")));
    }
    emit conversationsChanged(transport);
}

void MockTransportRegistry::leaveRoom(const QString& transport, const QString& conversation) {
    QVariantList& rooms = m_rooms[transport];
    for (int i = 0; i < rooms.size(); ++i) {
        if (rooms.at(i).toMap().value(QStringLiteral("id")).toString() == conversation) {
            rooms.removeAt(i);
            break;
        }
    }
    emit conversationsChanged(transport);
}

void MockTransportRegistry::deleteRoom(const QString& transport, const QString& conversation) {
    leaveRoom(transport, conversation);
}

void MockTransportRegistry::conversationMembers(const QString& transport,
                                                const QString& conversation) {
    emit membersChanged(transport, conversation, membersFor(conversation));
}

void MockTransportRegistry::memberInvite(const QString& transport, const QString& conversation,
                                         const QString& contactId) {
    m_members[conversation].append(
        memberRow(contactId, QString(), QStringLiteral("offline"), QStringLiteral("None")));
    emit membersChanged(transport, conversation, membersFor(conversation));
}

void MockTransportRegistry::memberKick(const QString& transport, const QString& conversation,
                                       const QString& contactId) {
    QList<QVariantMap>& members = m_members[conversation];
    for (int i = 0; i < members.size(); ++i) {
        if (members.at(i).value(QStringLiteral("contactId")).toString() == contactId) {
            members.removeAt(i);
            break;
        }
    }
    emit membersChanged(transport, conversation, membersFor(conversation));
}

void MockTransportRegistry::memberBan(const QString& transport, const QString& conversation,
                                      const QString& contactId) {
    m_banned[conversation].insert(contactId);
    memberKick(transport, conversation, contactId);
}

void MockTransportRegistry::memberSetRole(const QString& transport, const QString& conversation,
                                          const QString& contactId, const QString& role) {
    QList<QVariantMap>& members = m_members[conversation];
    for (QVariantMap& m : members) {
        if (m.value(QStringLiteral("contactId")).toString() == contactId) {
            m.insert(QStringLiteral("role"), role);
            break;
        }
    }
    emit membersChanged(transport, conversation, membersFor(conversation));
}

} // namespace transports
