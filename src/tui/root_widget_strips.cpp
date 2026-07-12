// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "app/code_editor_controller.h"
#include "app/transcript_log.h"
#include "command_registry.h"
#include "composer_session_controller.h"
#include "daemon/daemon_connection_service.h" // complete type for the managed-daemon shutdown hook
#include "display_role_adapter.h"
#include "fs/ifs_service.h"
#include "fs_explorer_model.h"
#include "memory/imemory_service.h"
#include "memory_graph_model.h"
#include "memory_list_model.h"
#include "memory_stats_model.h"
#include "memory_timeline_model.h"
#include "persistence/isession_store.h"
#include "root_widget.h"
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

void RootWidget::openTranscriptSearch() {
    // Find only applies to a live transcript tab (page tabs have no session).
    if (m_active == nullptr || m_searchRow == nullptr || m_transcriptSearch == nullptr) {
        return;
    }
    m_searchActive = true;
    m_searchRow->setVisible(true);
    m_transcriptSearch->setText(m_active->search.query());
    m_transcriptSearch->setFocus();
    updateSearchCounter();
}

void RootWidget::closeTranscriptSearch() {
    if (!m_searchActive) {
        return;
    }
    m_searchActive = false;
    if (m_active != nullptr) {
        m_active->search.clear();
    }
    if (m_transcriptSearch != nullptr) {
        m_transcriptSearch->setText(QString());
    }
    if (m_searchRow != nullptr) {
        m_searchRow->setVisible(false);
    }
    if (m_transcript != nullptr) {
        m_transcript->setFocus();
        m_transcript->reload(); // drop the highlight
    }
}

void RootWidget::updateSearchCounter() {
    if (m_searchCounter == nullptr) {
        return;
    }
    if (m_active == nullptr) {
        m_searchCounter->setText(QString());
        return;
    }
    const be::TranscriptSearchController& s = m_active->search;
    if (s.query().isEmpty()) {
        m_searchCounter->setText(QString());
    } else if (s.matchCount() == 0) {
        m_searchCounter->setText(tr(" 0/0 ", "search match counter (no matches)"));
    } else {
        m_searchCounter->setText(tr(" %1/%2 ", "search match counter (current/total)")
                                     .arg(s.currentMatch() + 1)
                                     .arg(s.matchCount()));
    }
}

void RootWidget::rewindActiveTab(const QString& messageId, bool editMode) {
    if (m_active == nullptr || messageId.isEmpty()) {
        return;
    }
    // Interrupt-first: pre-empt a live turn so the truncation and replay are the
    // only stream.
    if (m_active->turn != nullptr && m_active->turn->active()) {
        m_active->orchestrator->cancel();
    }
    // Hard-truncate the active document at the message (it and everything after it
    // are dropped) and adopt the result as the persisted content so the controller
    // (the TUI's source of truth) and the document stay in sync.
    const QString text = m_active->doc.rewindToMessage(messageId);
    if (text.isEmpty()) {
        return;
    }
    if (m_active->controller->hasSession()) {
        m_active->controller->updateContent(m_active->doc.toMarkdown());
    }
    m_active->search.refresh();
    m_transcript->reload();

    if (editMode) {
        // Seed the composer with the message text; the user edits and submits,
        // which re-appends it (to the truncated content) and re-runs the turn.
        if (m_composer != nullptr) {
            m_composer->setText(text);
            m_composer->moveCursorToEnd();
            m_composer->setFocus();
        }
    } else {
        // Restore: re-submit the same text as a fresh turn (append + run).
        m_active->orchestrator->submit(text);
    }
}

void RootWidget::onTurnEvents(TabSession* session, const QVariantList& events) {
    // Route the daemon-shaped events into THIS session's ingest: it appends/patches
    // typed reasoning/tool/content blocks on the session's document keyed by callId,
    // exactly as the GUI's EditorController does. Repaint only when it is on screen.
    session->ingest.ingestAll(events);
    // Keep this session's find matches in step with the freshly ingested tail.
    session->search.refresh();
    // The same stream carries status-only events (usage/context/rateLimit); the
    // ingest skips them, the footer status model consumes them. Only the active
    // tab drives the shared footer.
    if (session == m_active && m_status != nullptr) {
        m_status->applyTurnEvents(events);
    }
    if (session == m_active && m_transcript != nullptr) {
        m_transcript->reload();
    }
}

void RootWidget::updateTodos() {
    if (m_todos == nullptr) {
        return;
    }
    if (m_active == nullptr) {
        m_todos->setText(QString());
        return;
    }
    TodoListModel* todos = m_active->orchestrator->todos();
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
    m_todos->setText(tr("Tasks:  ") + parts.join(QStringLiteral("   ")));
}

void RootWidget::updateSubagents() {
    if (m_subagents == nullptr) {
        return;
    }
    if (m_active == nullptr) {
        m_subagents->setText(QString());
        if (m_status != nullptr) {
            m_status->setAgentsRunning(0);
            m_status->setAgentsFailed(0);
        }
        return;
    }
    SubagentModel* subs = m_active->orchestrator->subagents();
    if (m_status != nullptr) {
        m_status->setAgentsRunning(subs->runningCount());
        m_status->setAgentsFailed(subs->failedCount());
    }
    const int n = subs->count();
    if (n == 0) {
        m_subagents->setText(QString());
        return;
    }
    // Compact one-line strip: "Subagents:  \u25cf explore (42 files)  \u2713 run-tests ...".
    QStringList parts;
    for (int i = 0; i < n; ++i) {
        const QString status = subs->statusAt(i);
        const QString glyph = status == QStringLiteral("done")    ? QStringLiteral("\u2713")
                              : status == QStringLiteral("error") ? QStringLiteral("\u2717")
                                                                  : QStringLiteral("\u25cf");
        QString row = glyph + QStringLiteral(" ") + subs->titleAt(i);
        const QString detail = subs->detailAt(i);
        if (!detail.isEmpty()) {
            row += QStringLiteral(" (") + detail + QStringLiteral(")");
        }
        parts << row;
    }
    m_subagents->setText(tr("Subagents:  ") + parts.join(QStringLiteral("   ")));
}

void RootWidget::updateCompletion() {
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
    y = std::max(y, 0);
    m_completionPopup->setGeometry(QRect(composerGeo.x(), y, composerGeo.width(), height));
    m_completionPopup->setVisible(true);
    m_completionPopup->raise();
}
