#include "core/block_record.h"

namespace be {

QString BlockRecord::markdown() const
{
    return QString::fromUtf8(markdownUtf8);
}

QString blockTypeName(BlockType type)
{
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
