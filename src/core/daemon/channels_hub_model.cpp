// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/channels_hub_model.h"

#include "mirror/generated/entities_gen.h"
#include "mirror/store.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace daemonapp::daemon {

namespace {
constexpr auto kConsumer = "channels_hub";

// Re-hydrate one canonical-JSON column into the registry-row QVariant shape ({} when empty).
QVariantMap jsonObjectColumn(const QString& json) {
    if (json.isEmpty()) {
        return {};
    }
    return QJsonDocument::fromJson(json.toUtf8()).object().toVariantMap();
}

QVariantList jsonArrayColumn(const QString& json) {
    if (json.isEmpty()) {
        return {};
    }
    return QJsonDocument::fromJson(json.toUtf8()).array().toVariantList();
}
} // namespace

ChannelsHubModel::ChannelsHubModel(mirror::Store* mirror, QObject* parent)
    : QObject(parent), m_mirror(mirror) {
    if (m_mirror != nullptr) {
        m_mirror->journal().registerConsumer(QLatin1String(kConsumer));
        m_watermark = m_mirror->journal().headRev();
        m_mirror->journal().advanceWatermark(QLatin1String(kConsumer), m_watermark);
        connect(m_mirror, &mirror::Store::committed, this, &ChannelsHubModel::onCommitted);
        trackNewConversations(); // seed the baseline from whatever the boot load restored
        m_newConvs.clear();      // a reloaded conversation is never "new"
    }
}

void ChannelsHubModel::onCommitted() {
    const auto deltas = m_mirror->journal().since(m_watermark);
    m_watermark = m_mirror->journal().headRev();
    m_mirror->journal().advanceWatermark(QLatin1String(kConsumer), m_watermark);

    bool adapters = false;
    bool accounts = false;
    bool pins = false;
    QSet<QString> convTransports;
    QSet<QString> contactTransports;
    for (const auto& r : deltas) {
        switch (r.kind) {
        case mirror::EntityKind::Adapter:
            adapters = true;
            break;
        case mirror::EntityKind::TransportAccount:
            accounts = true;
            break;
        case mirror::EntityKind::Conversation:
            convTransports.insert(r.key.section(QChar(0x1f), 0, 0));
            break;
        case mirror::EntityKind::Contact:
            contactTransports.insert(r.key.section(QChar(0x1f), 0, 0));
            break;
        case mirror::EntityKind::RoutePin:
            pins = true;
            break;
        default:
            break;
        }
    }
    if (!convTransports.isEmpty()) {
        trackNewConversations();
    }
    if (adapters) {
        emit adaptersChanged();
    }
    if (accounts) {
        emit accountsChanged();
    }
    for (const QString& t : convTransports) {
        emit conversationsChanged(t);
    }
    for (const QString& t : contactTransports) {
        emit contactsChanged(t);
    }
    if (pins) {
        emit pinsChanged();
    }
}

void ChannelsHubModel::trackNewConversations() {
    // Per-transport diff against the last-known id set: the FIRST commit for a transport is the
    // operator's baseline (nothing is "new"); later additions surface as new until seen.
    QHash<QString, QSet<QString>> current;
    for (const mirror::Conversation& c : m_mirror->snapshot().conversations) {
        current[c.transport].insert(c.id);
    }
    for (auto it = current.constBegin(); it != current.constEnd(); ++it) {
        const QSet<QString>& known = m_knownConvs.value(it.key());
        if (m_baselined.contains(it.key())) {
            for (const QString& id : it.value()) {
                if (!known.contains(id)) {
                    m_newConvs[it.key()].insert(id);
                }
            }
        } else {
            m_baselined.insert(it.key());
        }
        // Rooms that left the listing drop from the "new" set too (never a ghost badge).
        if (auto nit = m_newConvs.find(it.key()); nit != m_newConvs.end()) {
            nit->intersect(it.value());
        }
    }
    m_knownConvs = std::move(current);
}

QVariantList ChannelsHubModel::adapters() const {
    QVariantList out;
    if (m_mirror == nullptr) {
        return out;
    }
    for (const mirror::Adapter& a : m_mirror->snapshot().adapters) {
        QVariantMap row;
        row.insert(QStringLiteral("family"), a.family);
        row.insert(QStringLiteral("displayName"), a.display_name);
        row.insert(QStringLiteral("capabilities"),
                   QVariantMap{{QStringLiteral("rooms"), a.cap_rooms},
                               {QStringLiteral("directMessages"), a.cap_direct_messages},
                               {QStringLiteral("fileTransfer"), a.cap_file_transfer}});
        row.insert(QStringLiteral("directory"), a.directory);
        // Per-verb ops: each key present ONLY when the node reported that map (ops_json carries
        // exactly the reported set) — QML's undefined-check fallback survives the port.
        const QVariantMap ops = jsonObjectColumn(a.ops_json);
        for (auto it = ops.constBegin(); it != ops.constEnd(); ++it) {
            row.insert(it.key(), it.value());
        }
        row.insert(QStringLiteral("schema"), jsonArrayColumn(a.schema_json));
        row.insert(QStringLiteral("policies"), jsonArrayColumn(a.policies_json));
        out.append(row);
    }
    return out;
}

