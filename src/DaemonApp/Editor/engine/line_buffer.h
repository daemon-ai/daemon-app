// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace editor {

// A block-based line buffer (KTextEditor TextBlock pattern): lines are stored in
// blocks of up to ~128 lines (target 64), so line access is O(1)-ish and edits
// stay local. Lines do not carry trailing newlines; the document joins with '\n'.
// This is the storage layer only - cursors, undo, and highlighting live above it.
class LineBuffer {
public:
    LineBuffer();

    [[nodiscard]] int count() const { return m_count; }
    [[nodiscard]] const QString& at(int line) const;
    void setAt(int line, const QString& text);

    // Replace `removeCount` lines at `line` with `lines` (either may be empty).
    void replace(int line, int removeCount, const QStringList& lines);

    void clear();
    void setAll(const QStringList& lines);
    [[nodiscard]] QStringList toList() const;

private:
    static constexpr int kTarget = 64;
    static constexpr int kSplit = 128;
    static constexpr int kMerge = 32;

    // Locate (blockIndex, offsetInBlock) for a global line.
    void locate(int line, int& block, int& off) const;
    void rebalanceFrom(int block);

    QList<QStringList> m_blocks;
    int m_count = 0;
    static const QString s_empty;
};

} // namespace editor
