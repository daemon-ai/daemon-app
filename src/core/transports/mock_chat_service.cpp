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
    Q_UNUSED(transport)
    Q_UNUSED(conv)
}

void MockChatService::send(const QString& transport, const QString& conv, const QString& text) {
    Q_UNUSED(transport)
    Q_UNUSED(conv)
    Q_UNUSED(text)
}

void MockChatService::seed(const QString& transport, const QString& conv,
                           const QVariantList& messages) {
    m_messages.insert(key(transport, conv), messages);
}

} // namespace transports
