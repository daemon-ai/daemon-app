// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "mirror/chat_window_model.h"

namespace mirror {

ChatWindowModel::ChatWindowModel(Store& store, ChatMessageScope scope, QObject* parent)
    : WindowModel<ChatMessage>(store, std::move(scope), parent) {}

QHash<int, QByteArray> ChatWindowModel::roleNames() const {
    QHash<int, QByteArray> names;
    names.insert(StableIdRole, QByteArrayLiteral("stableId"));
    names.insert(TransportRole, QByteArrayLiteral("transport"));
    names.insert(ConversationRole, QByteArrayLiteral("conversationId"));
    names.insert(CursorRole, QByteArrayLiteral("cursor"));
    names.insert(AuthorRole, QByteArrayLiteral("author"));
    names.insert(TextRole, QByteArrayLiteral("text"));
    names.insert(TimestampRole, QByteArrayLiteral("timestamp"));
    names.insert(EditedAtRole, QByteArrayLiteral("editedAt"));
    names.insert(ReplyingToRole, QByteArrayLiteral("replyingTo"));
    names.insert(ErrorRole, QByteArrayLiteral("error"));
    names.insert(AttachmentCountRole, QByteArrayLiteral("attachmentCount"));
    names.insert(OriginOpRole, QByteArrayLiteral("originOp"));
    return names;
}

QVariant ChatWindowModel::roleData(const ChatMessage& m, int role) const {
    switch (role) {
    case TransportRole:
        return m.transport;
    case ConversationRole:
        return m.conv;
    case CursorRole:
        return QVariant::fromValue(m.cursor);
    case AuthorRole:
        return m.author;
    case TextRole:
        return m.text;
    case TimestampRole:
        return QVariant::fromValue(m.timestamp);
    case EditedAtRole:
        return QVariant::fromValue(m.edited_at);
    case ReplyingToRole:
        return m.replying_to;
    case ErrorRole:
        return m.error;
    case AttachmentCountRole:
        return m.attachment_count;
    case OriginOpRole:
        return m.origin_op;
    default:
        return {};
    }
}

} // namespace mirror
