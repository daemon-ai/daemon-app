#include "root_widget.h"

#include "display_role_adapter.h"
#include "tui_palette.h"

#include "session_controller.h"
#include "session_orchestrator.h"
#include "sessions_list_model.h"
#include "sidebar_model.h"
#include "todo_list_model.h"

#include "composer_session_controller.h"
#include "command_registry.h"
#include "status_bar_model.h"
#include "transcript_exporter.h"
#include "tab_model.h"
#include "turn_controller.h"

#include "persistence/sqlite_session_store.h"

#include "fs_explorer_model.h"
#include "app/code_editor_controller.h"
#include "fs/local_disk_fs_service.h"

#include "memory_graph_model.h"
#include "memory_list_model.h"
#include "memory_stats_model.h"
#include "memory_timeline_model.h"
#include "memory/mock_memory_service.h"

#include "accounts/mock_accounts_service.h"
#include "config/mock_daemon_config.h"
#include "connection/mock_connection_service.h"
#include "automation/mock_cron_store.h"
#include "automation/mock_routing_store.h"
#include "fleet/mock_approvals_inbox.h"
#include "fleet/mock_dashboard.h"
#include "fleet/mock_fleet_tree.h"
#include "fleet/mock_session_roster.h"
#include "models/mock_model_catalog.h"
#include "profiles/mock_profile_store.h"
#include "session/mock_checkpoint_timeline.h"
#include "session/mock_session_settings.h"
#include "uimodels/variant_list_model.h"
#include "nav/nav_controller.h"
#include "settings/qt_settings_store.h"

#include <Tui/ZButton.h>
#include <Tui/ZCommon.h>
#include <Tui/ZDialog.h>
#include <Tui/ZDocument.h>
#include <Tui/ZDocumentCursor.h>
#include <Tui/ZEvent.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>
#include <Tui/ZPainter.h>
#include <Tui/ZRoot.h>
#include <Tui/ZShortcut.h>
#include <Tui/ZTerminal.h>

#include <cstdio>
#include <Tui/ZTextOption.h>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWidget.h>
#include <Tui/ZWindow.h>

// Vendored same-rev private header: exposes ZListViewPrivate so we can read the
// sidebar list's private scroll offset (no public accessor exists).
#include <Tui/ZListView_p.h>

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QItemSelectionModel>
#include <QProcess>
#include <QRect>
#include <QSettings>
#include <QStandardPaths>
#include <QVariantMap>

namespace {

// Best-effort desktop notification from the TUI: notify-send when present
// (reliable on Linux desktops), plus an OSC 9 escape as a terminal-native
// fallback. The BEL urgency hint is rung separately by the caller.
void emitDesktopNotification(const QString& title, const QString& body)
{
    const QString notifySend = QStandardPaths::findExecutable(QStringLiteral("notify-send"));
    if (!notifySend.isEmpty()) {
        QProcess::startDetached(notifySend, QStringList{ title, body });
    }
    // OSC 9 (iTerm2-style growl): ESC ] 9 ; <text> BEL.
    const QString msg = body.isEmpty() ? title : (title + QStringLiteral(" \u2014 ") + body);
    std::fputs(QStringLiteral("\033]9;%1\a").arg(msg).toUtf8().constData(), stdout);
    std::fflush(stdout);
}

} // namespace

// Per-transcript-tab backend state (see the forward declaration in root_widget.h).
// Owns its QObjects unparented and deletes them on close; the document + ingest are
// value members (the ingest holds a stable &doc back-pointer because sessions live
// behind pointers in the map and are never moved).
struct TabSession {
    int tabId = -1;
    int sessionId = -1;
    SessionController* controller = nullptr;
    SessionOrchestrator* orchestrator = nullptr; // owns `turn`
    TurnController* turn = nullptr;
    InteractiveTurnHost* host = nullptr;
    be::DocumentStore doc;
    be::TranscriptIngest ingest { &doc };
    // Per-tab in-transcript find engine, bound to this tab's document. refresh()
    // is called at the transcript-reload sites so matches track the live content.
    be::TranscriptSearchController search;

    TabSession() { search.setDocument(&doc); }

    ~TabSession()
    {
        delete host;
        delete orchestrator; // deletes its child TurnController
        delete controller;
    }
};

namespace {

// A short tab title for a session: the first non-empty content line (heading
// markers stripped, capped), falling back to a generic label.
QString titleForContent(const QString& markdown)
{
    const QString trimmed = markdown.trimmed();
    if (trimmed.isEmpty()) {
        return QStringLiteral("New session");
    }
    QString first = trimmed.section(QLatin1Char('\n'), 0, 0);
    while (first.startsWith(QLatin1Char('#'))) {
        first.remove(0, 1);
    }
    first = first.trimmed();
    if (first.isEmpty()) {
        return QStringLiteral("Session");
    }
    return first.left(24);
}

// The static markdown shown for a (non-transcript) page tab.
QString pageMarkdown(int kind)
{
    if (kind == TabModel::Settings) {
        return QStringLiteral(
            "# Settings\n\n"
            "A generic, non-transcript page hosted by the same tab strip.\n\n"
            "- Press **F8** to cycle the theme (Light / Dark / Sepia / Midnight)\n"
            "- **Ctrl+T** opens a new transcript tab\n"
            "- **Ctrl+W** closes the current tab\n"
            "- **Ctrl+Tab** / **Ctrl+Shift+Tab** switch tabs (wrapping)\n"
            "- **Alt+1..9** jumps directly to tab N\n"
            "- **Tab** cycles panes, including the tab strip; when it is focused, "
            "**Left**/**Right** switch tabs, **Enter** pins a preview, **Delete** closes\n"
            "- In an interactive block (approval / clarify): **Up**/**Down**/**Left**/"
            "**Right** move between controls, **Space** toggles, **Enter** submits, "
            "**Tab** leaves to the next pane\n"
            "- In the transcript: **r** opens the rewind picker over your prior "
            "messages (**Up**/**Down** to choose, **Enter** restores & re-runs, "
            "**e** edits in the composer, **Esc** cancels)\n"
            "- **/retry**, **/edit**, **/undo** rewind the last message from the "
            "composer\n");
    }
    return QString();
}

} // namespace

SubmitInputBox::SubmitInputBox(const Tui::ZTextMetrics& metrics, Tui::ZWidget* parent)
    : Tui::ZTextEdit(metrics, parent)
{
    setShowLineNumbers(false);
    setWordWrapMode(Tui::ZTextOption::WrapAnywhere);
    setTabChangesFocus(true); // Tab moves focus (completion accept is intercepted)
    setUndoRedoEnabled(true);
    setSizePolicyV(Tui::SizePolicy::Fixed);    // height follows sizeHint() (m_rows)
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
    const QString hint = QStringLiteral("Message daemon\u2026  (Enter to send, Shift+Enter newline)");
    p->writeWithColors(0, 0, hint.left(w), tpal::muted(), fieldBg);
}

void SearchInputBox::setTypingActive(bool active)
{
    if (m_typingActive == active) {
        return;
    }
    m_typingActive = active;
    update();
}

void SearchInputBox::paintEvent(Tui::ZPaintEvent* event)
{
    // Fully custom paint: this box never edits text itself (the session list
    // forwards keystrokes), so we draw a search affordance instead of the stock
    // line-edit chrome.
    Tui::ZPainter* p = event->painter();
    const Tui::ZColor bg = tpal::surfaceAlt();
    p->clear(tpal::fg(), bg);
    const int w = geometry().width();
    if (w <= 0) {
        return;
    }
    // Always-visible magnifier marks the row as a search field.
    int x = 0;
    p->writeWithColors(x, 0, QStringLiteral("\u2315 "), tpal::muted(), bg);
    x += 2;

    const QString q = text();
    if (q.isEmpty()) {
        if (x < w) {
            p->writeWithColors(x, 0, QStringLiteral("Search sessions").left(w - x),
                               tpal::muted(), bg);
        }
        return;
    }
    // Show the query (its tail if it overflows) and, while the list is focused, an
    // accent bar caret after it to mark where typing lands.
    const int caretReserve = m_typingActive ? 1 : 0;
    const int budget = qMax(0, w - x - caretReserve);
    const QString shown = q.size() > budget ? q.right(budget) : q;
    p->writeWithColors(x, 0, shown, tpal::fg(), bg);
    x += static_cast<int>(shown.size());
    if (m_typingActive && x < w) {
        p->writeWithColors(x, 0, QStringLiteral("\u258f"), tpal::accent(), bg);
    }
}

void TranscriptSearchBox::keyEvent(Tui::ZKeyEvent* event)
{
    const int key = event->key();
    const bool shift = (event->modifiers() & Qt::ShiftModifier) != 0;
    if (key == Qt::Key_Escape) {
        emit closeRequested();
        event->accept();
        return;
    }
    if (key == Qt::Key_Enter || key == Qt::Key_Return
        || event->text() == QStringLiteral("\r")) {
        if (shift) {
            emit previousRequested();
        } else {
            emit nextRequested();
        }
        event->accept();
        return;
    }
    if (key == Qt::Key_Down) {
        emit nextRequested();
        event->accept();
        return;
    }
    if (key == Qt::Key_Up) {
        emit previousRequested();
        event->accept();
        return;
    }
    Tui::ZInputBox::keyEvent(event);
}

void TreeListView::keyEvent(Tui::ZKeyEvent* event)
{
    if (event->modifiers() == Qt::NoModifier) {
        if (event->key() == Qt::Key_Left) {
            emit collapseRequested();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Right) {
            emit expandRequested();
            event->accept();
            return;
        }
    }
    Tui::ZListView::keyEvent(event);
}

int TreeListView::scrollOffset() const
{
    // ZListView paints model row (scrollPosition + i) at local y == i. The offset
    // lives in the private object with no public accessor; read it through the
    // vendored same-rev ZListViewPrivate (validated by the tui_magic in the base).
    const auto* priv = static_cast<const Tui::ZListViewPrivate*>(
        Tui::ZWidgetPrivate::get(this));
    if (priv == nullptr || priv->tui_magic != tui_magic_v0) {
        return 0;
    }
    return priv->scrollPosition;
}

void TreeListView::scrollByLines(int delta)
{
    QAbstractItemModel* m = model();
    if (m == nullptr) {
        return;
    }
    const int rows = m->rowCount();
    if (rows <= 0) {
        return;
    }
    const int cur = currentIndex().isValid() ? currentIndex().row() : 0;
    const int next = qBound(0, cur + delta, rows - 1);
    if (next != cur) {
        setCurrentIndex(m->index(next, 0));
    }
}

void TreeListView::clickAt(QPoint local)
{
    QAbstractItemModel* m = model();
    if (m == nullptr) {
        return;
    }
    const int rows = m->rowCount();
    if (rows <= 0) {
        return;
    }
    // ZListView paints model row (scrollPosition + i) at local y == i; add the
    // (otherwise-private) scroll offset so clicks map to the right row even when
    // the tree is taller than the viewport and scrolled.
    const int row = local.y() + scrollOffset();
    if (row < 0 || row >= rows) {
        return;
    }
    const QModelIndex idx = m->index(row, 0);

    // Disclosure toggle: a click on a parent row's expand triangle expands or
    // collapses it (via the existing signals, which act on the model's current
    // row - so set the current index first). ZListView lays each row out as
    // "1 margin col + left-decoration + decoration-space + display string", and
    // the display string (DisplayRoleAdapter) begins with depth*2 indent spaces
    // then the triangle, so that is where the triangle sits.
    if (m->data(idx, SidebarModel::HasChildrenRole).toBool()) {
        const int depth = m->data(idx, SidebarModel::DepthRole).toInt();
        const bool hasDecoration = !m->data(idx, Tui::LeftDecorationRole).toString().isEmpty();
        const int decorationSpace = m->data(idx, Tui::LeftDecorationSpaceRole).toInt();
        const int triangleX = 1 + (hasDecoration ? 1 : 0) + decorationSpace + depth * 2;
        if (local.x() <= triangleX + 1) { // +1 tolerance: triangle + trailing space
            setCurrentIndex(idx);
            if (m->data(idx, SidebarModel::ExpandedRole).toBool()) {
                emit collapseRequested();
            } else {
                emit expandRequested();
            }
            return;
        }
    }
    setCurrentIndex(idx);
}

QuitDialog::QuitDialog(Tui::ZWidget* parent) : Tui::ZDialog(parent)
{
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(QStringLiteral("Quit"));
    setContentsMargins({ 2, 1, 2, 1 });

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    auto* message = new Tui::ZLabel(QStringLiteral("Quit daemon-app?"), this);
    layout->addWidget(message);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();

    auto* quitButton = new Tui::ZButton(QStringLiteral("Quit"), this);
    quitButton->setDefault(true); // Enter confirms
    buttons->addWidget(quitButton);

    auto* cancelButton = new Tui::ZButton(QStringLiteral("Cancel"), this);
    buttons->addWidget(cancelButton);

    connect(quitButton, &Tui::ZButton::clicked, this, &QuitDialog::quitConfirmed);
    connect(cancelButton, &Tui::ZButton::clicked, this, &Tui::ZDialog::reject);

    // The user explicitly asked to quit, so focus Quit: Enter confirms, while Esc
    // (ZDialog::reject) and Tab->Cancel back out.
    quitButton->setFocus();
}

TextPromptDialog::TextPromptDialog(const QString& title, const QString& initial, bool masked,
                                   Tui::ZWidget* parent)
    : Tui::ZDialog(parent)
{
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(title);
    setContentsMargins({ 2, 1, 2, 1 });

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    m_input = new Tui::ZInputBox(initial, this);
    if (masked) {
        m_input->setEchoMode(Tui::ZInputBox::Password);
    }
    layout->addWidget(m_input);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();

    auto* ok = new Tui::ZButton(QStringLiteral("OK"), this);
    ok->setDefault(true);
    buttons->addWidget(ok);
    auto* cancel = new Tui::ZButton(QStringLiteral("Cancel"), this);
    buttons->addWidget(cancel);

    connect(ok, &Tui::ZButton::clicked, this, [this] {
        emit submitted(m_input->text());
        close();
    });
    connect(cancel, &Tui::ZButton::clicked, this, &Tui::ZDialog::reject);
    connect(this, &Tui::ZDialog::rejected, this, &TextPromptDialog::canceled);

    m_input->setFocus();
    setGeometry(QRect(0, 0, qMax(40, static_cast<int>(initial.size()) + 16), 7));
}

ConfirmDialog::ConfirmDialog(const QString& title, const QString& message, Tui::ZWidget* parent)
    : Tui::ZDialog(parent)
{
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(title);
    setContentsMargins({ 2, 1, 2, 1 });

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    auto* label = new Tui::ZLabel(message, this);
    layout->addWidget(label);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();

    auto* yes = new Tui::ZButton(QStringLiteral("Yes"), this);
    buttons->addWidget(yes);
    auto* no = new Tui::ZButton(QStringLiteral("Cancel"), this);
    no->setDefault(true); // safe default for a destructive action
    buttons->addWidget(no);

    connect(yes, &Tui::ZButton::clicked, this, [this] {
        emit confirmed();
        close();
    });
    connect(no, &Tui::ZButton::clicked, this, &Tui::ZDialog::reject);

    no->setFocus();
    setGeometry(QRect(0, 0, qMax(40, static_cast<int>(message.size()) + 8), 7));
}

