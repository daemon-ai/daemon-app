#include "core/selection.h"

namespace be {

bool Selection::isActive() const {
    return anchor.blockId != 0 && head.blockId != 0 &&
           (anchor.blockId != head.blockId || anchor.rawUtf16Offset != head.rawUtf16Offset);
}

bool Selection::isBackward() const {
    if (anchor.blockIndex == head.blockIndex) {
        return head.rawUtf16Offset < anchor.rawUtf16Offset;
    }
    return head.blockIndex < anchor.blockIndex;
}

DocPos Selection::start() const {
    return isBackward() ? head : anchor;
}

DocPos Selection::end() const {
    return isBackward() ? anchor : head;
}

void SelectionControllerCore::clear() {
    m_selection = {};
    ++m_selection.revision;
}

void SelectionControllerCore::setAnchor(const DocPos& pos) {
    m_selection.anchor = pos;
    m_selection.head = pos;
    ++m_selection.revision;
}

void SelectionControllerCore::setHead(const DocPos& pos) {
    m_selection.head = pos;
    ++m_selection.revision;
}

const Selection& SelectionControllerCore::selection() const {
    return m_selection;
}

SelectionSpan SelectionControllerCore::spanForBlock(BlockId id, qsizetype row,
                                                    qsizetype visualLength,
                                                    qsizetype rawLength) const {
    if (!m_selection.isActive()) {
        return {};
    }

    const DocPos start = m_selection.start();
    const DocPos end = m_selection.end();

    if (row < start.blockIndex || row > end.blockIndex) {
        return {};
    }
    if (row > start.blockIndex && row < end.blockIndex) {
        return {SelectionSpan::Kind::FullBlock, 0, visualLength, 0, rawLength};
    }
    if (id == start.blockId && id == end.blockId) {
        return {SelectionSpan::Kind::Partial, start.visualOffset, end.visualOffset,
                start.rawUtf16Offset, end.rawUtf16Offset};
    }
    if (id == start.blockId) {
        return {SelectionSpan::Kind::Partial, start.visualOffset, visualLength,
                start.rawUtf16Offset, rawLength};
    }
    if (id == end.blockId) {
        return {SelectionSpan::Kind::Partial, 0, end.visualOffset, 0, end.rawUtf16Offset};
    }
    return {SelectionSpan::Kind::FullBlock, 0, visualLength, 0, rawLength};
}

QString SelectionControllerCore::copyMarkdown(const QVector<BlockRecord>& blocks) const {
    if (!m_selection.isActive()) {
        return {};
    }

    const DocPos start = m_selection.start();
    const DocPos end = m_selection.end();
    QString out;
    for (qsizetype row = start.blockIndex; row <= end.blockIndex && row < blocks.size(); ++row) {
        if (row < 0) {
            continue;
        }

        const QString markdown = blocks[row].markdown();
        if (row == start.blockIndex && row == end.blockIndex) {
            out += markdown.mid(start.rawUtf16Offset, end.rawUtf16Offset - start.rawUtf16Offset);
        } else if (row == start.blockIndex) {
            out += markdown.mid(start.rawUtf16Offset);
        } else if (row == end.blockIndex) {
            out += markdown.left(end.rawUtf16Offset);
        } else {
            out += markdown;
        }
    }
    return out;
}

QPair<qsizetype, qsizetype> SelectionControllerCore::wordRangeAt(const QString& text,
                                                                 qsizetype offset) {
    QTextBoundaryFinder finder(QTextBoundaryFinder::Word, text);
    finder.setPosition(qBound<qsizetype>(0, offset, text.size()));
    qsizetype start = finder.toPreviousBoundary();
    if (start < 0) {
        start = 0;
    }
    qsizetype end = finder.toNextBoundary();
    if (end < 0) {
        end = text.size();
    }
    return {start, end};
}

QPair<qsizetype, qsizetype> SelectionControllerCore::lineRangeAt(const QString& text,
                                                                 qsizetype offset) {
    offset = qBound<qsizetype>(0, offset, text.size());
    qsizetype start = text.lastIndexOf(QLatin1Char('\n'), offset - 1);
    qsizetype end = text.indexOf(QLatin1Char('\n'), offset);
    return {start < 0 ? 0 : start + 1, end < 0 ? text.size() : end};
}

} // namespace be
