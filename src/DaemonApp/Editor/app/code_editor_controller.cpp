#include "app/code_editor_controller.h"

#include "app/line_model.h"
#include "engine/code_highlighter.h"

namespace editor {

CodeEditorController::CodeEditorController(QObject* parent)
    : QObject(parent), m_doc(new TextDocument(this)), m_hl(new CodeHighlighter(m_doc, this)),
      m_lines(new LineModel(m_doc, m_hl, this)) {
    connect(m_doc, &TextDocument::modifiedChanged, this, &CodeEditorController::modifiedChanged);
    connect(m_doc, &TextDocument::revisionChanged, this, &CodeEditorController::documentChanged);
    connect(m_doc, &TextDocument::linesReset, this, &CodeEditorController::documentChanged);
}

int CodeEditorController::lineCount() const {
    return m_doc->lineCount();
}
bool CodeEditorController::modified() const {
    return m_doc->modified();
}
bool CodeEditorController::canUndo() const {
    return m_doc->canUndo();
}
bool CodeEditorController::canRedo() const {
    return m_doc->canRedo();
}
QString CodeEditorController::languageName() const {
    return m_hl->definitionName();
}
QString CodeEditorController::text() const {
    return m_doc->text();
}
int CodeEditorController::columnsInLine(int line) const {
    return m_doc->line(line).size();
}

Pos CodeEditorController::selMin() const {
    return (m_caret < m_anchor) ? m_caret : m_anchor;
}
Pos CodeEditorController::selMax() const {
    return (m_caret < m_anchor) ? m_anchor : m_caret;
}
int CodeEditorController::selStartLine() const {
    return selMin().line;
}
int CodeEditorController::selStartColumn() const {
    return selMin().col;
}
int CodeEditorController::selEndLine() const {
    return selMax().line;
}
int CodeEditorController::selEndColumn() const {
    return selMax().col;
}

void CodeEditorController::load(const QString& text, const QString& fileName,
                                const QString& revision) {
    m_doc->setText(text);
    m_hl->setLanguageForFile(fileName);
    m_revision = revision;
    m_caret = Pos{0, 0};
    m_anchor = m_caret;
    emit revisionChanged();
    emit languageChanged();
    emit cursorChanged();
    emit selectionChanged();
    emit documentChanged();
}

void CodeEditorController::loadBytes(const QByteArray& bytes, const QString& fileName,
                                     const QString& revision) {
    load(QString::fromUtf8(bytes), fileName, revision);
}

QByteArray CodeEditorController::textBytes() const {
    return text().toUtf8();
}

void CodeEditorController::replaceTextFromEditor(const QString& text) {
    m_doc->replaceTextFromEditor(text);
    m_caret = m_doc->clamp(m_caret);
    m_anchor = m_doc->clamp(m_anchor);
    emit cursorChanged();
    emit selectionChanged();
}

void CodeEditorController::markSaved(const QString& newRevision) {
    m_revision = newRevision;
    emit revisionChanged();
    m_doc->markSaved();
}

void CodeEditorController::setDarkTheme(bool dark) {
    m_hl->setDarkTheme(dark);
}

void CodeEditorController::ensureViewport(int firstRow, int lastRow) {
    Q_UNUSED(firstRow); // highlighting is always contiguous from the watermark
    if (!m_hl || !m_lines)
        return;
    const int realLast = m_lines->realLine(lastRow);
    if (realLast >= 0)
        m_hl->ensureHighlighted(realLast);
}

void CodeEditorController::setCaret(const Pos& p, bool extend) {
    m_caret = m_doc->clamp(p);
    if (!extend)
        m_anchor = m_caret;
    emit cursorChanged();
    emit selectionChanged();
}

void CodeEditorController::deleteSelection() {
    const Pos a = selMin();
    const Pos b = selMax();
    m_doc->remove(a, b);
    m_caret = a;
    m_anchor = a;
}

void CodeEditorController::insertText(const QString& s) {
    if (s.isEmpty())
        return;
    m_doc->beginEdit();
    if (hasSelection())
        deleteSelection();
    m_caret = m_doc->insert(m_caret, s);
    m_anchor = m_caret;
    m_doc->endEdit();
    emit cursorChanged();
    emit selectionChanged();
    emit documentChanged();
}

void CodeEditorController::insertNewline() {
    insertText(QStringLiteral("\n"));
}

void CodeEditorController::removeBackward() {
    m_doc->beginEdit();
    if (hasSelection()) {
        deleteSelection();
    } else {
        const Pos c = m_caret;
        if (c.col > 0) {
            m_doc->remove(Pos{c.line, c.col - 1}, c);
            m_caret = Pos{c.line, c.col - 1};
        } else if (c.line > 0) {
            const int prevLen = m_doc->line(c.line - 1).size();
            m_doc->remove(Pos{c.line - 1, prevLen}, Pos{c.line, 0});
            m_caret = Pos{c.line - 1, prevLen};
        }
        m_anchor = m_caret;
    }
    m_doc->endEdit();
    emit cursorChanged();
    emit selectionChanged();
    emit documentChanged();
}

void CodeEditorController::removeForward() {
    m_doc->beginEdit();
    if (hasSelection()) {
        deleteSelection();
    } else {
        const Pos c = m_caret;
        const int len = m_doc->line(c.line).size();
        if (c.col < len)
            m_doc->remove(c, Pos{c.line, c.col + 1});
        else if (c.line < m_doc->lineCount() - 1)
            m_doc->remove(c, Pos{c.line + 1, 0});
        m_caret = m_doc->clamp(c);
        m_anchor = m_caret;
    }
    m_doc->endEdit();
    emit cursorChanged();
    emit selectionChanged();
    emit documentChanged();
}

void CodeEditorController::undo() {
    const Pos p = m_doc->undo();
    if (p.line >= 0)
        setCaret(p, false);
    emit documentChanged();
}

void CodeEditorController::redo() {
    const Pos p = m_doc->redo();
    if (p.line >= 0)
        setCaret(p, false);
    emit documentChanged();
}

void CodeEditorController::setCursor(int line, int col, bool extend) {
    setCaret(Pos{line, col}, extend);
}

void CodeEditorController::moveLeft(bool extend) {
    Pos c = m_caret;
    if (c.col > 0)
        --c.col;
    else if (c.line > 0) {
        --c.line;
        c.col = m_doc->line(c.line).size();
    }
    setCaret(c, extend);
}

void CodeEditorController::moveRight(bool extend) {
    Pos c = m_caret;
    if (c.col < m_doc->line(c.line).size())
        ++c.col;
    else if (c.line < m_doc->lineCount() - 1) {
        ++c.line;
        c.col = 0;
    }
    setCaret(c, extend);
}

void CodeEditorController::moveUp(bool extend) {
    Pos c = m_caret;
    if (c.line > 0)
        --c.line;
    setCaret(c, extend);
}

void CodeEditorController::moveDown(bool extend) {
    Pos c = m_caret;
    if (c.line < m_doc->lineCount() - 1)
        ++c.line;
    setCaret(c, extend);
}

void CodeEditorController::moveHome(bool extend) {
    setCaret(Pos{m_caret.line, 0}, extend);
}

void CodeEditorController::moveEnd(bool extend) {
    setCaret(Pos{m_caret.line, static_cast<int>(m_doc->line(m_caret.line).size())}, extend);
}

void CodeEditorController::selectAll() {
    const int last = m_doc->lineCount() - 1;
    m_anchor = Pos{0, 0};
    m_caret = Pos{last, static_cast<int>(m_doc->line(last).size())};
    emit cursorChanged();
    emit selectionChanged();
}

void CodeEditorController::toggleFoldAt(int realLine) {
    m_lines->toggleFold(realLine);
}

bool CodeEditorController::find(const QString& query, bool forward, bool caseSensitive) {
    if (query.isEmpty())
        return false;
    const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    const int n = m_doc->lineCount();
    if (n == 0)
        return false;
    const Pos from = hasSelection() ? (forward ? selMax() : selMin()) : m_caret;

    for (int offset = 0; offset <= n; ++offset) {
        if (forward) {
            int line = (from.line + offset) % n;
            const QString t = m_doc->line(line);
            const int startCol = (offset == 0) ? from.col : 0;
            const int idx = t.indexOf(query, startCol, cs);
            if (idx >= 0) {
                m_anchor = Pos{line, idx};
                m_caret = Pos{line, idx + static_cast<int>(query.size())};
                emit cursorChanged();
                emit selectionChanged();
                return true;
            }
        } else {
            int line = ((from.line - offset) % n + n) % n;
            const QString t = m_doc->line(line);
            const int fromCol = (offset == 0) ? qMax(0, from.col - 1) : t.size();
            const int idx = t.lastIndexOf(query, fromCol, cs);
            if (idx >= 0) {
                m_anchor = Pos{line, idx};
                m_caret = Pos{line, idx + static_cast<int>(query.size())};
                emit cursorChanged();
                emit selectionChanged();
                return true;
            }
        }
    }
    return false;
}

} // namespace editor
