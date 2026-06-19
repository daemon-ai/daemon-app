#include "core/piece_table.h"

#include <QtGlobal>

namespace be {

void PieceTable::setOriginal(QByteArray text)
{
    m_original = std::move(text);
    m_add.clear();
    m_pieces.clear();

    if (!m_original.isEmpty()) {
        m_pieces.push_back({Source::Original, 0, m_original.size()});
    }
}

qsizetype PieceTable::size() const
{
    qsizetype total = 0;
    for (const Piece &piece : m_pieces) {
        total += piece.length;
    }
    return total;
}

bool PieceTable::isEmpty() const
{
    return size() == 0;
}

QByteArray PieceTable::toUtf8() const
{
    QByteArray result;
    result.reserve(size());
    for (const Piece &piece : m_pieces) {
        result += pieceBytes(piece);
    }
    return result;
}

QByteArray PieceTable::slice(qsizetype offset, qsizetype length) const
{
    const QByteArray all = toUtf8();
    if (offset < 0 || length <= 0 || offset >= all.size()) {
        return {};
    }
    return all.mid(offset, length);
}

void PieceTable::replace(qsizetype offset, qsizetype length, QByteArray inserted)
{
    offset = qBound<qsizetype>(0, offset, size());
    length = qBound<qsizetype>(0, length, size() - offset);

    QVector<Piece> next;
    qsizetype cursor = 0;

    for (const Piece &piece : m_pieces) {
        const qsizetype pieceStart = cursor;
        const qsizetype pieceEnd = cursor + piece.length;
        cursor = pieceEnd;

        if (pieceEnd <= offset || pieceStart >= offset + length) {
            appendPiece(next, piece);
            continue;
        }

        if (offset > pieceStart) {
            appendPiece(next, {piece.source, piece.offset, offset - pieceStart});
        }

        if (pieceEnd > offset + length) {
            const qsizetype suffixStartInPiece = offset + length - pieceStart;
            appendPiece(next, {piece.source, piece.offset + suffixStartInPiece, pieceEnd - (offset + length)});
        }
    }

    if (!inserted.isEmpty()) {
        const qsizetype addOffset = m_add.size();
        m_add += inserted;

        QVector<Piece> withInsert;
        cursor = 0;
        bool insertedPiece = false;
        for (const Piece &piece : next) {
            if (!insertedPiece && cursor >= offset) {
                appendPiece(withInsert, {Source::Add, addOffset, inserted.size()});
                insertedPiece = true;
            }
            appendPiece(withInsert, piece);
            cursor += piece.length;
        }
        if (!insertedPiece) {
            appendPiece(withInsert, {Source::Add, addOffset, inserted.size()});
        }
        next = std::move(withInsert);
    }

    m_pieces = std::move(next);
}

const QVector<PieceTable::Piece> &PieceTable::pieces() const
{
    return m_pieces;
}

QByteArray PieceTable::pieceBytes(const Piece &piece) const
{
    const QByteArray &source = piece.source == Source::Original ? m_original : m_add;
    return source.mid(piece.offset, piece.length);
}

void PieceTable::appendPiece(QVector<Piece> &out, const Piece &piece) const
{
    if (piece.length <= 0) {
        return;
    }

    if (!out.isEmpty()) {
        Piece &last = out.last();
        if (last.source == piece.source && last.offset + last.length == piece.offset) {
            last.length += piece.length;
            return;
        }
    }

    out.push_back(piece);
}

} // namespace be
