#include "root_widget.h"

#include "display_role_adapter.h"
#include "tui_palette.h"

#include "conversation_controller.h"
#include "conversations_list_model.h"
#include "sidebar_model.h"

#include "persistence/in_memory_conversation_store.h"

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
    Tui::ZInputBox::keyEvent(event);
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

void RootWidget::buildUi()
{
    auto* quit = new Tui::ZShortcut(Tui::ZKeySequence::forKey(Qt::Key_Escape), this,
                                    Qt::ApplicationShortcut);
    connect(quit, &Tui::ZShortcut::activated, this, [] { QCoreApplication::quit(); });

    m_window = new Tui::ZWindow(QStringLiteral("daemon-app \u00b7 TUI spike"), this);
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
    m_sidebarView = new Tui::ZListView(m_window);
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

    // Conversation highlight / Enter -> open it in the transcript.
    const auto openRow = [this](int row) {
        const int id = m_list->idAt(row);
        if (id >= 0) {
            m_controller->open(id);
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

    connect(m_composer, &SubmitInputBox::submitted, this, [this](const QString& text) {
        if (!text.trimmed().isEmpty()) {
            m_controller->appendUserText(text);
            m_composer->setText(QString());
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
