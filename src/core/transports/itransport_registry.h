// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace transports {

// Transport-adapter registry seam (daemon-transport-adapter-spec.md): the client-side view of the
// node's events-IO transport adapters (Matrix, the internal Rooms loopback, HTTP, a future A2A).
// `availableAdapters()` backs the multi-protocol "Add channel" picker (family, display name,
// capability flags, account-setup schema) — the Pidgin/Kopete/Adium service-picker pattern;
// `instances()` lists the configured accounts the unified conversation roster groups by. A daemon
// adapter later decodes the node's `transport_adapters` / `transport_instances` ops over the wire
// and emits the same rows; the UI never sees the codec. The mock returns canned data so the surface
// can be built UI-first.
//
// Skeleton: the live per-account connection/presence state is owned by IPresenceService (sibling
// seam); this registry carries only identity + capabilities + binding.
class ITransportRegistry : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~ITransportRegistry() override = default;

    // Available adapter families for the "Add channel" picker. Each entry is a map:
    //   family (QString), displayName (QString), capabilities (QVariantMap of bool flags),
    //   schema (QVariantList of field maps: key, label, required).
    // [acct-mgmt] Per-verb ops (wire v33): rows MAY also carry conversationOps (create/joinChannel/
    // leave/delete/send/setTopic/setTitle/setDescription -> bool), membershipOps (invite/remove/
    // ban/setRole), contactsOps (getProfile/actionMenu/setAlias), rosterOps (add/update/remove)
    // and directory (bool). Each key is present ONLY when the node reported a concrete ops map —
    // a missing key (v32 node, or the adapter reported null) means "no per-verb info" and the UI
    // falls back to the coarse capability heuristic; a present map is authoritative.
    [[nodiscard]] Q_INVOKABLE virtual QVariantList availableAdapters() const = 0;

    // Configured transport instances (accounts). Each entry is a map:
    //   transport (QString), family (QString), displayName (QString), boundProfile (QString).
    // [waveB:app-v30] D1: rows also carry connectionReason (coarse token), connectionMessage (the
    // node's verbatim human string), and fatal (bool: gates the re-auth affordance).
    [[nodiscard]] Q_INVOKABLE virtual QVariantList instances() const = 0;

    // [waveB:app-v30] D1: per-instance teardown. `disconnect` tears down the live session;
    // `remove` fully removes the account (the node sequences the full teardown). Default no-ops so
    // the mock + non-daemon seams need not implement them.
    Q_INVOKABLE virtual void disconnect(const QString& transport) { Q_UNUSED(transport) }
    Q_INVOKABLE virtual void remove(const QString& transport) { Q_UNUSED(transport) }

    // Live conversations/rooms for a transport (EIO-8). Each entry is a map:
    //   transport, id, kind, title, topic. `conversations()` returns the last-known (cached) set;
    //   `refreshConversations()` triggers a live enumeration (ConvList) and fires
    //   conversationsChanged when it lands. Default no-ops so the mock + non-daemon seams need not
    //   implement them.
    [[nodiscard]] Q_INVOKABLE virtual QVariantList conversations(const QString& transport) const {
        Q_UNUSED(transport)
        return {};
    }
    Q_INVOKABLE virtual void refreshConversations(const QString& transport) { Q_UNUSED(transport) }

    // [wave2:app-channels-liveness] B2: presentation-only "new room" affordance. A conversation
    // that surfaced (via ConvList) after the operator's baseline for its transport — e.g. an
    // auto-accepted invite the node joined — reads back true until viewed. Membership itself is the
    // node's; this only tracks what the operator has already seen. Default no-op/false so the mock
    // and non-daemon seams need not implement it.
    [[nodiscard]] Q_INVOKABLE virtual bool isNewConversation(const QString& transport,
                                                             const QString& conversation) const {
        Q_UNUSED(transport)
        Q_UNUSED(conversation)
        return false;
    }
    Q_INVOKABLE virtual void markConversationSeen(const QString& transport,
                                                  const QString& conversation) {
        Q_UNUSED(transport)
        Q_UNUSED(conversation)
    }

    // [acct-mgmt] Room lifecycle + member management (Phase B). Two-phase form ops:
    // conversationJoinDetails / conversationCreateDetails fetch the node-described form and fire
    // joinDetailsReady / createDetailsReady; joinRoom / createRoom send the filled form.
    // Single-verb ops (leaveRoom / deleteRoom) name the conversation; conversationMembers issues
    // ConvGet and fires membersChanged; the member verbs act and re-hydrate the member list.
    // Refresh piggybacks the existing ConversationsChanged / MembershipChanged events — no polling.
    // Default no-ops so the mock + non-daemon seams need not implement every verb.
    Q_INVOKABLE virtual void conversationJoinDetails(const QString& transport) {
        Q_UNUSED(transport)
    }
    Q_INVOKABLE virtual void joinRoom(const QString& transport, const QVariantMap& form) {
        Q_UNUSED(transport)
        Q_UNUSED(form)
    }
    Q_INVOKABLE virtual void conversationCreateDetails(const QString& transport) {
        Q_UNUSED(transport)
    }
    Q_INVOKABLE virtual void createRoom(const QString& transport, const QVariantMap& form) {
        Q_UNUSED(transport)
        Q_UNUSED(form)
    }
    Q_INVOKABLE virtual void leaveRoom(const QString& transport, const QString& conversation) {
        Q_UNUSED(transport)
        Q_UNUSED(conversation)
    }
    Q_INVOKABLE virtual void deleteRoom(const QString& transport, const QString& conversation) {
        Q_UNUSED(transport)
        Q_UNUSED(conversation)
    }
    Q_INVOKABLE virtual void conversationMembers(const QString& transport,
                                                 const QString& conversation) {
        Q_UNUSED(transport)
        Q_UNUSED(conversation)
    }
    Q_INVOKABLE virtual void memberInvite(const QString& transport, const QString& conversation,
                                          const QString& contactId) {
        Q_UNUSED(transport)
        Q_UNUSED(conversation)
        Q_UNUSED(contactId)
    }
    Q_INVOKABLE virtual void memberKick(const QString& transport, const QString& conversation,
                                        const QString& contactId) {
        Q_UNUSED(transport)
        Q_UNUSED(conversation)
        Q_UNUSED(contactId)
    }
    Q_INVOKABLE virtual void memberBan(const QString& transport, const QString& conversation,
                                       const QString& contactId) {
        Q_UNUSED(transport)
        Q_UNUSED(conversation)
        Q_UNUSED(contactId)
    }
    Q_INVOKABLE virtual void memberSetRole(const QString& transport, const QString& conversation,
                                           const QString& contactId, const QString& role) {
        Q_UNUSED(transport)
        Q_UNUSED(conversation)
        Q_UNUSED(contactId)
        Q_UNUSED(role)
    }

signals:
    void adaptersChanged();
    void instancesChanged();
    void conversationsChanged(const QString& transport);
    // [acct-mgmt] Two-phase form descriptors + the member list for the member palette + a
    // room/member operation error (surfaced as a toast / TUI notice by both surfaces).
    void joinDetailsReady(const QString& transport, const QVariantMap& form);
    void createDetailsReady(const QString& transport, const QVariantMap& form);
    void membersChanged(const QString& transport, const QString& conversation,
                        const QVariantList& members);
    void roomOperationFailed(const QString& message);
};

} // namespace transports
