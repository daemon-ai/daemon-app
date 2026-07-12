// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "chat_conversation_controller.h"

#include "transports/ichat_service.h"

#include <QStringList>

namespace {

QString fieldText(const QVariantMap& row, const char* key) {
    return row.value(QLatin1String(key)).toString();
}

bool fieldFlag(const QVariantMap& row, const char* key) {
    return row.value(QLatin1String(key)).toBool();
}

} // namespace

ChatConversationController::ChatConversationController(QObject* parent) : QObject(parent) {}

QObject* ChatConversationController::service() const {
    return m_serviceObject;
}

void ChatConversationController::setService(QObject* service) {
    if (m_serviceObject == service) {
        return;
    }
    if (m_service != nullptr) {
        disconnect(m_service, nullptr, this, nullptr);
    }
    m_serviceObject = service;
    m_service = qobject_cast<transports::IChatService*>(service);
    if (m_service != nullptr) {
        connect(m_service, &transports::IChatService::messagesChanged, this,
                &ChatConversationController::onMessagesChanged);
        connect(m_service, &transports::IChatService::sendFailed, this,
                &ChatConversationController::onServiceSendFailed);
    }
    emit serviceChanged();
}

void ChatConversationController::open(const QString& transport, const QString& conversation) {
    if (m_transport != transport || m_conversation != conversation) {
        m_transport = transport;
        m_conversation = conversation;
        emit conversationChanged();
        // Rebinding to a new conversation must not leak the prior transcript: clear
        // first, then project whatever the service already has cached.
        applyRows(m_service != nullptr ? m_service->messages(transport, conversation)
                                       : QVariantList());
    }
    // Ask the node for the full transcript (the v38 wire pages forward-only, so the
    // initial fetch is the whole history). The authoritative rows land via
    // messagesChanged and re-project.
    if (m_service != nullptr) {
        m_service->refreshHistory(transport, conversation);
    }
}

void ChatConversationController::send(const QString& text) {
    const QString trimmed = text.trimmed();
    if (m_service == nullptr || m_transport.isEmpty() || trimmed.isEmpty()) {
        return;
    }
    // No optimistic local echo — the node is authoritative. The sent line reappears
    // when the resulting MessagesChanged round-trips back through onMessagesChanged.
    m_service->send(m_transport, m_conversation, text);
}

void ChatConversationController::onMessagesChanged(const QString& transport, const QString& conv,
                                                   const QVariantList& rows) {
    // Filter to the bound conversation so a backgrounded tab's controller ignores
    // other conversations' traffic.
    if (transport != m_transport || conv != m_conversation) {
        return;
    }
    applyRows(rows);
}

void ChatConversationController::onServiceSendFailed(const QString& transport, const QString& conv,
                                                     const QString& message) {
    if (transport != m_transport || conv != m_conversation) {
        return;
    }
    emit sendFailed(message);
}

void ChatConversationController::applyRows(const QVariantList& rows) {
    m_rows = rows;
    m_markdown = projectMarkdown(rows);
    emit markdownChanged();
}

QString ChatConversationController::projectMarkdown(const QVariantList& rows) {
    // Oldest-first: each row becomes one or more markdown blocks the BlockEditor
    // renders. Messages emphasise their author; system/notice events and /me actions
    // render as distinct notes so the transcript styles events vs. lines.
    QStringList blocks;
    blocks.reserve(rows.size());
    for (const QVariant& v : rows) {
        const QVariantMap row = v.toMap();
        const QString text = fieldText(row, "text");
        const QString author = fieldText(row, "authorName");
        if (fieldFlag(row, "system") || fieldFlag(row, "notice")) {
            // A room event / ephemeral notice: an italic blockquote note.
            blocks.append(QStringLiteral("> _%1_").arg(text));
            continue;
        }
        if (fieldFlag(row, "action")) {
            // An IRC-style "/me" action line.
            blocks.append(QStringLiteral("_%1 %2_").arg(author, text));
            continue;
        }
        // A normal message: bold author header, then the markdown body. Failed sends
        // get a leading warning glyph (no translatable string, so the projection
        // stays i18n-free).
        const QString body = fieldFlag(row, "error") ? (QStringLiteral("\u26a0 ") + text) : text;
        blocks.append(QStringLiteral("**%1**\n\n%2").arg(author, body));
    }
    return blocks.join(QStringLiteral("\n\n"));
}
