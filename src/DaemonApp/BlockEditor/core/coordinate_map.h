#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

namespace be {

struct LineColumn {
    qsizetype line = 0;
    qsizetype column = 0;
};

class CoordinateMap
{
public:
    void rebuild(const QByteArray &utf8);
    void rebuild(const QString &text);

    const QString &text() const;
    qsizetype utf16Length() const;
    qsizetype utf8Length() const;
    qsizetype lineCount() const;

    qsizetype utf16ToUtf8(qsizetype utf16Offset) const;
    qsizetype utf8ToUtf16(qsizetype utf8Offset) const;
    LineColumn utf16ToLineColumn(qsizetype utf16Offset) const;
    qsizetype lineColumnToUtf16(qsizetype line, qsizetype column) const;
    qsizetype lineColumnToUtf8(qsizetype line, qsizetype column) const;

private:
    QString m_text;
    QByteArray m_utf8;
    QVector<qsizetype> m_utf16ToUtf8;
    QVector<qsizetype> m_utf8ToUtf16;
    QVector<qsizetype> m_lineStartsUtf16;
};

} // namespace be
