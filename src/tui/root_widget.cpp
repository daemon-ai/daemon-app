#include "root_widget.h"

#include "display_role_adapter.h"
#include "tui_file_tab_controller.h"
#include "tui_overlay_host.h"
#include "tui_page_hub.h"
#include "tui_palette.h"
#include "tui_shell_layout.h"
#include "tab_session_manager.h"

#include "session_controller.h"
#include "session_orchestrator.h"
#include "sessions_list_model.h"
#include "sidebar_model.h"
#include "todo_list_model.h"

#include "daemonnet/idaemonnet.h" // complete type for setDaemonNet(QObject*)

#include "composer_session_controller.h"
#include "command_registry.h"
#include "status_bar_model.h"
#include "transcript_exporter.h"
#include "tab_model.h"
#include "turn_controller.h"

#include "app/transcript_log.h"

#include "fs_explorer_model.h"
#include "app/code_editor_controller.h"
#include "fs/ifs_service.h"

#include "memory_graph_model.h"
#include "memory/imemory_service.h"
#include "memory_list_model.h"
#include "memory_stats_model.h"
#include "memory_timeline_model.h"
#include "uimodels/variant_list_model.h"
#include "persistence/isession_store.h"

#include <Tui/ZButton.h>
#include <Tui/ZCommon.h>
#include <Tui/ZDialog.h>
#include <Tui/ZEvent.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>
#include <Tui/ZRoot.h>
#include <Tui/ZShortcut.h>
#include <Tui/ZTerminal.h>

#include <cstdio>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWidget.h>
#include <Tui/ZWindow.h>

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QEventLoop>
#include <QItemSelectionModel>
#include <QProcess>
#include <QTimer>
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

