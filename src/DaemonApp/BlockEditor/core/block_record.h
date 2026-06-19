#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QVariantMap>

#include <cstdint>

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
    Unknown,
};

struct TextRangeUtf8 {
    qsizetype byteOffset = 0;
    qsizetype byteLength = 0;
};

struct BlockRecord {
    BlockId id = 0;
    BlockType type = BlockType::Unknown;
    quint16 indent = 0;
    quint16 headingLevel = 0;
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