FirstRunDialog::FirstRunDialog(firstrun::FirstRunModel* model,
                               connection::IConnectionService* connection,
                               settings::ISettingsStore* settings, const QString& defaultTarget,
                               Tui::ZWidget* parent)
    : Tui::ZDialog(parent)
    , m_model(model)
    , m_connection(connection)
    , m_settings(settings)
{
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(QStringLiteral("Setup Required"));
    setContentsMargins({ 2, 1, 2, 1 });

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    auto* intro = new Tui::ZLabel(
        QStringLiteral("Connect to a daemon node to get started. Choose a transport:"), this);
    layout->addWidget(intro);

    // Transport mode "cards": the TUI analog of ConnectionPicker's cards. Embedded
    // is shown disabled (coming soon); Local (Unix socket) / Remote (network).
    auto* modes = new Tui::ZHBoxLayout();
    layout->add(modes);
    auto* embeddedBtn = new Tui::ZButton(QStringLiteral("Embedded"), this);
    embeddedBtn->setEnabled(false); // in-process node: coming soon
    modes->addWidget(embeddedBtn);
    m_localBtn = new Tui::ZButton(QStringLiteral("Local"), this);
    modes->addWidget(m_localBtn);
    m_remoteBtn = new Tui::ZButton(QStringLiteral("Remote"), this);
    modes->addWidget(m_remoteBtn);
    layout->addSpacing(1);

    m_target = new Tui::ZInputBox(defaultTarget, this);
    layout->addWidget(m_target);

    // Auth token: only relevant for a remote node; hidden in local mode.
    m_token = new Tui::ZInputBox(QString(), this);
    m_token->setEchoMode(Tui::ZInputBox::Password);
    layout->addWidget(m_token);
    layout->addSpacing(1);

    m_status = new Tui::ZLabel(QString(), this);
    layout->addWidget(m_status);
    m_testResult = new Tui::ZLabel(QString(), this);
    layout->addWidget(m_testResult);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    m_testBtn = new Tui::ZButton(QStringLiteral("Test"), this);
    buttons->addWidget(m_testBtn);
    buttons->addStretch();

    m_primary = new Tui::ZButton(QStringLiteral("Connect"), this);
    m_primary->setDefault(true);
    buttons->addWidget(m_primary);

    auto* skip = new Tui::ZButton(QStringLiteral("Skip"), this);
    buttons->addWidget(skip);

    connect(m_localBtn, &Tui::ZButton::clicked, this,
            [this] { applyMode(QStringLiteral("local")); });
    connect(m_remoteBtn, &Tui::ZButton::clicked, this,
            [this] { applyMode(QStringLiteral("remote")); });

    connect(m_testBtn, &Tui::ZButton::clicked, this, [this] {
        if (m_connection != nullptr && !m_target->text().isEmpty()) {
            m_testResult->setText(QStringLiteral("Testing..."));
            m_connection->testConnection(m_mode, m_target->text(), m_token->text());
        }
    });
    if (m_connection != nullptr) {
        connect(m_connection, &connection::IConnectionService::testResult, this,
                [this](bool ok, const QString& message) {
                    m_testResult->setText((ok ? QStringLiteral("OK — ")
                                              : QStringLiteral("Failed — "))
                                          + message);
                });
    }

    connect(m_primary, &Tui::ZButton::clicked, this, [this] {
        if (m_model->phase() == QStringLiteral("inference")) {
            m_model->completeInference();
        } else if (m_connection != nullptr) {
            // Persist the choice so the next boot reuses it (parity with the GUI
            // picker writing AppSettings.setLastConnection before connecting).
            if (m_settings != nullptr) {
                m_settings->setLastConnection(m_mode, m_target->text());
            }
            m_connection->connectTo(m_mode, m_target->text(), m_token->text());
        }
    });
    connect(skip, &Tui::ZButton::clicked, this, [this] { m_model->skip(); });

    // The model owns the flow; reflect each phase and close once done.
    connect(m_model, &firstrun::FirstRunModel::phaseChanged, this, [this] {
        if (m_model->phase() == QStringLiteral("done")) {
            close();
        } else {
            syncToPhase();
        }
    });

    applyMode(m_mode);
    syncToPhase();
    m_target->setFocus();
    setGeometry(QRect(0, 0, 66, 16));
}

void FirstRunDialog::applyMode(const QString& mode)
{
    m_mode = mode;
    const bool remote = mode == QStringLiteral("remote");
    // Mark the active card by (de)emphasising via the default flag, and seed the
    // target with a representative placeholder if the field is empty.
    if (m_localBtn != nullptr) {
        m_localBtn->setText(remote ? QStringLiteral("Local")
                                   : QStringLiteral("[Local]"));
    }
    if (m_remoteBtn != nullptr) {
        m_remoteBtn->setText(remote ? QStringLiteral("[Remote]")
                                    : QStringLiteral("Remote"));
    }
    if (m_token != nullptr) {
        m_token->setVisible(remote);
    }
    if (m_target != nullptr && m_target->text().isEmpty()) {
        m_target->setText(remote ? QStringLiteral("https://node.example:8080")
                                 : QStringLiteral("/run/daemon/daemon.sock"));
    }
    if (m_testResult != nullptr) {
        m_testResult->setText(QString());
    }
}

void FirstRunDialog::syncToPhase()
{
    const QString p = m_model->phase();
    if (p == QStringLiteral("connecting")) {
        m_status->setText(QStringLiteral("Connecting..."));
        m_primary->setText(QStringLiteral("Connect"));
        m_primary->setEnabled(false);
        m_target->setEnabled(false);
    } else if (p == QStringLiteral("inference")) {
        m_status->setText(QStringLiteral("Connected. A default model will be used."));
        m_primary->setText(QStringLiteral("Finish"));
        m_primary->setEnabled(true);
        m_target->setEnabled(false);
    } else { // connect
        m_status->setText(m_model->error().isEmpty() ? QString() : m_model->error());
        m_primary->setText(QStringLiteral("Connect"));
        m_primary->setEnabled(true);
        m_target->setEnabled(true);
    }
}

RootWidget::RootWidget()
{
    // Build the stock-widget palette (incl. the quit dialog frame/body) from the
    // active theme - set from the persisted ui/theme in main() before we run.
    setPalette(daemonPalette(tpal::activeTheme()));

    // The reused layer: store + view models, wired exactly as in the GUI. None
    // of this depends on Tui Widgets - the same objects back the QML frontend.
    m_store = new persistence::SqliteSessionStore(QString(), this);

    m_sidebar = new SidebarModel(this);
    m_sidebar->setStore(m_store);

    m_list = new SessionsListModel(this);
    m_list->setStore(m_store);

    // The shared composer FSM - the same C++ class the QML composer drives. The
    // TUI binds its single-line input to it, gaining draft persistence, history,
    // and the submit/queue/drain logic without re-implementing any of it. One
    // composer is shared across tabs; its sessionId follows the active tab and
    // its intents are dispatched to the active tab's orchestrator.
    m_composerSession = new ComposerSessionController(this);

    // The shared status-bar model - the same C++ class StatusBar.qml binds. One
    // footer for the whole app; the active tab's turn drives its busy/timer.
    m_status = new StatusBarModel(this);
    // StatusBarModel seeds sessionStartedAt at construction (= launch), mirroring
    // StatusBar.qml's Component.onCompleted.

    // The shared command-palette catalog (same class the GUI binds as `Commands`).
    m_commands = new CommandRegistry(this);

    // Transcript exporter for the list "export" action + /save.
    m_exporter = new TranscriptExporter(this);

    // The shared pane-tab model (the same C++ class the QML TabBar binds). It is
    // the single source of truth for the open tabs and the active one; the TUI
    // creates a per-tab TabSession on demand and binds the views to the active one.
    m_tabModel = new TabModel(this);
    connect(m_tabModel, &TabModel::currentTabChanged, this, [this](int tabId) {
        if (tabId >= 0) {
            activateTab(tabId);
        }
    });
    connect(m_tabModel, &TabModel::tabClosed, this, &RootWidget::destroySession);
    // A preview tab was reassigned to another session: rebind its session
    // in place rather than spawning a new one.
    connect(m_tabModel, &TabModel::tabSessionChanged, this,
            [this](int tabId, int sessionId) { rebindSession(tabId, sessionId); });
    connect(m_tabModel, &TabModel::tabKindChanged, this, [this](int tabId) {
        destroySession(tabId);
    });
    connect(m_tabModel, &TabModel::tabFileChanged, this, [this](int tabId, const QString&, const QString&) {
        const bool active = m_tabModel->indexOfTabId(tabId) == m_tabModel->currentIndex();
        destroySession(tabId);
        if (active)
            activateTab(tabId);
    });

    // Sidebar scope selection drives the session list - the model-to-model
    // contract is untouched by the choice of toolkit.
    connect(m_sidebar, &SidebarModel::scopeSelected, m_list, &SessionsListModel::setScope);

    // File tree seam + model, shared verbatim with the GUI. Serve the daemon-config
    // workspace root (the GUI's Application does the same); a daemon adapter
    // replaces it later. The editor controllers are created per File tab.
    m_daemonConfig = new config::MockDaemonConfig(this);
    m_fs = new fs::LocalDiskFsService(
        m_daemonConfig->value(QStringLiteral("workspace/root")).toString(), QString(), this);
    m_fileTree = new files::FsExplorerModel(this);
    m_fileTree->setService(m_fs);
    // Resolve async file reads/writes back to the owning File tab's controller.
    connect(m_fs, &fs::IFsService::fileRead, this,
            [this](const QString& rootId, const QString& path, const QByteArray& bytes,
                   const QString& revision, bool binary, bool /*truncated*/) {
                editor::CodeEditorController* c =
                    m_fileByKey.value(rootId + QChar(0x1f) + path, nullptr);
                if (c != nullptr && !binary) {
                    c->loadBytes(bytes, path, revision);
                    if (m_fileStatus != nullptr)
                        m_fileStatus->setText(QString());
                } else if (c != nullptr && m_fileStatus != nullptr) {
                    m_fileStatus->setText(QStringLiteral("Binary file - not editable"));
                }
            });
    connect(m_fs, &fs::IFsService::writeResult, this,
            [this](const QString& rootId, const QString& path, bool ok, const QString& revision,
                   const QString& error) {
                editor::CodeEditorController* c =
                    m_fileByKey.value(rootId + QChar(0x1f) + path, nullptr);
                if (c != nullptr && ok) {
                    c->markSaved(revision);
                    if (m_fileStatus != nullptr)
                        m_fileStatus->setText(QStringLiteral("Saved"));
                } else if (c != nullptr && m_fileStatus != nullptr) {
                    m_fileStatus->setText(error.isEmpty() ? QStringLiteral("Save failed") : error);
                }
            });

    // Phase 0 shared seams (identical classes to the GUI). The connection seam
    // owns liveness; mirror its state into the footer's gateway indicator, then
    // open the saved (or default local) connection so the state machine runs.
    m_appSettings = new settings::QtSettingsStore(this);
    m_connection = new connection::MockConnectionService(this);
    m_nav = new nav::NavController(this);
    m_firstRun = new firstrun::FirstRunModel(m_appSettings, m_connection, this);
    m_modelCatalog = new models::MockModelCatalog(this);
    // Single source of truth for the composer's model list/selection (shared with
    // the Models hub), exactly as the GUI wires it via Composer.qml's modelSource.
    m_composerSession->setModelSource(m_modelCatalog);
    m_accounts = new accounts::MockAccountsService(this);
    m_profiles = new profiles::MockProfileStore(this);
    m_roster = new fleet::MockSessionRoster(this);
    m_fleetTree = new fleet::MockFleetTree(this);
    m_approvals = new fleet::MockApprovalsInbox(this);
    m_dashboard = new fleet::MockDashboard(m_roster, m_fleetTree, m_approvals, this);
    m_routing = new automation::MockRoutingStore(this);
    m_cron = new automation::MockCronStore(this);
    m_sessionSettings = new session::MockSessionSettings(this);
    m_checkpoints = new session::MockCheckpointTimeline(this);

    // Memory-inspection seam (seeded mock) + the shared view-models. Setting the
    // service kicks off the initial async requests; the page markdown re-renders
    // when results land (see the liveRefresh wiring in wireViews).
    m_memory = new memory::MockMemoryService(this);
    m_memList = new memoryui::MemoryListModel(this);
    m_memStats = new memoryui::MemoryStatsModel(this);
    m_memTimeline = new memoryui::MemoryTimelineModel(this);
    m_memGraph = new memoryui::MemoryGraphModel(this);
    m_memList->setService(m_memory);
    m_memStats->setService(m_memory);
    m_memTimeline->setService(m_memory);
    m_memGraph->setService(m_memory);

    // Wire the app-level navigation seam (constructed-but-unused until now): an
    // open() from anywhere (slash, palette, a future cog menu) raises the matching
    // manager page tab, exactly like the GUI mounts the page overlay.
    connect(m_nav, &nav::NavController::openRequested, this,
            [this](const QString& page, const QString&) { openManagerPage(page); });
    // Per-agent (ProfileRef-keyed) Memory / Profile tabs.
    connect(m_nav, &nav::NavController::openAgentRequested, this,
            [this](const QString& kind, const QString& profileRef, const QString& title) {
                if (m_tabModel == nullptr || profileRef.isEmpty())
                    return;
                const int tabKind = kind == QStringLiteral("profile") ? TabModel::Profile
                                                                       : TabModel::Memory;
                const QString base = tabKind == TabModel::Profile ? QStringLiteral("Profile")
                                                                  : QStringLiteral("Memory");
                const QString label = title.isEmpty()
                    ? base
                    : base + QStringLiteral(" \u00b7 ") + title;
                m_tabModel->openAgentTab(tabKind, profileRef, label);
            });

    connect(m_connection, &connection::IConnectionService::stateChanged, this,
            [this] { m_status->setGatewayState(m_connection->state()); });
    m_firstRun->begin();
    // Returning users auto-open the saved connection; on first launch the gate's
    // connection picker drives connectTo instead.
    if (m_appSettings->setupComplete()) {
        m_connection->connectTo(m_appSettings->lastConnectionMode(),
                                m_appSettings->lastConnectionTarget().isEmpty()
                                    ? QStringLiteral("/run/daemon.sock")
                                    : m_appSettings->lastConnectionTarget());
    }
}

void RootWidget::terminalChanged()
{
    if (m_built || terminal() == nullptr) {
        return;
    }
    m_built = true;
    buildUi();
    wireViews();

    // First-run gate (parity with the GUI): on first launch, raise the lighter
    // "Setup Required" modal over the shell until setup completes.
    if (m_firstRun != nullptr && m_firstRun->active()) {
        auto* gate = new FirstRunDialog(m_firstRun, m_connection, m_appSettings,
                                        m_appSettings->lastConnectionTarget().isEmpty()
                                            ? QStringLiteral("/run/daemon/daemon.sock")
                                            : m_appSettings->lastConnectionTarget(),
                                        this);
        gate->setFocus();
    }
}

void RootWidget::resizeEvent(Tui::ZResizeEvent* event)
{
    Tui::ZRoot::resizeEvent(event);
    if (m_window != nullptr) {
        m_window->setGeometry(QRect(QPoint(0, 0), geometry().size()));
    }
}

void RootWidget::keyEvent(Tui::ZKeyEvent* event)
{
    // Tab management keys are handled here at the ZRoot, i.e. only AFTER the
    // focused pane leaves them unhandled. This keeps them contextual: the composer
    // consumes Ctrl+W (word-rubout) and Ctrl+T (transpose) for its readline editing
    // while it has focus, so tab open/close fire only from the panes/transcript.
    // Ctrl+Tab switching has no composer conflict and works everywhere.
    const Qt::KeyboardModifiers mods = event->modifiers();
    if (mods == Qt::ControlModifier
        && event->text().compare(QStringLiteral("t"), Qt::CaseInsensitive) == 0) {
        newTranscriptTab();
        event->accept();
        return;
    }
    if (mods == Qt::ControlModifier
        && event->text().compare(QStringLiteral("w"), Qt::CaseInsensitive) == 0) {
        closeCurrentTab();
        event->accept();
        return;
    }
    if (mods == Qt::ControlModifier
        && event->text().compare(QStringLiteral("f"), Qt::CaseInsensitive) == 0) {
        openTranscriptSearch();
        event->accept();
        return;
    }
    // Sidebar agent shortcuts (parity with the GUI right-click menu): with the
    // Fleet tree focused, 'p' opens the selected agent's Profile and 'm' its
    // Memory. Only fires on profile-backed (agent) rows.
    if (mods == Qt::NoModifier && m_sidebarView != nullptr && m_sidebarView->focus()
        && m_sidebar != nullptr && m_nav != nullptr
        && (event->text() == QStringLiteral("p") || event->text() == QStringLiteral("m"))) {
        const int row = m_sidebar->currentRow();
        if (row >= 0) {
            const QModelIndex idx = m_sidebar->index(row);
            const QString profile = m_sidebar->data(idx, SidebarModel::ProfileRole).toString();
            const QString label = m_sidebar->data(idx, SidebarModel::LabelRole).toString();
            if (!profile.isEmpty()) {
                m_nav->openAgent(event->text() == QStringLiteral("p") ? QStringLiteral("profile")
                                                                      : QStringLiteral("memory"),
                                 profile, label);
                event->accept();
                return;
            }
        }
    }
    if ((mods & Qt::ControlModifier) && m_tabModel != nullptr
        && (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab)) {
        const bool backward = event->key() == Qt::Key_Backtab || (mods & Qt::ShiftModifier);
        m_tabModel->cycle(backward ? -1 : 1);
        event->accept();
        return;
    }
    // Alt+1..9 jumps directly to tab N (1-based). Unlike Ctrl+Tab this is reliably
    // delivered by terminals (as ESC + digit), so it is the primary keyboard nav.
    if ((mods & Qt::AltModifier) && m_tabModel != nullptr && event->text().size() == 1) {
        const QChar ch = event->text().at(0);
        if (ch >= QLatin1Char('1') && ch <= QLatin1Char('9')) {
            const int target = ch.digitValue() - 1;
            if (target < m_tabModel->count()) {
                m_tabModel->activate(target);
            }
            event->accept();
            return;
        }
    }

    // Interactive manager-hub pages: while one is the active tab, no-modifier keys
    // that bubble up to the root (the transcript shows the page doc but only
    // consumes its scroll keys) drive the page's seam via j/k + action keys.
    if (handlePageActionKey(event)) {
        return;
    }

    // Ctrl+E, when it bubbles up unconsumed (i.e. the composer/list did not claim
    // it for end-of-line / export), toggles the file Explorer. Tui delivers
    // Ctrl+letter as a char event, so match the text as well as the key.
    if ((event->modifiers() & Qt::ControlModifier)
        && (event->key() == Qt::Key_E
            || event->text().compare(QStringLiteral("e"), Qt::CaseInsensitive) == 0)) {
        toggleExplorer();
        event->accept();
        return;
    }

    // Final fallback for the context-sensitive Esc chain: when a pane leaves Esc
    // unhandled it bubbles up the parent chain to here (the ZRoot), where it
    // opens the quit confirmation. The composer consumes Esc itself; the quit
    // dialog consumes it while open, so this only fires from the panes.
    if (event->key() == Qt::Key_Escape && event->modifiers() == Qt::NoModifier) {
        promptQuit();
        event->accept();
        return;
    }
    Tui::ZRoot::keyEvent(event);
}

