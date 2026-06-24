#pragma once

#include "app/line_model.h"
#include "engine/text_document.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace editor {

class CodeHighlighter;

// The shared editor controller: owns the TextDocument, the incremental
// CodeHighlighter, and the LineModel used by the TUI. The GUI editor renders via
// native TextArea/QTextDocument, while the TUI consumes `lines` as raw text plus
// structured style runs. Multi-cursor and LSP completion are deliberate follow-ups.
class CodeEditorController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(editor::LineModel* lines READ lines CONSTANT)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY documentChanged)
    Q_PROPERTY(int cursorLine READ cursorLine NOTIFY cursorChanged)
    Q_PROPERTY(int cursorColumn READ cursorColumn NOTIFY cursorChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(int selStartLine READ selStartLine NOTIFY selectionChanged)
    Q_PROPERTY(int selStartColumn READ selStartColumn NOTIFY selectionChanged)
    Q_PROPERTY(int selEndLine READ selEndLine NOTIFY selectionChanged)
    Q_PROPERTY(int selEndColumn READ selEndColumn NOTIFY selectionChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(QString revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(QString languageName READ languageName NOTIFY languageChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY documentChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY documentChanged)

public:
    explicit CodeEditorController(QObject* parent = nullptr);

    [[nodiscard]] LineModel* lines() const { return m_lines; }
    [[nodiscard]] int lineCount() const;
    [[nodiscard]] int cursorLine() const { return m_caret.line; }
    [[nodiscard]] int cursorColumn() const { return m_caret.col; }
    [[nodiscard]] bool hasSelection() const { return !(m_caret == m_anchor); }
    [[nodiscard]] int selStartLine() const;
    [[nodiscard]] int selStartColumn() const;
    [[nodiscard]] int selEndLine() const;
    [[nodiscard]] int selEndColumn() const;
    [[nodiscard]] bool modified() const;
    [[nodiscard]] QString revision() const { return m_revision; }
    [[nodiscard]] QString languageName() const;
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;

    // Load file content (resets undo); `fileName` selects the syntax definition.
    Q_INVOKABLE void load(const QString& text, const QString& fileName, const QString& revision);
    // Convenience for the FilePage: load raw bytes (decoded as UTF-8).
    Q_INVOKABLE void loadBytes(const QByteArray& bytes, const QString& fileName,
                               const QString& revision);
    [[nodiscard]] Q_INVOKABLE QString text() const;
    [[nodiscard]] Q_INVOKABLE QByteArray textBytes() const;
    // Called by the native QML text surface for user-originated edits. Loads use
    // load()/loadBytes() so they do not mark the document dirty.
    Q_INVOKABLE void replaceTextFromEditor(const QString& text);
    Q_INVOKABLE void markSaved(const QString& newRevision);
    Q_INVOKABLE void setDarkTheme(bool dark);
    [[nodiscard]] Q_INVOKABLE int columnsInLine(int line) const;
    // Kate-style viewport drive: ensure highlighting is up to date for the band of
    // visible rows [firstRow, lastRow] (mapped to real lines) in one call, instead
    // of relying solely on per-row lazy pulls. Called from the view on scroll.
    Q_INVOKABLE void ensureViewport(int firstRow, int lastRow);

    // Editing.
    Q_INVOKABLE void insertText(const QString& s);
    Q_INVOKABLE void insertNewline();
    Q_INVOKABLE void removeBackward(); // Backspace
    Q_INVOKABLE void removeForward();  // Delete
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    // Caret / selection.
    Q_INVOKABLE void setCursor(int line, int col, bool extend);
    Q_INVOKABLE void moveLeft(bool extend);
    Q_INVOKABLE void moveRight(bool extend);
    Q_INVOKABLE void moveUp(bool extend);
    Q_INVOKABLE void moveDown(bool extend);
    Q_INVOKABLE void moveHome(bool extend);
    Q_INVOKABLE void moveEnd(bool extend);
    Q_INVOKABLE void selectAll();

    // Folding + find.
    Q_INVOKABLE void toggleFoldAt(int realLine);
    // Find from the caret; selects the match and returns true if found.
    Q_INVOKABLE bool find(const QString& query, bool forward, bool caseSensitive);

signals:
    void documentChanged();
    void cursorChanged();
    void selectionChanged();
    void modifiedChanged();
    void revisionChanged();
    void languageChanged();

private:
    void setCaret(const Pos& p, bool extend);
    void deleteSelection(); // assumes hasSelection
    [[nodiscard]] Pos selMin() const;
    [[nodiscard]] Pos selMax() const;

    TextDocument* m_doc = nullptr;
    CodeHighlighter* m_hl = nullptr;
    LineModel* m_lines = nullptr;
    Pos m_caret;
    Pos m_anchor;
    QString m_revision; // the IFsService content revision (save base)
};

} // namespace editor
