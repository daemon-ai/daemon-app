#include "root_widget.h"

#include "display_role_adapter.h"
#include "tui_palette.h"

#include "conversation_controller.h"
#include "conversation_orchestrator.h"
#include "conversations_list_model.h"
#include "sidebar_model.h"
#include "todo_list_model.h"

#include "composer_session_controller.h"
#include "status_bar_model.h"
#include "turn_controller.h"

#include "persistence/in_memory_conversation_store.h"

#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZEvent.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>
#include <Tui/ZPainter.h>
#include <Tui/ZRoot.h>
#include <Tui/ZShortcut.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWidget.h>
#include <Tui/ZWindow.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QItemSelectionModel>
#include <QRect>
#include <QSettings>
#include <QVariantMap>

void SubmitInputBox::keyEvent(Tui::ZKeyEvent* event)
{
    // Completion navigation takes priority while the popup is open: Up/Down move
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
    // Ctrl+Enter mid-turn steers the running turn (when the controller allows it).
    if (isEnter && event->modifiers() == Qt::ControlModifier) {
        if (m_session != nullptr && m_session->canSteer()) {
            m_session->steerDraft();
            event->accept();
            return;
        }
    }
    if (isEnter && event->modifiers() == Qt::NoModifier) {
        emit submitted(text());
        event->accept();
        return;
    }
    // Ctrl+O: add a (mock) attachment, mirroring the GUI's "+" attachment menu.
    if (event->key() == Qt::Key_O && event->modifiers() == Qt::ControlModifier) {
        emit attachRequested();
        event->accept();
        return;
    }
    // Context-sensitive Esc. Precedence: cancel an in-progress queue edit, then
    // stop a running turn, then clear a non-empty draft, then hand focus back to
    // the panes (where a further Esc bubbles to RootWidget's quit prompt).
    if (event->key() == Qt::Key_Escape && event->modifiers() == Qt::NoModifier) {
        if (m_session != nullptr && m_session->editingIndex() >= 0) {
            m_session->exitEdit(true); // restore the pre-edit draft
        } else if (m_session != nullptr && m_session->busy()) {
            m_session->cancel(); // -> cancelRequested -> orchestrator.cancel()
        } else if (!text().isEmpty()) {
            setText(QString());
            if (m_session != nullptr) {
                m_session->refreshTrigger(text(), cursorPosition());
            }
        } else {
            emit leaveRequested();
        }
        event->accept();
        return;
    }
    // Single-line box: Up/Down walk the sent-message history, gated like the GUI -
    // Up only with an empty draft or while already browsing, Down only while
    // browsing (so an in-progress draft is never clobbered by a stray Up).
    if (event->modifiers() == Qt::NoModifier && m_session != nullptr) {
        if (event->key() == Qt::Key_Up && (text().isEmpty() || m_session->browsing())) {
            emit historyPrevious();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Down && m_session->browsing()) {
            emit historyNext();
            event->accept();
            return;
        }
    }
    Tui::ZInputBox::keyEvent(event);
    // A typed character or caret move may have changed the slash/@ trigger token.
    if (m_session != nullptr) {
        m_session->refreshTrigger(text(), cursorPosition());
    }
}