void RootWidget::handleMouse(QPoint termPos, MouseTerminal::MouseAction action, int button,
                             Qt::KeyboardModifiers modifiers)
{
    using MA = MouseTerminal::MouseAction;
    // Click + wheel only: a primary-button press focuses + selects/opens, the wheel
    // scrolls the pane under the cursor. Release/move and middle/right are ignored.
    const bool isPress = action == MA::Press && button == 0;
    const bool isWheel = action == MA::WheelUp || action == MA::WheelDown;
    if (!isPress && !isWheel) {
        return;
    }

    // Hit-test: is the terminal point inside `w`? If so, report the widget-local
    // coordinate. mapFromTerminal walks the full ancestor chain, so this works for
    // the deeply nested panes regardless of column/layout offsets.
    QPoint local;
    const auto hit = [&](Tui::ZWidget* w) -> bool {
        if (w == nullptr) {
            return false;
        }
        local = w->mapFromTerminal(termPos);
        const QSize sz = w->geometry().size();
        return local.x() >= 0 && local.y() >= 0 && local.x() < sz.width()
            && local.y() < sz.height();
    };

    if (isWheel) {
        const int delta = (action == MA::WheelUp) ? -3 : 3;
        if (m_editorView != nullptr && m_editorView->isVisible() && hit(m_editorView)) {
            m_editorView->scrollByLines(delta);
        } else if (m_fileTreeView != nullptr && m_fileTreeView->isVisible()
                   && hit(m_fileTreeView)) {
            m_fileTreeView->scrollByLines(delta);
        } else if (hit(m_transcript)) {
            m_transcript->scrollByLines(delta);
        } else if (hit(m_listView)) {
            m_listView->scrollByLines(delta);
        } else if (hit(m_sidebarView)) {
            m_sidebarView->scrollByLines(delta);
        }
        return;
    }

    // The modal quit dialog is topmost: a press activates its button (or is
    // swallowed) so clicks never leak to the panes behind it.
    if (m_quitDialog != nullptr) {
        const QList<Tui::ZButton*> buttons = m_quitDialog->findChildren<Tui::ZButton*>();
        for (Tui::ZButton* b : buttons) {
            if (hit(b)) {
                b->click();
                break;
            }
        }
        return;
    }

    // The completion popup floats above the composer: a click on an item row
    // selects it and accepts (group-header lines return -1 and are ignored).
    if (m_completionPopup != nullptr && m_completionPopup->isLocallyVisible()
        && hit(m_completionPopup)) {
        const int row = m_completionPopup->modelRowAt(local.y());
        if (row >= 0 && m_composerSession != nullptr) {
            m_composerSession->setActiveIndex(row);
            m_composerSession->acceptActive();
        }
        return;
    }

    // The tab strip is topmost in the session column: a click activates /
    // closes a tab or hits the "+" affordance (handled inside TabStripView).
    if (hit(m_tabStrip)) {
        m_tabStrip->setFocus(); // clicking the strip focuses it, like any pane
        m_tabStrip->clickAt(local);
        return;
    }

    // The file Explorer column (right side, shown via Ctrl+E): a click focuses it
    // and selects/opens the row under the cursor.
    if (m_fileTreeView != nullptr && m_fileTreeView->isVisible() && hit(m_fileTreeView)) {
        m_fileTreeView->setFocus();
        m_fileTreeView->clickAt(local);
        return;
    }
    // The code editor (shown in the session column for a File tab): a click
    // focuses it (caret placement is keyboard-driven for now).
    if (m_editorView != nullptr && m_editorView->isVisible() && hit(m_editorView)) {
        m_editorView->setFocus();
        m_editorView->clickAt(local, modifiers);
        return;
    }

    // Primary-button press: focus the clicked pane, then run its select/open.
    if (hit(m_sidebarView)) {
        m_sidebarView->setFocus();
        m_sidebarView->clickAt(local);
    } else if (hit(m_listView)) {
        m_listView->setFocus();
        m_listView->activateAtLocalY(local.y());
    } else if (hit(m_search)) {
        // The search box is not a focus stop; clicking it focuses the list (typed
        // characters are type-ahead routed through the list to the search box).
        m_listView->setFocus();
    } else if (hit(m_transcript)) {
        m_transcript->setFocus();
        m_transcript->clickAt(local); // activate an approval/clarify control, if any
    } else if (hit(m_queue) && m_queue->geometry().height() > 0) {
        m_queue->setFocus();
        m_queue->clickAt(local);
    } else if (hit(m_attachments) && m_attachments->geometry().height() > 0) {
        m_attachments->setFocus();
        m_attachments->clickAt(local);
    } else if (hit(m_composer)) {
        m_composer->setFocus();
    }
}

void RootWidget::buildUi()
{
    // Exit affordances. Ctrl+Q opens a confirmation modal; Ctrl+C is the
    // terminal-convention hard exit (no prompt). Esc is NOT a global shortcut:
    // it is handled contextually by the focused widget and only bubbles up to
    // RootWidget::keyEvent (-> promptQuit) when a pane leaves it unhandled.
    auto* quitShortcut = new Tui::ZShortcut(Tui::ZKeySequence::forShortcut(QStringLiteral("q")),
                                            this, Qt::ApplicationShortcut);
    connect(quitShortcut, &Tui::ZShortcut::activated, this, &RootWidget::promptQuit);

    auto* forceQuitShortcut = new Tui::ZShortcut(Tui::ZKeySequence::forShortcut(QStringLiteral("c")),
                                                 this, Qt::ApplicationShortcut);
    connect(forceQuitShortcut, &Tui::ZShortcut::activated, this, [] { QCoreApplication::quit(); });

    // F8 cycles the theme live (Light -> Dark -> Sepia -> Midnight), the TUI analog
    // of the GUI's theme picker. The choice persists to the same QSettings key the
    // GUI uses, so the two front ends stay in sync.
    auto* themeShortcut = new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F8), this,
                                             Qt::ApplicationShortcut);
    connect(themeShortcut, &Tui::ZShortcut::activated, this, &RootWidget::cycleTheme);

    // Ctrl+P opens the command palette (the TUI analog of the GUI's Mod+K),
    // filterable over the shared CommandRegistry.
    auto* paletteShortcut = new Tui::ZShortcut(
        Tui::ZKeySequence::forShortcut(QStringLiteral("p")), this, Qt::ApplicationShortcut);
    connect(paletteShortcut, &Tui::ZShortcut::activated, this, &RootWidget::openCommandPalette);

    // F9 opens the Settings page (the TUI settings keybinding), routed through the
    // shared NavController so it goes through the same open() path as the GUI.
    auto* settingsShortcut = new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F9), this,
                                                Qt::ApplicationShortcut);
    connect(settingsShortcut, &Tui::ZShortcut::activated, this,
            [this] { m_nav->open(QStringLiteral("settings")); });

    // F2 / F3 raise the composer overlays (session settings / checkpoints) for the
    // active transcript tab, the TUI analog of the GUI composer popovers.
    auto* sessionShortcut = new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F2), this,
                                               Qt::ApplicationShortcut);
    connect(sessionShortcut, &Tui::ZShortcut::activated, this,
            &RootWidget::openSessionSettingsOverlay);
    auto* checkpointShortcut = new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3), this,
                                                  Qt::ApplicationShortcut);
    connect(checkpointShortcut, &Tui::ZShortcut::activated, this,
            &RootWidget::openCheckpointsOverlay);

    // Ctrl+E toggles the file Explorer (parity with the GUI). It is handled on the
    // key-bubble path in keyEvent() rather than as an ApplicationShortcut, so a
    // focused composer (readline C-e = end-of-line) or session list (C-e =
    // export) consume it first; it only toggles when those panes don't. This
    // resolves the Ctrl+E collision without rebinding either widget's binding.

    m_window = new Tui::ZWindow(QStringLiteral("Daemon"), this);
    m_window->setOptions({});
    m_window->setFocusMode(Tui::FocusContainerMode::Cycle); // Tab cycles the panes
    m_window->setGeometry(QRect(QPoint(0, 0), geometry().size()));

    // The window stacks the three-column row above a one-line status footer.
    auto* outer = new Tui::ZVBoxLayout();
    m_window->setLayout(outer);

    auto* columns = new Tui::ZHBoxLayout();
    outer->add(columns);

    // --- Column 1: sidebar (fixed width) ---
    // min == max width pins the column; Preferred (not Fixed, which would shrink
    // to the sizeHint/content width) lets the clamp take effect.
    // Preferred sizes each list column to its content (clamped to a sensible
    // minimum); the session pane is the sole Expanding child, so it absorbs
    // the remaining width.
    m_sidebarView = new TreeListView(m_window);
    m_sidebarView->setMinimumSize(26, 3);
    m_sidebarView->setSizePolicyH(Tui::SizePolicy::Preferred);
    m_sidebarView->setSizePolicyV(Tui::SizePolicy::Expanding);
    columns->addWidget(m_sidebarView);

    // --- Column 2: search field + session list (custom-painted cards) ---
    auto* listCol = new Tui::ZWidget(m_window);
    listCol->setMinimumSize(34, 3);
    listCol->setSizePolicyH(Tui::SizePolicy::Preferred);
    listCol->setSizePolicyV(Tui::SizePolicy::Expanding);
    auto* listColLayout = new Tui::ZVBoxLayout();
    listCol->setLayout(listColLayout);

    m_search = new SearchInputBox(listCol);
    m_search->setText(QString());
    m_search->setMaximumSize(Tui::tuiMaxSize, 1);
    // Not a focus stop: the session list owns focus and forwards typed
    // characters here (type-ahead), so the box is a passive query display and is
    // skipped by the Tab cycle.
    m_search->setFocusPolicy(Tui::NoFocus);
    listColLayout->addWidget(m_search);

    m_listView = new SessionListView(listCol);
    m_listView->setMinimumSize(34, 3);
    m_listView->setSizePolicyV(Tui::SizePolicy::Expanding);
    listColLayout->addWidget(m_listView);

    columns->addWidget(listCol);

    // --- Column 3: session pane (transcript + composer), expanding ---
    auto* right = new Tui::ZWidget(m_window);
    right->setSizePolicyH(Tui::SizePolicy::Expanding);
    auto* rightCol = new Tui::ZVBoxLayout();
    right->setLayout(rightCol);

    // Pane-level tab strip (replaces the old single header label): a one-line row
    // of tab chips bound to the shared TabModel, with a trailing "+" affordance.
    m_tabStrip = new TabStripView(right);
    m_tabStrip->setModel(m_tabModel);
    rightCol->addWidget(m_tabStrip);
    connect(m_tabStrip, &TabStripView::newTabRequested, this, &RootWidget::newTranscriptTab);

    // In-transcript find bar (Ctrl+F / /find): a one-line query field + match
    // counter, hidden until opened. Placed at the top of the transcript column so
    // it reads like a find bar; toggling its visibility collapses the layout row.
    m_searchRow = new Tui::ZWidget(right);
    m_searchRow->setMaximumSize(Tui::tuiMaxSize, 1);
    auto* searchRowLayout = new Tui::ZHBoxLayout();
    searchRowLayout->setSpacing(1);
    m_searchRow->setLayout(searchRowLayout);
    m_transcriptSearch = new TranscriptSearchBox(m_searchRow);
    m_transcriptSearch->setSizePolicyH(Tui::SizePolicy::Expanding);
    searchRowLayout->addWidget(m_transcriptSearch);
    m_searchCounter = new Tui::ZLabel(m_searchRow);
    m_searchCounter->setSizePolicyH(Tui::SizePolicy::Fixed);
    searchRowLayout->addWidget(m_searchCounter);
    m_searchRow->setVisible(false);
    rightCol->addWidget(m_searchRow);
    connect(m_transcriptSearch, &Tui::ZInputBox::textChanged, this, [this](const QString& q) {
        if (m_active != nullptr) {
            m_active->search.setQuery(q);
        }
        updateSearchCounter();
    });
    connect(m_transcriptSearch, &TranscriptSearchBox::nextRequested, this, [this] {
        if (m_active != nullptr) {
            m_active->search.next();
        }
    });
    connect(m_transcriptSearch, &TranscriptSearchBox::previousRequested, this, [this] {
        if (m_active != nullptr) {
            m_active->search.previous();
        }
    });
    connect(m_transcriptSearch, &TranscriptSearchBox::closeRequested, this,
            &RootWidget::closeTranscriptSearch);

    // Custom-painted transcript: renders the shared be::DocumentStore (the GUI's
    // parse/ingest engine) as colored tool/reasoning cards, ANSI output, diffs,
    // and YOU/DAEMON headers instead of a raw-markdown text dump. The active tab's
    // session swaps its document in; before any tab opens it shows the (empty)
    // page document.
    m_transcript = new TranscriptView(right);
    m_transcript->setDocument(&m_pageDoc);
    m_transcript->setSizePolicyV(Tui::SizePolicy::Expanding);
    rightCol->addWidget(m_transcript);

    // Code editor view (shown in place of the transcript + composer when a File
    // tab is active). Bound to the active File tab's CodeEditorController.
    m_editorView = new CodeEditorView(right);
    m_editorView->setSizePolicyV(Tui::SizePolicy::Expanding);
    m_editorView->setVisible(false);
    rightCol->addWidget(m_editorView);
    connect(m_editorView, &CodeEditorView::saveRequested, this, [this] {
        if (m_fs != nullptr && m_editorView->controller() != nullptr && !m_activeFilePath.isEmpty())
            m_fs->write(m_activeFileRoot, m_activeFilePath, m_editorView->controller()->textBytes(),
                        m_editorView->controller()->revision(), false);
    });

    m_fileStatus = new Tui::ZLabel(right);
    m_fileStatus->setMaximumSize(Tui::tuiMaxSize, 1);
    m_fileStatus->setVisible(false);
    rightCol->addWidget(m_fileStatus);

    // One-line streaming/affordance chrome (Thinking.../error + send/stop/steer).
    m_composerChrome = new ComposerChrome(right);
    m_composerChrome->setMaximumSize(Tui::tuiMaxSize, 1);
    rightCol->addWidget(m_composerChrome);

    // Queued-prompt strip (auto-sized; 0 height when the queue is empty).
    m_queue = new QueueStripView(right);
    rightCol->addWidget(m_queue);

    // Compact status-stack subagent strip (above the composer); blank at rest.
    m_subagents = new Tui::ZLabel(right);
    m_subagents->setMaximumSize(Tui::tuiMaxSize, 1);
    rightCol->addWidget(m_subagents);

    // Compact status-stack todo strip (above the composer); blank at rest.
    m_todos = new Tui::ZLabel(right);
    m_todos->setMaximumSize(Tui::tuiMaxSize, 1);
    rightCol->addWidget(m_todos);

    // Attachment chips (auto-sized; 0 height when there are none).
    m_attachments = new AttachmentBarView(right);
    rightCol->addWidget(m_attachments);

    // Multiline composer (ZTextEdit). Height follows the document line count via
    // sizeHint() (1..kMaxRows); the transcript above it is the Expanding pane.
    m_composer = new SubmitInputBox(terminal()->textMetrics(), right);
    m_composer->setMinimumSize(8, 1);
    rightCol->addWidget(m_composer);

    // Completion overlay: a borderless custom-painted popup floated above the
    // composer. It is not in the layout - updateCompletion() positions it manually
    // over the transcript just above the input and raises it; the input box keeps
    // focus and routes navigation keys to the shared controller.
    m_completionPopup = new CompletionView(right);

    columns->addWidget(right);

    // --- Column 4: file Explorer (right side, like the GUI), hidden until Ctrl+E ---
    m_fileTreeView = new FileTreeView(m_window);
    m_fileTreeView->setMinimumSize(28, 3);
    m_fileTreeView->setSizePolicyH(Tui::SizePolicy::Preferred);
    m_fileTreeView->setSizePolicyV(Tui::SizePolicy::Expanding);
    m_fileTreeView->setModel(m_fileTree);
    // Restore the persisted open/closed state (shared "ui/showFileExplorer" key).
    m_fileTreeView->setVisible(
        QSettings().value(QStringLiteral("ui/showFileExplorer"), false).toBool());
    columns->addWidget(m_fileTreeView);
    connect(m_fileTreeView, &FileTreeView::fileChosen, this,
            [this](const QString& rootId, const QString& path, bool pinned) {
                const int slash = static_cast<int>(path.lastIndexOf(QLatin1Char('/')));
                const QString title = slash >= 0 ? path.mid(slash + 1) : path;
                if (pinned)
                    m_tabModel->openFilePinned(rootId, path, title);
                else
                    m_tabModel->previewFile(rootId, path, title);
            });

    // --- Footer: StatusBarModel-driven colored status strip ---
    m_footer = new StatusBarView(m_window);
    outer->addWidget(m_footer);

    m_sidebarView->setFocus();
}

