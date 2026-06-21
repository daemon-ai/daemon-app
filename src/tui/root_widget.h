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

private:
    void buildUi();
    void wireViews();
    void refreshTranscript();
    void promptQuit(); // open the quit-confirmation modal (idempotent)

    // Reused, unchanged from the GUI build.
    persistence::InMemoryConversationStore* m_store = nullptr;
    SidebarModel* m_sidebar = nullptr;
    ConversationsListModel* m_list = nullptr;
    ConversationController* m_controller = nullptr;

    // TUI-only glue + widgets.
    DisplayRoleAdapter* m_sidebarAdapter = nullptr;
    DisplayRoleAdapter* m_listAdapter = nullptr;
    Tui::ZWindow* m_window = nullptr;
    TreeListView* m_sidebarView = nullptr;
    Tui::ZListView* m_listView = nullptr;
    Tui::ZTextEdit* m_transcript = nullptr;
    SubmitInputBox* m_composer = nullptr;
    Tui::ZLabel* m_header = nullptr;

    // Exit handling: Esc opens the confirmation modal, so it is disabled while the
    // modal is up (letting the dialog's own Esc cancel instead).
    Tui::ZShortcut* m_escShortcut = nullptr;
    QuitDialog* m_quitDialog = nullptr;

    bool m_built = false;
};
