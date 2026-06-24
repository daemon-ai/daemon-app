#include "submit_input_box.h"

#include "composer_session_controller.h"
#include "tui_palette.h"

#include <Tui/ZCommon.h>
#include <Tui/ZDocument.h>
#include <Tui/ZDocumentCursor.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>
#include <Tui/ZTextOption.h>

#include <QRect>

SubmitInputBox::SubmitInputBox(const Tui::ZTextMetrics& metrics, Tui::ZWidget* parent)
    : Tui::ZTextEdit(metrics, parent)
{
    setShowLineNumbers(false);
    setWordWrapMode(Tui::ZTextOption::WrapAnywhere);
    setTabChangesFocus(true); // Tab moves focus (completion accept is intercepted)
    setUndoRedoEnabled(true);
    setSizePolicyV(Tui::SizePolicy::Fixed);     // height follows sizeHint() (m_rows)
    setSizePolicyH(Tui::SizePolicy::Expanding); // fill the composer column width

    // Keep the laid-out height and draft in sync when the document changes outside
    // our keyEvent (notably paste). ZDocument::contentsChanged is delivered on the
    // next event-loop turn, so keyEvent also syncs synchronously (below) - that is
    // what keeps a fast Enter right after typing from seeing a stale draft.
    QObject::connect(document(), &Tui::ZDocument::contentsChanged, this, [this] {
        recomputeHeight();
        syncToSession();
    });
}

int SubmitInputBox::linearForPosition(Tui::ZTextEdit::Position pos) const
{
    int linear = 0;
    for (int i = 0; i < pos.line; ++i) {
        linear += document()->lineCodeUnits(i) + 1; // + the joining '\n'
    }
    return linear + pos.codeUnit;
}

Tui::ZTextEdit::Position SubmitInputBox::positionForLinear(int linear) const
{
    int remaining = qMax(0, linear);
    const int lines = document()->lineCount();
    for (int i = 0; i < lines; ++i) {
        const int len = document()->lineCodeUnits(i);
        if (remaining <= len) {
            return { remaining, i };
        }
        remaining -= len + 1; // consume the line plus its '\n'
    }
    const int last = qMax(0, lines - 1);
    return { document()->lineCodeUnits(last), last };
}

int SubmitInputBox::linearCursor() const
{
    return linearForPosition(cursorPosition());
}

void SubmitInputBox::setLinearCursor(int linear)
{
    setCursorPosition(positionForLinear(linear));
}

void SubmitInputBox::moveCursorToEnd()
{
    const int last = qMax(0, document()->lineCount() - 1);
    setCursorPosition({ document()->lineCodeUnits(last), last });
}

void SubmitInputBox::syncToSession()
{
    if (m_session == nullptr) {
        return;
    }
    // While reverse-searching the FSM owns the draft (it previews matches via
    // draftReset); don't echo it back or fire the completion trigger.
    if (m_session->reverseSearching()) {
        return;
    }
    m_session->setDraft(text());
    m_session->refreshTrigger(text(), linearCursor());
}

void SubmitInputBox::recomputeHeight()
{
    const int rows = qBound(1, document()->lineCount(), kMaxRows);
    if (rows == m_rows) {
        return;
    }
    m_rows = rows;
    // Pin the layout height to the row count (the transcript above is the Expanding
    // pane that gives/takes the space). Min == max forces an exact relayout.
    setMinimumSize(8, rows);
    setMaximumSize(Tui::tuiMaxSize, rows);
    updateGeometry();
}

QSize SubmitInputBox::sizeHint() const
{
    QSize hint = Tui::ZTextEdit::sizeHint();
    hint.setHeight(m_rows);
    return hint;
}

bool SubmitInputBox::applyReadline(lineedit::EditCommand cmd)
{
    using EC = lineedit::EditCommand;
    const Tui::ZTextEdit::Position pos = cursorPosition();
    const QString lineText = document()->line(pos.line);
    const int col = pos.codeUnit;

    // Line-boundary deletes join lines: defer to the base editor so a multiline
    // draft behaves like a normal text editor at the seams.
    if (cmd == EC::DeleteChar && col >= lineText.size()) {
        return false;
    }
    if (cmd == EC::BackwardDeleteChar && col == 0) {
        return false;
    }

    // All other emacs commands are line-relative; run the tested pure transform on
    // the current line, then write back the (possibly) changed line + caret column.
    QString edited = lineText;
    int caret = col;
    if (!lineedit::LineEditor::applyCommand(cmd, edited, caret, m_killRing)) {
        return false;
    }
    if (edited != lineText) {
        Tui::ZDocumentCursor cur = textCursor();
        cur.setPosition({ 0, pos.line });
        cur.moveToEndOfLine(true); // select the whole current line
        cur.removeSelectedText();
        cur.insertText(edited);
    }
    setCursorPosition({ qBound(0, caret, document()->lineCodeUnits(pos.line)), pos.line });
    return true;
}

