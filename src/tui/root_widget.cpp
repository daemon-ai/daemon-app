#include "root_widget.h"

#include "display_role_adapter.h"
#include "tui_palette.h"

#include "conversation_controller.h"
#include "conversations_list_model.h"
#include "sidebar_model.h"

#include "composer_session_controller.h"

#include "persistence/in_memory_conversation_store.h"

#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>
#include <Tui/ZRoot.h>
#include <Tui/ZShortcut.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTextEdit.h>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWidget.h>
#include <Tui/ZWindow.h>

#include <QCoreApplication>
#include <QItemSelectionModel>
#include <QRect>

void SubmitInputBox::keyEvent(Tui::ZKeyEvent* event)
{
    const bool isEnter = event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return
                         || event->text() == QStringLiteral("\r");
    if (isEnter && event->modifiers() == Qt::NoModifier) {
        emit submitted(text());
        event->accept();
        return;
    }
    // Context-sensitive Esc, level 1+2: a non-empty draft is cleared; an empty
    // composer hands focus back to the panes. Only when Esc reaches a pane
    // (unhandled) does it bubble up to RootWidget's quit prompt.
    if (event->key() == Qt::Key_Escape && event->modifiers() == Qt::NoModifier) {
        if (!text().isEmpty()) {
            setText(QString());
        } else {
            emit leaveRequested();
        }
        event->accept();
        return;
    }
    // The box is single-line, so Up/Down are free to walk the sent-message
    // history (the shared controller decides whether there is anything to recall).
    if (event->modifiers() == Qt::NoModifier) {
        if (event->key() == Qt::Key_Up) {
            emit historyPrevious();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Down) {
            emit historyNext();
            event->accept();
            return;
        }
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
    setPalette(daemonDarkPalette());

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

    m_window = new Tui::ZWindow(QStringLiteral("daemon-app \u00b7 TUI spike  (Ctrl+Q quit)"), this);
    m_window->setOptions({});
    m_window->setFocusMode(Tui::FocusContainerMode::Cycle); // Tab cycles the panes
    m_window->setGeometry(QRect(QPoint(0, 0), geometry().size()));

    auto* columns = new Tui::ZHBoxLayout();
    m_window->setLayout(columns);

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

    // --- Column 2: conversation list ---
    m_listView = new Tui::ZListView(m_window);
    m_listView->setMinimumSize(34, 3);
    m_listView->setSizePolicyH(Tui::SizePolicy::Preferred);
    m_listView->setSizePolicyV(Tui::SizePolicy::Expanding);
    columns->addWidget(m_listView);

    // --- Column 3: conversation pane (transcript + composer), expanding ---
    auto* right = new Tui::ZWidget(m_window);
    right->setSizePolicyH(Tui::SizePolicy::Expanding);
    auto* rightCol = new Tui::ZVBoxLayout();
    right->setLayout(rightCol);

    m_header = new Tui::ZLabel(right);
    m_header->setText(QStringLiteral("daemon-app TUI"));
    rightCol->addWidget(m_header);

    m_transcript = new Tui::ZTextEdit(terminal()->textMetrics(), right);
    m_transcript->setReadOnly(true);
    m_transcript->setShowLineNumbers(false);
    m_transcript->setWordWrapMode(Tui::ZTextOption::WordWrap);
    m_transcript->setSizePolicyV(Tui::SizePolicy::Expanding);
    rightCol->addWidget(m_transcript);

    m_composer = new SubmitInputBox(right);
    m_composer->setMinimumSize(8, 1);
    m_composer->setMaximumSize(Tui::tuiMaxSize, 1);
    rightCol->addWidget(m_composer);

    columns->addWidget(right);

    m_sidebarView->setFocus();
}

void RootWidget::wireViews()
{
    // Bind the reused models through the display-role adapters.
    m_sidebarAdapter = new DisplayRoleAdapter(DisplayRoleAdapter::Kind::Sidebar, this);
    m_sidebarAdapter->setSourceModel(m_sidebar);
    m_sidebarView->setModel(m_sidebarAdapter);

    m_listAdapter = new DisplayRoleAdapter(DisplayRoleAdapter::Kind::ConversationList, this);
    m_listAdapter->setSourceModel(m_list);
    m_listView->setModel(m_listAdapter);

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
        m_list->activate(row); // identity-based selection in the shared model
        const int id = m_list->idAt(row);
        if (id >= 0) {
            m_controller->open(id);
            m_composerSession->setConversationId(id);
        }
    };
    connect(m_listView->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [openRow](const QModelIndex& current, const QModelIndex&) {
                if (current.isValid()) {
                    openRow(current.row());
                }
            });
    connect(m_listView, &Tui::ZListView::enterPressed, this, openRow);

    connect(m_controller, &ConversationController::conversationChanged, this,
            &RootWidget::refreshTranscript);
    connect(m_controller, &ConversationController::contentChanged, this,
            &RootWidget::refreshTranscript);

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
                const QString full
                    = refs.isEmpty() ? text : (refs + QStringLiteral("\n") + text);
                m_controller->appendUserText(full);
            });
    connect(m_composer, &SubmitInputBox::historyPrevious, m_composerSession,
            [this] { m_composerSession->browseUp(); });
    connect(m_composer, &SubmitInputBox::historyNext, m_composerSession,
            [this] { m_composerSession->browseDown(); });

    // Esc on an empty composer hands focus back to the conversation list, where a
    // further Esc bubbles up to the quit prompt (context-sensitive Esc, level 2).
    connect(m_composer, &SubmitInputBox::leaveRequested, this, [this] {
        if (m_listView != nullptr) {
            m_listView->setFocus();
        }
    });

    // Initial selection: first sidebar row populates the list, then open its
    // first conversation so the transcript is non-empty on launch.
    if (m_sidebarAdapter->rowCount() > 0) {
        m_sidebarView->setCurrentIndex(m_sidebarAdapter->index(0, 0));
    }
    if (m_listAdapter->rowCount() > 0) {
        m_listView->setCurrentIndex(m_listAdapter->index(0, 0));
        openRow(0); // ensure the transcript is populated on launch
    }
    refreshTranscript();
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

void RootWidget::refreshTranscript()
{
    if (m_transcript == nullptr) {
        return;
    }
    if (m_controller->hasConversation()) {
        m_transcript->setText(m_controller->content());
        if (m_header != nullptr) {
            m_header->setText(QStringLiteral("Conversation #%1").arg(m_controller->currentId()));
        }
    } else {
        m_transcript->setText(QStringLiteral("Select a conversation on the left."));
        if (m_header != nullptr) {
            m_header->setText(QStringLiteral("daemon-app TUI"));
        }
    }
}
