#pragma once

// Include the concrete Tui headers (their classes live in the inline namespace
// Tui::v0, so forward-declaring them as Tui::Foo would mint a different type).
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>
#include <Tui/ZRoot.h>
#include <Tui/ZTextEdit.h>
#include <Tui/ZWindow.h>

#include <QString>

namespace persistence {
class InMemoryConversationStore;
}

class SidebarModel;
class ConversationsListModel;
class ConversationController;
class DisplayRoleAdapter;

// ZInputBox has no "submit" signal, only textChanged. Subclass it to emit on
// Enter so the composer can hand the line to the ConversationController.
class SubmitInputBox : public Tui::ZInputBox {
    Q_OBJECT

public:
    using Tui::ZInputBox::ZInputBox;

signals:
    void submitted(const QString& text);

protected:
    void keyEvent(Tui::ZKeyEvent* event) override;
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

private:
    void buildUi();
    void wireViews();
    void refreshTranscript();

    // Reused, unchanged from the GUI build.
    persistence::InMemoryConversationStore* m_store = nullptr;
    SidebarModel* m_sidebar = nullptr;
    ConversationsListModel* m_list = nullptr;
    ConversationController* m_controller = nullptr;

    // TUI-only glue + widgets.
    DisplayRoleAdapter* m_sidebarAdapter = nullptr;
    DisplayRoleAdapter* m_listAdapter = nullptr;
    Tui::ZWindow* m_window = nullptr;
    Tui::ZListView* m_sidebarView = nullptr;
    Tui::ZListView* m_listView = nullptr;
    Tui::ZTextEdit* m_transcript = nullptr;
    SubmitInputBox* m_composer = nullptr;
    Tui::ZLabel* m_header = nullptr;

    bool m_built = false;
};