bool SubmitInputBox::handleReverseSearch(Tui::ZKeyEvent* event)
{
    const int key = event->key();
    const Qt::KeyboardModifiers mods = event->modifiers();
    const QString t = event->text();

    // Ctrl+R steps to the next older match; Ctrl+G aborts (readline's abort).
    if (mods & Qt::ControlModifier) {
        if (t.compare(QStringLiteral("r"), Qt::CaseInsensitive) == 0) {
            m_session->reverseSearchNext();
            event->accept();
            return true;
        }
        if (t.compare(QStringLiteral("g"), Qt::CaseInsensitive) == 0) {
            m_session->reverseSearchCancel();
            event->accept();
            return true;
        }
    }
    if (key == Qt::Key_Escape) {
        m_session->reverseSearchCancel();
        event->accept();
        return true;
    }
    if (key == Qt::Key_Backspace) {
        m_session->reverseSearchBackspace();
        event->accept();
        return true;
    }
    if (key == Qt::Key_Enter || key == Qt::Key_Return || t == QStringLiteral("\r")) {
        m_session->reverseSearchAccept(); // keep the match in the draft, no submit
        event->accept();
        return true;
    }
    // A printable character (no Ctrl/Alt) narrows the query.
    if (!t.isEmpty() && !(mods & (Qt::ControlModifier | Qt::AltModifier)) && t.at(0).isPrint()) {
        m_session->reverseSearchType(t);
        event->accept();
        return true;
    }
    // Any other key (arrows, etc.): accept the match, then let the caller process
    // the key against the now-editable draft.
    m_session->reverseSearchAccept();
    return false;
}

