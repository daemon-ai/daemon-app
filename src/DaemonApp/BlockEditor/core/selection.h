// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "core/block_record.h"

#include <QTextBoundaryFinder>

namespace be {

struct DocPos {
    BlockId blockId = 0;
    qsizetype blockIndex = -1;
    qsizetype rawUtf16Offset = 0;
    qsizetype visualOffset = 0;
};

struct Selection {
    DocPos anchor;
    DocPos head;
    quint64 revision = 0;

    bool isActive() const;
    bool isBackward() const;
    DocPos start() const;
    DocPos end() const;
};

struct SelectionSpan {
    enum class Kind {
        None,
        FullBlock,
        Partial,
    };

    Kind kind = Kind::None;
    qsizetype visualStart = 0;
    qsizetype visualEnd = 0;
    qsizetype rawStart = 0;
    qsizetype rawEnd = 0;
};

class SelectionControllerCore {
public:
    void clear();
    void setAnchor(const DocPos& pos);
    void setHead(const DocPos& pos);
    const Selection& selection() const;
    SelectionSpan spanForBlock(BlockId id, qsizetype row, qsizetype visualLength,
                               qsizetype rawLength) const;
    QString copyMarkdown(const QVector<BlockRecord>& blocks) const;

    static QPair<qsizetype, qsizetype> wordRangeAt(const QString& text, qsizetype offset);
    static QPair<qsizetype, qsizetype> lineRangeAt(const QString& text, qsizetype offset);

private:
    Selection m_selection;
};

} // namespace be
