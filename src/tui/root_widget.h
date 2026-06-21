#pragma once

// Include the concrete Tui headers (their classes live in the inline namespace
// Tui::v0, so forward-declaring them as Tui::Foo would mint a different type).
#include <Tui/ZDialog.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>
#include <Tui/ZRoot.h>
#include <Tui/ZShortcut.h>
#include <Tui/ZWindow.h>

#include "completion_view.h"
#include "composer_chrome.h"
#include "conversation_list_view.h"
#include "status_bar_view.h"
#include "transcript_view.h"

#include "core/agent_ingest.h"
#include "core/document_store.h"

#include <QString>
#include <QStringList>
#include <QVariantList>

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
class DisplayRoleAdapter;

// ZInputBox has no "submit" signal, only textChanged. Subclass it to emit on
// Enter so the composer can hand the line to the ConversationController.
class SubmitInputBox : public Tui::ZInputBox {
    Q_OBJECT

public:
    using Tui::ZInputBox::ZInputBox;

    // The shared composer session. When set, the input box routes completion
    // navigation (Up/Down/Enter/Tab/Esc while completionActive) to it and refreshes
    // the slash/@ trigger after every key (text/caret change).
    void setSession(ComposerSessionController* session) { m_session = session; }

signals:
    void submitted(const QString& text);
    // Esc on an empty composer: ask the shell to move focus back to the panes.
    void leaveRequested();
    // Up/Down with the caret idle: walk the shared sent-message history.
    void historyPrevious();
    void historyNext();

protected:
    void keyEvent(Tui::ZKeyEvent* event) override;

private:
    ComposerSessionController* m_session = nullptr;
};

// ZListView handles Up/Down/Home/End but ignores Left/Right. The sidebar is a
// flattened tree, so map Left/Right to collapse/expand requests (mirroring the
// GUI's Keys.onLeft/RightPressed -> SidebarModel.collapse/expandCurrent).
class TreeListView : public Tui::ZListView {
    Q_OBJECT

public:
    using Tui::ZListView::ZListView;

signals:
    void collapseRequested();
    void expandRequested();

protected:
    void keyEvent(Tui::ZKeyEvent* event) override;
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

// The TUI shell: a single full-screen window holding the three-column layout
// (Sidebar | ConversationsList | Conversation), driven entirely by the app's
// existing C++ view models against the in-memory store.
class RootWidget : public Tui::ZRoot {
    Q_OBJECT

public:
    RootWidget();

    void dumpGeometry() const;  // debug helper for the offscreen self-check
    void focusComposer() const; // offscreen-test helper: focus the input box

protected:
    void terminalChanged() override;
    void resizeEvent(Tui::ZResizeEvent* event) override;
    void keyEvent(Tui::ZKeyEvent* event) override; // Esc fallback -> promptQuit

private:
    void buildUi();
    void wireViews();
    void refreshTranscript();
    void promptQuit(); // open the quit-confirmation modal (idempotent)

    // Live assistant-turn streaming: route the TurnController's daemon-shaped
    // events through be::TranscriptIngest so the document grows real typed blocks
    // (reasoning/tool/content), rendered identically to the persisted ones.
    void onTurnEvents(const QVariantList& events);
    // Render the orchestrator's status-stack todos as a compact strip above the
    // composer (cleared when the model empties after the turn settles).
    void updateTodos();
    // Sync the completion overlay (items / active row / visibility / geometry)
    // from the shared controller's completion state.
    void updateCompletion();

    // Reused, unchanged from the GUI build.
    persistence::InMemoryConversationStore* m_store = nullptr;
    SidebarModel* m_sidebar = nullptr;
    ConversationsListModel* m_list = nullptr;
    ConversationController* m_controller = nullptr;
    // Shared submit pipeline (owns the turn + todos), identical to the GUI.
    ConversationOrchestrator* m_orchestrator = nullptr;
    // Shared composer FSM (draft/queue/history/submit), identical to the GUI.
    ComposerSessionController* m_composerSession = nullptr;
    // The orchestrator's TurnController (cached), and the shared status-bar model
    // (DaemonApp.StatusModel) - the same C++ classes the GUI binds.
    TurnController* m_turn = nullptr;
    StatusBarModel* m_status = nullptr;

    // TUI-only glue + widgets.
    DisplayRoleAdapter* m_sidebarAdapter = nullptr;
    Tui::ZWindow* m_window = nullptr;
    TreeListView* m_sidebarView = nullptr;
    ConversationListView* m_listView = nullptr;
    TranscriptView* m_transcript = nullptr;
    // One-line streaming/affordance indicator above the composer (Thinking.../error
    // + send/stop/steer hint), driven by the TurnController.
    ComposerChrome* m_composerChrome = nullptr;
    SubmitInputBox* m_composer = nullptr;
    Tui::ZLabel* m_header = nullptr;
    // Custom-painted colored status footer (gateway/agents/context/session/version).
    StatusBarView* m_footer = nullptr;
    // Compact status-stack todo strip (above the composer).
    Tui::ZLabel* m_todos = nullptr;
    // Borderless completion overlay floated above the composer; driven entirely by
    // the shared controller's completion state (the input box keeps focus).
    CompletionView* m_completionPopup = nullptr;

    // Exit handling: the quit confirmation modal (nullptr when closed).
    QuitDialog* m_quitDialog = nullptr;

    // The shared parse/ingest engine: persisted markdown loads into m_doc and
    // live turn events stream into it via m_ingest, then TranscriptView paints it.
    be::DocumentStore m_doc;
    be::TranscriptIngest m_ingest { &m_doc };

    bool m_built = false;
};
