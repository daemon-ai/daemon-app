#include "core/agent_ingest.h"

namespace be {

namespace {

QString stringField(const QVariantMap &m, const QString &key)
{
    return m.value(key).toString();
}

// Copy the detail payload fields a ToolFinished event may carry through to the
// block metadata, so buildToolView's sub-renderer has what it needs.
void copyToolDetail(const QVariantMap &event, QVariantMap &meta)
{
    static const char *keys[] = {"detailKind", "stdout", "stderr", "body", "diff",
                                 "hits", "imageUrl", "count", "exitCode"};
    for (const char *key : keys) {
        const QString k = QString::fromLatin1(key);
        if (event.contains(k)) {
            meta.insert(k, event.value(k));
        }
    }
}

} // namespace

TranscriptIngest::TranscriptIngest(DocumentStore *store)
    : m_store(store)
{
}

QVector<BlockChangeSet> TranscriptIngest::ingest(const QVariantMap &event)
{
    QVector<BlockChangeSet> out;
    if (!m_store) {
        return out;
    }

    const QString type = stringField(event, QStringLiteral("type"));

    if (type == QStringLiteral("text")) {
        const QString text = stringField(event, QStringLiteral("text"));
        if (text.isEmpty()) {
            return out;
        }
        if (!m_textStreaming) {
            m_store->beginStreamAtEnd();
            m_textStreaming = true;
        }
        out.push_back(m_store->streamAppend(text));
        return out;
    }

    // Any non-text event ends the open text stream first, so the streamed
    // paragraph settles above the structured block.
    if (m_textStreaming) {
        m_store->endStream();
        m_textStreaming = false;
    }

    if (type == QStringLiteral("reasoningDelta")) {
        const QString text = stringField(event, QStringLiteral("text"));
        m_reasoningBody += text;
        if (m_reasoningBlock == 0) {
            QVariantMap meta;
            meta.insert(QStringLiteral("status"), QStringLiteral("running"));
            meta.insert(QStringLiteral("body"), m_reasoningBody);
            BlockChangeSet cs;
            m_reasoningBlock = m_store->appendTypedBlock(BlockType::Reasoning, meta, &cs);
            out.push_back(cs);
        } else {
            QVariantMap patch;
            patch.insert(QStringLiteral("body"), m_reasoningBody);
            out.push_back(m_store->updateBlockMetadata(m_reasoningBlock, patch));
        }
        return out;
    }

    if (type == QStringLiteral("reasoningDone")) {
        if (m_reasoningBlock != 0) {
            QVariantMap patch;
            patch.insert(QStringLiteral("status"), QStringLiteral("complete"));
            if (event.contains(QStringLiteral("durationMs"))) {
                patch.insert(QStringLiteral("durationMs"), event.value(QStringLiteral("durationMs")));
            }
            out.push_back(m_store->updateBlockMetadata(m_reasoningBlock, patch));
        }
        m_reasoningBlock = 0;
        m_reasoningBody.clear();
        return out;
    }

    if (type == QStringLiteral("toolStarted")) {
        QVariantMap meta;
        meta.insert(QStringLiteral("callId"), event.value(QStringLiteral("callId")));
        meta.insert(QStringLiteral("name"), stringField(event, QStringLiteral("name")));
        meta.insert(QStringLiteral("argsSummary"), stringField(event, QStringLiteral("argsSummary")));
        meta.insert(QStringLiteral("tone"), stringField(event, QStringLiteral("tone")));
        meta.insert(QStringLiteral("status"), QStringLiteral("running"));
        if (event.contains(QStringLiteral("detailKind"))) {
            meta.insert(QStringLiteral("detailKind"), event.value(QStringLiteral("detailKind")));
        }
        BlockChangeSet cs;
        m_store->appendTypedBlock(BlockType::ToolCall, meta, &cs);
        out.push_back(cs);
        return out;
    }

    if (type == QStringLiteral("toolFinished")) {
        const QVariant callId = event.value(QStringLiteral("callId"));
        const BlockId id = m_store->blockIdForMetadata(QStringLiteral("callId"), callId);
        if (id != 0) {
            QVariantMap patch;
            patch.insert(QStringLiteral("status"),
                         event.value(QStringLiteral("status"), QStringLiteral("ok")));
            if (event.contains(QStringLiteral("durationMs"))) {
                patch.insert(QStringLiteral("durationMs"), event.value(QStringLiteral("durationMs")));
            }
            copyToolDetail(event, patch);
            out.push_back(m_store->updateBlockMetadata(id, patch));
        }
        return out;
    }

    if (type == QStringLiteral("content")) {
        QVariantMap meta;
        meta.insert(QStringLiteral("kind"), stringField(event, QStringLiteral("kind")));
        meta.insert(QStringLiteral("body"), stringField(event, QStringLiteral("body")));
        BlockChangeSet cs;
        m_store->appendTypedBlock(BlockType::Content, meta, &cs);
        out.push_back(cs);
        return out;
    }

    return out;
}

QVector<BlockChangeSet> TranscriptIngest::ingestAll(const QVariantList &events)
{
    QVector<BlockChangeSet> out;
    for (const QVariant &e : events) {
        out += ingest(e.toMap());
    }
    return out;
}

QVector<BlockChangeSet> TranscriptIngest::finish()
{
    QVector<BlockChangeSet> out;
    if (!m_store) {
        return out;
    }
    if (m_textStreaming) {
        m_store->endStream();
        m_textStreaming = false;
    }
    if (m_reasoningBlock != 0) {
        QVariantMap patch;
        patch.insert(QStringLiteral("status"), QStringLiteral("complete"));
        out.push_back(m_store->updateBlockMetadata(m_reasoningBlock, patch));
        m_reasoningBlock = 0;
        m_reasoningBody.clear();
    }
    return out;
}

} // namespace be
