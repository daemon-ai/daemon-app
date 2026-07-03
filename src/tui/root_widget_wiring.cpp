// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "app/code_editor_controller.h"
#include "app/transcript_log.h"
#include "command_registry.h"
#include "composer_session_controller.h"
#include "daemon/daemon_connection_service.h" // complete type for the managed-daemon shutdown hook
#include "daemonnet/idaemonnet.h"             // complete type for setDaemonNet(QObject*)
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
#include "root_widget.h"
#include "root_widget_detail.h"
#include "session_controller.h"
#include "session_orchestrator.h"
#include "sessions_list_model.h"
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

// wireViews is a thin orchestrator: it calls the per-concern binders below IN THE
// SAME ORDER the connects used to appear, so the connect() set and ordering are
// byte-for-byte identical to the pre-split single method.
void RootWidget::wireViews() {
    wireModelBindings();
    wirePageLiveRefresh();
    wireSearchBox();
    wireSidebarTree();
    wireSessionList();
    wireTranscriptControls();
    wireComposer();
    wireStatusFooter();
    wireInitialSelection();
}

void RootWidget::wireModelBindings() {
    // Bind the reused models through the display-role adapters.
    m_sidebarAdapter = new DisplayRoleAdapter(DisplayRoleAdapter::Kind::Sidebar, this);
    m_sidebarAdapter->setSourceModel(m_sidebar);
    m_sidebarView->setModel(m_sidebarAdapter);

    // The session list is a custom-painted view bound straight to the shared
    // model (no display-role adapter): it reads title/snippet/timestamp/agent/tags
    // directly and renders multi-line cards.
    m_listView->setModel(m_list);

    // Re-resolve every open transcript tab's title on store change: the node
    // auto-titles a session from its first user message and the roster refetch
    // lands that title in the cache WITHOUT a content change, so the tab chip
    // would otherwise keep its stale "New session" fallback (GUI parity:
    // SessionStore.changed() -> _resolveTitle() in TranscriptPage.qml). The
    // mid-turn reload guard lives in refreshTranscript(); titles are safe to
    // refresh any time.
    connect(m_services.store, &persistence::ISessionStore::changed, this,
            [this] { rwdetail::refreshTranscriptTabTitles(m_tabModel, m_services.store); });
}

void RootWidget::wirePageLiveRefresh() {
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
    // The per-agent Profile tab renders the same rows; keep it live too (a
    // profile edit from either frontend repaints an open agent tab).
    liveRefresh(m_services.profiles->profiles(), TabModel::Profile);
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
}

void RootWidget::wireSearchBox() {
    // Type-ahead search. The session list is the only focus stop in the
    // column; printable keys it receives build the query in the passive search box,
    // whose textChanged drives the shared live filter. Backspace edits, Esc clears.
    connect(m_search, &Tui::ZInputBox::textChanged, m_list, &SessionsListModel::setSearch);
    connect(m_listView, &SessionListView::searchAppend, this,
            [this](const QString& text) { m_search->setText(m_search->text() + text); });
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
    connect(m_listView, &SessionListView::focusChanged, m_search, &SearchInputBox::setTypingActive);
    // After the query changes, re-anchor the selection onto the first match so the
    // highlight (and Enter) follow the filtered list instead of stranding off-list.
    // (The view already rebuilds from the model's reset/selection signals.)
    connect(m_list, &SessionsListModel::searchChanged, this, [this] {
        if (m_list->currentRow() < 0 && m_list->rowCount() > 0) {
            m_list->activate(0);
        }
    });
}

void RootWidget::wireSidebarTree() {
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
}

void RootWidget::openRow(int row, bool pinned) {
    // Session highlight / open -> record selection in the model (so the model
    // owns `current`, consistent with the sidebar) and surface it in a tab. A
    // transient activation (arrow nav / single click) loads into the VSCode-style
    // preview tab; a deliberate Enter opens a permanent (pinned) tab.
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
}

void RootWidget::wireSessionList() {
    connect(m_listView, &SessionListView::rowActivated, this, &RootWidget::openRow);

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
        auto* confirm =
            new ConfirmDialog(tr("Delete session"), tr("Permanently delete this session?"), this);
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
        auto* dialog = new TextPromptDialog(tr("Rename session"), m_services.store->title(id),
                                            /*masked=*/false, this);
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
}