void SubmitInputBox::keyEvent(Tui::ZKeyEvent* event)
{
    // (0) Reverse incremental search owns all keys while active.
    if (m_session != nullptr && m_session->reverseSearching()) {
        if (handleReverseSearch(event)) {
            return;
        }
        // Fell through: the FSM accepted the match; continue normal handling.
    }

    // (1) Completion navigation takes priority while the popup is open: Up/Down move
    // the active row, Enter/Tab accept it, Esc dismisses. Driven by the shared
    // controller so the TUI matches the GUI behavior exactly.
    if (m_session != nullptr && m_session->completionActive()
        && event->modifiers() == Qt::NoModifier) {
        const int key = event->key();
        if (key == Qt::Key_Down) {
            m_session->moveActive(1);
            event->accept();
            return;
        }
        if (key == Qt::Key_Up) {
            m_session->moveActive(-1);
            event->accept();
            return;
        }
        if (key == Qt::Key_Enter || key == Qt::Key_Return || key == Qt::Key_Tab
            || event->text() == QStringLiteral("\r")) {
            m_session->acceptActive();
            event->accept();
            return;
        }
        if (key == Qt::Key_Escape) {
            m_session->closeTrigger();
            event->accept();
            return;
        }
    }

    const bool isEnter = event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return
                         || event->text() == QStringLiteral("\r");
    // (2) Enter rules: Shift+Enter inserts a newline, Ctrl+Enter steers, plain Enter
    // submits (matching the GUI's multiline TextArea).
    if (isEnter) {
        if (event->modifiers() == Qt::ShiftModifier) {
            // ZDocument tracks a missing trailing newline implicitly; inserting "\n"
            // at the very end would just collapse that flag (no materialized line, so
            // no visual break). Clear it to force a real split, then restore it so the
            // serialized draft never grows a spurious trailing "\n" for the new empty
            // tail line (the composer's steady-state invariant is flag == true).
            document()->setNewlineAfterLastLineMissing(false);
            insertText(QStringLiteral("\n"));
            document()->setNewlineAfterLastLineMissing(true);
            recomputeHeight();
            // Mirror the base ZTextEdit Enter handler: keep the caret visible and
            // request a repaint now (our custom branch bypasses the base, which is
            // what would otherwise schedule the update).
            adjustScrollPosition();
            update();
            syncToSession();
            event->accept();
            return;
        }
        if (event->modifiers() == Qt::ControlModifier) {
            if (m_session != nullptr) {
                m_session->setDraft(text()); // ensure the controller has the latest draft
                if (m_session->canSteer()) {
                    m_session->steerDraft();
                }
            }
            event->accept(); // swallow either way so Ctrl+Enter never inserts a newline
            return;
        }
        if (event->modifiers() == Qt::NoModifier) {
            if (m_session != nullptr) {
                m_session->setDraft(text()); // submit() reads the controller's draft
            }
            emit submitted(text());
            event->accept();
            return;
        }
    }
    // (3) Ctrl+R enters reverse incremental search (delivered as a char event:
    // key()==Key_unknown, text()=="r", Ctrl held).
    if (m_session != nullptr && (event->modifiers() & Qt::ControlModifier)
        && event->text().compare(QStringLiteral("r"), Qt::CaseInsensitive) == 0) {
        m_session->reverseSearchStart();
        event->accept();
        return;
    }
    // (4) Ctrl+O: add a (mock) attachment, mirroring the GUI's "+" attachment menu.
    // Tui Widgets delivers Ctrl+letter as a char event (key()==Key_unknown, letter
    // in text()), so match the letter rather than Qt::Key_O.
    if ((event->modifiers() & Qt::ControlModifier)
        && (event->key() == Qt::Key_O
            || event->text().compare(QStringLiteral("o"), Qt::CaseInsensitive) == 0)) {
        emit attachRequested();
        event->accept();
        return;
    }
    // (5) Context-sensitive Esc. Precedence: cancel an in-progress queue edit, then
    // stop a running turn, then clear a non-empty draft, then hand focus back to
    // the panes (where a further Esc bubbles to RootWidget's quit prompt).
    if (event->key() == Qt::Key_Escape && event->modifiers() == Qt::NoModifier) {
        if (m_session != nullptr && m_session->editingIndex() >= 0) {
            m_session->exitEdit(true); // restore the pre-edit draft
        } else if (m_session != nullptr && m_session->busy()) {
            m_session->cancel(); // -> cancelRequested -> orchestrator.cancel()
        } else if (!text().isEmpty()) {
            if (m_session != nullptr) {
                m_session->clear(); // clears draft via draftReset (keeps both ends synced)
            } else {
                clear();
            }
        } else {
            emit leaveRequested();
        }
        event->accept();
        return;
    }
    // (6) History: Up/Down walk the sent-message ring, gated like the GUI - Up only
    // with an empty draft or while already browsing, Down only while browsing - and
    // only at the document's first/last line so interior line navigation still works.
    if (event->modifiers() == Qt::NoModifier && m_session != nullptr) {
        const Tui::ZTextEdit::Position pos = cursorPosition();
        const bool atFirstLine = pos.line == 0;
        const bool atLastLine = pos.line == document()->lineCount() - 1;
        if (event->key() == Qt::Key_Up && atFirstLine
            && (text().isEmpty() || m_session->browsing())) {
            emit historyPrevious();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Down && atLastLine && m_session->browsing()) {
            emit historyNext();
            event->accept();
            return;
        }
    }
    // (7) Readline/emacs-style editing (Ctrl+A/E/B/F, Alt+B/F, Ctrl+K/U/W/D, Ctrl+Y,
    // Alt+Y, Ctrl+T ...), honoring ~/.inputrc remaps. Runs after the app-priority
    // keys above so it never shadows submit/steer/attach/history/completion.
    const lineedit::EditCommand cmd
        = lineedit::LineEditor::lookupEvent(event->key(), event->modifiers(), event->text());
    if (cmd != lineedit::EditCommand::None && applyReadline(cmd)) {
        recomputeHeight();
        adjustScrollPosition();
        update();
        syncToSession();
        event->accept();
        return;
    }
    Tui::ZTextEdit::keyEvent(event);
    // Sync synchronously (a typed char or caret move may change the draft / the
    // slash-@ trigger token); the deferred contentsChanged signal also fires later.
    recomputeHeight();
    syncToSession();
}

void SubmitInputBox::paintEvent(Tui::ZPaintEvent* event)
{
    // Base paints the multiline field bg/text and positions the terminal caret.
    Tui::ZTextEdit::paintEvent(event);
    if (!text().isEmpty()) {
        return; // real draft text takes over
    }
    // Overlay a dim placeholder over the editor's (focus-aware) background.
    Tui::ZPainter* p = event->painter();
    const int w = geometry().width();
    if (w <= 0) {
        return;
    }
    const Tui::ZColor fieldBg = focus() ? getColor(QStringLiteral("textedit.focused.bg"))
                                        : getColor(QStringLiteral("textedit.bg"));
    const QString hint = QStringLiteral("Message daemon…  (Enter to send, Shift+Enter newline)");
    p->writeWithColors(0, 0, hint.left(w), tpal::muted(), fieldBg);
}
