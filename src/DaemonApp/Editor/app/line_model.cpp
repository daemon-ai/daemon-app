// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "app/line_model.h"

#include "engine/code_highlighter.h"
#include "engine/text_document.h"

namespace editor {
namespace {

int leadingIndent(const QString& s) {
    int i = 0;
    while (i < s.size() && (s[i] == QLatin1Char(' ') || s[i] == QLatin1Char('\t')))
        ++i;
    return i;
}

} // namespace

LineModel::LineModel(TextDocument* doc, CodeHighlighter* hl, QObject* parent)
    : QAbstractListModel(parent), m_doc(doc), m_hl(hl) {
    if (m_doc) {
        connect(m_doc, &TextDocument::lineChanged, this, &LineModel::onLineChanged);
        connect(m_doc, &TextDocument::linesReset, this, &LineModel::onLinesReset);
    }
    if (m_hl)
        connect(m_hl, &CodeHighlighter::highlightingChanged, this,
                &LineModel::onHighlightingChanged);
    rebuildVisible();
}

int LineModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_visible.size());
}

QVariant LineModel::data(const QModelIndex& index, int role) const {
    const int row = index.row();
    if (row < 0 || row >= m_visible.size() || !m_doc)
        return {};
    const int real = m_visible.at(row);
    switch (role) {
    case Qt::DisplayRole:
    case TextRole:
        return m_doc->line(real);
    case StyleRunsRole:
        return styleRunsForLine(real);
    case FoldStartRole:
        return m_hl && m_hl->foldingStartsAt(real);
    case FoldedRole:
        return m_folded.contains(real);
    case RealLineRole:
        return real;
    default:
        return {};
    }
}

QHash<int, QByteArray> LineModel::roleNames() const {
    return {
        {TextRole, "text"},     {StyleRunsRole, "styleRuns"}, {FoldStartRole, "foldStart"},
        {FoldedRole, "folded"}, {RealLineRole, "realLine"},
    };
}

QVariantList LineModel::styleRunsForLine(int line) const {
    QVariantList out;
    if (!m_hl)
        return out;
    const QList<FormatRun>& runs = m_hl->runs(line);
    out.reserve(runs.size());
    for (const FormatRun& r : runs) {
        QVariantMap m;
        m.insert(QStringLiteral("start"), r.start);
        m.insert(QStringLiteral("length"), r.length);
        m.insert(QStringLiteral("color"), r.color.name(QColor::HexRgb));
        m.insert(QStringLiteral("bold"), r.bold);
        m.insert(QStringLiteral("italic"), r.italic);
        out.push_back(m);
    }
    return out;
}

int LineModel::realLine(int row) const {
    return (row >= 0 && row < m_visible.size()) ? m_visible.at(row) : -1;
}

int LineModel::rowForLine(int line) const {
    return static_cast<int>(m_visible.indexOf(line));
}

int LineModel::foldRegionEnd(int startLine) const {
    if (!m_doc)
        return startLine;
    const int n = m_doc->lineCount();
    const int baseIndent = leadingIndent(m_doc->line(startLine));
    int end = startLine;
    for (int l = startLine + 1; l < n; ++l) {
        const QString t = m_doc->line(l);
        if (t.trimmed().isEmpty())
            continue; // blank lines belong to the region if a deeper line follows
        if (leadingIndent(t) > baseIndent)
            end = l;
        else
            break;
    }
    return end;
}

void LineModel::rebuildVisible() {
    m_visible.clear();
    if (!m_doc)
        return;
    const int n = m_doc->lineCount();
    QSet<int> hidden;
    for (int fs : std::as_const(m_folded)) {
        const int end = foldRegionEnd(fs);
        for (int l = fs + 1; l <= end; ++l)
            hidden.insert(l);
    }
    for (int l = 0; l < n; ++l) {
        if (!hidden.contains(l))
            m_visible.push_back(l);
    }
}

void LineModel::onLineChanged(int line) {
    const int row = rowForLine(line);
    if (row >= 0) {
        // Repaint from the edited row to the end of the visible list; the view
        // only re-fetches the rows actually on screen, each of which lazily
        // re-highlights via runs() -> highlightTo().
        const int last = static_cast<int>(m_visible.size()) - 1;
        emit dataChanged(index(row, 0), index(last, 0), {TextRole, StyleRunsRole, Qt::DisplayRole});
    }
}

void LineModel::onLinesReset() {
    beginResetModel();
    m_folded.clear();
    rebuildVisible();
    endResetModel();
}

void LineModel::onHighlightingChanged(int firstLine, int lastLine) {
    for (int l = firstLine; l <= lastLine; ++l) {
        const int row = rowForLine(l);
        if (row >= 0) {
            const QModelIndex idx = index(row, 0);
            emit dataChanged(idx, idx, {StyleRunsRole});
        }
    }
}

void LineModel::toggleFold(int realLine) {
    if (realLine < 0)
        return;
    beginResetModel();
    if (m_folded.contains(realLine))
        m_folded.remove(realLine);
    else
        m_folded.insert(realLine);
    rebuildVisible();
    endResetModel();
}

void LineModel::resetFolds() {
    beginResetModel();
    m_folded.clear();
    rebuildVisible();
    endResetModel();
}

} // namespace editor
