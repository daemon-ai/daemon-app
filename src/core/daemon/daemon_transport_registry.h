// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transports/itransport_registry.h"

namespace daemonapp::daemon {
class TransportRepository;
}

namespace transports {

// Daemon-backed, offline-first transport-adapter registry (Phase 6a, story 04). Projects the
// TransportRepository's node data into the ITransportRegistry rows the Channels surface renders:
// availableAdapters() from the live TransportAdapters list (the "Add channel" picker -
// connect-only, empty offline), instances() from the offline-first daemon_transport_instances cache
// (configured accounts, render with no connection). Rebuilds/re-emits on the repository's refresh
// signals.
//
// Lives under src/core/daemon (not transports/) because it depends on the daemon
// TransportRepository, which links the transports interface - keeping it here avoids a library
// cycle (cf. MirrorFleetTree).
class DaemonTransportRegistry : public ITransportRegistry {
    Q_OBJECT

public:
    explicit DaemonTransportRegistry(daemonapp::daemon::TransportRepository* repo,
                                     QObject* parent = nullptr);

    [[nodiscard]] QVariantList availableAdapters() const override;
    [[nodiscard]] QVariantList instances() const override;
    [[nodiscard]] QVariantList conversations(const QString& transport) const override;
    void refreshConversations(const QString& transport) override;
    // [waveB:app-v30] D1: per-instance teardown intents, delegated to the repository.
    void disconnect(const QString& transport) override;
    void remove(const QString& transport) override;
    // [acct-mgmt] wire v35: reversible lifecycle + persisted metadata, delegated to the repository.
    void connectAccount(const QString& transport) override;
    void setEnabled(const QString& transport, bool enabled) override;
    void setLabel(const QString& transport, const QString& label) override;
    // [wave2:app-channels-liveness] B2: "new room" affordance, delegated to the repository.
    [[nodiscard]] bool isNewConversation(const QString& transport,
                                         const QString& conversation) const override;
    void markConversationSeen(const QString& transport, const QString& conversation) override;

    // [acct-mgmt] Room lifecycle + member management, delegated to the repository (whose refresh
    // signals + the joinDetailsReady/createDetailsReady/membersChanged forwarders below drive both
    // surfaces). Refresh piggybacks the node's ConversationsChanged / MembershipChanged events.
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
    // [integrations wire v38] Account settings read + configure, delegated to the repository (whose
    // settingsRefreshed drives settingsChanged).
    [[nodiscard]] QVariantMap settings(const QString& transport) const override;
    void refreshSettings(const QString& transport) override;
    void configure(const QString& transport, const QVariantMap& values) override;

private:
    daemonapp::daemon::TransportRepository* m_repo = nullptr;
};

} // namespace transports