void SubmitInputBox::paintEvent(Tui::ZPaintEvent* event)
{
    // Base paints the field bg + (when focused) positions the terminal caret.
    Tui::ZInputBox::paintEvent(event);
    if (!text().isEmpty()) {
        return; // real draft text takes over
    }
    // Overlay a dim placeholder over the (focus-aware) field background.
    Tui::ZPainter* p = event->painter();
    const int w = geometry().width();
    if (w <= 0) {
        return;
    }
    const Tui::ZColor fieldBg = focus() ? tpal::activeFieldBg() : tpal::surfaceAlt();
    const QString hint = QStringLiteral("Message daemon\u2026  (Enter to send)");
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
    // Fully custom paint: this box never edits text itself (the conversation list
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
            p->writeWithColors(x, 0, QStringLiteral("Search conversations").left(w - x),
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

RootWidget::RootWidget()
{
    // Build the stock-widget palette (incl. the quit dialog frame/body) from the
    // active theme - set from the persisted ui/theme in main() before we run.
    setPalette(daemonPalette(tpal::activeTheme()));

    // The reused layer: store + view models, wired exactly as in the GUI. None
    // of this depends on Tui Widgets - the same objects back the QML frontend.
    m_store = new persistence::InMemoryConversationStore(this);

    m_sidebar = new SidebarModel(this);
    m_sidebar->setStore(m_store);

    m_list = new ConversationsListModel(this);
    m_list->setStore(m_store);

    m_controller = new ConversationController(this);
    m_controller->setStore(m_store);

    // The shared composer FSM - the same C++ class the QML composer drives. The
    // TUI binds its single-line input to it, gaining draft persistence, history,
    // and the submit/queue/drain logic without re-implementing any of it.
    m_composerSession = new ComposerSessionController(this);

    // The shared submit pipeline - the same C++ class Conversation.qml drives. It
    // owns the turn (the assistant-turn FSM) and the status-stack todos; the TUI
    // routes the composer's intents into it and renders turn/todos identically.
    m_orchestrator = new ConversationOrchestrator(this);
    m_orchestrator->setConversation(m_controller);
    m_turn = m_orchestrator->turn();

    // The shared status-bar model - the same C++ class StatusBar.qml binds. The
    // TUI consumes the turn's daemon-shaped events directly and renders the status
    // model into a footer.
    m_status = new StatusBarModel(this);
    // StatusBarModel seeds sessionStartedAt at construction (= launch), mirroring
    // StatusBar.qml's Component.onCompleted.

    // Mock agent host: stands in for the daemon runtime so the inline clarify /
    // approval blocks round-trip end-to-end (patches m_doc + ingests a follow-up).
    m_host = new InteractiveTurnHost(&m_doc, &m_ingest, this);

    // Sidebar scope selection drives the conversation list - the model-to-model
    // contract is untouched by the choice of toolkit.
    connect(m_sidebar, &SidebarModel::scopeSelected, m_list, &ConversationsListModel::setScope);
}

void RootWidget::terminalChanged()
{
    if (m_built || terminal() == nullptr) {
        return;
    }
    m_built = true;
    buildUi();
    wireViews();
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
    // minimum); the conversation pane is the sole Expanding child, so it absorbs
    // the remaining width.
    m_sidebarView = new TreeListView(m_window);
    m_sidebarView->setMinimumSize(26, 3);
    m_sidebarView->setSizePolicyH(Tui::SizePolicy::Preferred);
    m_sidebarView->setSizePolicyV(Tui::SizePolicy::Expanding);
    columns->addWidget(m_sidebarView);

    // --- Column 2: search field + conversation list (custom-painted cards) ---
    auto* listCol = new Tui::ZWidget(m_window);
    listCol->setMinimumSize(34, 3);
    listCol->setSizePolicyH(Tui::SizePolicy::Preferred);
    listCol->setSizePolicyV(Tui::SizePolicy::Expanding);
    auto* listColLayout = new Tui::ZVBoxLayout();
    listCol->setLayout(listColLayout);

    m_search = new SearchInputBox(listCol);
    m_search->setText(QString());
    m_search->setMaximumSize(Tui::tuiMaxSize, 1);
    // Not a focus stop: the conversation list owns focus and forwards typed
    // characters here (type-ahead), so the box is a passive query display and is
    // skipped by the Tab cycle.
    m_search->setFocusPolicy(Tui::NoFocus);
    listColLayout->addWidget(m_search);

    m_listView = new ConversationListView(listCol);
    m_listView->setMinimumSize(34, 3);
    m_listView->setSizePolicyV(Tui::SizePolicy::Expanding);
    listColLayout->addWidget(m_listView);

    columns->addWidget(listCol);

    // --- Column 3: conversation pane (transcript + composer), expanding ---
    auto* right = new Tui::ZWidget(m_window);
    right->setSizePolicyH(Tui::SizePolicy::Expanding);
    auto* rightCol = new Tui::ZVBoxLayout();
    right->setLayout(rightCol);

    m_header = new Tui::ZLabel(right);
    m_header->setText(QStringLiteral("daemon-app TUI"));
    rightCol->addWidget(m_header);

    // Custom-painted transcript: renders the shared be::DocumentStore (the GUI's
    // parse/ingest engine) as colored tool/reasoning cards, ANSI output, diffs,
    // and YOU/DAEMON headers instead of a raw-markdown text dump.
    m_transcript = new TranscriptView(right);
    m_transcript->setDocument(&m_doc);
    m_transcript->setSizePolicyV(Tui::SizePolicy::Expanding);
    rightCol->addWidget(m_transcript);

    // One-line streaming/affordance chrome (Thinking.../error + send/stop/steer).
    m_composerChrome = new ComposerChrome(right);
    m_composerChrome->setMaximumSize(Tui::tuiMaxSize, 1);
    rightCol->addWidget(m_composerChrome);

    // Queued-prompt strip (auto-sized; 0 height when the queue is empty).
    m_queue = new QueueStripView(right);
    rightCol->addWidget(m_queue);

    // Compact status-stack todo strip (above the composer); blank at rest.
    m_todos = new Tui::ZLabel(right);
    m_todos->setMaximumSize(Tui::tuiMaxSize, 1);
    rightCol->addWidget(m_todos);

    // Attachment chips (auto-sized; 0 height when there are none).
    m_attachments = new AttachmentBarView(right);
    rightCol->addWidget(m_attachments);

    m_composer = new SubmitInputBox(right);
    m_composer->setMinimumSize(8, 1);
    m_composer->setMaximumSize(Tui::tuiMaxSize, 1);
    rightCol->addWidget(m_composer);

    // Completion overlay: a borderless custom-painted popup floated above the
    // composer. It is not in the layout - updateCompletion() positions it manually
    // over the transcript just above the input and raises it; the input box keeps
    // focus and routes navigation keys to the shared controller.
    m_completionPopup = new CompletionView(right);

    columns->addWidget(right);

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

    // The conversation list is a custom-painted view bound straight to the shared
    // model (no display-role adapter): it reads title/snippet/timestamp/agent/tags
    // directly and renders multi-line cards.
    m_listView->setModel(m_list);

    // Type-ahead search. The conversation list is the only focus stop in the
    // column; printable keys it receives build the query in the passive search box,
    // whose textChanged drives the shared live filter. Backspace edits, Esc clears.
    connect(m_search, &Tui::ZInputBox::textChanged, m_list, &ConversationsListModel::setSearch);
    connect(m_listView, &ConversationListView::searchAppend, this, [this](const QString& text) {
        m_search->setText(m_search->text() + text);
    });
    connect(m_listView, &ConversationListView::searchBackspace, this, [this] {
        QString q = m_search->text();
        if (!q.isEmpty()) {
            q.chop(1);
            m_search->setText(q);
        }
    });
    connect(m_listView, &ConversationListView::searchClear, this,
            [this] { m_search->setText(QString()); });
    // The search box shows its typing caret only while the list is focused.
    connect(m_listView, &ConversationListView::focusChanged, m_search,
            &SearchInputBox::setTypingActive);
    // After the query changes, re-anchor the selection onto the first match so the
    // highlight (and Enter) follow the filtered list instead of stranding off-list.
    // (The view already rebuilds from the model's reset/selection signals.)
    connect(m_list, &ConversationsListModel::searchChanged, this, [this] {
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

    // Conversation highlight / Enter -> record selection in the model (so the
    // model owns `current`, consistent with the sidebar) and open it. Opening a
    // conversation also points the composer session at it, so the draft/queue/
    // history swap to that conversation (mirrors the GUI's conversationId bind).
    const auto openRow = [this](int row) {
        if (row < 0) {
            return;
        }
        m_list->activate(row); // identity-based selection in the shared model
        const int id = m_list->idAt(row);
        if (id >= 0) {
            // Switching conversations abandons any in-flight simulated turn; the
            // open() below refreshes the content and reloads the document.
            m_orchestrator->cancel();
            m_ingest = be::TranscriptIngest(&m_doc); // drop any open turn state
            m_controller->open(id);
            m_composerSession->setConversationId(id);
        }
    };
    connect(m_listView, &ConversationListView::rowActivated, this, openRow);

    connect(m_controller, &ConversationController::conversationChanged, this,
            &RootWidget::refreshTranscript);
    connect(m_controller, &ConversationController::contentChanged, this,
            &RootWidget::refreshTranscript);

    // Inline interactive answers: the transcript's controls emit decisions which
    // the mock host applies to m_doc; the host then asks us to persist the patched
    // markdown (-> contentChanged -> refreshTranscript reloads + repaints).
    connect(m_transcript, &TranscriptView::approvalDecided, m_host,
            &InteractiveTurnHost::onApprovalDecided);
    connect(m_transcript, &TranscriptView::clarifySubmitted, m_host,
            &InteractiveTurnHost::onClarifySubmitted);
    // A live turn that paused at an approval gate resumes once answered (no-op for
    // a seeded block when no turn is running).
    connect(m_transcript, &TranscriptView::approvalDecided, this,
            [this](const QString&, const QString&, bool) {
                if (m_turn != nullptr) {
                    m_turn->resume();
                }
            });
    connect(m_host, &InteractiveTurnHost::documentChanged, this, [this] {
        if (m_controller->hasConversation()) {
            m_controller->updateContent(m_doc.toMarkdown());
        }
        if (m_transcript != nullptr) {
            m_transcript->reload();
        }
    });

    // Composer <-> session controller. The controller is the source of truth for
    // the draft; the input box is its view. Typed edits flow in via textChanged;
    // programmatic changes (history recall, conversation swap, clear-on-send) flow
    // back out via draftReset. Enter routes to submit(); the controller emits the
    // submitted turn, which the conversation controller appends.
    connect(m_composer, &Tui::ZInputBox::textChanged, m_composerSession,
            &ComposerSessionController::setDraft);
    connect(m_composerSession, &ComposerSessionController::draftReset, m_composer,
            [this](const QString& text) {
                if (m_composer->text() != text) {
                    m_composer->setText(text);
                }
            });
    connect(m_composer, &SubmitInputBox::submitted, m_composerSession,
            [this](const QString&) { m_composerSession->submit(); });
    connect(m_composerSession, &ComposerSessionController::submitted, this,
            [this](const QString& text, const QString& refs) {
                // The orchestrator owns the whole submit pipeline: it persists the
                // user's text (-> contentChanged -> refreshTranscript reloads the
                // doc with the user message) then starts the scripted assistant
                // turn, whose events stream into the same doc via onTurnEvents.
                m_orchestrator->submit(text, refs);
            });
    // Slash commands (e.g. /new) routed through the shared orchestrator: it
    // handles "new" (create a conversation); front-end-only commands surface via
    // commandRequested (no-ops in the TUI for now).
    connect(m_composerSession, &ComposerSessionController::commandInvoked, m_orchestrator,
            &ConversationOrchestrator::invokeCommand);
    connect(m_composer, &SubmitInputBox::historyPrevious, m_composerSession,
            [this] { m_composerSession->browseUp(); });
    connect(m_composer, &SubmitInputBox::historyNext, m_composerSession,
            [this] { m_composerSession->browseDown(); });

    // Completion: the input box routes trigger/navigation to the shared controller;
    // a caret request after an accept repositions the input cursor; the overlay
    // mirrors the controller's completion state.
    m_composer->setSession(m_composerSession);
    connect(m_composerSession, &ComposerSessionController::cursorRequested, m_composer,
            [this](int pos) { m_composer->setCursorPosition(pos); });
    connect(m_composerSession, &ComposerSessionController::completionActiveChanged, this,
            &RootWidget::updateCompletion);
    connect(m_composerSession, &ComposerSessionController::completionActiveIndexChanged, this,
            &RootWidget::updateCompletion);
    connect(m_composerSession->completionItems(), &QAbstractItemModel::modelReset, this,
            &RootWidget::updateCompletion);

    // The orchestrator's todo model drives the compact strip above the composer.
    connect(m_orchestrator->todos(), &TodoListModel::countChanged, this, &RootWidget::updateTodos);
    connect(m_orchestrator->todos(), &QAbstractItemModel::modelReset, this,
            &RootWidget::updateTodos);

    // Mid-turn nudge + stop: the composer's Ctrl+Enter/Esc call steerDraft()/
    // cancel() on the session, which surface as these host-facing signals.
    connect(m_composerSession, &ComposerSessionController::steer, m_orchestrator,
            &ConversationOrchestrator::steer);
    connect(m_composerSession, &ComposerSessionController::cancelRequested, m_orchestrator,
            &ConversationOrchestrator::cancel);

    // Queued-prompt strip: bound to the same session; editing a queued entry loads
    // it into the draft, so move focus to the composer to edit + save on Enter.
    m_queue->setController(m_composerSession);
    connect(m_queue, &QueueStripView::editRequested, this, [this](int) {
        if (m_composer != nullptr) {
            m_composer->setFocus();
        }
    });

    // Attachment chips bind to the same session; Ctrl+O on the composer adds a
    // canned attachment (mock-parity with the GUI's "+" menu / drag-drop).
    m_attachments->setController(m_composerSession);
    connect(m_composer, &SubmitInputBox::attachRequested, m_composerSession, [this] {
        const int n = m_composerSession->attachments()->count();
        m_composerSession->addAttachment(
            QStringLiteral("attachment-%1.txt").arg(n + 1), QStringLiteral("file"));
    });

    // Mirror the turn's busy state into the composer session so mid-turn submits
    // queue and drain (the GUI binds composer.busy to the turn the same way).
    connect(m_turn, &TurnController::activeChanged, this,
            [this] { m_composerSession->setBusy(m_turn->active()); });

    // Esc on an empty composer hands focus back to the conversation list, where a
    // further Esc bubbles up to the quit prompt (context-sensitive Esc, level 2).
    connect(m_composer, &SubmitInputBox::leaveRequested, this, [this] {
        if (m_listView != nullptr) {
            m_listView->setFocus();
        }
    });

    // Turn lifecycle -> transcript stream + status busy/turn timer. The footer
    // (and transcript stream) refresh as events arrive and as the status model
    // ticks once per second.
    connect(m_turn, &TurnController::eventsEmitted, this, &RootWidget::onTurnEvents);
    connect(m_turn, &TurnController::turnStarted, this, [this] {
        m_status->setBusy(true);
        m_status->setTurnStartedAt(static_cast<double>(QDateTime::currentMSecsSinceEpoch()));
        if (m_transcript != nullptr) {
            m_transcript->reload(); // re-pin to the bottom for the new turn
        }
    });
    connect(m_turn, &TurnController::turnFinished, this, [this] {
        m_status->setBusy(false);
        // Settle any open stream/reasoning, then persist the grown document back
        // to the store (mirrors the GUI round-trip). updateContent adopts the
        // markdown locally, so it does not trigger a reparse/reload here.
        m_ingest.finish();
        if (m_controller->hasConversation()) {
            m_controller->updateContent(m_doc.toMarkdown());
        }
        if (m_transcript != nullptr) {
            m_transcript->reload();
        }
    });

    // The colored status footer + the streaming composer chrome bind directly to
    // the shared models/controllers and repaint off their own NOTIFYs (including
    // the StatusBarModel's 1s elapsed tick).
    m_footer->setModel(m_status);
    m_composerChrome->setTurn(m_turn);
    m_composerChrome->setSession(m_composerSession);

    // Initial selection: first sidebar row populates the list, then open its
    // first conversation so the transcript is non-empty on launch.
    if (m_sidebarAdapter->rowCount() > 0) {
        m_sidebarView->setCurrentIndex(m_sidebarAdapter->index(0, 0));
    }
    if (m_list->rowCount() > 0) {
        openRow(0); // select + open the first conversation so the transcript is non-empty
    }
    refreshTranscript();
    updateTodos();
    updateCompletion();
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
    // The conversation list and transcript bake tpal::* colors into cached span
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
                              m_header,       m_todos };
    for (Tui::ZWidget* w : views) {
        if (w != nullptr) {
            w->update();
        }
    }

    // Persist to the GUI-shared key so both front ends honor the same choice.
    QSettings settings(QStringLiteral("daemon-app"), QStringLiteral("daemon-app"));
    settings.setValue(QStringLiteral("ui/theme"), theme::ThemePalette::toString(next));
}

void RootWidget::refreshTranscript()
{
    if (m_transcript == nullptr) {
        return;
    }
    if (m_header != nullptr) {
        m_header->setText(m_controller->hasConversation()
                              ? QStringLiteral("Conversation #%1").arg(m_controller->currentId())
                              : QStringLiteral("daemon-app TUI"));
    }
    // Mid-turn the live ingest owns the document tail; reloading from the store
    // would clobber the streamed blocks. Only reparse the persisted markdown when
    // idle (open/switch, the post-submit user-message append, or a finished turn).
    if (m_turn != nullptr && m_turn->active()) {
        return;
    }
    m_doc.loadMarkdown(m_controller->hasConversation() ? m_controller->content() : QString());
    m_transcript->reload();
}

void RootWidget::onTurnEvents(const QVariantList& events)
{
    // Route the daemon-shaped events straight into the shared ingest: it appends/
    // patches typed reasoning/tool/content blocks on m_doc keyed by callId, exactly
    // as the GUI's EditorController does. Then repaint (pinned to the bottom).
    m_ingest.ingestAll(events);
    if (m_transcript != nullptr) {
        m_transcript->reload();
    }
}

void RootWidget::updateTodos()
{
    if (m_todos == nullptr) {
        return;
    }
    TodoListModel* todos = m_orchestrator->todos();
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
