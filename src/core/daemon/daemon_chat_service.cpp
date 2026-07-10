// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_chat_service.h"

#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"

namespace transports {

// ===== STUB (red) — implemented in the green commit =====

DaemonChatService::DaemonChatService(daemonapp::daemon::ChatRepository* repo, QObject* parent)
    : IChatService(parent), m_repo(repo) {}

QVariantList DaemonChatService::messages(const QString& transport, const QString& conv) const {
    Q_UNUSED(transport)
    Q_UNUSED(conv)
    return {};
}

void DaemonChatService::refreshHistory(const QString& transport, const QString& conv) {
    Q_UNUSED(transport)
    Q_UNUSED(conv)
}

void DaemonChatService::send(const QString& transport, const QString& conv, const QString& text) {
    Q_UNUSED(transport)
    Q_UNUSED(conv)
    Q_UNUSED(text)
}

} // namespace transports
