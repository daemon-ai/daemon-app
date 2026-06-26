#include "engine/line_buffer.h"

namespace editor {

const QString LineBuffer::s_empty;

LineBuffer::LineBuffer() {
    m_blocks.append(QStringList{QString()});
    m_count = 1;
}

void LineBuffer::locate(int line, int& block, int& off) const {
    int acc = 0;
    for (int b = 0; b < m_blocks.size(); ++b) {
        const int n = m_blocks[b].size();
        if (line < acc + n) {
            block = b;
            off = line - acc;
            return;
        }
        acc += n;
    }
    block = m_blocks.isEmpty() ? 0 : m_blocks.size() - 1;
    off = m_blocks.isEmpty() ? 0 : m_blocks.last().size();
}

const QString& LineBuffer::at(int line) const {
    if (line < 0 || line >= m_count)
        return s_empty;
    int b = 0;
    int off = 0;
    locate(line, b, off);
    return m_blocks[b].at(off);
}

void LineBuffer::setAt(int line, const QString& text) {
    if (line < 0 || line >= m_count)
        return;
    int b = 0;
    int off = 0;
    locate(line, b, off);
    m_blocks[b][off] = text;
}

void LineBuffer::replace(int line, int removeCount, const QStringList& lines) {
    if (line < 0)
        line = 0;
    if (line > m_count)
        line = m_count;
    removeCount = qBound(0, removeCount, m_count - line);

    // Remove first (block by block).
    int remaining = removeCount;
    while (remaining > 0) {
        int b = 0;
        int off = 0;
        locate(line, b, off);
        const int take = qMin(remaining, m_blocks[b].size() - off);
        m_blocks[b].remove(off, take);
        remaining -= take;
        m_count -= take;
        if (m_blocks[b].isEmpty() && m_blocks.size() > 1)
            m_blocks.removeAt(b);
    }

    // Insert the new lines into the block containing `line`.
    if (!lines.isEmpty()) {
        int b = 0;
        int off = 0;
        if (m_count == 0) {
            m_blocks.clear();
            m_blocks.append(QStringList());
            b = 0;
            off = 0;
        } else {
            locate(qMin(line, m_count), b, off);
        }
        for (int i = 0; i < lines.size(); ++i)
            m_blocks[b].insert(off + i, lines[i]);
        m_count += lines.size();
        rebalanceFrom(b);
    }

    if (m_count == 0) {
        m_blocks.clear();
        m_blocks.append(QStringList{QString()});
        m_count = 1;
    }
}

void LineBuffer::rebalanceFrom(int block) {
    if (block < 0 || block >= m_blocks.size())
        return;
    // Split oversized blocks.
    while (m_blocks[block].size() > kSplit) {
        const int half = m_blocks[block].size() / 2;
        QStringList tail = m_blocks[block].mid(half);
        m_blocks[block] = m_blocks[block].mid(0, half);
        m_blocks.insert(block + 1, tail);
    }
    // Merge an undersized block with its successor.
    if (m_blocks[block].size() < kMerge && block + 1 < m_blocks.size()) {
        m_blocks[block] += m_blocks[block + 1];
        m_blocks.removeAt(block + 1);
        if (m_blocks[block].size() > kSplit)
            rebalanceFrom(block);
    }
}

void LineBuffer::clear() {
    m_blocks.clear();
    m_blocks.append(QStringList{QString()});
    m_count = 1;
}

void LineBuffer::setAll(const QStringList& lines) {
    m_blocks.clear();
    if (lines.isEmpty()) {
        m_blocks.append(QStringList{QString()});
        m_count = 1;
        return;
    }
    m_count = lines.size();
    for (int i = 0; i < lines.size(); i += kTarget)
        m_blocks.append(lines.mid(i, kTarget));
}

QStringList LineBuffer::toList() const {
    QStringList out;
    out.reserve(m_count);
    for (const QStringList& b : m_blocks)
        out += b;
    return out;
}

} // namespace editor
