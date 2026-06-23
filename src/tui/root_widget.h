#pragma once

// Include the concrete Tui headers (their classes live in the inline namespace
// Tui::v0, so forward-declaring them as Tui::Foo would mint a different type).
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>
#include <Tui/ZRoot.h>
#include <Tui/ZShortcut.h>
#include <Tui/ZTextEdit.h>
#include <Tui/ZTextMetrics.h>
#include <Tui/ZWindow.h>

#include "attachment_bar_view.h"
#include "completion_view.h"
#include "composer_chrome.h"
#include "conversation_list_view.h"
#include "interactive_turn_host.h"
#include "line_editor.h"
#include "mouse_terminal.h"
#include "palette_dialog.h"
#include "queue_strip_view.h"
#include "status_bar_view.h"
#include "tab_strip_view.h"
#include "transcript_view.h"

#include "core/agent_ingest.h"
#include "core/document_store.h"
#include "core/transcript_search.h"

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace persistence {
class InMemoryConversationStore;
}

class SidebarModel;
class ConversationsListModel;
class ConversationController;
class ConversationOrchestrator;
class ComposerSessionController;
class TurnController;
class StatusBarModel;
class CommandRegistry;
class TranscriptExporter;
class DisplayRoleAdapter;
class TabModel;

// Per-transcript-tab backend state. Each transcript tab owns an independent
// controller / orchestrator (turn) / document / ingest / mock host, so a tab that
// is streaming keeps growing its own document in the background; the single set of
// views always binds to the active session. Stored by pointer (never moved) so the
// ingest's &doc back-pointer stays valid. Defined in root_widget.cpp.
struct TabSession;

// The composer input: a multiline editor (ZTextEdit) matching the GUI's TextArea.
// Plain Enter submits, Shift+Enter inserts a newline, Ctrl+Enter steers; it grows
// with content up to a row cap, supports readline-style editing (executed against
// the document cursor via the shared lineedit keymap), and drives the shared
// session's draft + completion + reverse-search state.
class SubmitInputBox : public Tui::ZTextEdit {
    Q_OBJECT

public:
    explicit SubmitInputBox(const Tui::ZTextMetrics& metrics, Tui::ZWidget* parent = nullptr);

    // The shared composer session. When set, the input box routes completion
    // navigation (Up/Down/Enter/Tab/Esc while completionActive) to it and refreshes
    // the slash/@ trigger after every key (text/caret change).
    void setSession(ComposerSessionController* session) { m_session = session; }

    // Flat offset into text() (counting embedded '\n') <-> document Position, so the
    // completion FSM and the controller's cursorRequested keep using one linear
    // caret offset across both front ends.
    [[nodiscard]] int linearCursor() const;
    void setLinearCursor(int linear);
    // Place the caret at the end of the document (draftReset's caret-to-end contract).
    void moveCursorToEnd();

signals:
    void submitted(const QString& text);
    // Esc on an empty composer: ask the shell to move focus back to the panes.
    void leaveRequested();
    // Up/Down at the first/last line with the caret idle: walk the shared history.
    void historyPrevious();
    void historyNext();
    // Ctrl+O: add a (mock) attachment, mirroring the GUI's attachment menu.
    void attachRequested();

protected:
    void keyEvent(Tui::ZKeyEvent* event) override;
    // Draw a dim placeholder when the draft is empty (over the base paint, so the
    // editor's own caret still shows).
    void paintEvent(Tui::ZPaintEvent* event) override;
    [[nodiscard]] QSize sizeHint() const override;

private:
    // Push the live text + caret into the shared session (draft + completion
    // trigger). The controller guards against echoing the value back.
    void syncToSession();
    // Re-evaluate the desired height (1..kMaxRows) from the document line count and
    // relayout if it changed.
    void recomputeHeight();
    // Execute a readline EditCommand against the current line via the tested
    // lineedit transform (line-relative motions/kills with kill-ring); line-boundary
    // deletes fall through to the base editor. Returns true if consumed.
    bool applyReadline(lineedit::EditCommand cmd);
    // Route a key to the active reverse-search FSM. Returns true if the key was
    // consumed by the search; false means the FSM accepted the match and the caller
    // should keep processing the key.
    bool handleReverseSearch(Tui::ZKeyEvent* event);