void RootWidget::wireTranscriptControls() {
    // Inline interactive answers: the transcript's controls emit decisions routed
    // to the ACTIVE session's mock host, which patches its document and asks us to
    // persist + reload. A live turn paused at an approval gate resumes once answered.
    connect(m_transcript, &TranscriptView::approvalDecided, this,
            [this](const QString& callId, const QString& decision, bool permanent) {
                if (m_active == nullptr) {
                    return;
                }
                // The mock host patches the local doc for the simulator's visual; the engine
                // (daemon mode) sends Respond{Approved} and resumes the Subscribe loop. The
                // mock engine maps respondApproval onto its scripted resume().
                m_active->host->onApprovalDecided(callId, decision, permanent);
                if (m_active->turn != nullptr) {
                    m_active->turn->respondApproval(callId, decision == QStringLiteral("approved"));
                }
            });
    connect(m_transcript, &TranscriptView::clarifySubmitted, this,
            [this](const QString& callId, const QString& requestId, const QVariantMap& answers) {
                if (m_active == nullptr) {
                    return;
                }
                m_active->host->onClarifySubmitted(callId, requestId, answers);
                if (m_active->turn != nullptr) {
                    // Flatten the answers to a readable string; the engine resolves a Choice gate
                    // to its option index, otherwise sends free-text Input.
                    QStringList parts;
                    for (const QVariant& value : answers) {
                        parts << (value.metaType().id() == QMetaType::QStringList
                                      ? value.toStringList().join(QStringLiteral(", "))
                                      : value.toString());
                    }
                    m_active->turn->respondInput(requestId, parts.join(QStringLiteral("; ")));
                }
            });
    // Rewind picker: restore re-runs the selected user message; edit seeds the
    // composer with its text. Both funnel through the single rewindActiveTab seam.
    connect(m_transcript, &TranscriptView::rewindRestoreRequested, this,
            [this](const QString& messageId) { rewindActiveTab(messageId, false); });
    connect(m_transcript, &TranscriptView::rewindEditRequested, this,
            [this](const QString& messageId, const QString&) { rewindActiveTab(messageId, true); });
}

