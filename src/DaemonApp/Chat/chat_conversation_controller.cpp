// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "chat_conversation_controller.h"

#include "transports/ichat_service.h"

// ===== STUB (red) — implemented in the green commit =====

ChatConversationController::ChatConversationController(QObject* parent) : QObject(parent) {}

QObject* ChatConversationController::service() const {
    return m_serviceObject;
}

void ChatConversationController::setService(QObject* service) {
    Q_UNUSED(service)
}

void ChatConversationController::open(const QString& transport, const QString& conversation) {
    Q_UNUSED(transport)
    Q_UNUSED(conversation)
}

void ChatConversationController::send(const QString& text) {
    Q_UNUSED(text)
}

QString ChatConversationController::projectMarkdown(const QVariantList& rows) {
    Q_UNUSED(rows)
    return {};
}

void ChatConversationController::onMessagesChanged(const QString& transport, const QString& conv,
                                                   const QVariantList& rows) {
    Q_UNUSED(transport)
    Q_UNUSED(conv)
    Q_UNUSED(rows)
}

void ChatConversationController::onServiceSendFailed(const QString& transport, const QString& conv,
                                                     const QString& message) {
    Q_UNUSED(transport)
    Q_UNUSED(conv)
    Q_UNUSED(message)
}

void ChatConversationController::applyRows(const QVariantList& rows) {
    Q_UNUSED(rows)
}
