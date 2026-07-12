// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_chat_service.h"

#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"

namespace transports {

namespace {

// Project one DecodedChatMessage into the display-only transcript row the surfaces render.
QVariantMap messageToVariant(const daemonapp::daemon::DecodedChatMessage& m) {
    QVariantMap row;
    row[QStringLiteral("cursor")] = static_cast<qulonglong>(m.cursor);
    row[QStringLiteral("id")] = m.id;
    row[QStringLiteral("authorId")] = m.authorId;
    row[QStringLiteral("authorName")] = m.authorName;
    row[QStringLiteral("authorIsAgent")] = m.authorIsAgent;
    row[QStringLiteral("replyingTo")] = m.replyingTo;
    row[QStringLiteral("text")] = m.text;
    row[QStringLiteral("timestamp")] =
        m.hasTimestamp ? QVariant(static_cast<qulonglong>(m.timestamp)) : QVariant();
    row[QStringLiteral("editedAt")] =
        m.hasEditedAt ? QVariant(static_cast<qulonglong>(m.editedAt)) : QVariant();
    row[QStringLiteral("error")] = m.error;
    row[QStringLiteral("title")] = m.title;
    row[QStringLiteral("system")] = m.system;
    row[QStringLiteral("notice")] = m.notice;
    row[QStringLiteral("action")] = m.action;
    row[QStringLiteral("event")] = m.event;
    row[QStringLiteral("highlighted")] = m.highlighted;
    return row;
}

QVariantList messagesToVariant(const QList<daemonapp::daemon::DecodedChatMessage>& msgs) {
    QVariantList out;
    out.reserve(msgs.size());
    for (const daemonapp::daemon::DecodedChatMessage& m : msgs) {
        out.append(messageToVariant(m));
    }
    return out;
}

} // namespace

DaemonChatService::DaemonChatService(daemonapp::daemon::ChatRepository* repo, QObject* parent)
    : IChatService(parent), m_repo(repo) {
    if (m_repo == nullptr) {
        return;
    }
    connect(m_repo, &daemonapp::daemon::ChatRepository::historyRefreshed, this,
            [this](const QString& transport, const QString& conv) {
                emit messagesChanged(transport, conv,
                                     messagesToVariant(m_repo->messages(transport, conv)));
            });
    connect(m_repo, &daemonapp::daemon::ChatRepository::operationFailed, this,
            &IChatService::sendFailed);
}

QVariantList DaemonChatService::messages(const QString& transport, const QString& conv) const {
    if (m_repo == nullptr) {
        return {};
    }
    return messagesToVariant(m_repo->messages(transport, conv));
}

void DaemonChatService::refreshHistory(const QString& transport, const QString& conv) {
    if (m_repo != nullptr) {
        m_repo->refreshHistory(transport, conv);
    }
}

void DaemonChatService::send(const QString& transport, const QString& conv, const QString& text) {
    if (m_repo != nullptr) {
        m_repo->send(transport, conv, text);
    }
}

} // namespace transports
