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

void RootWidget::previewSessionTab(const QString& sessionId) {
    if (m_tabModel == nullptr) {
        return;
    }
    m_tabModel->previewTranscript(sessionId,
                                  rwdetail::resolveSessionTabTitle(m_services.store, sessionId));
}

void RootWidget::openSessionPinnedTab(const QString& sessionId) {
    if (m_tabModel == nullptr) {
        return;
    }
    m_tabModel->openTranscriptPinned(sessionId,
                                     rwdetail::resolveSessionTabTitle(m_services.store, sessionId));
}

void RootWidget::newTranscriptTab() {
    if (m_services.store == nullptr || m_tabModel == nullptr) {
        return;
    }
    const QString id = m_services.store->newSessionId(domain::UnitId());
    m_tabModel->openTranscriptPinned(id, tr("New session"));
    // A new tab is a natural place to start typing.
    if (m_composer != nullptr) {
        m_composer->setFocus();
    }
}

void RootWidget::rebindSession(int tabId, const QString& sessionId) {
    if (m_tabSessions == nullptr) {
        return;
    }
    const bool wasActive =
        m_active != nullptr && m_active->tabId == tabId && m_active->sessionId != sessionId;
    m_tabSessions->rebindSession(tabId, sessionId, [this](TabSession* s) { wireSession(s); });
    if (wasActive) {
        m_composerSession->setSessionId(sessionId);
        refreshTranscript();
        updateTodos();
        updateSubagents();
    }
}

void RootWidget::closeCurrentTab() {
    if (m_tabModel != nullptr && m_tabModel->currentIndex() >= 0) {
        m_tabModel->closeTab(m_tabModel->currentIndex());
    }
}

TabSession* RootWidget::ensureSession(int tabId) {
    if (m_tabSessions == nullptr) {
        return nullptr;
    }
    return m_tabSessions->ensureSession(tabId, [this](TabSession* s) { wireSession(s); });
}

void RootWidget::activateTab(int tabId) {
    // Make the tab with `tabId` the active one, dispatching on its kind. Each
    // branch is a cohesive helper; the orchestrator only resolves the kind.
    const int row = m_tabModel->indexOfTabId(tabId);
    if (row < 0) {
        return;
    }
    const int kind = m_tabModel->kindAt(row);
    if (kind == TabModel::Transcript) {
        activateTranscriptTab(tabId);
    } else if (kind == TabModel::File) {
        activateFileTab(tabId);
    } else {
        activatePageTab(row);
    }
}

void RootWidget::activateTranscriptTab(int tabId) {
    TabSession* s = ensureSession(tabId);
    if (s == nullptr) {
        return;
    }
    // Switching tabs closes any open find bar so it never shows stale matches
    // from the previous tab's engine.
    closeTranscriptSearch();
    showEditor(false);
    m_active = s;
    if (m_transcript != nullptr) {
        m_transcript->setDocument(&s->doc);
        m_transcript->setSearch(&s->search);
    }
    if (m_composerChrome != nullptr) {
        m_composerChrome->setTurn(s->turn);
    }
    m_composerSession->setSessionId(s->sessionId);
    m_composerSession->setBusy(s->turn->active());
    m_status->setBusy(s->turn->active());
    // Resync the elapsed timer to THIS tab's live turn (mirrors the GUI's
    // onIsActiveChanged), so a switched-to busy tab shows the right elapsed.
    if (s->turn->active()) {
        m_status->setTurnStartedAt(static_cast<double>(QDateTime::currentMSecsSinceEpoch()) -
                                   static_cast<double>(s->turn->elapsedMs()));
    }
    refreshTranscript();
    updateTodos();
    updateSubagents();
}

void RootWidget::activateFileTab(int tabId) {
    // A file tab: bind the editor to this tab's controller (lazily created +
    // read through the fs seam) and show it in place of the transcript stack.
    closeTranscriptSearch();
    m_active = nullptr;
    editor::CodeEditorController* c =
        m_fileTabs != nullptr ? m_fileTabs->ensureFileSession(tabId) : nullptr;
    if (m_editorView != nullptr) {
        m_editorView->setController(c);
        m_editorView->setFocus();
    }
    showEditor(true);
    m_status->setBusy(false);
    updateTodos();
    updateSubagents();
}