QVariantList ChannelsHubModel::accounts() const {
    QVariantList out;
    if (m_mirror == nullptr) {
        return out;
    }
    for (const mirror::TransportAccount& t : m_mirror->snapshot().transport_accounts) {
        QVariantMap row;
        row.insert(QStringLiteral("transport"), t.transport);
        row.insert(QStringLiteral("family"), t.family);
        // wire v35: the node-persisted label overlays the display name; raw label for pre-fill.
        row.insert(QStringLiteral("displayName"), t.label.isEmpty() ? t.display_name : t.label);
        row.insert(QStringLiteral("label"), t.label);
        row.insert(QStringLiteral("enabled"), t.enabled);
        row.insert(QStringLiteral("boundProfile"), t.bound_profile);
        row.insert(QStringLiteral("connection"), t.connection);
        row.insert(QStringLiteral("presence"), t.presence);
        row.insert(QStringLiteral("connectionReason"), t.reason);
        row.insert(QStringLiteral("connectionMessage"), t.message);
        row.insert(QStringLiteral("fatal"), t.fatal);
        out.append(row);
    }
    return out;
}

QVariantList ChannelsHubModel::conversations(const QString& transport) const {
    QVariantList out;
    if (m_mirror == nullptr) {
        return out;
    }
    for (const mirror::Conversation& c : m_mirror->snapshot().conversations) {
        if (c.transport != transport) {
            continue;
        }
        QVariantMap row;
        row.insert(QStringLiteral("transport"), c.transport);
        row.insert(QStringLiteral("id"), c.id);
        row.insert(QStringLiteral("kind"), c.kind);
        row.insert(QStringLiteral("title"), c.title);
        row.insert(QStringLiteral("topic"), c.topic);
        row.insert(QStringLiteral("parent"), c.parent);
        out.append(row);
    }
    return out;
}

QVariantList ChannelsHubModel::contacts(const QString& transport) const {
    QVariantList out;
    if (m_mirror == nullptr) {
        return out;
    }
    for (const mirror::Contact& c : m_mirror->snapshot().contacts) {
        if (c.transport != transport) {
            continue;
        }
        QVariantMap row;
        row.insert(QStringLiteral("id"), c.id);
        row.insert(QStringLiteral("displayName"), c.display_name);
        row.insert(QStringLiteral("presence"), c.presence_primitive);
        row.insert(QStringLiteral("permission"), c.permission);
        out.append(row);
    }
    return out;
}

QVariantList ChannelsHubModel::pinsForSession(const QString& sessionId) const {
    QVariantList rows;
    if (m_mirror == nullptr) {
        return rows;
    }
    for (const mirror::RoutePin& p : m_mirror->snapshot().route_pins) {
        if (p.session != sessionId) {
            continue;
        }
        // origin_key formats: `t|dm|user`, `t|group|chat|thread`, `t|api|key`, `t|internal`.
        const QStringList parts = p.origin_key.split(QLatin1Char('|'));
        const QString kind = parts.value(1);
        QString label;
        if (kind == QStringLiteral("dm")) {
            label = QStringLiteral("@") + parts.value(2);
        } else if (kind == QStringLiteral("group")) {
            label = parts.value(2);
        } else if (kind == QStringLiteral("api")) {
            label = QStringLiteral("api:") + parts.value(2);
        } else {
            label = QStringLiteral("internal");
        }
        rows.append(QVariantMap{{QStringLiteral("key"), p.origin_key},
                                {QStringLiteral("transport"), p.transport},
                                {QStringLiteral("label"), label}});
    }
    return rows;
}

bool ChannelsHubModel::isNewConversation(const QString& transport,
                                         const QString& conversation) const {
    const auto it = m_newConvs.constFind(transport);
    return it != m_newConvs.constEnd() && it->contains(conversation);
}

void ChannelsHubModel::markConversationSeen(const QString& transport, const QString& conversation) {
    const auto it = m_newConvs.find(transport);
    if (it != m_newConvs.end()) {
        it->remove(conversation);
    }
}

} // namespace daemonapp::daemon
