#pragma once

#include <QByteArray>
#include <QVector>

namespace be {

class PieceTable {
public:
    enum class Source : quint8 {
        Original,
        Add,
    };

    struct Piece {
        Source source = Source::Original;
        qsizetype offset = 0;
        qsizetype length = 0;
    };

    void setOriginal(QByteArray text);
    qsizetype size() const;
    bool isEmpty() const;
    QByteArray toUtf8() const;
    QByteArray slice(qsizetype offset, qsizetype length) const;
    void replace(qsizetype offset, qsizetype length, const QByteArray& inserted);
    const QVector<Piece>& pieces() const;

private:
    QByteArray pieceBytes(const Piece& piece) const;
    void appendPiece(QVector<Piece>& out, const Piece& piece) const;

    QByteArray m_original;
    QByteArray m_add;
    QVector<Piece> m_pieces;
};

} // namespace be
