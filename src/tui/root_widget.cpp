// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "root_widget.h"

#include "app/code_editor_controller.h"
#include "app/transcript_log.h"
#include "command_registry.h"
#include "composer_session_controller.h"
#include "daemon/daemon_connection_service.h" // complete type for the managed-daemon shutdown hook
#include "daemonnet/idaemonnet.h"             // complete type for setDaemonNet(QObject*)
#include "dialogs/first_run_dialog.h"
#include "display_role_adapter.h"
#include "fs/ifs_service.h"
#include "fs_explorer_model.h"
#include "memory/imemory_service.h"
#include "memory_graph_model.h"
#include "memory_list_model.h"
#include "memory_stats_model.h"
#include "memory_timeline_model.h"
#include "participants_model.h"
#include "participants_view.h"
#include "persistence/isession_store.h"
#include "session_controller.h"
#include "session_orchestrator.h"
#include "sessions_list_model.h"
#include "settings_editor.h"
#include "sidebar_model.h"
#include "status_bar_model.h"
#include "tab_model.h"
#include "tab_session_manager.h"
#include "todo_list_model.h"
#include "transcript_exporter.h"
#include "tui_file_tab_controller.h"
#include "tui_overlay_host.h"
#include "tui_page_hub.h"
#include "tui_palette.h"
#include "tui_shell_layout.h"
#include "turn_controller.h"
#include "turn_engine_factory.h"
#include "uimodels/variant_list_model.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QItemSelectionModel>
#include <QProcess>
#include <QRect>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#include <QVariantMap>
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
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWidget.h>
#include <Tui/ZWindow.h>

// The RootWidget implementation is split across cohesive translation units to keep
// each file focused (and to lower its complexity / hotspot weight):
//   root_widget.cpp           - construction/teardown, terminalChanged, resize, buildUi,
//                               refreshTranscript
//   root_widget_wiring.cpp    - wireViews + wireSession
//   root_widget_input.cpp     - keyEvent + handleMouse + handlePageActionKey
//   root_widget_tabs.cpp      - tab lifecycle (preview/open/new/rebind/close/ensure/activate/
//                               destroy), showEditor, toggleExplorer
//   root_widget_pages.cpp     - manager-hub page projection + overlays + cycleTheme
//   root_widget_strips.cpp    - transcript find + status strips + turn-event ingest + rewind
//   root_widget_headless.cpp  - offscreen/headless E2E hooks + promptQuit
//   root_widget_detail.{h,cpp}- shared free helpers (rwdetail::*)

