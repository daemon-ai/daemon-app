#include "app/transcript_log.h"

#include "core/block_record.h"

#include <QStringList>

namespace be {
namespace {

QString blockKind(BlockType type) {
    switch (type) {
    case BlockType::Reasoning:
        return QStringLiteral("reasoning");
    case BlockType::ToolCall:
        return QStringLiteral("tool");
    case BlockType::Content:
        return QStringLiteral("content");
    default:
        return QStringLiteral("text");
    }
}

} // namespace

QList<domain::SessionLogEntry> decomposeMarkdown(const QString& markdown) {
    DocumentStore tmp;
    tmp.loadMarkdown(markdown);

    QList<domain::SessionLogEntry> out;
    quint64 seq = 0;
    for (const BlockRecord& block : tmp.blocks()) {
        domain::SessionLogEntry entry;
        entry.seq = seq++;
        // A user turn is an inbound command on the wire; everything else (assistant
        // events, system meta) is outbound/observability from the session's view.
        entry.direction = block.role == MessageRole::User ? domain::Direction::Inbound
                                                          : domain::Direction::Outbound;
        entry.kind = blockKind(block.type);
        // Carry the structured agent metadata (fence JSON) as the decoded payload, plus
        // the role envelope and the canonical block markdown the applier reassembles from.
        QVariantMap payload = block.metadata;
        payload.insert(QStringLiteral("role"), messageRoleToString(block.role));
        payload.insert(QStringLiteral("messageId"), block.messageId);
        payload.insert(QStringLiteral("markdown"), block.markdown());
        entry.payload = payload;
        out.push_back(entry);
    }
    return out;
}

void applyTranscriptLog(DocumentStore& store, const QList<domain::SessionLogEntry>& entries) {
    // Reset to an empty document, then rebuild group-by-group.
    store.loadMarkdown(QString());

    int i = 0;
    while (i < entries.size()) {
        const QString role = entries.at(i).payload.value(QStringLiteral("role")).toString();
        const QString messageId =
            entries.at(i).payload.value(QStringLiteral("messageId")).toString();

        // Gather the contiguous run of blocks in the same role/message.
        QStringList parts;
        int j = i;
        while (j < entries.size() &&
               entries.at(j).payload.value(QStringLiteral("role")).toString() == role &&
               entries.at(j).payload.value(QStringLiteral("messageId")).toString() == messageId) {
            parts << entries.at(j).payload.value(QStringLiteral("markdown")).toString();
            ++j;
        }

        // Reconstruct the message's blocks through the role-aware append API (preserving
        // the authored message id so the serialized boundary marker round-trips).
        store.appendMessageBlocks(messageRoleFromString(role), parts.join(QStringLiteral("\n\n")),
                                  messageId);
        i = j;
    }
}

} // namespace be