void RootWidget::wireViews()
{
    // Bind the reused models through the display-role adapters.
    m_sidebarAdapter = new DisplayRoleAdapter(DisplayRoleAdapter::Kind::Sidebar, this);
    m_sidebarAdapter->setSourceModel(m_sidebar);
    m_sidebarView->setModel(m_sidebarAdapter);

    // The session list is a custom-painted view bound straight to the shared
    // model (no display-role adapter): it reads title/snippet/timestamp/agent/tags
    // directly and renders multi-line cards.
    m_listView->setModel(m_list);

    // Keep an open interactive hub page live: when a seam's row model churns (an
    // activate flips a flag, an approval clears, an OAuth re-auth lands, a download
    // finishes), re-render the active page's markdown so its list + selection track
    // the change without the user re-opening the tab.
    const auto liveRefresh = [this](QObject* modelObj, int kind) {
        auto* m = qobject_cast<QAbstractItemModel*>(modelObj);
        if (m == nullptr) {
            return;
        }
        connect(m, &QAbstractItemModel::dataChanged, this,
                [this, kind] { refreshPageIfActive(kind); });
        connect(m, &QAbstractItemModel::rowsInserted, this,
                [this, kind] { refreshPageIfActive(kind); });
        connect(m, &QAbstractItemModel::rowsRemoved, this,
                [this, kind] { refreshPageIfActive(kind); });
        connect(m, &QAbstractItemModel::modelReset, this,
                [this, kind] { refreshPageIfActive(kind); });
    };
    liveRefresh(m_modelCatalog->installed(), TabModel::Models);
    liveRefresh(m_accounts->accounts(), TabModel::Accounts);
    liveRefresh(m_profiles->profiles(), TabModel::Profiles);
    liveRefresh(m_roster->sessions(), TabModel::Sessions);
    liveRefresh(m_fleetTree->nodes(), TabModel::Fleet);
    liveRefresh(m_approvals->pending(), TabModel::Approvals);
    liveRefresh(m_routing->rules(), TabModel::Routing);
    liveRefresh(m_cron->jobs(), TabModel::Cron);
    // The Dashboard is a read-only projection of the roster / approvals, so refresh
    // it when either of those churns too.
    liveRefresh(m_roster->sessions(), TabModel::Dashboard);
    liveRefresh(m_approvals->pending(), TabModel::Dashboard);
    // The Memory page is a read-only projection of the shared memory view-models;
    // re-render it when any of them deliver async results.
    liveRefresh(m_memList, TabModel::Memory);
    liveRefresh(m_memTimeline, TabModel::Memory);
    liveRefresh(m_memGraph, TabModel::Memory);
    connect(m_memStats, &memoryui::MemoryStatsModel::changed, this,
            [this] { refreshPageIfActive(TabModel::Memory); });

    // Type-ahead search. The session list is the only focus stop in the
    // column; printable keys it receives build the query in the passive search box,
    // whose textChanged drives the shared live filter. Backspace edits, Esc clears.
    connect(m_search, &Tui::ZInputBox::textChanged, m_list, &SessionsListModel::setSearch);
    connect(m_listView, &SessionListView::searchAppend, this, [this](const QString& text) {
        m_search->setText(m_search->text() + text);
    });
    connect(m_listView, &SessionListView::searchBackspace, this, [this] {
        QString q = m_search->text();
        if (!q.isEmpty()) {
            q.chop(1);
            m_search->setText(q);
        }
    });
    connect(m_listView, &SessionListView::searchClear, this,
            [this] { m_search->setText(QString()); });
    // The search box shows its typing caret only while the list is focused.
    connect(m_listView, &SessionListView::focusChanged, m_search,
            &SearchInputBox::setTypingActive);
    // After the query changes, re-anchor the selection onto the first match so the
    // highlight (and Enter) follow the filtered list instead of stranding off-list.
    // (The view already rebuilds from the model's reset/selection signals.)
    connect(m_list, &SessionsListModel::searchChanged, this, [this] {
        if (m_list->currentRow() < 0 && m_list->rowCount() > 0) {
            m_list->activate(0);
        }
    });

    // Sidebar highlight -> activate the scope (re-emits scopeSelected -> list).
    connect(m_sidebarView->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current, const QModelIndex&) {
                if (current.isValid()) {
                    m_sidebar->activate(current.row());
                }
            });

    // Left/Right collapse/expand the tree. The model owns the structural logic
    // and may move its selection (to parent/child); a full rebuild (model reset)
    // clears the view's current index, so re-sync it from the model afterwards
    // to keep the highlight on the selected row.
    const auto syncSidebarCurrent = [this] {
        const int row = m_sidebar->currentRow();
        if (row >= 0 && row < m_sidebarAdapter->rowCount()) {
            m_sidebarView->setCurrentIndex(m_sidebarAdapter->index(row, 0));
        }
    };
    connect(m_sidebarView, &TreeListView::collapseRequested, this, [this, syncSidebarCurrent] {
        m_sidebar->collapseCurrent();
        syncSidebarCurrent();
    });
    connect(m_sidebarView, &TreeListView::expandRequested, this, [this, syncSidebarCurrent] {
        m_sidebar->expandCurrent();
        syncSidebarCurrent();
    });

    // Session highlight / open -> record selection in the model (so the model
    // owns `current`, consistent with the sidebar) and surface it in a tab. A
    // transient activation (arrow nav / single click) loads into the VSCode-style
    // preview tab; a deliberate Enter opens a permanent (pinned) tab.
    const auto openRow = [this](int row, bool pinned) {
        if (row < 0) {
            return;
        }
        m_list->activate(row); // identity-based selection in the shared model
        const int id = m_list->idAt(row);
        if (id < 0) {
            return;
        }
        if (pinned) {
            openSessionPinnedTab(id);
        } else {
            previewSessionTab(id);
        }
    };
    connect(m_listView, &SessionListView::rowActivated, this, openRow);

    // Session actions on the focused list row (Ctrl+R rename, Ctrl+E export,
    // Ctrl+K pin, Delete delete). All run against the shared store.
    connect(m_listView, &SessionListView::pinToggleRequested, this, [this](int row) {
        const int id = m_list->idAt(row);
        if (id >= 0) {
            m_store->setPinned(id, !m_store->isPinned(id));
        }
    });
    connect(m_listView, &SessionListView::deleteRequested, this, [this](int row) {
        const int id = m_list->idAt(row);
        if (id < 0) {
            return;
        }
        auto* confirm = new ConfirmDialog(
            QStringLiteral("Delete session"),
            QStringLiteral("Permanently delete this session?"), this);
        connect(confirm, &ConfirmDialog::confirmed, this,
                [this, id] { m_store->deleteSession(id); });
    });
    connect(m_listView, &SessionListView::exportRequested, this, [this](int row) {
        const int id = m_list->idAt(row);
        if (id < 0) {
            return;
        }
        QString name = m_store->title(id);
        if (name.isEmpty()) {
            name = QStringLiteral("session");
        }
        const QString path = QDir(QDir::homePath()).filePath(name + QStringLiteral(".json"));
        m_exporter->exportToPath(m_store, id, path);
    });
    connect(m_listView, &SessionListView::renameRequested, this, [this](int row) {
        const int id = m_list->idAt(row);
        if (id < 0) {
            return;
        }
        auto* dialog = new TextPromptDialog(QStringLiteral("Rename session"),
                                            m_store->title(id), /*masked=*/false, this);
        connect(dialog, &TextPromptDialog::submitted, this, [this, id](const QString& text) {
            if (!text.trimmed().isEmpty()) {
                m_store->renameSession(id, text.trimmed());
            }
        });
    });
    connect(m_listView, &SessionListView::moveRequested, this, [this](int row, int delta) {
        const int id = m_list->idAt(row);
        if (id >= 0) {
            m_store->moveSession(id, delta);
        }
    });

    // Inline interactive answers: the transcript's controls emit decisions routed
    // to the ACTIVE session's mock host, which patches its document and asks us to
    // persist + reload. A live turn paused at an approval gate resumes once answered.
    connect(m_transcript, &TranscriptView::approvalDecided, this,
            [this](const QString& callId, const QString& decision, bool permanent) {
                if (m_active == nullptr) {
                    return;
                }
                m_active->host->onApprovalDecided(callId, decision, permanent);
                if (m_active->turn != nullptr) {
                    m_active->turn->resume();
                }
            });
    connect(m_transcript, &TranscriptView::clarifySubmitted, this,
            [this](const QString& callId, const QString& requestId, const QVariantMap& answers) {
                if (m_active != nullptr) {
                    m_active->host->onClarifySubmitted(callId, requestId, answers);
                }
            });
    // Rewind picker: restore re-runs the selected user message; edit seeds the
    // composer with its text. Both funnel through the single rewindActiveTab seam.
    connect(m_transcript, &TranscriptView::rewindRestoreRequested, this,
            [this](const QString& messageId) { rewindActiveTab(messageId, false); });
    connect(m_transcript, &TranscriptView::rewindEditRequested, this,
            [this](const QString& messageId, const QString&) { rewindActiveTab(messageId, true); });

    // Composer <-> session controller. The controller is the source of truth for
    // the draft; the input box is its view. Typed edits flow in via textChanged;
    // programmatic changes (history recall, session swap, clear-on-send) flow
    // back out via draftReset. Enter routes to submit(); the controller emits the
    // submitted turn, which the session controller appends.
    // The composer pushes live edits into the session itself (via its document /
    // cursor signals), so there is no textChanged wire here. Programmatic draft
    // changes flow back out via draftReset: replace the text and drop the caret at
    // the end (a completion accept then overrides the caret via cursorRequested).
    connect(m_composerSession, &ComposerSessionController::draftReset, m_composer,
            [this](const QString& text) {
                if (m_composer->text() != text) {
                    m_composer->setText(text);
                }
                m_composer->moveCursorToEnd();
            });
    connect(m_composer, &SubmitInputBox::submitted, m_composerSession,
            [this](const QString&) { m_composerSession->submit(); });
    // Submit is dispatched to the ACTIVE tab's orchestrator, which owns its submit
    // pipeline (persist user text -> start the scripted turn -> stream into its own
    // document). Background tabs keep their own orchestrators running independently.
    connect(m_composerSession, &ComposerSessionController::submitted, this,
            [this](const QString& text, const QString& refs) {
                if (m_active != nullptr) {
                    // Submitting commits to this session: a preview tab becomes
                    // permanent (VSCode-style) so the next preview opens elsewhere.
                    m_tabModel->pinCurrent();
                    m_active->orchestrator->submit(text, refs);
                }
            });
    // Slash commands: "/new" opens a new transcript tab; other commands route to
    // the active orchestrator (front-end-only ones surface via commandRequested,
    // no-ops in the TUI for now).
    connect(m_composerSession, &ComposerSessionController::commandInvoked, this,
            [this](const QString& command) {
                if (openManagerPage(command)) {
                    return; // a "/settings" / "/models" / ... page route
                }
                if (command == QStringLiteral("new")) {
                    newTranscriptTab();
                    return;
                }
                // Front-end overlays / client state the orchestrator cannot reach.
                if (command == QStringLiteral("model")) {
                    openModelPicker();
                    return;
                }
                if (command == QStringLiteral("help")) {
                    openCommandPalette();
                    return;
                }
                if (command == QStringLiteral("theme")) {
                    cycleTheme();
                    return;
                }
                if (command == QStringLiteral("reasoning")) {
                    m_composerSession->cycleReasoningEffort();
                    return;
                }
                if (command == QStringLiteral("fast")) {
                    m_composerSession->toggleFastMode();
                    return;
                }
                if (command == QStringLiteral("verbose")) {
                    m_composerSession->toggleVerbose();
                    return;
                }
                if (command == QStringLiteral("usage")) {
                    return; // usage is live in the footer status bar
                }
                if (command == QStringLiteral("find")) {
                    openTranscriptSearch();
                    return;
                }
                // Session actions on the ACTIVE session, so "/title" and
                // "/save" (and the palette) match the GUI; the list shortcuts
                // (Ctrl+R / Ctrl+E) act on the focused row instead.
                if (command == QStringLiteral("title")) {
                    if (m_active == nullptr || !m_active->controller->hasSession()) {
                        return;
                    }
                    const int id = m_active->sessionId;
                    auto* dialog = new TextPromptDialog(QStringLiteral("Rename session"),
                                                        m_store->title(id), /*masked=*/false, this);
                    connect(dialog, &TextPromptDialog::submitted, this,
                            [this, id](const QString& text) {
                                if (!text.trimmed().isEmpty()) {
                                    m_store->renameSession(id, text.trimmed());
                                }
                            });
                    return;
                }
                if (command == QStringLiteral("save")) {
                    if (m_active == nullptr || !m_active->controller->hasSession()) {
                        return;
                    }
                    const int id = m_active->sessionId;
                    QString name = m_store->title(id);
                    if (name.isEmpty()) {
                        name = QStringLiteral("session");
                    }
                    const QString path =
                        QDir(QDir::homePath()).filePath(name + QStringLiteral(".json"));
                    m_exporter->exportToPath(m_store, id, path);
                    return;
                }
                if (command == QStringLiteral("compress")) {
                    // Simulated compaction: free most of the live context window.
                    if (m_status != nullptr) {
                        m_status->setContextUsed(
                            qMax(1500, static_cast<int>(m_status->contextUsed() * 0.2)));
                    }
                    return;
                }
                if (command == QStringLiteral("clear")) {
                    if (m_active == nullptr) {
                        return;
                    }
                    auto* confirm = new ConfirmDialog(
                        QStringLiteral("Clear session"),
                        QStringLiteral("Remove all messages from this session?"), this);
                    connect(confirm, &ConfirmDialog::confirmed, this, [this] {
                        if (m_active != nullptr && m_active->controller->hasSession()) {
                            m_active->doc.loadMarkdown(QString());
                            m_active->controller->updateContent(QString());
                            m_active->search.refresh();
                            if (m_transcript != nullptr) {
                                m_transcript->reload();
                            }
                        }
                    });
                    return;
                }
                // Rewind commands act on the active tab's last user message. The
                // TUI owns the document, so it resolves the anchor here rather than
                // round-tripping through the orchestrator's rewind*Requested signals
                // (which the GUI uses).
                if (command == QStringLiteral("retry") || command == QStringLiteral("edit")
                    || command == QStringLiteral("undo")) {
                    if (m_active == nullptr) {
                        return;
                    }
                    QString lastUserId;
                    for (qsizetype row = 0; row < m_active->doc.blockCount(); ++row) {
                        const be::BlockRecord* b = m_active->doc.blockAt(row);
                        if (b != nullptr && b->role == be::MessageRole::User
                            && !b->messageId.isEmpty()) {
                            lastUserId = b->messageId;
                        }
                    }
                    if (lastUserId.isEmpty()) {
                        return;
                    }
                    if (command == QStringLiteral("undo")) {
                        // Drop the last exchange: truncate inclusive, no re-run.
                        if (m_active->turn != nullptr && m_active->turn->active()) {
                            m_active->orchestrator->cancel();
                        }
                        m_active->doc.rewindToMessage(lastUserId);
                        if (m_active->controller->hasSession()) {
                            m_active->controller->updateContent(m_active->doc.toMarkdown());
                        }
                        m_transcript->reload();
                    } else {
                        rewindActiveTab(lastUserId, command == QStringLiteral("edit"));
                    }
                    return;
                }
                if (m_active != nullptr) {
                    m_active->orchestrator->invokeCommand(command);
                }
            });
    connect(m_composer, &SubmitInputBox::historyPrevious, m_composerSession,
            [this] { m_composerSession->browseUp(); });
    connect(m_composer, &SubmitInputBox::historyNext, m_composerSession,
            [this] { m_composerSession->browseDown(); });

    // Completion: the input box routes trigger/navigation to the shared controller;
    // a caret request after an accept repositions the input cursor; the overlay
    // mirrors the controller's completion state.
    m_composer->setSession(m_composerSession);
    connect(m_composerSession, &ComposerSessionController::cursorRequested, m_composer,
            [this](int pos) { m_composer->setLinearCursor(pos); });
    connect(m_composerSession, &ComposerSessionController::completionActiveChanged, this,
            &RootWidget::updateCompletion);
    connect(m_composerSession, &ComposerSessionController::completionActiveIndexChanged, this,
            &RootWidget::updateCompletion);
    connect(m_composerSession->completionItems(), &QAbstractItemModel::modelReset, this,
            &RootWidget::updateCompletion);

    // Mid-turn nudge + stop dispatched to the active tab's orchestrator.
    connect(m_composerSession, &ComposerSessionController::steer, this,
            [this](const QString& text) {
                if (m_active != nullptr) {
                    m_active->orchestrator->steer(text);
                }
            });
    connect(m_composerSession, &ComposerSessionController::cancelRequested, this, [this] {
        if (m_active != nullptr) {
            m_active->orchestrator->cancel();
        }
    });

    // Queued-prompt strip: bound to the shared session; editing a queued entry
    // loads it into the draft, so move focus to the composer to edit + save.
    m_queue->setController(m_composerSession);
    connect(m_queue, &QueueStripView::editRequested, this, [this](int) {
        if (m_composer != nullptr) {
            m_composer->setFocus();
        }
    });

    // Attachment chips bind to the shared session; Ctrl+O on the composer adds a
    // canned attachment (mock-parity with the GUI's "+" menu / drag-drop).
    m_attachments->setController(m_composerSession);
    connect(m_composer, &SubmitInputBox::attachRequested, m_composerSession, [this] {
        const int n = m_composerSession->attachments()->count();
        m_composerSession->addAttachment(
            QStringLiteral("attachment-%1.txt").arg(n + 1), QStringLiteral("file"));
    });

    // Esc on an empty composer hands focus back to the session list, where a
    // further Esc bubbles up to the quit prompt (context-sensitive Esc, level 2).
    connect(m_composer, &SubmitInputBox::leaveRequested, this, [this] {
        if (m_listView != nullptr) {
            m_listView->setFocus();
        }
    });

    // The colored status footer + the streaming composer chrome bind to the shared
    // models; the chrome's turn is (re)bound to the active tab in activateTab().
    m_footer->setModel(m_status);
    m_composerChrome->setSession(m_composerSession);

    // Initial selection: first sidebar row populates the list, then open its first
    // session in a tab so the transcript is non-empty on launch.
    if (m_sidebarAdapter->rowCount() > 0) {
        m_sidebarView->setCurrentIndex(m_sidebarAdapter->index(0, 0));
    }
    if (m_list->rowCount() > 0) {
        openRow(0, true); // launch with a permanent (pinned) first tab
    }
    updateCompletion();
}

