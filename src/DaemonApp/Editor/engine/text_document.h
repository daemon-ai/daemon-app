#pragma once

#include "engine/line_buffer.h"

#include <QList>
#include <QObject>
#include <QString>

namespace editor {

// A text position (line, column). Column is a UTF-16 code-unit offset.
struct Pos {
    int line = 0;
    int col = 0;
    [[nodiscard]] bool operator==(const Pos& o) const { return line == o.line && col == o.col; }
    [[nodiscard]] bool operator<(const Pos& o) const {
        return line != o.line ? line < o.line : col < o.col;
    }
};

// The editable document: the block-based LineBuffer plus a single edit primitive
// (replaceRange) on which insert/remove are built, an inverse-op undo stack with
// transaction grouping and typing time-merge, and a monotonic revision. Cursors
// and selection live in the controller; the document only emits the structural
// signals the line model needs. Columns are UTF-16 code units.
class TextDocument : public QObject {
    Q_OBJECT

public:
    explicit TextDocument(QObject* parent = nullptr);

    void setText(const QString& text); // load: replaces all, clears undo, bumps revision
    // User-originated whole-document replacement from a native Qt text control.
    // Keeps the document dirty relative to the last loaded/saved revision.
    void replaceTextFromEditor(const QString& text);
    [[nodiscard]] QString text() const;
    [[nodiscard]] int lineCount() const { return m_buffer.count(); }
    [[nodiscard]] QString line(int i) const { return m_buffer.at(i); }

    // Group several edits into one undo step (re-entrant via a depth counter).
    void beginEdit();
    void endEdit();

    // Edit primitives (record undo). Return the end position of the change.
    Pos insert(const Pos& at, const QString& text);
    Pos remove(const Pos& from, const Pos& to);

    [[nodiscard]] bool canUndo() const { return !m_undo.isEmpty(); }
    [[nodiscard]] bool canRedo() const { return !m_redo.isEmpty(); }
    Pos undo(); // returns a caret position to restore (or {-1,-1} if nothing)
    Pos redo();

    [[nodiscard]] qint64 revision() const { return m_revision; }
    [[nodiscard]] bool modified() const { return m_revision != m_savedRevision; }
    void markSaved();

    [[nodiscard]] Pos clamp(const Pos& p) const;

signals:
    void lineChanged(int line); // in-place single-line edit
    void linesReset();          // structural change: model resets
    void revisionChanged();
    void modifiedChanged();

private:
    struct Edit {
        Pos start;        // replaced-range start
        QString removed;  // text removed (for undo)
        QString inserted; // text inserted (for redo)
        Pos insertedEnd;  // end after inserting `inserted`
        qint64 time = 0;  // ms, for typing merge
        int group = 0;    // transaction id
    };

    // The single mutation primitive: replace [from,to) with `replacement`.
    // Returns the removed text via `removedOut` and the post-insert end position.
    Pos replaceRange(const Pos& from, const Pos& to, const QString& replacement,
                     QString* removedOut);
    [[nodiscard]] QString textInRange(const Pos& from, const Pos& to) const;
    void pushUndo(Edit edit);
    void applyEdit(const Edit& e, bool inverse); // for undo(inverse=true)/redo(false)
    void bumpRevision();                         // ++revision + modified/revision signals

    LineBuffer m_buffer;
    QList<Edit> m_undo;
    QList<Edit> m_redo;
    qint64 m_revision = 0;
    qint64 m_savedRevision = 0;
    int m_editDepth = 0;
    int m_group = 0;
    bool m_inUndoRedo = false;
};

} // namespace editor
