// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transports/itransport_registry.h"

#include <QHash>
#include <QSet>

namespace transports {

// Inert-ish dev stand-in: advertises the two adapter families that exist today (Matrix, the
// internal Rooms loopback) for the "Add channel" picker and, so the room-lifecycle UI can be built
// and exercised offline (UI-first), carries one canned Matrix account with a couple of rooms +
// members. The room lifecycle + member verbs mutate this in-memory state and emit the same change
// signals the daemon registry does, so both surfaces render the flows without a live node. Replaced
// by DaemonTransportRegistry (decoding the node's ops) whenever a daemon connects.
class MockTransportRegistry : public ITransportRegistry {
    Q_OBJECT

public:
    explicit MockTransportRegistry(QObject* parent = nullptr);

    [[nodiscard]] QVariantList availableAdapters() const override;
    [[nodiscard]] QVariantList instances() const override;
    [[nodiscard]] QVariantList conversations(const QString& transport) const override;
    void refreshConversations(const QString& transport) override;

    // [acct-mgmt] wire v35: reversible lifecycle + persisted metadata over the canned account, so
    // the connect / enable-toggle / rename paths are exercisable offline (UI-first).
    void connectAccount(const QString& transport) override;
    void setEnabled(const QString& transport, bool enabled) override;
    void setLabel(const QString& transport, const QString& label) override;

    // [acct-mgmt] Room lifecycle + member management over the canned state.
    void conversationJoinDetails(const QString& transport) override;
    void joinRoom(const QString& transport, const QVariantMap& form) override;
    void conversationCreateDetails(const QString& transport) override;
    void createRoom(const QString& transport, const QVariantMap& form) override;
    void leaveRoom(const QString& transport, const QString& conversation) override;
    void deleteRoom(const QString& transport, const QString& conversation) override;
    void conversationMembers(const QString& transport, const QString& conversation) override;
    void memberInvite(const QString& transport, const QString& conversation,
                      const QString& contactId) override;
    void memberKick(const QString& transport, const QString& conversation,
                    const QString& contactId) override;
    void memberBan(const QString& transport, const QString& conversation,
                   const QString& contactId) override;
    void memberSetRole(const QString& transport, const QString& conversation,
                       const QString& contactId, const QString& role) override;

private:
    [[nodiscard]] QVariantList membersFor(const QString& conversation) const;

    // Per-account canned rooms; per-room canned members (contactId -> role). A tiny, deterministic
    // fixture — this is a dev stand-in, not a real store.
    QHash<QString, QVariantList> m_rooms;         // transport -> room maps
    QHash<QString, QList<QVariantMap>> m_members; // conversation id -> member rows
    QHash<QString, QSet<QString>> m_banned;       // conversation id -> banned ids
    // [acct-mgmt] wire v35: mutable enabled/label for the single canned account.
    bool m_enabled = true;
    QString m_label;
};

} // namespace transports