    [[nodiscard]] Tui::ZTextEdit::Position positionForLinear(int linear) const;
    [[nodiscard]] int linearForPosition(Tui::ZTextEdit::Position pos) const;

    ComposerSessionController* m_session = nullptr;
    // Kill buffer for the readline-style edit commands (Ctrl+K/U/W -> Ctrl+Y).
    lineedit::KillRing m_killRing;
    // Current laid-out height in rows (drives sizeHint); grows with the document.
    int m_rows = 1;

    static constexpr int kMaxRows = 6;
};

// A one-line search field above the conversation list. It is NOT a focus stop:
// the conversation list owns focus and forwards typed characters here (type-ahead),
// so this box is a passive display of the live query bound to
// ConversationsListModel::setSearch. When the list is focused it shows a caret
// after the query to mark where keystrokes land; when empty it shows a placeholder.
class SearchInputBox : public Tui::ZInputBox {
    Q_OBJECT

public:
    using Tui::ZInputBox::ZInputBox;

    // Toggle the typing caret (driven by the conversation list's focus state).
    void setTypingActive(bool active);

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;

private:
    bool m_typingActive = false;
};

// The one-line in-transcript find field (Ctrl+F). Live text edits drive the
// query; Enter / Down cycle to the next match, Up / Shift+Enter to the previous,
// Esc closes. Everything else edits the query text (so 'n'/'N' type normally).
class TranscriptSearchBox : public Tui::ZInputBox {
    Q_OBJECT

public:
    using Tui::ZInputBox::ZInputBox;

signals:
    void nextRequested();
    void previousRequested();
    void closeRequested();

protected:
    void keyEvent(Tui::ZKeyEvent* event) override;
};

// ZListView handles Up/Down/Home/End but ignores Left/Right. The sidebar is a
// flattened tree, so map Left/Right to collapse/expand requests (mirroring the
// GUI's Keys.onLeft/RightPressed -> SidebarModel.collapse/expandCurrent).
class TreeListView : public Tui::ZListView {
    Q_OBJECT

public:
    using Tui::ZListView::ZListView;

    // Mouse: act on a click at widget-local point `local`. Selects the row under
    // the cursor (sets the current index); a click on a parent row's disclosure
    // triangle expands/collapses it instead (via the signals below).
    void clickAt(QPoint local);

    // Mouse wheel: move the selection by `delta` rows (negative = up). ZListView
    // scrolls to keep the current index visible, so moving the selection is the
    // public-API way to scroll a flattened tree.
    void scrollByLines(int delta);

signals:
    void collapseRequested();
    void expandRequested();

protected:
    void keyEvent(Tui::ZKeyEvent* event) override;

private:
    // The first visible model row (ZListView's private scroll offset, read via
    // the vendored same-rev ZListViewPrivate). 0 when unavailable.
    [[nodiscard]] int scrollOffset() const;
};

// A small modal "Quit daemon-app?" confirmation. ZDialog auto-centers, handles
// Esc -> reject(), and routes Enter to the default button; we add Quit/Cancel and
// surface the confirmed choice via quitConfirmed().
class QuitDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    explicit QuitDialog(Tui::ZWidget* parent);

signals:
    void quitConfirmed();
};

// A small modal text-input dialog (ZInputBox + OK/Cancel), used for renaming a
// conversation. Seeded with the current title; Enter / OK emits accepted(text).
class TextPromptDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    // `masked` switches the input to password echo (sudo/secret prompts).
    TextPromptDialog(const QString& title, const QString& initial, bool masked,
                     Tui::ZWidget* parent);

signals:
    void submitted(const QString& text);
    // Emitted when the user cancels (Esc / Cancel), so a host gate can abort.
    void canceled();

private:
    Tui::ZInputBox* m_input = nullptr;
};

// A minimal modal Yes/No confirmation (destructive-confirm). Esc/No reject; the
// default focus is the cancelling choice. Surfaces the affirmative via confirmed().
class ConfirmDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    ConfirmDialog(const QString& title, const QString& message, Tui::ZWidget* parent);

signals:
    void confirmed();
};

