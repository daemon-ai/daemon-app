// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "tui_shell_layout.h"

#include "attachment_bar_view.h"
#include "code_editor_view.h"
#include "completion_view.h"
#include "composer_chrome.h"
#include "core/document_store.h"
#include "file_tree_view.h"
#include "fs_explorer_model.h"
#include "participants_view.h"
#include "queue_strip_view.h"
#include "search_input_box.h"
#include "session_list_view.h"
#include "status_bar_view.h"
#include "submit_input_box.h"
#include "tab_strip_view.h"
#include "transcript_view.h"
#include "tree_list_view.h"

#include <QObject>
#include <Tui/ZCommon.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZLabel.h>
#include <Tui/ZRoot.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWidget.h>
#include <Tui/ZWindow.h>

TuiShellWidgets TuiShellLayout::build(Tui::ZRoot* root, Tui::ZTerminal* terminal,
                                      const QRect& geometry, TabModel* tabModel,
                                      files::FsExplorerModel* fileTree,
                                      participants::ParticipantsModel* participants,
                                      be::DocumentStore* pageDoc) {
    TuiShellWidgets w;

    w.window = new Tui::ZWindow(QObject::tr("Daemon"), root);
    w.window->setOptions({});
    w.window->setFocusMode(Tui::FocusContainerMode::Cycle);
    w.window->setGeometry(geometry);

    auto* outer = new Tui::ZVBoxLayout();
    w.window->setLayout(outer);

    auto* columns = new Tui::ZHBoxLayout();
    outer->add(columns);

    // The left column stacks the Fleet/Tags supervision tree above the co-equal Integrations tree
    // (work package AC): both are TreeListViews over their own shared C++ model + display adapter.
    w.sidebarColumn = new Tui::ZWidget(w.window);
    w.sidebarColumn->setMinimumSize(26, 3);
    w.sidebarColumn->setSizePolicyH(Tui::SizePolicy::Preferred);
    w.sidebarColumn->setSizePolicyV(Tui::SizePolicy::Expanding);
    auto* sidebarColLayout = new Tui::ZVBoxLayout();
    w.sidebarColumn->setLayout(sidebarColLayout);

    w.sidebarView = new TreeListView(w.sidebarColumn);
    w.sidebarView->setMinimumSize(26, 3);
    w.sidebarView->setSizePolicyV(Tui::SizePolicy::Expanding);
    sidebarColLayout->addWidget(w.sidebarView);

    w.integrationsView = new TreeListView(w.sidebarColumn);
    w.integrationsView->setMinimumSize(26, 3);
    w.integrationsView->setSizePolicyV(Tui::SizePolicy::Expanding);
    sidebarColLayout->addWidget(w.integrationsView);

    columns->addWidget(w.sidebarColumn);

    w.listColumn = new Tui::ZWidget(w.window);
    w.listColumn->setMinimumSize(34, 3);
    w.listColumn->setSizePolicyH(Tui::SizePolicy::Preferred);
    w.listColumn->setSizePolicyV(Tui::SizePolicy::Expanding);
    auto* listColLayout = new Tui::ZVBoxLayout();
    w.listColumn->setLayout(listColLayout);

    w.search = new SearchInputBox(w.listColumn);
    w.search->setText(QString());
    w.search->setMaximumSize(Tui::tuiMaxSize, 1);
    w.search->setFocusPolicy(Tui::NoFocus);
    listColLayout->addWidget(w.search);

    w.listView = new SessionListView(w.listColumn);
    w.listView->setMinimumSize(34, 3);
    w.listView->setSizePolicyV(Tui::SizePolicy::Expanding);
    listColLayout->addWidget(w.listView);
    columns->addWidget(w.listColumn);

    auto* right = new Tui::ZWidget(w.window);
    right->setSizePolicyH(Tui::SizePolicy::Expanding);
    auto* rightCol = new Tui::ZVBoxLayout();
    right->setLayout(rightCol);

    w.tabStrip = new TabStripView(right);
    w.tabStrip->setModel(tabModel);
    rightCol->addWidget(w.tabStrip);

    w.searchRow = new Tui::ZWidget(right);
    w.searchRow->setMaximumSize(Tui::tuiMaxSize, 1);
    auto* searchRowLayout = new Tui::ZHBoxLayout();
    searchRowLayout->setSpacing(1);
    w.searchRow->setLayout(searchRowLayout);
    w.transcriptSearch = new TranscriptSearchBox(w.searchRow);
    w.transcriptSearch->setSizePolicyH(Tui::SizePolicy::Expanding);
    searchRowLayout->addWidget(w.transcriptSearch);
    w.searchCounter = new Tui::ZLabel(w.searchRow);
    w.searchCounter->setSizePolicyH(Tui::SizePolicy::Fixed);
    searchRowLayout->addWidget(w.searchCounter);
    w.searchRow->setVisible(false);
    rightCol->addWidget(w.searchRow);

    w.transcript = new TranscriptView(right);
    w.transcript->setDocument(pageDoc);
    w.transcript->setSizePolicyV(Tui::SizePolicy::Expanding);
    rightCol->addWidget(w.transcript);

    w.editorView = new CodeEditorView(right);
    w.editorView->setSizePolicyV(Tui::SizePolicy::Expanding);
    w.editorView->setVisible(false);
    rightCol->addWidget(w.editorView);

    w.fileStatus = new Tui::ZLabel(right);
    w.fileStatus->setMaximumSize(Tui::tuiMaxSize, 1);
    w.fileStatus->setVisible(false);
    rightCol->addWidget(w.fileStatus);

    w.composerChrome = new ComposerChrome(right);
    w.composerChrome->setMaximumSize(Tui::tuiMaxSize, 1);
    rightCol->addWidget(w.composerChrome);

    w.queue = new QueueStripView(right);
    rightCol->addWidget(w.queue);

    w.subagents = new Tui::ZLabel(right);
    w.subagents->setMaximumSize(Tui::tuiMaxSize, 1);
    rightCol->addWidget(w.subagents);

    w.todos = new Tui::ZLabel(right);
    w.todos->setMaximumSize(Tui::tuiMaxSize, 1);
    rightCol->addWidget(w.todos);

    w.attachments = new AttachmentBarView(right);
    rightCol->addWidget(w.attachments);

    w.composer = new SubmitInputBox(terminal->textMetrics(), right);
    w.composer->setMinimumSize(8, 1);
    rightCol->addWidget(w.composer);

    w.completionPopup = new CompletionView(right);

    columns->addWidget(right);

    // Right column: the Participants section above the file Explorer (mirrors the
    // GUI's RightPanel). The column holds both; Ctrl+E shows/hides them together.
    w.rightColumn = new Tui::ZWidget(w.window);
    w.rightColumn->setSizePolicyH(Tui::SizePolicy::Preferred);
    w.rightColumn->setSizePolicyV(Tui::SizePolicy::Expanding);
    auto* rightColLayout = new Tui::ZVBoxLayout();
    w.rightColumn->setLayout(rightColLayout);

    w.participantsView = new ParticipantsView(w.rightColumn);
    w.participantsView->setSizePolicyH(Tui::SizePolicy::Expanding);
    w.participantsView->setModel(participants);
    rightColLayout->addWidget(w.participantsView);

    w.fileTreeView = new FileTreeView(w.rightColumn);
    w.fileTreeView->setMinimumSize(28, 3);
    w.fileTreeView->setSizePolicyH(Tui::SizePolicy::Preferred);
    w.fileTreeView->setSizePolicyV(Tui::SizePolicy::Expanding);
    w.fileTreeView->setModel(fileTree);
    rightColLayout->addWidget(w.fileTreeView);

    columns->addWidget(w.rightColumn);

    w.footer = new StatusBarView(w.window);
    outer->addWidget(w.footer);

    w.sidebarView->setFocus();
    return w;
}