namespace {

// A short tab title for a session: the first non-empty content line (heading
// markers stripped, capped), falling back to a generic label.
QString titleForContent(const QString& markdown)
{
    const QString trimmed = markdown.trimmed();
    if (trimmed.isEmpty()) {
        return QObject::tr("New session");
    }
    QString first = trimmed.section(QLatin1Char('\n'), 0, 0);
    while (first.startsWith(QLatin1Char('#'))) {
        first.remove(0, 1);
    }
    first = first.trimmed();
    if (first.isEmpty()) {
        return QObject::tr("Session");
    }
    return first.left(24);
}

// The static markdown shown for a (non-transcript) page tab.
QString pageMarkdown(int kind)
{
    if (kind == TabModel::Settings) {
        return QObject::tr(
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

RootWidget::RootWidget()
    : m_services(daemonapp::daemon::createAppServiceGraph(
          daemonapp::daemon::serviceModeFromEnvironment(), this))
{
    // Build the stock-widget palette (incl. the quit dialog frame/body) from the
    // active theme - set from the persisted ui/theme in main() before we run.
    setPalette(daemonPalette(tpal::activeTheme()));

    // The reused layer: store + view models, wired exactly as in the GUI. None
    // of this depends on Tui Widgets - the same objects back the QML frontend.
    m_sidebar = new SidebarModel(this);
    m_sidebar->setStore(m_services.store);
    // The same co-equal Integrations section as the GUI (events-IO axis).
    m_sidebar->setDaemonNet(m_services.daemonNet);

    m_list = new SessionsListModel(this);
    m_list->setStore(m_services.store);

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
    m_overlays = std::make_unique<TuiOverlayHost>(this);

    // The shared pane-tab model (the same C++ class the QML TabBar binds). It is
    // the single source of truth for the open tabs and the active one; the TUI
    // creates a per-tab TabSession on demand and binds the views to the active one.
    m_tabModel = new TabModel(this);
    m_tabSessions = std::make_unique<TabSessionManager>(m_services.store, m_tabModel);
    connect(m_tabModel, &TabModel::currentTabChanged, this, [this](int tabId) {
        if (tabId >= 0) {
            activateTab(tabId);
        }
    });
    connect(m_tabModel, &TabModel::tabClosed, this, &RootWidget::destroySession);
    // A preview tab was reassigned to another session: rebind its session
    // in place rather than spawning a new one.
    connect(m_tabModel, &TabModel::tabSessionChanged, this,
            [this](int tabId, const QString& sessionId) { rebindSession(tabId, sessionId); });
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
    // An Integrations-tree session leaf opens its transcript in a pinned tab.
    connect(m_sidebar, &SidebarModel::sessionActivated, this,
            [this](const QString& sessionId) { openSessionPinnedTab(sessionId); });

    m_fileTree = new files::FsExplorerModel(this);
    m_fileTree->setService(m_services.fs);
    m_fileTabs = std::make_unique<TuiFileTabController>(m_services.fs, m_tabModel, this);

    // Phase 0 shared seams (identical classes to the GUI). The connection seam
    // owns liveness; mirror its state into the footer's gateway indicator, then
    // open the saved (or default local) connection so the state machine runs.
    // Single source of truth for the composer's model list/selection (shared with
    // the Models hub), exactly as the GUI wires it via Composer.qml's modelSource.
    m_composerSession->setModelSource(m_services.modelCatalog);

    // Memory-inspection seam (seeded mock) + the shared view-models. Setting the
    // service kicks off the initial async requests; the page markdown re-renders
    // when results land (see the liveRefresh wiring in wireViews).
    m_memList = new memoryui::MemoryListModel(this);
    m_memStats = new memoryui::MemoryStatsModel(this);
    m_memTimeline = new memoryui::MemoryTimelineModel(this);
    m_memGraph = new memoryui::MemoryGraphModel(this);
    m_memList->setService(m_services.memory);
    m_memStats->setService(m_services.memory);
    m_memTimeline->setService(m_services.memory);
    m_memGraph->setService(m_services.memory);

    m_pageHub = std::make_unique<TuiPageHub>(TuiPageHub::Dependencies{
        m_tabModel,
        m_services.daemonConfig,
        m_services.connection,
        m_services.modelCatalog,
        m_services.accounts,
        m_services.profiles,
        m_services.roster,
        m_services.fleetTree,
        m_services.approvals,
        m_services.dashboard,
        m_services.routing,
        m_services.cron,
        m_services.daemonNet,
        m_services.memory,
        m_memList,
        m_memStats,
        m_memTimeline,
        m_memGraph,
    });

    // Wire the app-level navigation seam (constructed-but-unused until now): an
    // open() from anywhere (slash, palette, a future cog menu) raises the matching
    // manager page tab, exactly like the GUI mounts the page overlay.
    connect(m_services.nav, &nav::NavController::openRequested, this,
            [this](const QString& page, const QString&) { openManagerPage(page); });
    // Per-agent (ProfileRef-keyed) Memory / Profile tabs.
    connect(m_services.nav, &nav::NavController::openAgentRequested, this,
            [this](const QString& kind, const QString& profileRef, const QString& title) {
                if (m_tabModel == nullptr || profileRef.isEmpty())
                    return;
                const int tabKind = kind == QStringLiteral("profile") ? TabModel::Profile
                                                                       : TabModel::Memory;
                const QString base = tabKind == TabModel::Profile ? tr("Profile")
                                                                  : tr("Memory");
                const QString label = title.isEmpty()
                    ? base
                    : base + QStringLiteral(" \u00b7 ") + title;
                m_tabModel->openAgentTab(tabKind, profileRef, label);
            });

    connect(m_services.connection, &connection::IConnectionService::stateChanged, this,
            [this] { m_status->setGatewayState(m_services.connection->state()); });
    m_services.firstRun->begin();
    // Returning users auto-open the saved connection; on first launch the gate's
    // connection picker drives connectTo instead.
    if (m_services.settings->setupComplete()) {
        m_services.connection->connectTo(m_services.settings->lastConnectionMode(),
                                         m_services.settings->resolvedConnectionTarget());
    }
}

RootWidget::~RootWidget() = default;

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
    if (m_services.firstRun != nullptr && m_services.firstRun->active()) {
        auto* gate = new FirstRunDialog(m_services.firstRun, m_services.connection, m_services.settings,
                                        m_services.settings->resolvedConnectionTarget(),
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
        && m_sidebar != nullptr && m_services.nav != nullptr
        && (event->text() == QStringLiteral("p") || event->text() == QStringLiteral("m"))) {
        const int row = m_sidebar->currentRow();
        if (row >= 0) {
            const QModelIndex idx = m_sidebar->index(row);
            const QString profile = m_sidebar->data(idx, SidebarModel::ProfileRole).toString();
            const QString label = m_sidebar->data(idx, SidebarModel::LabelRole).toString();
            if (!profile.isEmpty()) {
                m_services.nav->openAgent(event->text() == QStringLiteral("p") ? QStringLiteral("profile")
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
    if (m_overlays != nullptr && m_overlays->quitDialog() != nullptr) {
        const QList<Tui::ZButton*> buttons = m_overlays->quitDialog()->findChildren<Tui::ZButton*>();
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
            [this] { m_services.nav->open(QStringLiteral("settings")); });

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

    const TuiShellWidgets shell = TuiShellLayout::build(
        this, terminal(), QRect(QPoint(0, 0), geometry().size()), m_tabModel, m_fileTree,
        &m_pageDoc);
    m_window = shell.window;
    m_sidebarView = shell.sidebarView;
    m_search = shell.search;
    m_listView = shell.listView;
    m_tabStrip = shell.tabStrip;
    m_searchRow = shell.searchRow;
    m_transcriptSearch = shell.transcriptSearch;
    m_searchCounter = shell.searchCounter;
    m_transcript = shell.transcript;
    m_editorView = shell.editorView;
    m_fileStatus = shell.fileStatus;
    m_composerChrome = shell.composerChrome;
    m_queue = shell.queue;
    m_subagents = shell.subagents;
    m_todos = shell.todos;
    m_attachments = shell.attachments;
    m_composer = shell.composer;
    m_completionPopup = shell.completionPopup;
    m_fileTreeView = shell.fileTreeView;
    m_footer = shell.footer;

    connect(m_tabStrip, &TabStripView::newTabRequested, this, &RootWidget::newTranscriptTab);
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
    connect(m_editorView, &CodeEditorView::saveRequested, this, [this] {
        if (m_fileTabs != nullptr)
            m_fileTabs->saveActive(m_editorView);
    });
    if (m_fileTabs != nullptr)
        m_fileTabs->setStatusLabel(m_fileStatus);
    // Restore the persisted open/closed state (shared "ui/showFileExplorer" key).
    m_fileTreeView->setVisible(
        QSettings().value(QStringLiteral("ui/showFileExplorer"), false).toBool());
    connect(m_fileTreeView, &FileTreeView::fileChosen, this,
            [this](const QString& rootId, const QString& path, bool pinned) {
                const int slash = static_cast<int>(path.lastIndexOf(QLatin1Char('/')));
                const QString title = slash >= 0 ? path.mid(slash + 1) : path;
                if (pinned)
                    m_tabModel->openFilePinned(rootId, path, title);
                else
                    m_tabModel->previewFile(rootId, path, title);
            });
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
    liveRefresh(m_services.modelCatalog->installed(), TabModel::Models);
    liveRefresh(m_services.accounts->accounts(), TabModel::Accounts);
    liveRefresh(m_services.profiles->profiles(), TabModel::Profiles);
    liveRefresh(m_services.roster->sessions(), TabModel::Sessions);
    liveRefresh(m_services.fleetTree->nodes(), TabModel::Fleet);
    liveRefresh(m_services.approvals->pending(), TabModel::Approvals);
    liveRefresh(m_services.routing->rules(), TabModel::Routing);
    liveRefresh(m_services.cron->jobs(), TabModel::Cron);
    // The Dashboard is a read-only projection of the roster / approvals, so refresh
    // it when either of those churns too.
    liveRefresh(m_services.roster->sessions(), TabModel::Dashboard);
    liveRefresh(m_services.approvals->pending(), TabModel::Dashboard);
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
    // Section headers (Fleet/Tags/Integrations) fold/unfold by explicit row.
    connect(m_sidebarView, &TreeListView::toggleRowRequested, this,
            [this, syncSidebarCurrent](int row) {
                m_sidebar->toggleExpand(row);
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
        const QString id = m_list->idAt(row);
        if (id.isEmpty()) {
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
        const QString id = m_list->idAt(row);
        if (!id.isEmpty()) {
            m_services.store->setPinned(id, !m_services.store->isPinned(id));
        }
    });
    connect(m_listView, &SessionListView::deleteRequested, this, [this](int row) {
        const QString id = m_list->idAt(row);
        if (id.isEmpty()) {
            return;
        }
        auto* confirm = new ConfirmDialog(
            tr("Delete session"),
            tr("Permanently delete this session?"), this);
        connect(confirm, &ConfirmDialog::confirmed, this,
                [this, id] { m_services.store->deleteSession(id); });
    });
    connect(m_listView, &SessionListView::exportRequested, this, [this](int row) {
        const QString id = m_list->idAt(row);
        if (id.isEmpty()) {
            return;
        }
        QString name = m_services.store->title(id);
        if (name.isEmpty()) {
            name = QStringLiteral("session");
        }
        const QString path = QDir(QDir::homePath()).filePath(name + QStringLiteral(".json"));
        m_exporter->exportToPath(m_services.store, id, path);
    });
    connect(m_listView, &SessionListView::renameRequested, this, [this](int row) {
        const QString id = m_list->idAt(row);
        if (id.isEmpty()) {
            return;
        }
        auto* dialog = new TextPromptDialog(tr("Rename session"),
                                            m_services.store->title(id), /*masked=*/false, this);
        connect(dialog, &TextPromptDialog::submitted, this, [this, id](const QString& text) {
            if (!text.trimmed().isEmpty()) {
                m_services.store->renameSession(id, text.trimmed());
            }
        });
    });
    connect(m_listView, &SessionListView::moveRequested, this, [this](int row, int delta) {
        const QString id = m_list->idAt(row);
        if (!id.isEmpty()) {
            m_services.store->moveSession(id, delta);
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
                    const QString id = m_active->sessionId;
                    auto* dialog = new TextPromptDialog(tr("Rename session"),
                                                        m_services.store->title(id), /*masked=*/false, this);
                    connect(dialog, &TextPromptDialog::submitted, this,
                            [this, id](const QString& text) {
                                if (!text.trimmed().isEmpty()) {
                                    m_services.store->renameSession(id, text.trimmed());
                                }
                            });
                    return;
                }
                if (command == QStringLiteral("save")) {
                    if (m_active == nullptr || !m_active->controller->hasSession()) {
                        return;
                    }
                    const QString id = m_active->sessionId;
                    QString name = m_services.store->title(id);
                    if (name.isEmpty()) {
                        name = QStringLiteral("session");
                    }
                    const QString path =
                        QDir(QDir::homePath()).filePath(name + QStringLiteral(".json"));
                    m_exporter->exportToPath(m_services.store, id, path);
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
                        tr("Clear session"),
                        tr("Remove all messages from this session?"), this);
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

void RootWidget::previewSessionTab(const QString& sessionId)
{
    if (m_tabModel == nullptr) {
        return;
    }
    QString title = m_services.store->title(sessionId);
    if (title.isEmpty()) {
        title = titleForContent(m_services.store->content(sessionId));
    }
    m_tabModel->previewTranscript(sessionId, title);
}

void RootWidget::openSessionPinnedTab(const QString& sessionId)
{
    if (m_tabModel == nullptr) {
        return;
    }
    QString title = m_services.store->title(sessionId);
    if (title.isEmpty()) {
        title = titleForContent(m_services.store->content(sessionId));
    }
    m_tabModel->openTranscriptPinned(sessionId, title);
}

void RootWidget::newTranscriptTab()
{
    if (m_services.store == nullptr || m_tabModel == nullptr) {
        return;
    }
    const QString id = m_services.store->newSessionId(domain::UnitId());
    m_tabModel->openTranscriptPinned(id, tr("New session"));
    // A new tab is a natural place to start typing.
    if (m_composer != nullptr) {
        m_composer->setFocus();
    }
}

void RootWidget::rebindSession(int tabId, const QString& sessionId)
{
    if (m_tabSessions == nullptr) {
        return;
    }
    const bool wasActive = m_active != nullptr && m_active->tabId == tabId
        && m_active->sessionId != sessionId;
    m_tabSessions->rebindSession(tabId, sessionId, [this](TabSession* s) { wireSession(s); });
    if (wasActive) {
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
    if (m_tabSessions == nullptr) {
        return nullptr;
    }
    return m_tabSessions->ensureSession(tabId, [this](TabSession* s) { wireSession(s); });
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
            if (m_services.settings != nullptr
                && m_services.settings->value(QStringLiteral("notify/turnDone"), false).toBool()) {
                const QString convTitle = s->controller->hasSession()
                    ? m_services.store->title(s->sessionId)
                    : QStringLiteral("daemon");
                emitDesktopNotification(convTitle, tr("The turn finished."));
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
        if (m_services.settings != nullptr
            && !m_services.settings->value(QStringLiteral("notify/gates"), true).toBool()) {
            return;
        }
        std::fputs("\a", stdout); // BEL: most terminals raise an urgency hint
        std::fflush(stdout);
        const QString what = kind == QStringLiteral("approval") ? tr("approval")
                                                                : tr("credential");
        if (terminal() != nullptr) {
            terminal()->setTitle(tr("\u25cf daemon \u2014 needs %1").arg(what));
        }
        const QString convTitle = (m_active != nullptr && m_active->controller->hasSession())
            ? m_services.store->title(m_active->sessionId)
            : QStringLiteral("daemon");
        emitDesktopNotification(convTitle, tr("The turn needs your %1.").arg(what));
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
                    ? (kind == QStringLiteral("secret") ? tr("Secret required")
                                                        : tr("Password required"))
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
        editor::CodeEditorController* c =
            m_fileTabs != nullptr ? m_fileTabs->ensureFileSession(tabId) : nullptr;
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
        if (kind == TabModel::Memory && m_services.memory != nullptr) {
            m_services.memory->setScope(m_tabModel->agentRefAt(row), QString(), true);
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
    if (m_pageHub == nullptr) {
        return pageMarkdown(kind);
    }
    const QString profileRef =
        m_tabModel != nullptr ? m_tabModel->agentRefAt(m_tabModel->currentIndex()) : QString();
    const QString markdown = m_pageHub->pageMarkdownForKind(kind, profileRef);
    return markdown.isEmpty() ? pageMarkdown(kind) : markdown;
}

bool RootWidget::openManagerPage(const QString& id)
{
    return m_pageHub != nullptr && m_pageHub->openManagerPage(id);
}

int RootWidget::activePageKind() const
{
    return m_pageHub != nullptr ? m_pageHub->activePageKind(m_active != nullptr) : -1;
}

QList<QVariantMap> RootWidget::pageActionRows(int kind) const
{
    return m_pageHub != nullptr ? m_pageHub->pageActionRows(kind) : QList<QVariantMap>{};
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
    m_pageHub->clampSelection(kind);
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
    m_pageHub->moveSelection(kind, delta);
    refreshActivePage();
}

bool RootWidget::handlePageActionKey(Tui::ZKeyEvent* event)
{
    const int kind = activePageKind();
    if (kind < 0 || m_pageHub == nullptr) {
        return false;
    }
    if (m_pageHub->handlePageActionKey(kind, event)) {
        refreshActivePage();
        return true;
    }
    return false;
}

void RootWidget::openSessionSettingsOverlay()
{
    // Only meaningful over a transcript tab; bind the per-session overrides to
    // the focused chat first (parity with ComposerControls setting sessionId).
    if (m_active == nullptr || m_services.sessionSettings == nullptr) {
        return;
    }
    m_services.sessionSettings->setSessionId(m_active->sessionId);
    session::ISessionSettings* ss = m_services.sessionSettings;

    auto* dlg = new Tui::ZDialog(this);
    dlg->setOptions(Tui::ZWindow::DeleteOnClose);
    dlg->setWindowTitle(tr("Session settings"));
    dlg->setContentsMargins({ 2, 1, 2, 1 });
    auto* layout = new Tui::ZVBoxLayout();
    dlg->setLayout(layout);

    auto* hint = new Tui::ZLabel(
        tr("Enter / Space on a row cycles or toggles it · Esc closes"), dlg);
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
    auto* closeBtn = new Tui::ZButton(tr("Close"), dlg);
    closeBtn->setDefault(true);
    buttons->addWidget(closeBtn);

    const auto sync = [ss, profileBtn, effortBtn, fastBtn, verboseBtn] {
        profileBtn->setText(tr("Profile: %1").arg(ss->profile()));
        effortBtn->setText(tr("Effort:  %1").arg(ss->effort()));
        fastBtn->setText(tr("Fast:    %1")
                             .arg(ss->fast() ? tr("on") : tr("off")));
        verboseBtn->setText(tr("Verbose: %1")
                                .arg(ss->verbose() ? tr("on")
                                                   : tr("off")));
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
        const QStringList names = m_services.profiles->profileNames();
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
    if (m_active == nullptr || m_services.checkpoints == nullptr) {
        return;
    }
    m_services.checkpoints->setSessionId(m_active->sessionId);
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_services.checkpoints->checkpoints());
    const QList<QVariantMap> rows = model != nullptr ? model->rows() : QList<QVariantMap>{};

    auto* dlg = new Tui::ZDialog(this);
    dlg->setOptions(Tui::ZWindow::DeleteOnClose);
    dlg->setWindowTitle(tr("Checkpoints"));
    dlg->setContentsMargins({ 2, 1, 2, 1 });
    auto* layout = new Tui::ZVBoxLayout();
    dlg->setLayout(layout);

    auto* hint = new Tui::ZLabel(
        tr("Enter restores the selected checkpoint · Esc closes"), dlg);
    layout->addWidget(hint);

    auto* list = new Tui::ZListView(dlg);
    QStringList display;
    QStringList ids;
    for (const QVariantMap& c : rows) {
        ids << c.value(QStringLiteral("id")).toString();
        display << tr("%1  ·  %2  ·  %3 tok%4")
                       .arg(c.value(QStringLiteral("label")).toString(),
                            c.value(QStringLiteral("time")).toString(),
                            c.value(QStringLiteral("tokens")).toString(),
                            c.value(QStringLiteral("current")).toBool()
                                ? tr("  (current)")
                                : QString());
    }
    if (display.isEmpty()) {
        display << tr("(no checkpoints)");
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
    auto* restoreBtn = new Tui::ZButton(tr("Restore"), dlg);
    buttons->addWidget(restoreBtn);
    auto* closeBtn = new Tui::ZButton(tr("Close"), dlg);
    closeBtn->setDefault(true);
    buttons->addWidget(closeBtn);

    const auto doRestore = [this, dlg, list, ids] {
        const int row = list->currentIndex().row();
        if (row >= 0 && row < ids.size()) {
            m_services.checkpoints->restore(ids.at(row));
        }
        dlg->close();
    };
    connect(restoreBtn, &Tui::ZButton::clicked, dlg, doRestore);
    connect(list, &Tui::ZListView::enterPressed, dlg, [doRestore](int) { doRestore(); });
    connect(closeBtn, &Tui::ZButton::clicked, dlg, &Tui::ZDialog::close);

    dlg->setGeometry(QRect(0, 0, 62, qBound(9, static_cast<int>(rows.size()) + 8, 20)));
    list->setFocus();
}

void RootWidget::destroySession(int tabId)
{
    // File tabs hold an editor controller rather than a TabSession.
    if (m_fileTabs != nullptr && m_fileTabs->destroySession(tabId, m_editorView)) {
        return;
    }

    if (m_tabSessions == nullptr) {
        return;
    }
    m_tabSessions->destroySession(tabId, m_active, [this] {
        if (m_transcript != nullptr) {
            m_transcript->setDocument(&m_pageDoc);
        }
    });
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

bool RootWidget::awaitConnectionReady(int timeoutMs)
{
    auto* conn = m_services.connection;
    if (conn == nullptr) {
        return false;
    }
    if (conn->ready()) {
        return true;
    }
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(conn, &connection::IConnectionService::stateChanged, &loop, [&] {
        if (conn->ready()) {
            loop.quit();
        }
    });
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(timeoutMs);
    loop.exec();
    return conn->ready();
}

void RootWidget::promptQuit()
{
    if (m_overlays == nullptr) {
        return;
    }
    m_overlays->promptQuit([this] {
        if (m_sidebarView != nullptr) {
            m_sidebarView->setFocus(); // restore keyboard focus to the panes
        }
    });
}

void RootWidget::openModelPicker()
{
    if (m_overlays == nullptr) {
        return;
    }
    m_overlays->openModelPicker(m_composerSession, [this] {
        if (m_composer != nullptr)
            m_composer->setFocus();
    });
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
    if (m_overlays == nullptr) {
        return;
    }
    TuiOverlayHost::CommandCallbacks callbacks;
    callbacks.openManagerPage = [this](const QString& id) { return openManagerPage(id); };
    callbacks.newTranscriptTab = [this] { newTranscriptTab(); };
    callbacks.cycleTheme = [this] { cycleTheme(); };
    callbacks.openModelPicker = [this] { openModelPicker(); };
    callbacks.focusSearch = [this] {
        if (m_search != nullptr)
            m_listView->setFocus();
    };
    callbacks.toggleExplorer = [this] { toggleExplorer(); };
    callbacks.openTranscriptSearch = [this] { openTranscriptSearch(); };
    callbacks.invokeActiveCommand = [this](const QString& id) {
        if (id == QStringLiteral("reasoning")) {
            m_composerSession->cycleReasoningEffort();
        } else if (id == QStringLiteral("fast")) {
            m_composerSession->toggleFastMode();
        } else if (id == QStringLiteral("verbose")) {
            m_composerSession->toggleVerbose();
        } else if (m_active != nullptr) {
            m_composerSession->invokeCommand(id);
        }
    };
    callbacks.focusComposer = [this] {
        if (m_composer != nullptr)
            m_composer->setFocus();
    };
    m_overlays->openCommandPalette(m_commands, callbacks);
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
    if (m_fileTabs != nullptr)
        m_fileTabs->setDarkTheme(dark);

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
    // Drive the document from the session's SessionLogEntry sequence (decomposed from
    // its stored markdown) rather than the markdown blob (roadmap P4); same render path
    // a daemon adapter feeding decoded log pages will use.
    const QString md
        = m_active->controller->hasSession() ? m_active->controller->content() : QString();
    be::applyTranscriptLog(m_active->doc, be::decomposeMarkdown(md));
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
    m_todos->setText(tr("Tasks:  ") + parts.join(QStringLiteral("   ")));
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
    m_subagents->setText(tr("Subagents:  ") + parts.join(QStringLiteral("   ")));
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