// The TUI shell: a single full-screen window holding the three-column layout
// (Sidebar | ConversationsList | Conversation), driven entirely by the app's
// existing C++ view models against the in-memory store.
class RootWidget : public Tui::ZRoot {
    Q_OBJECT

public:
    RootWidget();

    void dumpGeometry() const;    // debug helper for the offscreen self-check
    void focusComposer() const;   // offscreen-test helper: focus the input box
    void focusTranscript() const; // offscreen-test helper: focus the transcript

    // Mouse entry point (connected to MouseTerminal::mouseInput). Hit-tests the
    // panes by terminal coordinate and routes a primary-button press to focus +
    // select/open, or a wheel event to scroll the pane under the cursor. No-op for
    // release/move/middle/right (this layer is click + wheel only).
    void handleMouse(QPoint termPos, MouseTerminal::MouseAction action, int button,
                     Qt::KeyboardModifiers modifiers);

protected:
    void terminalChanged() override;
    void resizeEvent(Tui::ZResizeEvent* event) override;
    void keyEvent(Tui::ZKeyEvent* event) override; // Esc fallback -> promptQuit

private:
    void buildUi();
    void wireViews();
    void refreshTranscript();
    // Rewind the active tab to a prior user message: interrupt a live turn, hard-
    // truncate the document at `messageId`, persist, and then either re-run with
    // the message's own text (editMode = false) or seed the composer with it for
    // the user to edit and submit (editMode = true). The single TUI rewind seam:
    // both the picker signals and the /retry,/edit,/undo slash commands funnel
    // through it.
    void rewindActiveTab(const QString& messageId, bool editMode);
    void promptQuit(); // open the quit-confirmation modal (idempotent)
    // Open the model picker overlay (filterable provider->model list) bound to the
    // shared composer session; selecting sets the active model. Opened by /model
    // and the command palette.
    void openModelPicker();
    // Open the command palette (Ctrl+P): a filterable list of nav / theme / mode /
    // slash actions, backed by the shared CommandRegistry, routed to existing
    // handlers on activation.
    void openCommandPalette();
    // Advance Light -> Dark -> Sepia -> Midnight, recolor the whole shell live
    // (stock palette + every custom-painted view), and persist the choice so the
    // GUI and TUI stay in sync.
    void cycleTheme();

    // Live assistant-turn streaming for a given session: route its TurnController's
    // daemon-shaped events through that session's ingest so its document grows real
    // typed blocks; repaint only when the session is the active one.
    void onTurnEvents(TabSession* session, const QVariantList& events);
    // Render the active session's status-stack todos as a compact strip above the
    // composer (cleared when the model empties after the turn settles).
    void updateTodos();
    // Render the active session's live subagent rows as a compact one-line strip
    // above the composer (cleared when the model empties after the turn settles).
    void updateSubagents();
    // Sync the completion overlay (items / active row / visibility / geometry)
    // from the shared controller's completion state.
    void updateCompletion();

    // --- Tabs -----------------------------------------------------------------
    // Transient open (arrow nav / single click): load the conversation into the
    // VSCode-style preview tab (reused on the next preview).
    void previewConversationTab(int conversationId);
    // Deliberate open (Enter / double-activate): a permanent, pinned tab.
    void openConversationPinnedTab(int conversationId);
    // Create a brand-new conversation in the store and open it in a pinned tab (Ctrl+T).
    void newTranscriptTab();
    // Close the active tab (Ctrl+W / tab "x").
    void closeCurrentTab();
    // A preview tab was reassigned to a different conversation: rebind its existing
    // session to `conversationId` (re-open + reload) instead of spawning a new one.
    void rebindSession(int tabId, int conversationId);
    // Lazily create the per-tab session for a transcript tab id (no-op if present
    // or if the tab is a non-transcript page).
    TabSession* ensureSession(int tabId);
    // Make the tab with `tabId` the active one: bind the views to its session (or
    // to the static page document for page tabs) and refresh.
    void activateTab(int tabId);
    // Tear down and delete the session for a closed tab id.
    void destroySession(int tabId);
    // Wire a freshly created session's per-session connections (streaming into its
    // own document, persistence, todos, busy), guarded so background sessions never
    // touch the shared views/status unless they are active.
    void wireSession(TabSession* session);

