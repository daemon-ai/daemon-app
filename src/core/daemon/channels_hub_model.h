// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// AD (1a.3) — the SHARED channels-hub view-model: the ONE mirror projection the GUI ChannelsPage
// (context property `ChannelsHub`) and the TUI channels hub page both render. Replaces the
// ITransportRegistry/IPresenceService/IContactsService READS those surfaces composed from (the
// registry/contacts seams survive as VERB sinks only): accounts, adapters (capabilities +
// per-verb ops + schema + policies re-hydrated from the canonical JSON columns), per-transport
// conversations and contacts — all read from the mirror snapshot, identical in daemon (ingestor-
// fed) and mock (scenario-seeded) modes (§9).
//
// The "new room" affordance (B2) is CLIENT-LOCAL view state (§8.5) and lives here: the first
// conversations commit per transport is the operator's baseline; a conversation appearing after
// it reads back "new" until marked seen. (Its previous home, the TransportRepository read path,
// dies with the tree/hub port.)

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace mirror {
class Store;
} // namespace mirror

namespace daemonapp::daemon {

class ChannelsHubModel : public QObject {
    Q_OBJECT

public:
    explicit ChannelsHubModel(mirror::Store* mirror = nullptr, QObject* parent = nullptr);

    // The adapter catalog rows, shaped exactly like the deleted registry rows: {family,
    // displayName, capabilities{rooms,directMessages,fileTransfer}, directory,
    // conversationOps?, membershipOps?, contactsOps?, rosterOps?, schema, policies}. A per-verb
    // ops key is present ONLY when the node reported a concrete map (ops_json carries it), so
    // QML keeps the "undefined ⇒ coarse capability fallback" distinction.
    [[nodiscard]] Q_INVOKABLE QVariantList adapters() const;
    // The configured accounts: {transport, family, displayName (label overlaid), label, enabled,
    // boundProfile, connection, presence, connectionReason, connectionMessage, fatal}.
    [[nodiscard]] Q_INVOKABLE QVariantList accounts() const;
    // A transport's conversations: {transport, id, kind, title, topic, parent}. `kind` uses the
    // MIRROR vocabulary ("dm" | "group_dm" | "channel" | "thread" | "space").
    [[nodiscard]] Q_INVOKABLE QVariantList conversations(const QString& transport) const;
    // A transport's roster contacts: {id, displayName, presence, permission}.
    [[nodiscard]] Q_INVOKABLE QVariantList contacts(const QString& transport) const;

    // B2 "new room" (client-local §8.5): true for a conversation that surfaced after the
    // operator's baseline for its transport and has not been viewed yet.
    [[nodiscard]] Q_INVOKABLE bool isNewConversation(const QString& transport,
                                                     const QString& conversation) const;
    Q_INVOKABLE void markConversationSeen(const QString& transport, const QString& conversation);

signals:
    void adaptersChanged();
    void accountsChanged();
    void conversationsChanged(const QString& transport);
    void contactsChanged(const QString& transport);

private:
    void onCommitted();
    // Track per-transport conversation ids to derive the baseline + "new" set.
    void trackNewConversations();

    mirror::Store* m_mirror = nullptr;
    quint64 m_watermark = 0;
    QHash<QString, QSet<QString>> m_knownConvs; // transport → last-seen id set (baseline+deltas)
    QHash<QString, QSet<QString>> m_newConvs;   // transport → surfaced-after-baseline, unseen
    QSet<QString> m_baselined;                  // transports whose first commit has landed
};

} // namespace daemonapp::daemon
