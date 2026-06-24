#pragma once

#include <Tui/ZLabel.h>
#include <Tui/ZRoot.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZWidget.h>
#include <Tui/ZWindow.h>

#include <QRect>

namespace be {
class DocumentStore;
}
namespace files {
class FsExplorerModel;
}

class AttachmentBarView;
class CodeEditorView;
class CompletionView;
class ComposerChrome;
class FileTreeView;
class QueueStripView;
class SearchInputBox;
class SessionListView;
class StatusBarView;
class SubmitInputBox;
class TabModel;
class TabStripView;
class TranscriptSearchBox;
class TranscriptView;
class TreeListView;

struct TuiShellWidgets {
    Tui::ZWindow* window = nullptr;
    TreeListView* sidebarView = nullptr;
    SearchInputBox* search = nullptr;
    SessionListView* listView = nullptr;
    TabStripView* tabStrip = nullptr;
    Tui::ZWidget* searchRow = nullptr;
    TranscriptSearchBox* transcriptSearch = nullptr;
    Tui::ZLabel* searchCounter = nullptr;
    TranscriptView* transcript = nullptr;
    CodeEditorView* editorView = nullptr;
    Tui::ZLabel* fileStatus = nullptr;
    ComposerChrome* composerChrome = nullptr;
    QueueStripView* queue = nullptr;
    Tui::ZLabel* subagents = nullptr;
    Tui::ZLabel* todos = nullptr;
    AttachmentBarView* attachments = nullptr;
    SubmitInputBox* composer = nullptr;
    CompletionView* completionPopup = nullptr;
    FileTreeView* fileTreeView = nullptr;
    StatusBarView* footer = nullptr;
};

class TuiShellLayout {
public:
    static TuiShellWidgets build(Tui::ZRoot* root, Tui::ZTerminal* terminal, const QRect& geometry,
                                 TabModel* tabModel, files::FsExplorerModel* fileTree,
                                 be::DocumentStore* pageDoc);
};
