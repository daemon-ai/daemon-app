// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "core/block_height_index.h"

#include <QtGlobal>

namespace be {

void BlockHeightIndex::reset(qsizetype count, qreal defaultHeight) {
    m_heights.fill(defaultHeight, count);
    m_tree.fill(0.0, count + 1);
    for (qsizetype row = 0; row < count; ++row) {
        add(row, defaultHeight);
    }
}

qsizetype BlockHeightIndex::size() const {
    return m_heights.size();
}

void BlockHeightIndex::setHeight(qsizetype row, qreal height) {
    if (row < 0 || row >= m_heights.size()) {
        return;
    }
    height = qMax<qreal>(1.0, height);
    const qreal delta = height - m_heights[row];
    m_heights[row] = height;
    add(row, delta);
}

qreal BlockHeightIndex::height(qsizetype row) const {
    if (row < 0 || row >= m_heights.size()) {
        return 0.0;
    }
    return m_heights[row];
}

qreal BlockHeightIndex::prefixHeight(qsizetype row) const {
    if (m_heights.isEmpty() || row <= 0) {
        return 0.0;
    }
    return prefixInclusive(qMin(row - 1, m_heights.size() - 1));
}

qsizetype BlockHeightIndex::rowAtContentY(qreal y) const {
    if (m_heights.isEmpty()) {
        return -1;
    }

    qsizetype low = 0;
    qsizetype high = m_heights.size() - 1;
    while (low < high) {
        const qsizetype mid = low + ((high - low) / 2);
        if (prefixInclusive(mid) <= y) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    return low;
}

qreal BlockHeightIndex::totalHeight() const {
    return prefixHeight(m_heights.size());
}

void BlockHeightIndex::add(qsizetype row, qreal delta) {
    for (qsizetype i = row + 1; i < m_tree.size(); i += i & -i) {
        m_tree[i] += delta;
    }
}

qreal BlockHeightIndex::prefixInclusive(qsizetype row) const {
    qreal sum = 0.0;
    for (qsizetype i = row + 1; i > 0; i -= i & -i) {
        sum += m_tree[i];
    }
    return sum;
}

} // namespace be
