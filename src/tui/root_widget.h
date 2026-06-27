#pragma once

// Include the concrete Tui headers (their classes live in the inline namespace
// Tui::v0, so forward-declaring them as Tui::Foo would mint a different type).
#include "accounts/iaccounts_service.h"
#include "attachment_bar_view.h"
#include "automation/icron_store.h"
#include "automation/irouting_store.h"
#include "code_editor_view.h"
#include "completion_view.h"
#include "composer_chrome.h"
#include "config/idaemon_config.h"
#include "connection/iconnection_service.h"
#include "core/agent_ingest.h"
#include "core/document_store.h"
#include "core/transcript_search.h"
#include "daemon/app_service_graph.h"
#include "file_tree_view.h"
#include "firstrun/first_run_model.h"
#include "fleet/iapprovals_inbox.h"
#include "fleet/idashboard.h"
#include "fleet/ifleet_tree.h"
#include "fleet/isession_roster.h"
#include "interactive_turn_host.h"
#include "line_editor.h"
#include "models/imodel_catalog.h"
#include "mouse_terminal.h"
#include "nav/nav_controller.h"
#include "palette_dialog.h"
#include "profiles/iprofile_store.h"
#include "queue_strip_view.h"
#include "search_input_box.h"
#include "session/icheckpoint_timeline.h"
#include "session/isession_settings.h"
#include "session_list_view.h"
#include "settings/isettings_store.h"
#include "status_bar_view.h"
#include "submit_input_box.h"
#include "tab_strip_view.h"
#include "transcript_view.h"
#include "tree_list_view.h"
#include "tui_dialogs.h"

#include <memory>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>
#include <Tui/ZRoot.h>
#include <Tui/ZShortcut.h>
#include <Tui/ZTextEdit.h>
#include <Tui/ZTextMetrics.h>
#include <Tui/ZWindow.h>

namespace persistence {
class ISessionStore;
}
namespace fs {
class IFsService;
}
namespace files {
class FsExplorerModel;
}
namespace participants {
class ParticipantsModel;
}
namespace editor {
class CodeEditorController;
}
namespace memory {
class IMemoryService;
}
namespace memoryui {
class MemoryListModel;
class MemoryStatsModel;
class MemoryTimelineModel;
class MemoryGraphModel;
} // namespace memoryui

class SidebarModel;
class SessionsListModel;
class SessionController;
class SessionOrchestrator;
class ComposerSessionController;
class TurnController;
class StatusBarModel;
class CommandRegistry;
class TranscriptExporter;
class DisplayRoleAdapter;
class ParticipantsView;
class TabModel;
class TuiFileTabController;
class TuiOverlayHost;
class TuiPageHub;
class TabSessionManager;

// Per-transcript-tab backend state. Each transcript tab owns an independent
// controller / orchestrator (turn) / document / ingest / mock host, so a tab that
// is streaming keeps growing its own document in the background; the single set of
// views always binds to the active session. Stored by pointer (never moved) so the
// ingest's &doc back-pointer stays valid. Defined in root_widget.cpp.
struct TabSession;

// The TUI shell: a single full-screen window holding the three-column layout
// (Sidebar | SessionsList | Session), driven entirely by the app's
// existing C++ view models against the in-memory store.
class RootWidget : public Tui::ZRoot {
    Q_OBJECT

public:
    RootWidget();
    ~RootWidget() override;

    void dumpGeometry() const;    // debug helper for the offscreen self-check
    void focusComposer() const;   // offscreen-test helper: focus the input box
    void focusTranscript() const; // offscreen-test helper: focus the transcript

    // Headless E2E hook (mirrors Application::awaitConnectionReady): spin the event loop until the
    // connection seam reaches "ready" or timeoutMs elapses; returns true iff ready.
    [[nodiscard]] bool awaitConnectionReady(int timeoutMs) const;