// --- Tabs --------------------------------------------------------------------

void RootWidget::previewSessionTab(int sessionId)
{
    if (m_tabModel == nullptr) {
        return;
    }
    QString title = m_store->title(sessionId);
    if (title.isEmpty()) {
        title = titleForContent(m_store->content(sessionId));
    }
    m_tabModel->previewTranscript(sessionId, title);
}

void RootWidget::openSessionPinnedTab(int sessionId)
{
    if (m_tabModel == nullptr) {
        return;
    }
    QString title = m_store->title(sessionId);
    if (title.isEmpty()) {
        title = titleForContent(m_store->content(sessionId));
    }
    m_tabModel->openTranscriptPinned(sessionId, title);
}

void RootWidget::newTranscriptTab()
{
    if (m_store == nullptr || m_tabModel == nullptr) {
        return;
    }
    const int id = m_store->createSession(domain::UnitId());
    m_tabModel->openTranscriptPinned(id, QStringLiteral("New session"));
    // A new tab is a natural place to start typing.
    if (m_composer != nullptr) {
        m_composer->setFocus();
    }
}

void RootWidget::rebindSession(int tabId, int sessionId)
{
    auto it = m_sessions.find(tabId);
    if (it == m_sessions.end()) {
        return; // no session yet; ensureSession will open the right session
    }
    TabSession* s = it.value();
    if (s->sessionId == sessionId) {
        return;
    }
    s->sessionId = sessionId;
    s->controller->open(sessionId);
    if (s == m_active) {
        m_composerSession->setSessionId(sessionId);
        refreshTranscript();
        updateTodos();
        updateSubagents();
    }
}

void RootWidget::closeCurrentTab()
{
    if (m_tabModel != nullptr && m_tabModel->currentIndex() >= 0) {
        m_tabModel->closeTab(m_tabModel->currentIndex());
    }
}

void RootWidget::openTranscriptSearch()
{
    // Find only applies to a live transcript tab (page tabs have no session).
    if (m_active == nullptr || m_searchRow == nullptr || m_transcriptSearch == nullptr) {
        return;
    }
    m_searchActive = true;
    m_searchRow->setVisible(true);
    m_transcriptSearch->setText(m_active->search.query());
    m_transcriptSearch->setFocus();
    updateSearchCounter();
}

void RootWidget::closeTranscriptSearch()
{
    if (!m_searchActive) {
        return;
    }
    m_searchActive = false;
    if (m_active != nullptr) {
        m_active->search.clear();
    }
    if (m_transcriptSearch != nullptr) {
        m_transcriptSearch->setText(QString());
    }
    if (m_searchRow != nullptr) {
        m_searchRow->setVisible(false);
    }
    if (m_transcript != nullptr) {
        m_transcript->setFocus();
        m_transcript->reload(); // drop the highlight
    }
}

void RootWidget::updateSearchCounter()
{
    if (m_searchCounter == nullptr) {
        return;
    }
    if (m_active == nullptr) {
        m_searchCounter->setText(QString());
        return;
    }
    const be::TranscriptSearchController& s = m_active->search;
    if (s.query().isEmpty()) {
        m_searchCounter->setText(QString());
    } else if (s.matchCount() == 0) {
        m_searchCounter->setText(QStringLiteral(" 0/0 "));
    } else {
        m_searchCounter->setText(
            QStringLiteral(" %1/%2 ").arg(s.currentMatch() + 1).arg(s.matchCount()));
    }
}

TabSession* RootWidget::ensureSession(int tabId)
{
    if (auto it = m_sessions.find(tabId); it != m_sessions.end()) {
        return it.value();
    }
    const int row = m_tabModel->indexOfTabId(tabId);
    if (row < 0 || m_tabModel->kindAt(row) != TabModel::Transcript) {
        return nullptr; // page tabs have no backend session
    }

    auto* s = new TabSession();
    s->tabId = tabId;
    s->sessionId = m_tabModel->sessionIdAt(row);
    s->controller = new SessionController();
    s->controller->setStore(m_store);
    s->orchestrator = new SessionOrchestrator();
    s->orchestrator->setSession(s->controller);
    s->turn = s->orchestrator->turn();
    s->host = new InteractiveTurnHost(&s->doc, &s->ingest);
    m_sessions.insert(tabId, s);
    wireSession(s);
    s->controller->open(s->sessionId);
    return s;
}

void RootWidget::wireSession(TabSession* s)
{
    // Stream this session's turn events into ITS OWN document (so a background tab
    // keeps growing while another is on screen); repaint only when it is active.
    connect(s->turn, &TurnController::eventsEmitted, this,
            [this, s](const QVariantList& events) { onTurnEvents(s, events); });
    connect(s->turn, &TurnController::turnStarted, this, [this, s] {
        if (s == m_active) {
            m_status->setBusy(true);
            m_status->setTurnStartedAt(
                static_cast<double>(QDateTime::currentMSecsSinceEpoch()));
            if (m_transcript != nullptr) {
                m_transcript->reload();
            }
        }
    });
    connect(s->turn, &TurnController::turnFinished, this, [this, s] {
        // Settle the open stream and persist the grown document regardless of which
        // tab is active, so a background turn's result is saved.
        s->ingest.finish();
        if (s->controller->hasSession()) {
            s->controller->updateContent(s->doc.toMarkdown());
        }
        if (s == m_active) {
            m_status->setBusy(false);
            if (m_transcript != nullptr) {
                m_transcript->reload();
            }
            // Turn-done desktop notification (opt-in via notify/turnDone, shared
            // with the GUI). The TUI cannot detect terminal focus, so this fires
            // whenever the pref is on rather than only when hidden.
            if (m_appSettings != nullptr
                && m_appSettings->value(QStringLiteral("notify/turnDone"), false).toBool()) {
                const QString convTitle = s->controller->hasSession()
                    ? m_store->title(s->sessionId)
                    : QStringLiteral("daemon");
                emitDesktopNotification(convTitle, QStringLiteral("The turn finished."));
            }
        }
    });
    connect(s->turn, &TurnController::activeChanged, this, [this, s] {
        if (s == m_active) {
            m_composerSession->setBusy(s->turn->active());
        }
    });
    // A gate (approval/clarify/host) blocked the turn on the user: ring the bell
    // and flag the terminal title so an unfocused terminal alerts (the TUI analog
    // of the GUI's OS notification). Only the active session drives the chrome.
    connect(s->turn, &TurnController::awaitingInput, this, [this, s](const QString& kind) {
        if (s != m_active) {
            return;
        }
        // Honor the "notify when a turn needs my input" preference (shared with the
        // GUI's notify/gates), defaulting on.
        if (m_appSettings != nullptr
            && !m_appSettings->value(QStringLiteral("notify/gates"), true).toBool()) {
            return;
        }
        std::fputs("\a", stdout); // BEL: most terminals raise an urgency hint
        std::fflush(stdout);
        const QString what = kind == QStringLiteral("approval") ? QStringLiteral("approval")
                                                                : QStringLiteral("credential");
        if (terminal() != nullptr) {
            terminal()->setTitle(QStringLiteral("\u25cf daemon \u2014 needs ") + what);
        }
        const QString convTitle = (m_active != nullptr && m_active->controller->hasSession())
            ? m_store->title(m_active->sessionId)
            : QStringLiteral("daemon");
        emitDesktopNotification(convTitle, QStringLiteral("The turn needs your ") + what
                                    + QStringLiteral("."));
    });
    // Clear the title alert once the turn settles.
    connect(s->turn, &TurnController::turnFinished, this, [this, s] {
        if (s == m_active && terminal() != nullptr) {
            terminal()->setTitle(QStringLiteral("daemon"));
        }
    });
    // The turn paused for a masked host input (sudo password / secret). Raise a
    // masked prompt; the answer resumes the turn (the daemon's host channel
    // replaces this mock responder later), cancel aborts it.
    connect(s->turn, &TurnController::hostRequested, this,
            [this, s](const QString& kind, const QString& prompt) {
                const QString title = prompt.isEmpty()
                    ? (kind == QStringLiteral("secret") ? QStringLiteral("Secret required")
                                                        : QStringLiteral("Password required"))
                    : prompt;
                auto* dialog = new TextPromptDialog(title, QString(), /*masked=*/true, this);
                connect(dialog, &TextPromptDialog::submitted, this,
                        [s](const QString&) { s->turn->resume(); });
                connect(dialog, &TextPromptDialog::canceled, this,
                        [s] { s->turn->cancel(); });
            });
    connect(s->controller, &SessionController::sessionChanged, this, [this, s] {
        if (s == m_active) {
            refreshTranscript();
        }
    });
    connect(s->controller, &SessionController::contentChanged, this, [this, s] {
        if (s == m_active) {
            refreshTranscript();
        }
    });
    connect(s->host, &InteractiveTurnHost::documentChanged, this, [this, s] {
        if (s->controller->hasSession()) {
            s->controller->updateContent(s->doc.toMarkdown());
        }
        if (s == m_active && m_transcript != nullptr) {
            m_transcript->reload();
        }
    });
    connect(s->orchestrator->todos(), &TodoListModel::countChanged, this, [this, s] {
        if (s == m_active) {
            updateTodos();
        }
    });
    connect(s->orchestrator->todos(), &QAbstractItemModel::modelReset, this, [this, s] {
        if (s == m_active) {
            updateTodos();
        }
    });
    // Live subagent rows mirror the todos plumbing: the model upserts the turn's
    // subagent.* events; repaint the strip only for the active session.
    connect(s->orchestrator->subagents(), &SubagentModel::countChanged, this, [this, s] {
        if (s == m_active) {
            updateSubagents();
        }
    });
    connect(s->orchestrator->subagents(), &QAbstractItemModel::dataChanged, this, [this, s] {
        if (s == m_active) {
            updateSubagents();
        }
    });
    connect(s->orchestrator->subagents(), &QAbstractItemModel::modelReset, this, [this, s] {
        if (s == m_active) {
            updateSubagents();
        }
    });

    // In-transcript find: only the active session with the bar open drives the
    // view. A match move scrolls its block into view; a (re)collect repaints the
    // highlight and refreshes the counter. Guarded so a streaming refresh() on a
    // background tab never touches the shared transcript/counter.
    connect(&s->search, &be::TranscriptSearchController::navigateTo, this,
            [this, s](int blockIndex, int /*charOffset*/) {
                if (s == m_active && m_searchActive && m_transcript != nullptr) {
                    m_transcript->scrollBlockIntoView(blockIndex);
                }
            });
    connect(&s->search, &be::TranscriptSearchController::matchesChanged, this, [this, s] {
        if (s == m_active && m_searchActive) {
            if (m_transcript != nullptr) {
                m_transcript->reload();
            }
            updateSearchCounter();
        }
    });
    connect(&s->search, &be::TranscriptSearchController::currentMatchChanged, this, [this, s] {
        if (s == m_active && m_searchActive) {
            if (m_transcript != nullptr) {
                m_transcript->reload();
            }
            updateSearchCounter();
        }
    });
}

void RootWidget::activateTab(int tabId)
{
    const int row = m_tabModel->indexOfTabId(tabId);
    if (row < 0) {
        return;
    }

    if (m_tabModel->kindAt(row) == TabModel::Transcript) {
        TabSession* s = ensureSession(tabId);
        if (s == nullptr) {
            return;
        }
        // Switching tabs closes any open find bar so it never shows stale matches
        // from the previous tab's engine.
        closeTranscriptSearch();
        showEditor(false);
        m_active = s;
        if (m_transcript != nullptr) {
            m_transcript->setDocument(&s->doc);
            m_transcript->setSearch(&s->search);
        }
        if (m_composerChrome != nullptr) {
            m_composerChrome->setTurn(s->turn);
        }
        m_composerSession->setSessionId(s->sessionId);
        m_composerSession->setBusy(s->turn->active());
        m_status->setBusy(s->turn->active());
        // Resync the elapsed timer to THIS tab's live turn (mirrors the GUI's
        // onIsActiveChanged), so a switched-to busy tab shows the right elapsed.
        if (s->turn->active()) {
            m_status->setTurnStartedAt(static_cast<double>(QDateTime::currentMSecsSinceEpoch())
                                       - static_cast<double>(s->turn->elapsedMs()));
        }
        refreshTranscript();
        updateTodos();
        updateSubagents();
    } else if (m_tabModel->kindAt(row) == TabModel::File) {
        // A file tab: bind the editor to this tab's controller (lazily created +
        // read through the fs seam) and show it in place of the transcript stack.
        closeTranscriptSearch();
        m_active = nullptr;
        editor::CodeEditorController* c = ensureFileSession(tabId);
        m_activeFileRoot = m_tabModel->fileRootAt(row);
        m_activeFilePath = m_tabModel->filePathAt(row);
        if (m_editorView != nullptr) {
            m_editorView->setController(c);
            m_editorView->setFocus();
        }
        showEditor(true);
        m_status->setBusy(false);
        updateTodos();
        updateSubagents();
    } else {
        // A non-transcript page tab (Settings): no backend session; show the static
        // page document and clear the per-tab strips.
        closeTranscriptSearch();
        showEditor(false);
        m_active = nullptr;
        const int kind = m_tabModel->kindAt(row);
        // Per-agent Memory tab: re-scope the shared memory models to this tab's
        // agent (profile == bank) before projecting. Switching between agents'
        // Memory tabs re-scopes the same shared service.
        if (kind == TabModel::Memory && m_memory != nullptr) {
            m_memory->setScope(m_tabModel->agentRefAt(row), QString(), true);
        }
        m_pageDoc.loadMarkdown(pageMarkdownForKind(kind));
        if (m_transcript != nullptr) {
            m_transcript->setSearch(nullptr);
            m_transcript->setDocument(&m_pageDoc);
            m_transcript->reload();
            // Focus the transcript so the interactive hubs' j/k + action keys (which
            // bubble up past the read-only page view) reach the root handler.
            if (activePageKind() >= 0) {
                m_transcript->setFocus();
            }
        }
        m_status->setBusy(false);
        updateTodos();
        updateSubagents();
    }
}

editor::CodeEditorController* RootWidget::ensureFileSession(int tabId)
{
    if (m_fileSessions.contains(tabId))
        return m_fileSessions.value(tabId);
    const int row = m_tabModel->indexOfTabId(tabId);
    if (row < 0)
        return nullptr;
    const QString rootId = m_tabModel->fileRootAt(row);
    const QString path = m_tabModel->filePathAt(row);

    auto* c = new editor::CodeEditorController(this);
    c->setDarkTheme(tpal::activeTheme() != theme::ThemeName::Light
                    && tpal::activeTheme() != theme::ThemeName::Sepia);
    m_fileSessions.insert(tabId, c);
    m_fileByKey.insert(rootId + QChar(0x1f) + path, c);
    connect(c, &editor::CodeEditorController::modifiedChanged, this,
            [this, tabId, c] { m_tabModel->setDirtyById(tabId, c->modified()); });
    // Kick off the async read; the fileRead handler loads bytes into this controller.
    if (m_fs != nullptr)
        m_fs->read(rootId, path);
    return c;
}

void RootWidget::showEditor(bool on)
{
    if (m_editorView != nullptr)
        m_editorView->setVisible(on);
    if (m_fileStatus != nullptr)
        m_fileStatus->setVisible(on);
    // Transcript + composer chrome are hidden while a file is open.
    const bool stack = !on;
    if (m_transcript != nullptr)
        m_transcript->setVisible(stack);
    if (m_composerChrome != nullptr)
        m_composerChrome->setVisible(stack);
    if (m_queue != nullptr)
        m_queue->setVisible(stack);
    if (m_subagents != nullptr)
        m_subagents->setVisible(stack);
    if (m_todos != nullptr)
        m_todos->setVisible(stack);
    if (m_attachments != nullptr)
        m_attachments->setVisible(stack);
    if (m_composer != nullptr)
        m_composer->setVisible(stack);
}

