// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transports/ichat_service.h"

namespace daemonapp::daemon {
class ChatRepository;
}

namespace transports {

// [integrations wire v38] Daemon-backed native-chat seam. Projects the ChatRepository's node data
// (ConvHistory -> ChatMessage) into the IChatService message rows the chat tab renders, and
// forwards refreshHistory/loadOlder/send. Re-projects/re-emits on the repository's historyRefreshed
// signal; the incremental refresh piggybacks the node's MessagesChanged feed event (wired in the
// service graph, via ChatRepository::applyMessagesChanged).
//
// Lives under src/core/daemon (not transports/) because it depends on the daemon ChatRepository,
// which links the transports interface — keeping it here avoids a library cycle (cf.
// DaemonContactsService).
class DaemonChatService : public IChatService {
    Q_OBJECT

public:
    explicit DaemonChatService(daemonapp::daemon::ChatRepository* repo, QObject* parent = nullptr);

    [[nodiscard]] QVariantList messages(const QString& transport,
                                        const QString& conv) const override;
    void refreshHistory(const QString& transport, const QString& conv) override;
    void send(const QString& transport, const QString& conv, const QString& text) override;

private:
    daemonapp::daemon::ChatRepository* m_repo = nullptr;
};

} // namespace transports
