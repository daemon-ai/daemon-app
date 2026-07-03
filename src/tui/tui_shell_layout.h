// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QRect>
#include <Tui/ZLabel.h>
#include <Tui/ZRoot.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZWidget.h>
#include <Tui/ZWindow.h>

namespace be {
class DocumentStore;
}
namespace files {
class FsExplorerModel;
}
namespace participants {
class ParticipantsModel;
}

class AttachmentBarView;
class CodeEditorView;
class CompletionView;
class ComposerChrome;
class FileTreeView;
class ParticipantsView;
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
    // The middle column wrapping the search box + session list. Exposed so
    // distraction-free mode can hide the pair as one unit (mirroring how the
    // right column toggles), without disturbing either child's own state.
    Tui::ZWidget* listColumn = nullptr;
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
    // Right column: the Participants section stacked above the file Explorer. The
    // column toggles as a unit (Ctrl+E) - both children show/hide together.
    Tui::ZWidget* rightColumn = nullptr;
    ParticipantsView* participantsView = nullptr;
    FileTreeView* fileTreeView = nullptr;
    StatusBarView* footer = nullptr;
};

class TuiShellLayout {
public:
    static TuiShellWidgets build(Tui::ZRoot* root, Tui::ZTerminal* terminal, const QRect& geometry,
                                 TabModel* tabModel, files::FsExplorerModel* fileTree,
                                 participants::ParticipantsModel* participants,
                                 be::DocumentStore* pageDoc);
};