void RootWidget::handleComposerCommand(const QString& command) {
    // Parity: a command id added here must also land in
    // tuiroutes::handledCommands() (tui_parity_routes.cpp) -
    // tests/tui/tui_parity_tests.cpp enforces it.
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
    if (command == QStringLiteral("distraction")) {
        // Zen mode (GUI: UiSettings.distractionFree = true in onCommandRequested);
        // the TUI toggles, and a bare Esc exits (the registry's "Esc to exit").
        toggleDistractionFree();
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
        auto* dialog = new TextPromptDialog(tr("Rename session"), m_services.store->title(id),
                                            /*masked=*/false, this);
        connect(dialog, &TextPromptDialog::submitted, this, [this, id](const QString& text) {
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
        const QString path = QDir(QDir::homePath()).filePath(name + QStringLiteral(".json"));
        m_exporter->exportToPath(m_services.store, id, path);
        return;
    }
    if (command == QStringLiteral("compress")) {
        // Simulated compaction: free most of the live context window.
        if (m_status != nullptr) {
            m_status->setContextUsed(qMax(1500, static_cast<int>(m_status->contextUsed() * 0.2)));
        }
        return;
    }
    if (command == QStringLiteral("clear")) {
        if (m_active == nullptr) {
            return;
        }
        auto* confirm = new ConfirmDialog(tr("Clear session"),
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
    const bool isRewindCommand = command == QStringLiteral("retry") ||
                                 command == QStringLiteral("edit") ||
                                 command == QStringLiteral("undo");
    if (isRewindCommand) {
        if (m_active == nullptr) {
            return;
        }
        QString lastUserId;
        for (qsizetype row = 0; row < m_active->doc.blockCount(); ++row) {
            const be::BlockRecord* b = m_active->doc.blockAt(row);
            if (b != nullptr && b->role == be::MessageRole::User && !b->messageId.isEmpty()) {
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
}

void RootWidget::wireComposer() {
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
            &RootWidget::handleComposerCommand);
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
            // CHA-6 Stop: graceful interrupt (daemon Submit{Interrupt}); mock maps onto cancel.
            m_active->orchestrator->interrupt();
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
        m_composerSession->addAttachment(QStringLiteral("attachment-%1.txt").arg(n + 1),
                                         QStringLiteral("file"));
    });

    // Esc on an empty composer hands focus back to the session list, where a
    // further Esc bubbles up to the quit prompt (context-sensitive Esc, level 2).
    // In distraction-free mode the list is hidden, so Esc exits zen instead.
    connect(m_composer, &SubmitInputBox::leaveRequested, this, [this] {
        if (m_distractionFree) {
            setDistractionFree(false);
            return;
        }
        if (m_listView != nullptr) {
            m_listView->setFocus();
        }
    });
}

void RootWidget::wireStatusFooter() {
    // The colored status footer + the streaming composer chrome bind to the shared
    // models; the chrome's turn is (re)bound to the active tab in activateTab().
    m_footer->setModel(m_status);
    m_composerChrome->setSession(m_composerSession);
}

void RootWidget::wireInitialSelection() {
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

// wireSession is a thin orchestrator over the per-concern session binders; the
// connect() set + order match the pre-split single method.
void RootWidget::wireSession(TabSession* s) {
    wireSessionStreaming(s);
    wireSessionPersistence(s);
    wireSessionStatus(s);
    wireSessionSearch(s);
}

void RootWidget::wireSessionStreaming(TabSession* s) {
    // Stream this session's turn events into ITS OWN document (so a background tab
    // keeps growing while another is on screen); repaint only when it is active.
    connect(s->turn, &ITurnEngine::eventsEmitted, this,
            [this, s](const QVariantList& events) { onTurnEvents(s, events); });
    connect(s->turn, &ITurnEngine::turnStarted, this, [this, s] {
        if (s == m_active) {
            m_status->setBusy(true);
            m_status->setTurnStartedAt(static_cast<double>(QDateTime::currentMSecsSinceEpoch()));
            if (m_transcript != nullptr) {
                m_transcript->reload();
            }
        }
    });
    connect(s->turn, &ITurnEngine::turnFinished, this, [this, s] {
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
            if (m_services.settings != nullptr &&
                m_services.settings->value(QStringLiteral("notify/turnDone"), false).toBool()) {
                const QString convTitle = s->controller->hasSession()
                                              ? m_services.store->title(s->sessionId)
                                              : QStringLiteral("daemon");
                rwdetail::emitDesktopNotification(convTitle, tr("The turn finished."));
            }
        }
    });
    connect(s->turn, &ITurnEngine::activeChanged, this, [this, s] {
        if (s == m_active) {
            m_composerSession->setBusy(s->turn->active());
        }
    });
    // A gate (approval/clarify/host) blocked the turn on the user: ring the bell
    // and flag the terminal title so an unfocused terminal alerts (the TUI analog
    // of the GUI's OS notification). Only the active session drives the chrome.
    connect(s->turn, &ITurnEngine::awaitingInput, this, [this, s](const QString& kind) {
        if (s != m_active) {
            return;
        }
        // Honor the "notify when a turn needs my input" preference (shared with the
        // GUI's notify/gates), defaulting on.
        if (m_services.settings != nullptr &&
            !m_services.settings->value(QStringLiteral("notify/gates"), true).toBool()) {
            return;
        }
        std::fputs("\a", stdout); // BEL: most terminals raise an urgency hint
        std::fflush(stdout);
        const QString what = kind == QStringLiteral("approval") ? tr("approval") : tr("credential");
        if (terminal() != nullptr) {
            terminal()->setTitle(tr("\u25cf daemon \u2014 needs %1").arg(what));
        }
        const QString convTitle = (m_active != nullptr && m_active->controller->hasSession())
                                      ? m_services.store->title(m_active->sessionId)
                                      : QStringLiteral("daemon");
        rwdetail::emitDesktopNotification(convTitle, tr("The turn needs your %1.").arg(what));
    });
    // Clear the title alert once the turn settles.
    connect(s->turn, &ITurnEngine::turnFinished, this, [this, s] {
        if (s == m_active && terminal() != nullptr) {
            terminal()->setTitle(QStringLiteral("daemon"));
        }
    });
    // The turn paused for a masked host input (sudo password / secret). Raise a
    // masked prompt; the answer resumes the turn (the daemon's host channel
    // replaces this mock responder later), cancel aborts it.
    connect(s->turn, &ITurnEngine::hostRequested, this,
            [this, s](const QString& kind, const QString& prompt) {
                const QString title =
                    prompt.isEmpty() ? (kind == QStringLiteral("secret") ? tr("Secret required")
                                                                         : tr("Password required"))
                                     : prompt;
                auto* dialog = new TextPromptDialog(title, QString(), /*masked=*/true, this);
                // Forward the typed secret to the parked Input host-request (empty requestId
                // resolves to the engine's pending gate); the mock maps respondInput onto resume().
                connect(dialog, &TextPromptDialog::submitted, this,
                        [s](const QString& text) { s->turn->respondInput(QString(), text); });
                connect(dialog, &TextPromptDialog::canceled, this, [s] { s->turn->cancel(); });
            });
}

void RootWidget::wireSessionPersistence(TabSession* s) {
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
}

void RootWidget::wireSessionStatus(TabSession* s) {
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
}

void RootWidget::wireSessionSearch(TabSession* s) {
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