    // --- In-transcript find (Ctrl+F / /find) ---------------------------------
    // Reveal the find field over the active transcript and focus it; close hides
    // it, clears the active session's query, and returns focus to the transcript.
    // updateSearchCounter refreshes the "n/m" label from the active search engine.
    void openTranscriptSearch();
    void closeTranscriptSearch();
    void updateSearchCounter();

    // Reused, unchanged from the GUI build.
    persistence::InMemoryConversationStore* m_store = nullptr;
    SidebarModel* m_sidebar = nullptr;
    ConversationsListModel* m_list = nullptr;
    // Shared composer FSM (draft/queue/history/submit), identical to the GUI. Its
    // conversationId is switched to the active tab; its intents are dispatched to
    // the active session's orchestrator.
    ComposerSessionController* m_composerSession = nullptr;
    // The shared status-bar model (DaemonApp.StatusModel) - one footer for the app.
    StatusBarModel* m_status = nullptr;

    // The shared tab model both the GUI and TUI bind; the single source of truth
    // for the open tabs and the active one.
    TabModel* m_tabModel = nullptr;
    // Per-transcript-tab backend state, keyed by tab id. Page tabs have no session.
    QHash<int, TabSession*> m_sessions;
    // The active transcript session (nullptr while a page tab is active).
    TabSession* m_active = nullptr;

    // TUI-only glue + widgets.
    DisplayRoleAdapter* m_sidebarAdapter = nullptr;
    Tui::ZWindow* m_window = nullptr;
    TreeListView* m_sidebarView = nullptr;
    // One-line search field above the conversation list (filters via setSearch).
    SearchInputBox* m_search = nullptr;
    ConversationListView* m_listView = nullptr;
    TranscriptView* m_transcript = nullptr;
    // In-transcript find bar (one-line field + match counter), inserted between the
    // tab strip and the transcript; hidden until Ctrl+F / /find. m_searchActive
    // gates the live wiring so a background refresh() never drives the hidden bar.
    Tui::ZWidget* m_searchRow = nullptr;
    TranscriptSearchBox* m_transcriptSearch = nullptr;
    Tui::ZLabel* m_searchCounter = nullptr;
    bool m_searchActive = false;
    // One-line streaming/affordance indicator above the composer (Thinking.../error
    // + send/stop/steer hint), driven by the active session's TurnController.
    ComposerChrome* m_composerChrome = nullptr;
    // Queued-prompt strip (custom-painted) above the composer; 0 height when empty.
    QueueStripView* m_queue = nullptr;
    // Attachment-chip row just above the composer; 0 height when empty.
    AttachmentBarView* m_attachments = nullptr;
    SubmitInputBox* m_composer = nullptr;
    // The pane tab strip (replaces the old single header label).
    TabStripView* m_tabStrip = nullptr;
    // Custom-painted colored status footer (gateway/agents/context/session/version).
    StatusBarView* m_footer = nullptr;
    // Compact status-stack todo strip (above the composer).
    Tui::ZLabel* m_todos = nullptr;
    // Compact status-stack subagent strip (above the composer); blank at rest.
    Tui::ZLabel* m_subagents = nullptr;
    // Borderless completion overlay floated above the composer; driven entirely by
    // the shared controller's completion state (the input box keeps focus).
    CompletionView* m_completionPopup = nullptr;

    // Exit handling: the quit confirmation modal (nullptr when closed).
    QuitDialog* m_quitDialog = nullptr;

    // Filterable overlays (created lazily, reused). The model picker lists the
    // shared composer catalog; the command palette lists the shared CommandRegistry.
    PaletteDialog* m_modelPicker = nullptr;
    PaletteDialog* m_commandPalette = nullptr;
    CommandRegistry* m_commands = nullptr;
    // Transcript exporter for the /save + list "export" action (writes JSON).
    TranscriptExporter* m_exporter = nullptr;

    // Static document backing non-transcript page tabs (e.g. Settings): the
    // transcript view points here while a page tab is active.
    be::DocumentStore m_pageDoc;

    bool m_built = false;
};