RootWidget::RootWidget()
    : m_services(daemonapp::daemon::createAppServiceGraph(
          daemonapp::daemon::serviceModeFromEnvironment(), this)) {
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
    // The per-session profile drives the turn (#6b): give every orchestrator the shared override
    // seam + profile store (both modes), matching the GUI TranscriptPage bindings.
    m_tabSessions->setSessionSettings(m_services.sessionSettings);
    m_tabSessions->setProfileStore(m_services.profiles);
    // In daemon mode every session's orchestrator drives a real Submit + Subscribe turn; in mock
    // mode it keeps its default canned simulator. Detect daemon mode the same way the service graph
    // does (the connection seam is the DaemonConnectionService).
    if (qobject_cast<daemonapp::daemon::DaemonConnectionService*>(m_services.connection) !=
        nullptr) {
        m_tabSessions->setTurnEngines(new DaemonTurnEngineFactory(
            m_services.nodeApi, m_services.cache, m_services.subscriptions, this));
    }
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
    connect(m_tabModel, &TabModel::tabKindChanged, this,
            [this](int tabId) { destroySession(tabId); });
    connect(m_tabModel, &TabModel::tabFileChanged, this,
            [this](int tabId, const QString&, const QString&) {
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

    // The right sidebar's Participants section: the same shared model the GUI binds.
    m_participants = new participants::ParticipantsModel(this);
    m_participants->setStore(m_services.store);

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
        m_services.settings,
    });

    // Settings-page dialog edits (theme/language pickers, text prompts, zen):
    // the hub flips plain toggles itself; rows needing a dialog parent or a live
    // re-apply route here with the RootWidget hooks.
    m_settingsEditor =
        std::make_unique<TuiSettingsEditor>(m_pageHub.get(), this,
                                            TuiSettingsEditor::Hooks{
                                                [this](theme::ThemeName name) { applyTheme(name); },
                                                [this] { toggleDistractionFree(); },
                                                [this] { refreshActivePage(); },
                                                [this] {
                                                    if (m_transcript != nullptr) {
                                                        m_transcript->setFocus();
                                                    }
                                                },
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
                const int tabKind =
                    kind == QStringLiteral("profile") ? TabModel::Profile : TabModel::Memory;
                const QString base = tabKind == TabModel::Profile ? tr("Profile") : tr("Memory");
                const QString label =
                    title.isEmpty() ? base : base + QStringLiteral(" \u00b7 ") + title;
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

RootWidget::~RootWidget() {
    // Stop a daemon this client spawned if the user opted into shutdown-on-exit (persistent
    // otherwise). A daemon the client only attached to is never affected.
    if (auto* daemonConnection =
            qobject_cast<daemonapp::daemon::DaemonConnectionService*>(m_services.connection)) {
        daemonConnection->shutdownManagedDaemon();
    }
}

void RootWidget::terminalChanged() {
    if (m_built || terminal() == nullptr) {
        return;
    }
    m_built = true;
    buildUi();
    wireViews();

    // First-run gate (parity with the GUI): on first launch, raise the lighter
    // "Setup Required" modal over the shell until setup completes.
    if (m_services.firstRun != nullptr && m_services.firstRun->active()) {
        auto* gate = new FirstRunDialog(m_services.firstRun, m_services.connection,
                                        m_services.settings, m_services.providerCatalog,
                                        m_services.settings->resolvedConnectionTarget(), this);
        // Local "Discover More Models" in the gate opens the shared model-download flow.
        connect(gate, &FirstRunDialog::modelDiscoverRequested, this,
                &RootWidget::openModelDownload);
        gate->setFocus();
    }
}

void RootWidget::resizeEvent(Tui::ZResizeEvent* event) {
    Tui::ZRoot::resizeEvent(event);
    if (m_window != nullptr) {
        m_window->setGeometry(QRect(QPoint(0, 0), geometry().size()));
    }
}

void RootWidget::buildUi() {
    // Exit affordances. Ctrl+Q opens a confirmation modal; Ctrl+C is the
    // terminal-convention hard exit (no prompt). Esc is NOT a global shortcut:
    // it is handled contextually by the focused widget and only bubbles up to
    // RootWidget::keyEvent (-> promptQuit) when a pane leaves it unhandled.
    auto* quitShortcut = new Tui::ZShortcut(Tui::ZKeySequence::forShortcut(QStringLiteral("q")),
                                            this, Qt::ApplicationShortcut);
    connect(quitShortcut, &Tui::ZShortcut::activated, this, &RootWidget::promptQuit);

    auto* forceQuitShortcut = new Tui::ZShortcut(
        Tui::ZKeySequence::forShortcut(QStringLiteral("c")), this, Qt::ApplicationShortcut);
    connect(forceQuitShortcut, &Tui::ZShortcut::activated, this, [] { QCoreApplication::quit(); });

    // F8 cycles the theme live (Light -> Dark -> Sepia -> Midnight), the TUI analog
    // of the GUI's theme picker. The choice persists to the same QSettings key the
    // GUI uses, so the two front ends stay in sync.
    auto* themeShortcut =
        new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F8), this, Qt::ApplicationShortcut);
    connect(themeShortcut, &Tui::ZShortcut::activated, this, &RootWidget::cycleTheme);

    // Ctrl+P opens the command palette (the TUI analog of the GUI's Mod+K),
    // filterable over the shared CommandRegistry.
    auto* paletteShortcut = new Tui::ZShortcut(Tui::ZKeySequence::forShortcut(QStringLiteral("p")),
                                               this, Qt::ApplicationShortcut);
    connect(paletteShortcut, &Tui::ZShortcut::activated, this, &RootWidget::openCommandPalette);

    // F9 opens the Settings page (the TUI settings keybinding), routed through the
    // shared NavController so it goes through the same open() path as the GUI.
    auto* settingsShortcut =
        new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F9), this, Qt::ApplicationShortcut);
    connect(settingsShortcut, &Tui::ZShortcut::activated, this,
            [this] { m_services.nav->open(QStringLiteral("settings")); });

    // F2 / F3 raise the composer overlays (session settings / checkpoints) for the
    // active transcript tab, the TUI analog of the GUI composer popovers.
    auto* sessionShortcut =
        new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F2), this, Qt::ApplicationShortcut);
    connect(sessionShortcut, &Tui::ZShortcut::activated, this,
            &RootWidget::openSessionSettingsOverlay);
    auto* checkpointShortcut =
        new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_F3), this, Qt::ApplicationShortcut);
    connect(checkpointShortcut, &Tui::ZShortcut::activated, this,
            &RootWidget::openCheckpointsOverlay);

    // Ctrl+E toggles the file Explorer (parity with the GUI). It is handled on the
    // key-bubble path in keyEvent() rather than as an ApplicationShortcut, so a
    // focused composer (readline C-e = end-of-line) or session list (C-e =
    // export) consume it first; it only toggles when those panes don't. This
    // resolves the Ctrl+E collision without rebinding either widget's binding.

    const TuiShellWidgets shell =
        TuiShellLayout::build(this, terminal(), QRect(QPoint(0, 0), geometry().size()), m_tabModel,
                              m_fileTree, m_participants, &m_pageDoc);
    m_window = shell.window;
    m_sidebarView = shell.sidebarView;
    m_listColumn = shell.listColumn;
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
    m_rightColumn = shell.rightColumn;
    m_participantsView = shell.participantsView;
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
    // The Participants section and the Explorer toggle together as one right column.
    {
        const bool showExplorer =
            QSettings().value(QStringLiteral("ui/showFileExplorer"), false).toBool();
        m_fileTreeView->setVisible(showExplorer);
        if (m_participantsView != nullptr)
            m_participantsView->setVisible(showExplorer);
    }
    // Restore distraction-free mode the same way (the GUI restores its persisted
    // UiSettings.distractionFree at startup too).
    if (QSettings().value(QStringLiteral("ui/distractionFree"), false).toBool()) {
        setDistractionFree(true);
    }
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

void RootWidget::refreshTranscript() {
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
    const QString md =
        m_active->controller->hasSession() ? m_active->controller->content() : QString();
    be::applyTranscriptLog(m_active->doc, be::decomposeMarkdown(md));
    m_active->search.refresh();
    m_transcript->reload();
}