QString RootWidget::pageMarkdownForKind(int kind) const
{
    const int sel = m_pageSel.value(kind, 0);
    switch (kind) {
    case TabModel::Settings:
        return buildSettingsMarkdown();
    case TabModel::Models:
        return buildModelsMarkdown(sel);
    case TabModel::Accounts:
        return buildAccountsMarkdown(sel);
    case TabModel::Profiles:
        return buildProfilesMarkdown(sel);
    case TabModel::Dashboard:
        return buildDashboardMarkdown();
    case TabModel::Fleet:
        return buildFleetMarkdown(sel);
    case TabModel::Sessions:
        return buildSessionsMarkdown(sel);
    case TabModel::Approvals:
        return buildApprovalsMarkdown(sel);
    case TabModel::Routing:
        return buildRoutingMarkdown(sel);
    case TabModel::Cron:
        return buildCronMarkdown(sel);
    case TabModel::Memory:
        return buildMemoryMarkdown();
    case TabModel::Profile:
        return buildProfileMarkdown(
            m_tabModel != nullptr ? m_tabModel->agentRefAt(m_tabModel->currentIndex()) : QString());
    default:
        return pageMarkdown(kind);
    }
}

bool RootWidget::openManagerPage(const QString& id)
{
    // Maps a command/palette/slash id to its singleton page tab. Shared by the
    // command palette and the composer's slash dispatcher so both routes open
    // the same tabs from the same seam-backed markdown.
    static const QHash<QString, QPair<int, QString>> kPageRoutes = {
        { QStringLiteral("settings"), { TabModel::Settings, QStringLiteral("Settings") } },
        { QStringLiteral("dashboard"), { TabModel::Dashboard, QStringLiteral("Dashboard") } },
        { QStringLiteral("models"), { TabModel::Models, QStringLiteral("Models") } },
        { QStringLiteral("accounts"), { TabModel::Accounts, QStringLiteral("Accounts") } },
        { QStringLiteral("profiles"), { TabModel::Profiles, QStringLiteral("Profiles") } },
        { QStringLiteral("fleet"), { TabModel::Fleet, QStringLiteral("Fleet") } },
        { QStringLiteral("sessions"), { TabModel::Sessions, QStringLiteral("Sessions") } },
        { QStringLiteral("approvals"), { TabModel::Approvals, QStringLiteral("Approvals") } },
        { QStringLiteral("routing"), { TabModel::Routing, QStringLiteral("Routing") } },
        { QStringLiteral("cron"), { TabModel::Cron, QStringLiteral("Scheduled jobs") } },
        // Memory/Profile are NOT here: they are per-agent (keyed by ProfileRef) and
        // open via openAgentTab(), not as singleton manager pages.
    };
    const auto route = kPageRoutes.constFind(id);
    if (route == kPageRoutes.constEnd()) {
        return false;
    }
    if (m_tabModel != nullptr) {
        m_tabModel->openPage(route->first, route->second);
    }
    return true;
}

namespace {
// The rows of a seam's VariantListModel (empty for a null / wrong-typed model).
QList<QVariantMap> rowsOfModel(QObject* m)
{
    auto* vlm = qobject_cast<uimodels::VariantListModel*>(m);
    return vlm ? vlm->rows() : QList<QVariantMap>{};
}
} // namespace

int RootWidget::activePageKind() const
{
    if (m_active != nullptr || m_tabModel == nullptr) {
        return -1; // a transcript tab is active (or none)
    }
    const int idx = m_tabModel->currentIndex();
    if (idx < 0) {
        return -1;
    }
    switch (m_tabModel->kindAt(idx)) {
    case TabModel::Models:
    case TabModel::Accounts:
    case TabModel::Profiles:
    case TabModel::Sessions:
    case TabModel::Fleet:
    case TabModel::Approvals:
    case TabModel::Routing:
    case TabModel::Cron:
        return m_tabModel->kindAt(idx);
    default:
        return -1; // Settings / Dashboard are read-only projections
    }
}

QList<QVariantMap> RootWidget::pageActionRows(int kind) const
{
    switch (kind) {
    case TabModel::Models:
        return rowsOfModel(m_modelCatalog->installed());
    case TabModel::Accounts:
        return rowsOfModel(m_accounts->accounts());
    case TabModel::Profiles:
        return rowsOfModel(m_profiles->profiles());
    case TabModel::Sessions:
        return rowsOfModel(m_roster->sessions());
    case TabModel::Fleet:
        return rowsOfModel(m_fleetTree->nodes());
    case TabModel::Approvals:
        return rowsOfModel(m_approvals->pending());
    case TabModel::Routing:
        return rowsOfModel(m_routing->rules());
    case TabModel::Cron:
        return rowsOfModel(m_cron->jobs());
    default:
        return {};
    }
}

void RootWidget::refreshPageIfActive(int kind)
{
    // Re-render `kind`'s page doc in place if it is the active page tab. Works for
    // both interactive hubs and the read-only Dashboard projection (so a live
    // roster/approvals change repaints the counters).
    if (m_active != nullptr || m_tabModel == nullptr) {
        return;
    }
    const int idx = m_tabModel->currentIndex();
    if (idx < 0 || m_tabModel->kindAt(idx) != kind) {
        return;
    }
    // Clamp the interactive selection to the (possibly shrunk) row count.
    const int rows = static_cast<int>(pageActionRows(kind).size());
    if (rows > 0) {
        int& sel = m_pageSel[kind];
        sel = qBound(0, sel, rows - 1);
    }
    m_pageDoc.loadMarkdown(pageMarkdownForKind(kind));
    if (m_transcript != nullptr) {
        m_transcript->reload();
    }
}

void RootWidget::refreshActivePage()
{
    refreshPageIfActive(activePageKind());
}

void RootWidget::movePageSelection(int delta)
{
    const int kind = activePageKind();
    if (kind < 0) {
        return;
    }
    const int rows = static_cast<int>(pageActionRows(kind).size());
    if (rows == 0) {
        return;
    }
    int& sel = m_pageSel[kind];
    sel = qBound(0, sel + delta, rows - 1);
    refreshActivePage();
}

bool RootWidget::handlePageActionKey(Tui::ZKeyEvent* event)
{
    if (event->modifiers() != Qt::NoModifier) {
        return false;
    }
    const int kind = activePageKind();
    if (kind < 0) {
        return false;
    }
    const QList<QVariantMap> rows = pageActionRows(kind);
    const QString text = event->text();
    const int key = event->key();

    // j/k (and Up/Down, in case the transcript is not the focus owner) move the row
    // cursor. The transcript eats Up/Down for scrolling when it is focused, so j/k
    // are the canonical hub-navigation keys.
    if (key == Qt::Key_Up || text == QStringLiteral("k")) {
        movePageSelection(-1);
        event->accept();
        return true;
    }
    if (key == Qt::Key_Down || text == QStringLiteral("j")) {
        movePageSelection(1);
        event->accept();
        return true;
    }

    if (rows.isEmpty()) {
        return false;
    }
    const int sel = qBound(0, m_pageSel.value(kind, 0), static_cast<int>(rows.size()) - 1);
    const QVariantMap row = rows.at(sel);
    const QString id = row.value(QStringLiteral("id")).toString();
    const bool enter = key == Qt::Key_Enter || key == Qt::Key_Return;
    bool acted = false;

    switch (kind) {
    case TabModel::Models:
        if (enter) {
            m_modelCatalog->activate(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_modelCatalog->remove(id);
            acted = true;
        }
        break;
    case TabModel::Accounts:
        if (enter || text == QStringLiteral("R")) {
            m_accounts->reauth(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_accounts->remove(id);
            acted = true;
        }
        break;
    case TabModel::Profiles:
        if (enter) {
            m_profiles->setDefault(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_profiles->remove(id);
            acted = true;
        }
        break;
    case TabModel::Sessions:
        if (text == QStringLiteral("s")) {
            m_roster->suspend(id);
            acted = true;
        } else if (text == QStringLiteral("R") || enter) {
            m_roster->resume(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_roster->close(id);
            acted = true;
        }
        break;
    case TabModel::Fleet:
        if (key == Qt::Key_Space || enter || text == QStringLiteral("p")) {
            const bool paused = row.value(QStringLiteral("status")).toString()
                == QLatin1String("paused");
            if (paused) {
                m_fleetTree->resume(id);
            } else {
                m_fleetTree->pause(id);
            }
            acted = true;
        }
        break;
    case TabModel::Approvals:
        if (text == QStringLiteral("a") || enter) {
            m_approvals->approve(id);
            acted = true;
        } else if (text == QStringLiteral("d")) {
            m_approvals->deny(id);
            acted = true;
        }
        break;
    case TabModel::Routing:
        if (key == Qt::Key_Space || enter) {
            m_routing->setEnabled(id, !row.value(QStringLiteral("enabled")).toBool());
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_routing->remove(id);
            acted = true;
        }
        break;
    case TabModel::Cron:
        if (key == Qt::Key_Space || enter) {
            m_cron->setEnabled(id, !row.value(QStringLiteral("enabled")).toBool());
            acted = true;
        } else if (text == QStringLiteral("t")) {
            m_cron->runNow(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_cron->remove(id);
            acted = true;
        }
        break;
    default:
        break;
    }

    if (acted) {
        refreshActivePage();
        event->accept();
        return true;
    }
    return false;
}

void RootWidget::openSessionSettingsOverlay()
{
    // Only meaningful over a transcript tab; bind the per-session overrides to
    // the focused chat first (parity with ComposerControls setting sessionId).
    if (m_active == nullptr || m_sessionSettings == nullptr) {
        return;
    }
    m_sessionSettings->setSessionId(m_active->sessionId);
    session::ISessionSettings* ss = m_sessionSettings;

    auto* dlg = new Tui::ZDialog(this);
    dlg->setOptions(Tui::ZWindow::DeleteOnClose);
    dlg->setWindowTitle(QStringLiteral("Session settings"));
    dlg->setContentsMargins({ 2, 1, 2, 1 });
    auto* layout = new Tui::ZVBoxLayout();
    dlg->setLayout(layout);

    auto* hint = new Tui::ZLabel(
        QStringLiteral("Enter / Space on a row cycles or toggles it · Esc closes"), dlg);
    layout->addWidget(hint);
    layout->addSpacing(1);

    auto* profileBtn = new Tui::ZButton(QString(), dlg);
    auto* effortBtn = new Tui::ZButton(QString(), dlg);
    auto* fastBtn = new Tui::ZButton(QString(), dlg);
    auto* verboseBtn = new Tui::ZButton(QString(), dlg);
    layout->addWidget(profileBtn);
    layout->addWidget(effortBtn);
    layout->addWidget(fastBtn);
    layout->addWidget(verboseBtn);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* closeBtn = new Tui::ZButton(QStringLiteral("Close"), dlg);
    closeBtn->setDefault(true);
    buttons->addWidget(closeBtn);

    const auto sync = [ss, profileBtn, effortBtn, fastBtn, verboseBtn] {
        profileBtn->setText(QStringLiteral("Profile: %1").arg(ss->profile()));
        effortBtn->setText(QStringLiteral("Effort:  %1").arg(ss->effort()));
        fastBtn->setText(QStringLiteral("Fast:    %1")
                             .arg(ss->fast() ? QStringLiteral("on") : QStringLiteral("off")));
        verboseBtn->setText(QStringLiteral("Verbose: %1")
                                .arg(ss->verbose() ? QStringLiteral("on")
                                                   : QStringLiteral("off")));
    };
    sync();

    connect(effortBtn, &Tui::ZButton::clicked, dlg, [ss, sync] {
        const QStringList opts = ss->effortOptions();
        if (!opts.isEmpty()) {
            const int idx = static_cast<int>(qMax<qsizetype>(0, opts.indexOf(ss->effort())));
            ss->setEffort(opts.at((idx + 1) % static_cast<int>(opts.size())));
        }
        sync();
    });
    connect(profileBtn, &Tui::ZButton::clicked, dlg, [this, ss, sync] {
        const QStringList names = m_profiles->profileNames();
        if (!names.isEmpty()) {
            const int idx = static_cast<int>(qMax<qsizetype>(0, names.indexOf(ss->profile())));
            ss->setProfile(names.at((idx + 1) % static_cast<int>(names.size())));
        }
        sync();
    });
    connect(fastBtn, &Tui::ZButton::clicked, dlg, [ss, sync] {
        ss->setFast(!ss->fast());
        sync();
    });
    connect(verboseBtn, &Tui::ZButton::clicked, dlg, [ss, sync] {
        ss->setVerbose(!ss->verbose());
        sync();
    });
    connect(closeBtn, &Tui::ZButton::clicked, dlg, &Tui::ZDialog::close);

    dlg->setGeometry(QRect(0, 0, 48, 13));
    effortBtn->setFocus();
}

void RootWidget::openCheckpointsOverlay()
{
    if (m_active == nullptr || m_checkpoints == nullptr) {
        return;
    }
    m_checkpoints->setSessionId(m_active->sessionId);
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_checkpoints->checkpoints());
    const QList<QVariantMap> rows = model != nullptr ? model->rows() : QList<QVariantMap>{};

    auto* dlg = new Tui::ZDialog(this);
    dlg->setOptions(Tui::ZWindow::DeleteOnClose);
    dlg->setWindowTitle(QStringLiteral("Checkpoints"));
    dlg->setContentsMargins({ 2, 1, 2, 1 });
    auto* layout = new Tui::ZVBoxLayout();
    dlg->setLayout(layout);

    auto* hint = new Tui::ZLabel(
        QStringLiteral("Enter restores the selected checkpoint · Esc closes"), dlg);
    layout->addWidget(hint);

    auto* list = new Tui::ZListView(dlg);
    QStringList display;
    QStringList ids;
    for (const QVariantMap& c : rows) {
        ids << c.value(QStringLiteral("id")).toString();
        display << QStringLiteral("%1  ·  %2  ·  %3 tok%4")
                       .arg(c.value(QStringLiteral("label")).toString(),
                            c.value(QStringLiteral("time")).toString(),
                            c.value(QStringLiteral("tokens")).toString(),
                            c.value(QStringLiteral("current")).toBool()
                                ? QStringLiteral("  (current)")
                                : QString());
    }
    if (display.isEmpty()) {
        display << QStringLiteral("(no checkpoints)");
    }
    list->setItems(display);
    if (list->model() != nullptr && !rows.isEmpty()) {
        list->setCurrentIndex(list->model()->index(0, 0));
    }
    layout->addWidget(list);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* restoreBtn = new Tui::ZButton(QStringLiteral("Restore"), dlg);
    buttons->addWidget(restoreBtn);
    auto* closeBtn = new Tui::ZButton(QStringLiteral("Close"), dlg);
    closeBtn->setDefault(true);
    buttons->addWidget(closeBtn);

    const auto doRestore = [this, dlg, list, ids] {
        const int row = list->currentIndex().row();
        if (row >= 0 && row < ids.size()) {
            m_checkpoints->restore(ids.at(row));
        }
        dlg->close();
    };
    connect(restoreBtn, &Tui::ZButton::clicked, dlg, doRestore);
    connect(list, &Tui::ZListView::enterPressed, dlg, [doRestore](int) { doRestore(); });
    connect(closeBtn, &Tui::ZButton::clicked, dlg, &Tui::ZDialog::close);

    dlg->setGeometry(QRect(0, 0, 62, qBound(9, static_cast<int>(rows.size()) + 8, 20)));
    list->setFocus();
}

QString RootWidget::buildModelsMarkdown(int sel) const
{
    const auto rowsOf = [](QObject* m) {
        auto* vlm = qobject_cast<uimodels::VariantListModel*>(m);
        return vlm ? vlm->rows() : QList<QVariantMap>{};
    };
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += QStringLiteral("# Models\n\n");
    md += QStringLiteral("Installed models, shared with the GUI. **j/k** move · **Enter** "
                         "activates · **x** removes.\n\n");

    md += QStringLiteral("## Installed\n\n");
    const auto installed = rowsOf(m_modelCatalog->installed());
    if (installed.isEmpty()) {
        md += QStringLiteral("_None installed._\n\n");
    } else {
        for (int i = 0; i < installed.size(); ++i) {
            const QVariantMap& m = installed.at(i);
            md += QStringLiteral("- %1**%2** (%3, %4 GiB)%5\n")
                      .arg(mark(i), m.value(QStringLiteral("name")).toString(),
                           m.value(QStringLiteral("params")).toString(),
                           m.value(QStringLiteral("sizeGiB")).toString(),
                           m.value(QStringLiteral("active")).toBool()
                               ? QStringLiteral(" — **active**")
                               : QString());
        }
        md += QLatin1Char('\n');
    }

    md += QStringLiteral("## Discover\n\n");
    for (const QVariantMap& m : rowsOf(m_modelCatalog->discover())) {
        md += QStringLiteral("- %1 — %2 · %3 GiB · %4%5\n")
                  .arg(m.value(QStringLiteral("name")).toString(),
                       m.value(QStringLiteral("params")).toString(),
                       m.value(QStringLiteral("sizeGiB")).toString(),
                       m.value(QStringLiteral("provider")).toString(),
                       m.value(QStringLiteral("installed")).toBool()
                           ? QStringLiteral(" (installed)")
                           : QString());
    }

    md += QStringLiteral("\n## Providers\n\n");
    const QVariantList provs = m_modelCatalog->providers();
    for (const QVariant& v : provs) {
        const QVariantMap m = v.toMap();
        md += QStringLiteral("- %1 (%2)%3\n")
                  .arg(m.value(QStringLiteral("name")).toString(),
                       m.value(QStringLiteral("kind")).toString(),
                       m.value(QStringLiteral("configured")).toBool()
                           ? QStringLiteral(" — configured")
                           : QString());
    }

    return md;
}

QString RootWidget::buildRoutingMarkdown(int sel) const
{
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_routing->rules());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += QStringLiteral("# Routing\n\n");
    md += QStringLiteral("Intent → model rules, shared with the GUI. **j/k** move · "
                         "**Space/Enter** toggle · **x** delete.\n\n");
    if (model != nullptr) {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& r = rows.at(i);
            md += QStringLiteral("- %1**%2** → `%3` (fallback `%4`)%5\n")
                      .arg(mark(i), r.value(QStringLiteral("intent")).toString(),
                           r.value(QStringLiteral("target")).toString(),
                           r.value(QStringLiteral("fallback")).toString(),
                           r.value(QStringLiteral("enabled")).toBool()
                               ? QString()
                               : QStringLiteral(" — _disabled_"));
        }
    }
    return md;
}

QString RootWidget::buildCronMarkdown(int sel) const
{
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_cron->jobs());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += QStringLiteral("# Scheduled jobs\n\n");
    md += QStringLiteral("**j/k** move · **Space/Enter** enable/disable · **t** run now · "
                         "**x** delete.\n\n");
    if (model != nullptr) {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& j = rows.at(i);
            md += QStringLiteral("## %1%2%3\n\n")
                      .arg(mark(i), j.value(QStringLiteral("name")).toString(),
                           j.value(QStringLiteral("enabled")).toBool()
                               ? QString()
                               : QStringLiteral(" (disabled)"));
            md += QStringLiteral("- Schedule: `%1`\n")
                      .arg(j.value(QStringLiteral("schedule")).toString());
            md += QStringLiteral("- Profile: %1\n").arg(j.value(QStringLiteral("profile")).toString());
            md += QStringLiteral("- Next: %1 · Last: %2\n\n")
                      .arg(j.value(QStringLiteral("nextRun")).toString(),
                           j.value(QStringLiteral("lastRun")).toString());
        }
    }
    return md;
}

