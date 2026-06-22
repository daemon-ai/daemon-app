#include "core/transcript_search.h"

#include "core/document_store.h"

namespace be {

TranscriptSearchController::TranscriptSearchController(QObject* parent) : QObject(parent) {}

void TranscriptSearchController::setDocument(DocumentStore* document) {
    if (m_document == document) {
        return;
    }
    m_document = document;
    refresh();
}

void TranscriptSearchController::setQuery(const QString& query) {
    if (m_query == query) {
        return;
    }
    m_query = query;
    emit queryChanged();
    refresh();
}

void TranscriptSearchController::setCaseSensitive(bool on) {
    if (m_caseSensitive == on) {
        return;
    }
    m_caseSensitive = on;
    emit caseSensitiveChanged();
    refresh();
}

void TranscriptSearchController::clear() {
    if (m_query.isEmpty() && m_matches.isEmpty() && m_current == -1) {
        return;
    }
    const bool hadQuery = !m_query.isEmpty();
    m_query.clear();
    m_matches.clear();
    m_current = -1;
    if (hadQuery) {
        emit queryChanged();
    }
    emit matchesChanged();
    emit currentMatchChanged();
}

QString TranscriptSearchController::searchableTextFor(const BlockRecord& block) {
    // Typed agent blocks persist as fenced JSON, but their *visible* text lives in
    // metadata; search what the user reads, not the serialized envelope. Everything
    // else (paragraphs, headings, lists, quotes, code) searches its markdown source.
    switch (block.type) {
    case BlockType::Reasoning:
    case BlockType::Content:
        return block.metadata.value(QStringLiteral("body")).toString();
    case BlockType::ToolCall:
        return block.metadata.value(QStringLiteral("argsSummary")).toString();
    default:
        return block.markdown();
    }
}

QString TranscriptSearchController::searchableText(int blockIndex) const {
    if (!m_document) {
        return {};
    }
    const BlockRecord* block = m_document->blockAt(blockIndex);
    return block ? searchableTextFor(*block) : QString();
}

void TranscriptSearchController::refresh() {
    // Remember the active match's block so the cursor survives a re-collect when
    // possible (e.g. a streaming refresh): we re-anchor to the first match at or
    // after the old block rather than snapping back to match 0.
    const int previousBlock = currentBlockIndex();

    m_matches.clear();
    m_current = -1;

    if (m_document && !m_query.isEmpty()) {
        const Qt::CaseSensitivity cs =
            m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        const int needleLen = static_cast<int>(m_query.size());
        const qsizetype blockCount = m_document->blockCount();
        for (qsizetype row = 0; row < blockCount; ++row) {
            const QString hay = searchableTextFor(*m_document->blockAt(row));
            int from = 0;
            while (true) {
                const int at = hay.indexOf(m_query, from, cs);
                if (at < 0) {
                    break;
                }
                m_matches.push_back({static_cast<int>(row), at, needleLen});
                from = at + needleLen; // non-overlapping
            }
        }
    }

    if (!m_matches.isEmpty()) {
        m_current = 0;
        if (previousBlock >= 0) {
            for (int i = 0; i < m_matches.size(); ++i) {
                if (m_matches[i].blockIndex >= previousBlock) {
                    m_current = i;
                    break;
                }
            }
        }
    }

    emit matchesChanged();
    emit currentMatchChanged();
    emitNavigateToCurrent();
}

void TranscriptSearchController::next() {
    if (m_matches.isEmpty()) {
        return;
    }
    m_current = (m_current + 1) % m_matches.size();
    emit currentMatchChanged();
    emitNavigateToCurrent();
}

void TranscriptSearchController::previous() {
    if (m_matches.isEmpty()) {
        return;
    }
    m_current = (m_current - 1 + m_matches.size()) % m_matches.size();
    emit currentMatchChanged();
    emitNavigateToCurrent();
}

void TranscriptSearchController::setCurrent(int index) {
    if (index < 0 || index >= m_matches.size() || index == m_current) {
        return;
    }
    m_current = index;
    emit currentMatchChanged();
    emitNavigateToCurrent();
}

void TranscriptSearchController::emitNavigateToCurrent() {
    if (m_current < 0 || m_current >= m_matches.size()) {
        return;
    }
    const Match& m = m_matches[m_current];
    emit navigateTo(m.blockIndex, m.charStart);
}

int TranscriptSearchController::currentBlockIndex() const {
    return (m_current >= 0 && m_current < m_matches.size()) ? m_matches[m_current].blockIndex : -1;
}

int TranscriptSearchController::currentCharStart() const {
    return (m_current >= 0 && m_current < m_matches.size()) ? m_matches[m_current].charStart : 0;
}

int TranscriptSearchController::currentCharLen() const {
    return (m_current >= 0 && m_current < m_matches.size()) ? m_matches[m_current].charLen : 0;
}

int TranscriptSearchController::matchBlockIndex(int index) const {
    return (index >= 0 && index < m_matches.size()) ? m_matches[index].blockIndex : -1;
}

int TranscriptSearchController::matchCharStart(int index) const {
    return (index >= 0 && index < m_matches.size()) ? m_matches[index].charStart : 0;
}

int TranscriptSearchController::matchCharLen(int index) const {
    return (index >= 0 && index < m_matches.size()) ? m_matches[index].charLen : 0;
}

} // namespace be
