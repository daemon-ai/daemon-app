// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "transports/mock_chat_service.h"

namespace transports {

// ===== STUB (red) — implemented in the green commit =====

QString MockChatService::key(const QString& transport, const QString& conv) {
    return transport + QChar(0x1f) + conv;
}

MockChatService::MockChatService(QObject* parent) : IChatService(parent) {}

QVariantList MockChatService::messages(const QString& transport, const QString& conv) const {
    return m_messages.value(key(transport, conv));
}

void MockChatService::refreshHistory(const QString& transport, const QString& conv) {
    // The mock has no node to fetch from — just re-emit the current (seeded/echoed) transcript.
    emit messagesChanged(transport, conv, m_messages.value(key(transport, conv)));
}

void MockChatService::send(const QString& transport, const QString& conv, const QString& text) {
    // The mock is its own authority (no node): append a local echo + re-emit.
    QVariantList& msgs = m_messages[key(transport, conv)];
    QVariantMap row;
    row[QStringLiteral("id")] = QStringLiteral("mock-%1").arg(msgs.size());
    row[QStringLiteral("authorId")] = QStringLiteral("me");
    row[QStringLiteral("authorName")] = QStringLiteral("Me");
    row[QStringLiteral("authorIsAgent")] = false;
    row[QStringLiteral("text")] = text;
    row[QStringLiteral("system")] = false;
    msgs.append(row);
    emit messagesChanged(transport, conv, msgs);
}

void MockChatService::seed(const QString& transport, const QString& conv,
                           const QVariantList& messages) {
    m_messages.insert(key(transport, conv), messages);
}

} // namespace transports