QString RootWidget::buildMemoryMarkdown() const
{
    QString md;
    const QString agent = m_memory != nullptr ? m_memory->profile() : QString();
    md += QStringLiteral("# Memory - %1\n\n").arg(agent.isEmpty() ? QStringLiteral("agent") : agent);
    md += QStringLiteral("Mnemosyne memory for the active scope, shared with the GUI. The "
                         "knowledge graph renders as a node-link diagram in the GUI; here it is "
                         "an adjacency listing.\n\n");

    // --- Overview ---------------------------------------------------------
    if (m_memStats != nullptr) {
        md += QStringLiteral("## Overview\n\n");
        md += QStringLiteral("- Working: **%1** · Episodic: **%2** · Scratchpad: **%3**\n")
                  .arg(m_memStats->working())
                  .arg(m_memStats->episodic())
                  .arg(m_memStats->scratchpad());
        md += QStringLiteral("- Facts: **%1** · Conflicts: **%2**\n\n")
                  .arg(m_memStats->facts())
                  .arg(m_memStats->conflicts());

        const auto gauges = [&md](const QString& title, const QVariantList& rows) {
            if (rows.isEmpty())
                return;
            md += QStringLiteral("**%1**\n\n").arg(title);
            for (const QVariant& v : rows) {
                const QVariantMap r = v.toMap();
                const double frac = r.value(QStringLiteral("fraction")).toDouble();
                const int filled = qBound(0, static_cast<int>(frac * 12.0 + 0.5), 12);
                const QString bar = QString(filled, QChar('#')) + QString(12 - filled, QChar('-'));
                md += QStringLiteral("- `%1` %2 %3\n")
                          .arg(bar, r.value(QStringLiteral("key")).toString())
                          .arg(r.value(QStringLiteral("count")).toInt());
            }
            md += QStringLiteral("\n");
        };
        gauges(QStringLiteral("By source"), m_memStats->bySource());
        gauges(QStringLiteral("By veracity"), m_memStats->byVeracity());
        gauges(QStringLiteral("By lifecycle"), m_memStats->byDegradation());
    }

    // --- Memories ---------------------------------------------------------
    if (m_memList != nullptr) {
        md += QStringLiteral("## Memories\n\n");
        const int n = m_memList->rowCount();
        if (n == 0)
            md += QStringLiteral("_No memories in scope._\n\n");
        for (int i = 0; i < n; ++i) {
            const QVariantMap e = m_memList->entryAt(i);
            md += QStringLiteral("- **%1** _(%2 · %3 · imp %4)_\n")
                      .arg(e.value(QStringLiteral("content")).toString(),
                           e.value(QStringLiteral("tier")).toString(),
                           e.value(QStringLiteral("veracity")).toString())
                      .arg(e.value(QStringLiteral("importance")).toDouble(), 0, 'f', 2);
        }
        md += QStringLiteral("\n");
    }

    // --- Knowledge graph adjacency (GUI renders this as a node-link graph) -
    if (m_memGraph != nullptr) {
        md += QStringLiteral("## Graph adjacency\n\n");
        const QVariantList edges = m_memGraph->edges();
        if (edges.isEmpty())
            md += QStringLiteral("_No edges in scope._\n\n");
        for (const QVariant& v : edges) {
            const QVariantMap e = v.toMap();
            md += QStringLiteral("- `%1` --%2--> `%3`\n")
                      .arg(e.value(QStringLiteral("source")).toString(),
                           e.value(QStringLiteral("edgeType")).toString(),
                           e.value(QStringLiteral("target")).toString());
        }
        md += QStringLiteral("\n");
    }

    // --- Timeline ---------------------------------------------------------
    if (m_memTimeline != nullptr) {
        md += QStringLiteral("## Timeline\n\n");
        const int n = m_memTimeline->rowCount();
        for (int i = 0; i < n; ++i) {
            const QModelIndex idx = m_memTimeline->index(i);
            const bool header =
                m_memTimeline->data(idx, memoryui::MemoryTimelineModel::IsHeaderRole).toBool();
            if (header) {
                md += QStringLiteral("\n**%1**\n\n")
                          .arg(m_memTimeline
                                   ->data(idx, memoryui::MemoryTimelineModel::GroupKeyRole)
                                   .toString());
            } else {
                md += QStringLiteral("- _%1_ %2\n")
                          .arg(m_memTimeline->data(idx, memoryui::MemoryTimelineModel::KindRole)
                                   .toString(),
                               m_memTimeline
                                   ->data(idx, memoryui::MemoryTimelineModel::SummaryRole)
                                   .toString());
            }
        }
        md += QStringLiteral("\n");
    }

    return md;
}

QString RootWidget::buildProfileMarkdown(const QString& profileRef) const
{
    QString md;
    if (profileRef.isEmpty() || m_profiles == nullptr) {
        md += QStringLiteral("# Profile\n\n_No agent selected._\n");
        return md;
    }
    const QVariantMap p = m_profiles->profile(profileRef);
    const auto val = [&p](const QString& key, const QString& fallback) {
        const QString v = p.value(key).toString();
        return v.isEmpty() ? fallback : v;
    };

    md += QStringLiteral("# Profile - %1\n\n")
              .arg(val(QStringLiteral("name"), profileRef));
    md += QStringLiteral("Agent == profile. Memory lives in this agent's bank (`%1`).\n\n")
              .arg(profileRef);

    md += QStringLiteral("## Engine\n\n");
    md += QStringLiteral("- Provider: **%1**\n").arg(val(QStringLiteral("provider"), QStringLiteral("-")));
    md += QStringLiteral("- Model: **%1**\n").arg(val(QStringLiteral("model"), QStringLiteral("-")));
    md += QStringLiteral("- Base URL: %1\n")
              .arg(val(QStringLiteral("baseUrl"), QStringLiteral("(provider default)")));
    md += QStringLiteral("- Context engine: %1\n\n")
              .arg(val(QStringLiteral("contextEngine"), QStringLiteral("lcm")));

    md += QStringLiteral("## Memory\n\n");
    md += QStringLiteral("- Memory provider: **%1**\n\n")
              .arg(val(QStringLiteral("memoryProvider"), QStringLiteral("mnemosyne")));

    md += QStringLiteral("## Persona\n\n");
    md += val(QStringLiteral("systemPrompt"), QStringLiteral("-")) + QStringLiteral("\n\n");

    const auto chips = [&md, &p](const QString& title, const QString& key) {
        const QStringList items = p.value(key).toStringList();
        md += QStringLiteral("## %1\n\n").arg(title);
        if (items.isEmpty()) {
            md += QStringLiteral("_none_\n\n");
            return;
        }
        for (const QString& it : items)
            md += QStringLiteral("- `%1`\n").arg(it);
        md += QStringLiteral("\n");
    };
    chips(QStringLiteral("Tool allowlist"), QStringLiteral("toolAllowlist"));
    chips(QStringLiteral("Skills"), QStringLiteral("skills"));
    return md;
}

QString RootWidget::buildDashboardMarkdown() const
{
    auto* activity = qobject_cast<uimodels::VariantListModel*>(m_dashboard->activity());

    QString md;
    md += QStringLiteral("# Dashboard\n\n");
    md += QStringLiteral("- Active sessions: **%1**\n").arg(m_dashboard->activeSessions());
    md += QStringLiteral("- Running agents: **%1**\n").arg(m_dashboard->runningAgents());
    md += QStringLiteral("- Pending approvals: **%1**\n").arg(m_dashboard->pendingApprovals());
    md += QStringLiteral("- Tokens today: **%1**\n").arg(m_dashboard->tokensToday());
    md += QStringLiteral("- Daemon health: **%1**\n\n")
              .arg(m_dashboard->healthy() ? QStringLiteral("OK") : QStringLiteral("degraded"));

    md += QStringLiteral("## Recent activity\n\n");
    if (activity != nullptr) {
        for (const QVariantMap& a : activity->rows()) {
            md += QStringLiteral("- _%1_ — %2\n")
                      .arg(a.value(QStringLiteral("time")).toString(),
                           a.value(QStringLiteral("text")).toString());
        }
    }
    return md;
}

QString RootWidget::buildFleetMarkdown(int sel) const
{
    auto* nodes = qobject_cast<uimodels::VariantListModel*>(m_fleetTree->nodes());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += QStringLiteral("# Fleet\n\n");
    md += QStringLiteral("Orchestrator/worker tree, shared with the GUI. **j/k** move · "
                         "**Space/Enter** pause/resume.\n\n");
    if (nodes != nullptr) {
        const auto rows = nodes->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& n = rows.at(i);
            const int depth = n.value(QStringLiteral("depth")).toInt();
            md += QString(depth * 2, QLatin1Char(' '));
            md += QStringLiteral("- %1%2 — %3 (`%4`)\n")
                      .arg(mark(i), n.value(QStringLiteral("name")).toString(),
                           n.value(QStringLiteral("status")).toString(),
                           n.value(QStringLiteral("model")).toString());
        }
    }
    return md;
}

QString RootWidget::buildSessionsMarkdown(int sel) const
{
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_roster->sessions());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += QStringLiteral("# Sessions\n\n");
    md += QStringLiteral("**j/k** move · **s** suspend · **R**/**Enter** resume · "
                         "**x** close.\n\n");
    if (model != nullptr) {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& s = rows.at(i);
            md += QStringLiteral("- %1**%2** — %3 · %4 · %5 · %6 tok\n")
                      .arg(mark(i), s.value(QStringLiteral("title")).toString(),
                           s.value(QStringLiteral("state")).toString(),
                           s.value(QStringLiteral("lifecycle")).toString(),
                           s.value(QStringLiteral("profile")).toString(),
                           s.value(QStringLiteral("tokens")).toString());
        }
    }
    return md;
}

QString RootWidget::buildApprovalsMarkdown(int sel) const
{
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_approvals->pending());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += QStringLiteral("# Approvals\n\n");
    if (model == nullptr || model->count() == 0) {
        md += QStringLiteral("_Inbox zero — no pending approvals._\n");
        return md;
    }
    md += QStringLiteral("**j/k** move · **a**/**Enter** approve · **d** deny.\n\n");
    const auto rows = model->rows();
    for (int i = 0; i < rows.size(); ++i) {
        const QVariantMap& a = rows.at(i);
        md += QStringLiteral("## %1%2 (%3 risk)\n\n")
                  .arg(mark(i), a.value(QStringLiteral("tool")).toString(),
                       a.value(QStringLiteral("risk")).toString());
        md += QStringLiteral("- Session: %1\n").arg(a.value(QStringLiteral("session")).toString());
        md += QStringLiteral("- Command: `%1`\n\n")
                  .arg(a.value(QStringLiteral("command")).toString());
    }
    return md;
}

QString RootWidget::buildProfilesMarkdown(int sel) const
{
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_profiles->profiles());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += QStringLiteral("# Profiles\n\n");
    md += QStringLiteral("Agent profiles, shared with the GUI. **j/k** move · **Enter** "
                         "set default · **x** delete. Use the GUI editor for model / "
                         "prompt / skills.\n\n");

    if (model != nullptr) {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& p = rows.at(i);
            md += QStringLiteral("## %1%2%3\n\n")
                      .arg(mark(i), p.value(QStringLiteral("name")).toString(),
                           p.value(QStringLiteral("isDefault")).toBool()
                               ? QStringLiteral(" (default)")
                               : QString());
            md += QStringLiteral("- Model: `%1`\n").arg(p.value(QStringLiteral("model")).toString());
            const QString desc = p.value(QStringLiteral("description")).toString();
            if (!desc.isEmpty()) {
                md += QStringLiteral("- %1\n").arg(desc);
            }
            md += QStringLiteral("- Skills: %1\n")
                      .arg(p.value(QStringLiteral("skills")).toStringList().join(
                          QStringLiteral(", ")));
            md += QStringLiteral("- Tools: %1\n\n")
                      .arg(p.value(QStringLiteral("tools")).toStringList().join(
                          QStringLiteral(", ")));
        }
    }

    return md;
}

QString RootWidget::buildAccountsMarkdown(int sel) const
{
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_accounts->accounts());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += QStringLiteral("# Accounts\n\n");
    md += QStringLiteral("Connected provider accounts, shared with the GUI. **j/k** move · "
                         "**R**/**Enter** re-auth · **x** remove. Use the GUI wizard to add "
                         "accounts.\n\n");

    md += QStringLiteral("## Connected\n\n");
    if (model == nullptr || model->count() == 0) {
        md += QStringLiteral("_No accounts._\n\n");
    } else {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& a = rows.at(i);
            md += QStringLiteral("- %1**%2** — %3 (%4) · %5\n")
                      .arg(mark(i), a.value(QStringLiteral("label")).toString(),
                           a.value(QStringLiteral("kind")).toString() == QLatin1String("oauth")
                               ? QStringLiteral("OAuth")
                               : QStringLiteral("API key"),
                           a.value(QStringLiteral("status")).toString(),
                           a.value(QStringLiteral("detail")).toString());
        }
        md += QLatin1Char('\n');
    }

    md += QStringLiteral("## Available providers\n\n");
    for (const QVariant& v : m_accounts->availableProviders()) {
        const QVariantMap p = v.toMap();
        md += QStringLiteral("- %1 (%2)\n")
                  .arg(p.value(QStringLiteral("name")).toString(),
                       p.value(QStringLiteral("kinds")).toStringList().join(QStringLiteral(", ")));
    }

    return md;
}

