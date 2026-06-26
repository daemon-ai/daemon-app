#include "core/coordinate_map.h"

#include <algorithm>
#include <QtGlobal>

namespace be {

void CoordinateMap::rebuild(const QByteArray& utf8) {
    m_utf8 = utf8;
    m_text = QString::fromUtf8(utf8);
    rebuild(m_text);
}

void CoordinateMap::rebuild(const QString& text) {
    m_text = text;
    m_utf8 = text.toUtf8();
    m_utf16ToUtf8.clear();
    m_utf8ToUtf16.clear();
    m_lineStartsUtf16.clear();

    m_utf16ToUtf8.resize(m_text.size() + 1);
    m_utf8ToUtf16.resize(m_utf8.size() + 1);

    qsizetype utf8Offset = 0;
    for (qsizetype utf16Offset = 0; utf16Offset < m_text.size();) {
        const QChar current = m_text.at(utf16Offset);
        const bool surrogatePair = current.isHighSurrogate() && utf16Offset + 1 < m_text.size() &&
                                   m_text.at(utf16Offset + 1).isLowSurrogate();
        const qsizetype utf16Width = surrogatePair ? 2 : 1;
        const qsizetype utf8Width =
            QStringView(m_text).mid(utf16Offset, utf16Width).toUtf8().size();

        for (qsizetype i = 0; i < utf16Width; ++i) {
            m_utf16ToUtf8[utf16Offset + i] = utf8Offset;
        }
        for (qsizetype i = 0; i < utf8Width; ++i) {
            m_utf8ToUtf16[utf8Offset + i] = utf16Offset;
        }

        utf16Offset += utf16Width;
        utf8Offset += utf8Width;
        m_utf16ToUtf8[utf16Offset] = utf8Offset;
        m_utf8ToUtf16[utf8Offset] = utf16Offset;
    }

    m_lineStartsUtf16.push_back(0);
    for (qsizetype i = 0; i < m_text.size(); ++i) {
        if (m_text.at(i) == QLatin1Char('\n')) {
            m_lineStartsUtf16.push_back(i + 1);
        }
    }
}

const QString& CoordinateMap::text() const {
    return m_text;
}

qsizetype CoordinateMap::utf16Length() const {
    return m_text.size();
}

qsizetype CoordinateMap::utf8Length() const {
    return m_utf8.size();
}

qsizetype CoordinateMap::lineCount() const {
    return m_lineStartsUtf16.size();
}

qsizetype CoordinateMap::utf16ToUtf8(qsizetype utf16Offset) const {
    if (m_utf16ToUtf8.isEmpty()) {
        return 0;
    }
    utf16Offset = qBound<qsizetype>(0, utf16Offset, m_utf16ToUtf8.size() - 1);
    return m_utf16ToUtf8[utf16Offset];
}

qsizetype CoordinateMap::utf8ToUtf16(qsizetype utf8Offset) const {
    if (m_utf8ToUtf16.isEmpty()) {
        return 0;
    }
    utf8Offset = qBound<qsizetype>(0, utf8Offset, m_utf8ToUtf16.size() - 1);
    return m_utf8ToUtf16[utf8Offset];
}

LineColumn CoordinateMap::utf16ToLineColumn(qsizetype utf16Offset) const {
    if (m_lineStartsUtf16.isEmpty()) {
        return {};
    }

    utf16Offset = qBound<qsizetype>(0, utf16Offset, m_text.size());
    const auto it = std::ranges::upper_bound(m_lineStartsUtf16, utf16Offset);
    const qsizetype line =
        std::max<qsizetype>(0, std::distance(m_lineStartsUtf16.cbegin(), it) - 1);
    return {line, utf16Offset - m_lineStartsUtf16[line]};
}

qsizetype CoordinateMap::lineColumnToUtf16(qsizetype line, qsizetype column) const {
    if (m_lineStartsUtf16.isEmpty()) {
        return 0;
    }

    line = qBound<qsizetype>(0, line, m_lineStartsUtf16.size() - 1);
    const qsizetype lineStart = m_lineStartsUtf16[line];
    const qsizetype nextLineStart =
        line + 1 < m_lineStartsUtf16.size() ? m_lineStartsUtf16[line + 1] : m_text.size() + 1;
    const qsizetype lineEnd = std::max<qsizetype>(lineStart, nextLineStart - 1);
    return qBound<qsizetype>(lineStart, lineStart + column, lineEnd);
}

qsizetype CoordinateMap::lineColumnToUtf8(qsizetype line, qsizetype column) const {
    return utf16ToUtf8(lineColumnToUtf16(line, column));
}

} // namespace be
