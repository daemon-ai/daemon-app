// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QVector>

namespace be {

class BlockHeightIndex {
public:
    void reset(qsizetype count, qreal defaultHeight = 28.0);
    qsizetype size() const;
    void setHeight(qsizetype row, qreal height);
    qreal height(qsizetype row) const;
    qreal prefixHeight(qsizetype row) const;
    qsizetype rowAtContentY(qreal y) const;
    qreal totalHeight() const;

private:
    void add(qsizetype row, qreal delta);
    qreal prefixInclusive(qsizetype row) const;

    QVector<qreal> m_heights;
    QVector<qreal> m_tree;
};

} // namespace be