QString RootWidget::buildSettingsMarkdown() const
{
    const auto cfg = [this](const char* key) {
        return m_daemonConfig->value(QString::fromLatin1(key)).toString();
    };
    const auto onoff = [this](const char* key) {
        return m_daemonConfig->value(QString::fromLatin1(key)).toBool() ? QStringLiteral("on")
                                                                        : QStringLiteral("off");
    };

    QString md;
    md += QStringLiteral("# Settings\n\n");
    md += QStringLiteral("The settings page, shared with the GUI. Edit values in the GUI; "
                         "the TUI reflects the same daemon-config + app prefs. **F8** cycles "
                         "the theme.\n\n");

    md += QStringLiteral("## Connection\n\n");
    md += QStringLiteral("- State: **%1**\n").arg(m_connection->state());
    md += QStringLiteral("- Mode: %1\n").arg(m_connection->mode());
    md += QStringLiteral("- Target: `%1`\n\n").arg(m_connection->target());

    md += QStringLiteral("## Model\n\n");
    md += QStringLiteral("- Default: `%1`\n").arg(cfg("model/default"));
    md += QStringLiteral("- Reasoning effort: %1\n").arg(cfg("model/effort"));
    md += QStringLiteral("- Fast mode: %1\n\n").arg(onoff("model/fast"));

    md += QStringLiteral("## Chat\n\n");
    md += QStringLiteral("- Stream responses: %1\n").arg(onoff("chat/streaming"));
    md += QStringLiteral("- Send on Enter: %1\n").arg(onoff("chat/sendOnEnter"));
    md += QStringLiteral("- Show token counts: %1\n\n").arg(onoff("chat/showTokenCounts"));

    md += QStringLiteral("## Safety\n\n");
    md += QStringLiteral("- Approval policy: %1\n").arg(cfg("safety/approvalPolicy"));
    md += QStringLiteral("- Filesystem access: %1\n").arg(cfg("safety/sandbox"));
    md += QStringLiteral("- Allow network: %1\n\n").arg(onoff("safety/allowNetwork"));

    md += QStringLiteral("## Memory & Context\n\n");
    md += QStringLiteral("- Max context tokens: %1\n")
              .arg(m_daemonConfig->value(QStringLiteral("memory/contextWindow")).toInt());
    md += QStringLiteral("- Auto-compact: %1\n").arg(onoff("memory/autoCompact"));
    md += QStringLiteral("- Persist memory: %1\n\n").arg(onoff("memory/persistMemory"));

    md += QStringLiteral("## Workspace\n\n");
    md += QStringLiteral("- Root: `%1`\n").arg(cfg("workspace/root"));
    md += QStringLiteral("- Respect .gitignore: %1\n\n").arg(onoff("workspace/followGitignore"));

    md += QStringLiteral("## Voice\n\n");
    md += QStringLiteral("- Enabled: %1\n").arg(onoff("voice/enabled"));
    md += QStringLiteral("- Transcription model: %1\n\n").arg(cfg("voice/model"));

    md += QStringLiteral("## Advanced\n\n");
    md += QStringLiteral("- Log level: %1\n").arg(cfg("advanced/logLevel"));
    md += QStringLiteral("- Telemetry: %1\n").arg(onoff("advanced/telemetry"));
    md += QStringLiteral("- Experimental tools: %1\n").arg(onoff("advanced/experimentalTools"));

    return md;
}

void RootWidget::destroySession(int tabId)
{
    // File tabs hold an editor controller rather than a TabSession.
    if (auto fit = m_fileSessions.find(tabId); fit != m_fileSessions.end()) {
        editor::CodeEditorController* c = fit.value();
        if (m_editorView != nullptr && m_editorView->controller() == c)
            m_editorView->setController(nullptr);
        for (auto kit = m_fileByKey.begin(); kit != m_fileByKey.end();) {
            if (kit.value() == c)
                kit = m_fileByKey.erase(kit);
            else
                ++kit;
        }
        m_fileSessions.erase(fit);
        delete c;
        return;
    }

    auto it = m_sessions.find(tabId);
    if (it == m_sessions.end()) {
        return;
    }
    TabSession* s = it.value();
    if (m_active == s) {
        m_active = nullptr;
        // Detach the transcript from the doc we are about to delete; the follow-up
        // currentTabChanged -> activateTab repoints it at the new active tab.
        if (m_transcript != nullptr) {
            m_transcript->setDocument(&m_pageDoc);
        }
    }
    m_sessions.erase(it);
    delete s;
}

void RootWidget::dumpGeometry() const
{
    const auto g = [](const char* n, const Tui::ZWidget* w) {
        if (w == nullptr) {
            return;
        }
        const QRect r = w->geometry();
        fprintf(stderr, "%-10s x=%d y=%d w=%d h=%d\n", n, r.x(), r.y(), r.width(), r.height());
    };
    g("window", m_window);
    g("sidebar", m_sidebarView);
    g("list", m_listView);
    g("transcript", m_transcript);
    g("chrome", m_composerChrome);
    g("composer", m_composer);
    g("filetree", m_fileTreeView);
    g("editor", m_editorView);
    g("footer", m_footer);
}

void RootWidget::focusComposer() const
{
    if (m_composer != nullptr) {
        m_composer->setFocus();
    }
}

void RootWidget::focusTranscript() const
{
    if (m_transcript != nullptr) {
        m_transcript->setFocus();
    }
}

void RootWidget::promptQuit()
{
    if (m_quitDialog != nullptr) {
        return; // already asking
    }
    // While the modal is up it holds focus, so its own keyEvent maps Esc ->
    // reject(); the RootWidget Esc fallback never fires (no juggling needed).
    m_quitDialog = new QuitDialog(this);

    connect(m_quitDialog, &QuitDialog::quitConfirmed, this, [] { QCoreApplication::quit(); });
    connect(m_quitDialog, &Tui::ZDialog::rejected, this, [this] {
        m_quitDialog = nullptr; // DeleteOnClose frees it
        if (m_sidebarView != nullptr) {
            m_sidebarView->setFocus(); // restore keyboard focus to the panes
        }
    });
}

void RootWidget::openModelPicker()
{
    if (m_composerSession == nullptr) {
        return;
    }
    if (m_modelPicker == nullptr) {
        m_modelPicker = new PaletteDialog(QStringLiteral("Model"), this);
        connect(m_modelPicker, &PaletteDialog::activated, this, [this](const QString& id) {
            // Entry ids are the catalog index (stringified).
            bool ok = false;
            const int index = id.toInt(&ok);
            if (ok) {
                m_composerSession->selectModel(index);
            }
            if (m_composer != nullptr) {
                m_composer->setFocus();
            }
        });
    }
    const QVariantList catalog = m_composerSession->modelCatalog();
    QVector<PaletteDialog::Item> items;
    items.reserve(catalog.size());
    for (int i = 0; i < catalog.size(); ++i) {
        const QVariantMap m = catalog.at(i).toMap();
        const bool current = i == m_composerSession->currentModelIndex();
        items.push_back({ QString::number(i),
                          (current ? QStringLiteral("\u2713 ") : QStringLiteral("  "))
                              + m.value(QStringLiteral("label")).toString(),
                          m.value(QStringLiteral("provider")).toString() });
    }
    m_modelPicker->setItems(items);
    m_modelPicker->openCentered();
}

void RootWidget::toggleExplorer()
{
    if (m_fileTreeView == nullptr) {
        return;
    }
    const bool show = !m_fileTreeView->isVisible();
    m_fileTreeView->setVisible(show);
    if (show) {
        m_fileTreeView->setFocus();
    }
    // Persist via the same "ui/showFileExplorer" key the GUI's UiSettings uses, so
    // the explorer's open/closed state survives a restart (and stays in sync when
    // both shells share an org/app QSettings scope).
    QSettings().setValue(QStringLiteral("ui/showFileExplorer"), show);
}

void RootWidget::openCommandPalette()
{
    if (m_commands == nullptr) {
        return;
    }
    if (m_commandPalette == nullptr) {
        m_commandPalette = new PaletteDialog(QStringLiteral("Commands"), this);
        connect(m_commandPalette, &PaletteDialog::activated, this, [this](const QString& id) {
            // Route palette ids to existing actions; session-scoped verbs go to
            // the active orchestrator (which has no UI of its own here, so the slash
            // handlers above cover the rest).
            // App-level manager pages open as singleton page tabs (the TUI's
            // equivalent of the GUI's Nav overlay).
            if (openManagerPage(id)) {
                // routed to a page tab
            } else if (id == QStringLiteral("new")) {
                newTranscriptTab();
            } else if (id == QStringLiteral("theme")) {
                cycleTheme();
            } else if (id == QStringLiteral("model")) {
                openModelPicker();
            } else if (id == QStringLiteral("search")) {
                if (m_search != nullptr) {
                    m_listView->setFocus();
                }
            } else if (id == QStringLiteral("files")) {
                toggleExplorer();
            } else if (id == QStringLiteral("reasoning")) {
                m_composerSession->cycleReasoningEffort();
            } else if (id == QStringLiteral("fast")) {
                m_composerSession->toggleFastMode();
            } else if (id == QStringLiteral("verbose")) {
                m_composerSession->toggleVerbose();
            } else if (m_active != nullptr) {
                // retry/edit/undo/clear/usage/compress/save/title/help/distraction.
                m_composerSession->invokeCommand(id);
            }
            if (m_composer != nullptr) {
                m_composer->setFocus();
            }
        });
    }
    // Reset the filter to show the full catalog, then build the items.
    m_commands->search(QString());
    const QVector<CommandRegistry::Command> cmds = m_commands->visibleCommands();
    QVector<PaletteDialog::Item> items;
    items.reserve(cmds.size());
    for (const CommandRegistry::Command& c : cmds) {
        QString hint = c.group;
        if (!c.shortcut.isEmpty()) {
            hint += QStringLiteral("  ") + c.shortcut;
        }
        items.push_back({ c.id, c.title, hint });
    }
    m_commandPalette->setItems(items);
    m_commandPalette->openCentered();
}

void RootWidget::cycleTheme()
{
    using theme::ThemeName;
    // Advance through the four themes in a fixed order.
    ThemeName next = ThemeName::Light;
    switch (tpal::activeTheme()) {
    case ThemeName::Light:
        next = ThemeName::Dark;
        break;
    case ThemeName::Dark:
        next = ThemeName::Sepia;
        break;
    case ThemeName::Sepia:
        next = ThemeName::Midnight;
        break;
    case ThemeName::Midnight:
        next = ThemeName::Light;
        break;
    }

    tpal::setActiveTheme(next);
    // Recolor stock widgets (window/dialog/lists/inputs) via the palette...
    setPalette(daemonPalette(next));
    // Keep the accent-tinted text caret in sync with the new theme.
    if (terminal() != nullptr) {
        const Tui::ZColor accent = tpal::accent();
        terminal()->setCursorColor(accent.red(), accent.green(), accent.blue());
    }
    // The session list and transcript bake tpal::* colors into cached span
    // lists at build time, so a bare update() would repaint stale colors: rebuild
    // them. (Selection + scroll survive the rebuild.)
    if (m_listView != nullptr) {
        m_listView->relayout();
    }
    if (m_transcript != nullptr) {
        m_transcript->reload();
    }
    // The remaining custom-painted views sample tpal::* live at paint, so a plain
    // repaint suffices.
    Tui::ZWidget* views[] = { m_window,       m_sidebarView,    m_composerChrome,
                              m_queue,        m_attachments,    m_footer,
                              m_completionPopup, m_search,       m_composer,
                              m_tabStrip,     m_todos,          m_subagents,
                              m_fileTreeView, m_editorView };
    for (Tui::ZWidget* w : views) {
        if (w != nullptr) {
            w->update();
        }
    }
    // The editor's syntax colors are baked by KSyntaxHighlighting per the theme;
    // re-pick the light/dark definition theme for every open file controller.
    const bool dark = next != ThemeName::Light && next != ThemeName::Sepia;
    for (editor::CodeEditorController* c : std::as_const(m_fileSessions)) {
        if (c != nullptr)
            c->setDarkTheme(dark);
    }

    // Persist to the GUI-shared key so both front ends honor the same choice.
    QSettings settings(QStringLiteral("daemon-app"), QStringLiteral("daemon-app"));
    settings.setValue(QStringLiteral("ui/theme"), theme::ThemePalette::toString(next));
}

void RootWidget::refreshTranscript()
{
    if (m_transcript == nullptr || m_active == nullptr) {
        return;
    }
    // Mid-turn the live ingest owns the document tail; reloading from the store
    // would clobber the streamed blocks. Only reparse the persisted markdown when
    // idle (open/switch, the post-submit user-message append, or a finished turn).
    if (m_active->turn != nullptr && m_active->turn->active()) {
        return;
    }
    m_active->doc.loadMarkdown(
        m_active->controller->hasSession() ? m_active->controller->content() : QString());
    m_active->search.refresh();
    m_transcript->reload();
}

void RootWidget::rewindActiveTab(const QString& messageId, bool editMode)
{
    if (m_active == nullptr || messageId.isEmpty()) {
        return;
    }
    // Interrupt-first: pre-empt a live turn so the truncation and replay are the
    // only stream.
    if (m_active->turn != nullptr && m_active->turn->active()) {
        m_active->orchestrator->cancel();
    }
    // Hard-truncate the active document at the message (it and everything after it
    // are dropped) and adopt the result as the persisted content so the controller
    // (the TUI's source of truth) and the document stay in sync.
    const QString text = m_active->doc.rewindToMessage(messageId);
    if (text.isEmpty()) {
        return;
    }
    if (m_active->controller->hasSession()) {
        m_active->controller->updateContent(m_active->doc.toMarkdown());
    }
    m_active->search.refresh();
    m_transcript->reload();

    if (editMode) {
        // Seed the composer with the message text; the user edits and submits,
        // which re-appends it (to the truncated content) and re-runs the turn.
        if (m_composer != nullptr) {
            m_composer->setText(text);
            m_composer->moveCursorToEnd();
            m_composer->setFocus();
        }
    } else {
        // Restore: re-submit the same text as a fresh turn (append + run).
        m_active->orchestrator->submit(text);
    }
}

void RootWidget::onTurnEvents(TabSession* session, const QVariantList& events)
{
    // Route the daemon-shaped events into THIS session's ingest: it appends/patches
    // typed reasoning/tool/content blocks on the session's document keyed by callId,
    // exactly as the GUI's EditorController does. Repaint only when it is on screen.
    session->ingest.ingestAll(events);
    // Keep this session's find matches in step with the freshly ingested tail.
    session->search.refresh();
    // The same stream carries status-only events (usage/context/rateLimit); the
    // ingest skips them, the footer status model consumes them. Only the active
    // tab drives the shared footer.
    if (session == m_active && m_status != nullptr) {
        m_status->applyTurnEvents(events);
    }
    if (session == m_active && m_transcript != nullptr) {
        m_transcript->reload();
    }
}

void RootWidget::updateTodos()
{
    if (m_todos == nullptr) {
        return;
    }
    if (m_active == nullptr) {
        m_todos->setText(QString());
        return;
    }
    TodoListModel* todos = m_active->orchestrator->todos();
    const int n = todos->count();
    if (n == 0) {
        m_todos->setText(QString());
        return;
    }
    // Compact one-line strip: "Tasks:  \u2713 done  \u25cb pending  \u2026".
    QStringList parts;
    for (int i = 0; i < n; ++i) {
        const QString glyph =
            todos->doneAt(i) ? QStringLiteral("\u2713") : QStringLiteral("\u25cb");
        parts << (glyph + QStringLiteral(" ") + todos->textAt(i));
    }
    m_todos->setText(QStringLiteral("Tasks:  ") + parts.join(QStringLiteral("   ")));
}

void RootWidget::updateSubagents()
{
    if (m_subagents == nullptr) {
        return;
    }
    if (m_active == nullptr) {
        m_subagents->setText(QString());
        if (m_status != nullptr) {
            m_status->setAgentsRunning(0);
            m_status->setAgentsFailed(0);
        }
        return;
    }
    SubagentModel* subs = m_active->orchestrator->subagents();
    if (m_status != nullptr) {
        m_status->setAgentsRunning(subs->runningCount());
        m_status->setAgentsFailed(subs->failedCount());
    }
    const int n = subs->count();
    if (n == 0) {
        m_subagents->setText(QString());
        return;
    }
    // Compact one-line strip: "Subagents:  \u25cf explore (42 files)  \u2713 run-tests ...".
    QStringList parts;
    for (int i = 0; i < n; ++i) {
        const QString status = subs->statusAt(i);
        const QString glyph = status == QStringLiteral("done") ? QStringLiteral("\u2713")
            : status == QStringLiteral("error")                ? QStringLiteral("\u2717")
                                                               : QStringLiteral("\u25cf");
        QString row = glyph + QStringLiteral(" ") + subs->titleAt(i);
        const QString detail = subs->detailAt(i);
        if (!detail.isEmpty()) {
            row += QStringLiteral(" (") + detail + QStringLiteral(")");
        }
        parts << row;
    }
    m_subagents->setText(QStringLiteral("Subagents:  ") + parts.join(QStringLiteral("   ")));
}

void RootWidget::updateCompletion()
{
    if (m_completionPopup == nullptr) {
        return;
    }
    if (!m_composerSession->completionActive()) {
        m_completionPopup->setVisible(false);
        return;
    }

    CompletionModel* items = m_composerSession->completionItems();
    const int n = items->count();
    if (n == 0) {
        m_completionPopup->setVisible(false);
        return;
    }

    // Hand the model + active row + trigger kind to the custom-painted popup; it
    // renders kind badges, bold labels, muted hints, group headers and the active
    // wash itself.
    m_completionPopup->setData(items, m_composerSession->completionActiveIndex(),
                               m_composerSession->completionKind());

    // Float the overlay just above the composer, spanning its width, sized to the
    // rendered-line count (capped). Geometry is in `right`'s coordinates.
    const QRect composerGeo = m_composer->geometry();
    const int height = qBound(1, m_completionPopup->desiredHeight(), 8);
    int y = composerGeo.y() - height;
    if (y < 0) {
        y = 0;
    }
    m_completionPopup->setGeometry(QRect(composerGeo.x(), y, composerGeo.width(), height));
    m_completionPopup->setVisible(true);
    m_completionPopup->raise();
}
