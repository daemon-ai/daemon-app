// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "core/block_record.h"

namespace be {

QString BlockRecord::markdown() const {
    return QString::fromUtf8(markdownUtf8);
}

QString messageRoleToString(MessageRole role) {
    switch (role) {
    case MessageRole::User:
        return QStringLiteral("user");
    case MessageRole::Assistant:
        return QStringLiteral("assistant");
    case MessageRole::System:
        return QStringLiteral("system");
    case MessageRole::None:
        break;
    }
    return QStringLiteral("none");
}

MessageRole messageRoleFromString(const QString& name) {
    const QString key = name.trimmed().toLower();
    if (key == QStringLiteral("user")) {
        return MessageRole::User;
    }
    if (key == QStringLiteral("assistant")) {
        return MessageRole::Assistant;
    }
    if (key == QStringLiteral("system")) {
        return MessageRole::System;
    }
    return MessageRole::None;
}

QString blockTypeName(BlockType type) {
    switch (type) {
    case BlockType::Paragraph:
        return QStringLiteral("paragraph");
    case BlockType::Heading:
        return QStringLiteral("heading");
    case BlockType::BulletListItem:
        return QStringLiteral("bulletListItem");
    case BlockType::OrderedListItem:
        return QStringLiteral("orderedListItem");
    case BlockType::TaskListItem:
        return QStringLiteral("taskListItem");
    case BlockType::Quote:
        return QStringLiteral("quote");
    case BlockType::CodeFence:
        return QStringLiteral("codeFence");
    case BlockType::Table:
        return QStringLiteral("table");
    case BlockType::HorizontalRule:
        return QStringLiteral("horizontalRule");
    case BlockType::Image:
        return QStringLiteral("image");
    case BlockType::Math:
        return QStringLiteral("math");
    case BlockType::RawHtml:
        return QStringLiteral("rawHtml");
    case BlockType::Custom:
        return QStringLiteral("custom");
    case BlockType::Reasoning:
        return QStringLiteral("reasoning");
    case BlockType::ToolCall:
        return QStringLiteral("toolCall");
    case BlockType::Content:
        return QStringLiteral("content");
    case BlockType::Unknown:
        return QStringLiteral("unknown");
    }

    return QStringLiteral("unknown");
}

} // namespace be