void RootWidget::activatePageTab(int row) {
    // A non-transcript page tab (Settings): no backend session; show the static
    // page document and clear the per-tab strips.
    closeTranscriptSearch();
    showEditor(false);
    m_active = nullptr;
    const int kind = m_tabModel->kindAt(row);
    // Per-agent Memory tab: re-scope the shared memory models to this tab's
    // agent (profile == bank) before projecting. Switching between agents'
    // Memory tabs re-scopes the same shared service.
    if (kind == TabModel::Memory && m_services.memory != nullptr) {
        m_services.memory->setScope(m_tabModel->agentRefAt(row), QString(), true);
    }
    m_pageDoc.loadMarkdown(pageMarkdownForKind(kind));
    if (m_transcript != nullptr) {
        m_transcript->setSearch(nullptr);
        m_transcript->setDocument(&m_pageDoc);
        m_transcript->reload();
        // Pages are documents, not chat logs: open at the title, not pinned to
        // the bottom (which hides the top rows of a page taller than the view).
        m_transcript->scrollBlockIntoView(0);
        // Focus the transcript so the interactive hubs' j/k + action keys (which
        // bubble up past the read-only page view) reach the root handler.
        if (activePageKind() >= 0) {
            m_transcript->setFocus();
        }
    }
    m_status->setBusy(false);
    updateTodos();
    updateSubagents();
}

void RootWidget::showEditor(bool on) {
    if (m_editorView != nullptr)
        m_editorView->setVisible(on);
    if (m_fileStatus != nullptr)
        m_fileStatus->setVisible(on);
    // Transcript + composer chrome are hidden while a file is open.
    const bool stack = !on;
    if (m_transcript != nullptr)
        m_transcript->setVisible(stack);
    if (m_composerChrome != nullptr)
        m_composerChrome->setVisible(stack);
    if (m_queue != nullptr)
        m_queue->setVisible(stack);
    if (m_subagents != nullptr)
        m_subagents->setVisible(stack);
    if (m_todos != nullptr)
        m_todos->setVisible(stack);
    if (m_attachments != nullptr)
        m_attachments->setVisible(stack);
    if (m_composer != nullptr)
        m_composer->setVisible(stack);
}

void RootWidget::destroySession(int tabId) {
    // File tabs hold an editor controller rather than a TabSession.
    if (m_fileTabs != nullptr && m_fileTabs->destroySession(tabId, m_editorView)) {
        return;
    }

    if (m_tabSessions == nullptr) {
        return;
    }
    m_tabSessions->destroySession(tabId, m_active, [this] {
        if (m_transcript != nullptr) {
            m_transcript->setDocument(&m_pageDoc);
        }
    });
}

void RootWidget::toggleExplorer() {
    if (m_fileTreeView == nullptr) {
        return;
    }
    const bool show = !m_fileTreeView->isVisible();
    m_fileTreeView->setVisible(show);
    if (m_participantsView != nullptr) {
        m_participantsView->setVisible(show);
    }
    if (show) {
        m_fileTreeView->setFocus();
    }
    // Persist via the same "ui/showFileExplorer" key the GUI's UiSettings uses, so
    // the explorer's open/closed state survives a restart (and stays in sync when
    // both shells share an org/app QSettings scope).
    QSettings().setValue(QStringLiteral("ui/showFileExplorer"), show);
}

void RootWidget::toggleDistractionFree() {
    setDistractionFree(!m_distractionFree);
}

void RootWidget::setDistractionFree(bool on) {
    if (m_distractionFree == on) {
        return;
    }
    m_distractionFree = on;
    // Zen mode hides the navigation chrome - the sidebar, the session-list
    // column (search + list) and the whole right Participants/Explorer column -
    // leaving the tab strip, transcript, composer and status footer (the pane
    // set the GUI hides on UiSettings.distractionFree in Main.qml). Hiding the
    // COLUMNS preserves the explorer's own open/closed state for when zen ends.
    if (m_sidebarView != nullptr) {
        m_sidebarView->setVisible(!on);
    }
    if (m_listColumn != nullptr) {
        m_listColumn->setVisible(!on);
    }
    if (m_rightColumn != nullptr) {
        m_rightColumn->setVisible(!on);
    }
    if (on && m_composer != nullptr) {
        // The hidden sidebar/list may hold focus (palette entry path); the
        // composer is the natural zen focus, and its Esc exits the mode.
        m_composer->setFocus();
    }
    // Persist like toggleExplorer's "ui/showFileExplorer" so zen survives a
    // restart (the GUI persists its UiSettings.distractionFree equivalently).
    QSettings().setValue(QStringLiteral("ui/distractionFree"), on);
}
