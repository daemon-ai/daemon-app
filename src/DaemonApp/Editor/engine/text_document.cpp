#include "engine/text_document.h"

#include <QDateTime>

#include <utility>

namespace editor {
namespace {

// The end position after laying `text` down at `start` (before any trailing
// suffix). A newline in `text` advances the line; the column resets to the
// length of the text after the last newline.
Pos advance(const Pos& start, const QString& text)
{
    const int nl = static_cast<int>(text.count(QLatin1Char('\n')));
    if (nl == 0)
        return Pos{ start.line, start.col + static_cast<int>(text.size()) };
    const int lastNl = static_cast<int>(text.lastIndexOf(QLatin1Char('\n')));
    return Pos{ start.line + nl, static_cast<int>(text.size()) - lastNl - 1 };
}

} // namespace

TextDocument::TextDocument(QObject* parent)
    : QObject(parent)
{
    // m_buffer constructs with a single empty line.
}

void TextDocument::setText(const QString& text)
{
    m_buffer.setAll(text.split(QLatin1Char('\n')));
    m_undo.clear();
    m_redo.clear();
    m_editDepth = 0;

    const bool wasModified = (m_revision != m_savedRevision);
    ++m_revision;
    m_savedRevision = m_revision; // a freshly loaded document is its own save base
    emit linesReset();
    emit revisionChanged();
    if (wasModified)
        emit modifiedChanged();
}

void TextDocument::replaceTextFromEditor(const QString& text)
{
    if (text == this->text())
        return;
    m_buffer.setAll(text.split(QLatin1Char('\n')));
    m_undo.clear();
    m_redo.clear();
    m_editDepth = 0;
    bumpRevision();
    emit linesReset();
}

QString TextDocument::text() const
{
    return m_buffer.toList().join(QLatin1Char('\n'));
}

Pos TextDocument::clamp(const Pos& p) const
{
    Pos c = p;
    const int n = m_buffer.count();
    if (c.line < 0)
        c.line = 0;
    if (c.line >= n)
        c.line = n - 1;
    const int len = static_cast<int>(m_buffer.at(c.line).size());
    if (c.col < 0)
        c.col = 0;
    if (c.col > len)
        c.col = len;
    return c;
}

QString TextDocument::textInRange(const Pos& from, const Pos& to) const
{
    if (from.line == to.line)
        return m_buffer.at(from.line).mid(from.col, to.col - from.col);
    QString out = m_buffer.at(from.line).mid(from.col);
    out += QLatin1Char('\n');
    for (int l = from.line + 1; l < to.line; ++l) {
        out += m_buffer.at(l);
        out += QLatin1Char('\n');
    }
    out += m_buffer.at(to.line).left(to.col);
    return out;
}

Pos TextDocument::replaceRange(const Pos& fromIn, const Pos& toIn, const QString& replacement,
                               QString* removedOut)
{
    Pos from = clamp(fromIn);
    Pos to = clamp(toIn);
    if (to < from)
        std::swap(from, to);

    if (removedOut)
        *removedOut = textInRange(from, to);

    // Splice: keep the prefix of the first line and the suffix of the last line,
    // put the replacement in between, then re-split into whole lines.
    const QString prefix = m_buffer.at(from.line).left(from.col);
    const QString suffix = m_buffer.at(to.line).mid(to.col);
    const QStringList newLines = (prefix + replacement + suffix).split(QLatin1Char('\n'));
    const int removeCount = to.line - from.line + 1;
    m_buffer.replace(from.line, removeCount, newLines);

    bumpRevision();
    // A pure in-place single-line edit can be served by the model without a full
    // reset; anything that changes the line count is structural.
    if (removeCount == 1 && newLines.size() == 1)
        emit lineChanged(from.line);
    else
        emit linesReset();

    return advance(from, replacement);
}

Pos TextDocument::insert(const Pos& at, const QString& text)
{
    if (text.isEmpty())
        return clamp(at);
    const Pos start = clamp(at);
    Edit e;
    e.start = start;
    e.removed.clear();
    e.inserted = text;
    e.insertedEnd = replaceRange(start, start, text, nullptr);
    pushUndo(e);
    return e.insertedEnd;
}

Pos TextDocument::remove(const Pos& from, const Pos& to)
{
    Pos a = clamp(from);
    Pos b = clamp(to);
    if (b < a)
        std::swap(a, b);
    if (a == b)
        return a;
    Edit e;
    e.start = a;
    e.inserted.clear();
    replaceRange(a, b, QString(), &e.removed);
    e.insertedEnd = a; // after removal the caret sits at the start
    pushUndo(e);
    return a;
}

void TextDocument::beginEdit()
{
    if (m_editDepth == 0)
        ++m_group; // open a new transaction id
    ++m_editDepth;
}

void TextDocument::endEdit()
{
    if (m_editDepth > 0)
        --m_editDepth;
}

void TextDocument::pushUndo(Edit edit)
{
    if (m_inUndoRedo)
        return;
    if (m_editDepth == 0)
        ++m_group; // an unbracketed edit is its own transaction
    edit.group = m_group;
    edit.time = QDateTime::currentMSecsSinceEpoch();
    m_redo.clear();
    m_undo.append(edit);
}

void TextDocument::applyEdit(const Edit& e, bool inverse)
{
    if (inverse) {
        // Undo: restore the removed text in place of what was inserted.
        replaceRange(e.start, e.insertedEnd, e.removed, nullptr);
    } else {
        // Redo: replace the removed span with the inserted text again.
        const Pos oldEnd = advance(e.start, e.removed);
        replaceRange(e.start, oldEnd, e.inserted, nullptr);
    }
}

Pos TextDocument::undo()
{
    if (m_undo.isEmpty())
        return Pos{ -1, -1 };
    m_inUndoRedo = true;
    const int group = m_undo.last().group;
    QList<Edit> edits; // collected in forward order
    while (!m_undo.isEmpty() && m_undo.last().group == group)
        edits.prepend(m_undo.takeLast());

    Pos caret{ 0, 0 };
    for (int i = static_cast<int>(edits.size()) - 1; i >= 0; --i) { // apply in reverse
        applyEdit(edits[i], true);
        caret = edits[i].start;
    }
    for (const Edit& e : edits)
        m_redo.append(e);
    m_inUndoRedo = false;
    return caret;
}

Pos TextDocument::redo()
{
    if (m_redo.isEmpty())
        return Pos{ -1, -1 };
    m_inUndoRedo = true;
    const int group = m_redo.last().group;
    QList<Edit> edits; // collected in forward order
    while (!m_redo.isEmpty() && m_redo.last().group == group)
        edits.prepend(m_redo.takeLast());

    Pos caret{ 0, 0 };
    for (const Edit& e : edits) {
        applyEdit(e, false);
        caret = e.insertedEnd;
    }
    for (const Edit& e : edits)
        m_undo.append(e);
    m_inUndoRedo = false;
    return caret;
}

void TextDocument::markSaved()
{
    if (m_savedRevision != m_revision) {
        m_savedRevision = m_revision;
        emit modifiedChanged();
    }
}

void TextDocument::bumpRevision()
{
    const bool wasModified = (m_revision != m_savedRevision);
    ++m_revision;
    emit revisionChanged();
    const bool nowModified = (m_revision != m_savedRevision);
    if (nowModified != wasModified)
        emit modifiedChanged();
}

} // namespace editor