    // Headless E2E hook (mirrors Application::driveFirstRunConnect): drive a first-run "Local"
    // connect programmatically, using the resolved target (honors DAEMON_APP_SOCKET). No-op once
    // setup is complete, so it is safe to call before awaitConnectionReady.
    void driveFirstRunConnect() const;

    // Headless E2E hook (mirrors Application::runHeadlessOnboarding): connect, add the API key,
    // pick the first discovered model, and finish - pumping the event loop so the credential/model
    // requests round-trip. Returns true iff the connection reached ready.
    [[nodiscard]] bool runHeadlessOnboarding(const QString& provider, const QString& key,
                                             int timeoutMs) const;

    // Headless E2E hook (CHA-1 / CHA-2): connect, drive one real turn through a DaemonTurnEngine
    // and return the assistant text accumulated from the AgentEvent stream.
    [[nodiscard]] QString runHeadlessChat(const QString& prompt, int timeoutMs) const;

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
    // Route a page-tab kind to the right markdown projection.
    [[nodiscard]] QString pageMarkdownForKind(int kind) const;

    // --- Interactive manager-hub pages ---------------------------------------
    // The kind of the active page tab if it is an interactive manager hub (Models /
    // Accounts / ... ), or -1 (a transcript tab, the Settings/Dashboard read-only
    // pages, or no tab). Drives whether page-action keys are live.
    [[nodiscard]] int activePageKind() const;
    // The actionable rows for an interactive hub kind (the primary list the keys
    // operate on: installed models, accounts, sessions, ...). Empty for read-only.
    [[nodiscard]] QList<QVariantMap> pageActionRows(int kind) const;
    // Move the highlighted row for the active hub by `delta` (clamped) and repaint.
    void movePageSelection(int delta);
    // Re-render the active page document in place (after a seam mutation), keeping
    // the selection clamped to the (possibly shrunk) row count.
    void refreshActivePage();
    // If the active page is `kind`, re-render it (so a seam's changed() / a list
    // model's row churn updates the markdown live).
    void refreshPageIfActive(int kind);
    // Handle a no-modifier key while an interactive hub page is active: j/k move the
    // selection, action keys (Enter/x/a/d/s/t/Space/...) drive the matching seam.
    // Returns true if the key was consumed.
    bool handlePageActionKey(Tui::ZKeyEvent* event);

    // Composer overlays (parity with the GUI popovers), bound to the active tab's
    // session: session settings (profile/effort/fast/verbose) and the
    // checkpoint/rewind timeline.
    void openSessionSettingsOverlay();
    void openCheckpointsOverlay();
    // Routes a command/palette/slash id ("settings", "models", ...) to its
    // singleton page tab. Returns false if the id is not a manager page.
    bool openManagerPage(const QString& id);
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
    // Transient open (arrow nav / single click): load the session into the
    // VSCode-style preview tab (reused on the next preview).
    void previewSessionTab(const QString& sessionId);
    // Deliberate open (Enter / double-activate): a permanent, pinned tab.
    void openSessionPinnedTab(const QString& sessionId);
    // Create a brand-new session in the store and open it in a pinned tab (Ctrl+T).
    void newTranscriptTab();
    // Close the active tab (Ctrl+W / tab "x").
    void closeCurrentTab();
    // A preview tab was reassigned to a different session: rebind its existing
    // session to `sessionId` (re-open + reload) instead of spawning a new one.
    void rebindSession(int tabId, const QString& sessionId);
    // Lazily create the per-tab session for a transcript tab id (no-op if present
    // or if the tab is a non-transcript page).
    TabSession* ensureSession(int tabId);
    // Show the code editor view (File tab) or the transcript+composer stack.
    void showEditor(bool on);
    // Toggle the right-side file Explorer column (Ctrl+E / palette "files"), and
    // persist the choice to the shared "ui/showFileExplorer" setting.
    void toggleExplorer();
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

