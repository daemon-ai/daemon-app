// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <cstdint>
#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QVariantMap>

namespace be {

using BlockId = quint64;

enum class BlockType : quint16 {
    Paragraph,
    Heading,
    BulletListItem,
    OrderedListItem,
    TaskListItem,
    Quote,
    CodeFence,
    Table,
    HorizontalRule,
    Image,
    Math,
    RawHtml,
    Custom,
    // Agent transcript blocks (hybrid: structured metadata at runtime, a fenced
    // markdown form for persistence/export). Appended AFTER the markdown types so
    // the existing numeric BlockDelegate gates (list 2-4, CodeFence 6, Table 7,
    // Image 9) keep their values; only Unknown shifts.
    Reasoning, // ```reasoning - a model chain-of-thought disclosure
    ToolCall,  // ```tool - a tool invocation + its result/detail
    Content,   // ```content - a typed content stream (ansi/pty/...) outside a tool
    Unknown,
};

// Session message role a block belongs to. Derived at parse time from
// persisted boundary markers (see agent_block message-marker helpers) and used
// by the QML grouping layer to draw user bubbles / assistant containers /
// system notices. None == an un-roled (legacy / plain-markdown) block.
enum class MessageRole : quint8 {
    None,
    User,
    Assistant,
    System,
};

QString messageRoleToString(MessageRole role);
MessageRole messageRoleFromString(const QString& name);

struct TextRangeUtf8 {
    qsizetype byteOffset = 0;
    qsizetype byteLength = 0;
};

struct BlockRecord {
    BlockId id = 0;
    BlockType type = BlockType::Unknown;
    quint16 indent = 0;
    quint16 headingLevel = 0;
    // Message grouping (Strategy C): the role + stable message id this block
    // belongs to. Assigned from boundary markers on load and from the
    // current-message latch on the runtime (typed/stream) path.
    MessageRole role = MessageRole::None;
    QString messageId;
    TextRangeUtf8 source;
    QByteArray markdownUtf8;
    QVariantMap metadata;
    quint32 revision = 0;
    quint32 parseRevision = 0;
    quint32 renderRevision = 0;
    float heightHint = 24.0f;
    bool dirtyText = false;
    bool dirtyRender = true;
    bool dirtyPersistence = false;
    bool tombstoned = false;

    QString markdown() const;
};

QString blockTypeName(BlockType type);

} // namespace be

Q_DECLARE_METATYPE(be::BlockType)