    // Shared GUI/TUI service graph (store, connection, fs, memory, nav, first-run, config,
    // models, accounts, profiles, fleet, automation, session facades). Read members via
    // m_services.* instead of mirroring them as duplicate pointers; only TUI-local widgets and
    // view-models live as their own members below.
    daemonapp::daemon::AppServiceGraph m_services;
    SidebarModel* m_sidebar = nullptr;
    SessionsListModel* m_list = nullptr;
    // Shared composer FSM (draft/queue/history/submit), identical to the GUI. Its
    // sessionId is switched to the active tab; its intents are dispatched to
    // the active session's orchestrator.
    ComposerSessionController* m_composerSession = nullptr;
    // The shared status-bar model (DaemonApp.StatusModel) - one footer for the app.
    StatusBarModel* m_status = nullptr;

    // The shared tab model both the GUI and TUI bind; the single source of truth
    // for the open tabs and the active one.
    TabModel* m_tabModel = nullptr;
    // Per-transcript-tab backend state, keyed by tab id. Page tabs have no session.
    std::unique_ptr<TabSessionManager> m_tabSessions;
    // The active transcript session (nullptr while a page tab is active).
    TabSession* m_active = nullptr;

    // TUI-only glue + widgets.
    DisplayRoleAdapter* m_sidebarAdapter = nullptr;
    Tui::ZWindow* m_window = nullptr;
    TreeListView* m_sidebarView = nullptr;
    // One-line search field above the session list (filters via setSearch).
    SearchInputBox* m_search = nullptr;
    SessionListView* m_listView = nullptr;
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
    // File tree (right Explorer column, toggled with Ctrl+E) + code editor view
    // (shown in the session column when a File tab is active). Both render the
    // same shared C++ view-models the GUI binds.
    files::FsExplorerModel* m_fileTree = nullptr;
    FileTreeView* m_fileTreeView = nullptr;
    // Right-sidebar Participants section (above the Explorer, toggled as one column).
    participants::ParticipantsModel* m_participants = nullptr;
    Tui::ZWidget* m_rightColumn = nullptr;
    ParticipantsView* m_participantsView = nullptr;
    CodeEditorView* m_editorView = nullptr;
    Tui::ZLabel* m_fileStatus = nullptr;
    // Per-File-tab editor controllers and async fs resolution.
    std::unique_ptr<TuiFileTabController> m_fileTabs;
    // Custom-painted colored status footer (gateway/agents/context/session/version).
    StatusBarView* m_footer = nullptr;
    // Compact status-stack todo strip (above the composer).
    Tui::ZLabel* m_todos = nullptr;
    // Compact status-stack subagent strip (above the composer); blank at rest.
    Tui::ZLabel* m_subagents = nullptr;
    // Borderless completion overlay floated above the composer; driven entirely by
    // the shared controller's completion state (the input box keeps focus).
    CompletionView* m_completionPopup = nullptr;

    // Filterable overlays and modal host (quit/model picker/command palette).
    std::unique_ptr<TuiOverlayHost> m_overlays;
    CommandRegistry* m_commands = nullptr;
    // Transcript exporter for the /save + list "export" action (writes JSON).
    TranscriptExporter* m_exporter = nullptr;

    // Phase 0 shared seams now live in m_services (prefs, connection liveness, nav, first-run,
    // config, models, accounts, profiles, fleet, automation, session overrides/checkpoints,
    // memory). The TUI keeps only the view-models it composes on top of them.
    // Memory-page view-models (same models the GUI binds; the TUI renders them as markdown + a
    // graph adjacency listing) backed by m_services.memory.
    memoryui::MemoryListModel* m_memList = nullptr;
    memoryui::MemoryStatsModel* m_memStats = nullptr;
    memoryui::MemoryTimelineModel* m_memTimeline = nullptr;
    memoryui::MemoryGraphModel* m_memGraph = nullptr;

    // Manager-page projection + keyboard actions for seam-backed hub tabs.
    std::unique_ptr<TuiPageHub> m_pageHub;

    // Static document backing non-transcript page tabs (e.g. Settings): the
    // transcript view points here while a page tab is active.
    be::DocumentStore m_pageDoc;

    bool m_built = false;
};
