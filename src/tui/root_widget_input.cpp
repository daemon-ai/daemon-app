// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "app/code_editor_controller.h"
#include "app/transcript_log.h"
#include "command_registry.h"
#include "composer_session_controller.h"
#include "daemon/daemon_connection_service.h" // complete type for the managed-daemon shutdown hook
#include "daemonnet/idaemonnet.h"             // complete type for setDaemonNet(QObject*)
#include "dialogs/agents_dialog.h" // [wave2:app-engines] foreign-agent management dialog (C1)
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
#include "session_controller.h"
#include "session_orchestrator.h"
#include "sessions_list_model.h"
#include "settings_editor.h"
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

// keyEvent is a dispatch chain: each handler consumes (accepts + returns true) or
// declines (returns false) so the next is tried, preserving the original order and
// fall-through semantics. Tab management keys are handled here at the ZRoot, i.e.
// only AFTER the focused pane leaves them unhandled. This keeps them contextual:
// the composer consumes Ctrl+W (word-rubout) and Ctrl+T (transpose) for its
// readline editing while it has focus, so tab open/close fire only from the
// panes/transcript. Ctrl+Tab switching has no composer conflict and works
// everywhere.
void RootWidget::keyEvent(Tui::ZKeyEvent* event) {
    if (handleTabShortcuts(event)) {
        return;
    }
    if (handleSidebarAgentShortcuts(event)) {
        return;
    }
    if (handleTabNavigation(event)) {
        return;
    }
    // Interactive manager-hub pages: while one is the active tab, no-modifier keys
    // that bubble up to the root (the transcript shows the page doc but only
    // consumes its scroll keys) drive the page's seam via j/k + action keys.
    if (handlePageActionKey(event)) {
        return;
    }
    if (handleFinderShortcut(event)) {
        return;
    }
    if (handleExplorerToggle(event)) {
        return;
    }
    if (handleEscapeQuit(event)) {
        return;
    }
    Tui::ZRoot::keyEvent(event);
}

bool RootWidget::handleTabShortcuts(Tui::ZKeyEvent* event) {
    const Qt::KeyboardModifiers mods = event->modifiers();
    if (mods == Qt::ControlModifier &&
        event->text().compare(QStringLiteral("t"), Qt::CaseInsensitive) == 0) {
        newTranscriptTab();
        event->accept();
        return true;
    }
    if (mods == Qt::ControlModifier &&
        event->text().compare(QStringLiteral("w"), Qt::CaseInsensitive) == 0) {
        closeCurrentTab();
        event->accept();
        return true;
    }
    if (mods == Qt::ControlModifier &&
        event->text().compare(QStringLiteral("f"), Qt::CaseInsensitive) == 0) {
        openTranscriptSearch();
        event->accept();
        return true;
    }
    return false;
}

bool RootWidget::handleSidebarAgentShortcuts(Tui::ZKeyEvent* event) {
    // Sidebar agent shortcuts (parity with the GUI right-click menu): with the
    // Fleet tree focused, 'p' opens the selected agent's Profile and 'm' its
    // Memory. Only fires on profile-backed (agent) rows; otherwise declines so the
    // key bubbles on to the rest of the chain (matching the original fall-through).
    const Qt::KeyboardModifiers mods = event->modifiers();
    // 'a' with the Fleet tree focused mints a NEW agent (the GUI "+ New agent" analog): a minimal
    // name + engine list (daemon-core + the foreign catalog names) over the shared create path.
    if (mods == Qt::NoModifier && m_sidebarView != nullptr && m_sidebarView->focus() &&
        event->text() == QStringLiteral("a") && m_services.profiles != nullptr) {
        auto* dialog = new NewAgentDialog(m_services.profiles, m_services.agents, this);
        dialog->setVisible(true);
        event->accept();
        return true;
    }
    const bool isAgentShortcutKey =
        mods == Qt::NoModifier && m_sidebarView != nullptr && m_sidebarView->focus() &&
        m_sidebar != nullptr && m_services.nav != nullptr &&
        (event->text() == QStringLiteral("p") || event->text() == QStringLiteral("m"));
    if (!isAgentShortcutKey) {
        return false;
    }
    const int row = m_sidebar->currentRow();
    if (row < 0) {
        return false;
    }
    const QModelIndex idx = m_sidebar->index(row);
    const QString profile = m_sidebar->data(idx, SidebarModel::ProfileRole).toString();
    const QString label = m_sidebar->data(idx, SidebarModel::LabelRole).toString();
    if (profile.isEmpty()) {
        return false;
    }
    m_services.nav->openAgent(event->text() == QStringLiteral("p") ? QStringLiteral("profile")
                                                                   : QStringLiteral("memory"),
                              profile, label);
    event->accept();
    return true;
}

bool RootWidget::handleTabNavigation(Tui::ZKeyEvent* event) {
    const Qt::KeyboardModifiers mods = event->modifiers();
    const bool cycleTabsKey = (mods & Qt::ControlModifier) && m_tabModel != nullptr &&
                              (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab);
    if (cycleTabsKey) {
        const bool backward = event->key() == Qt::Key_Backtab || (mods & Qt::ShiftModifier);
        m_tabModel->cycle(backward ? -1 : 1);
        event->accept();
        return true;
    }
    // Alt+1..9 jumps directly to tab N (1-based). Unlike Ctrl+Tab this is reliably
    // delivered by terminals (as ESC + digit), so it is the primary keyboard nav.
    const bool altDigitKey =
        (mods & Qt::AltModifier) && m_tabModel != nullptr && event->text().size() == 1;
    if (altDigitKey) {
        const QChar ch = event->text().at(0);
        if (ch >= QLatin1Char('1') && ch <= QLatin1Char('9')) {
            const int target = ch.digitValue() - 1;
            if (target < m_tabModel->count()) {
                m_tabModel->activate(target);
            }
            event->accept();
            return true;
        }
    }
    return false;
}

bool RootWidget::handleFinderShortcut(Tui::ZKeyEvent* event) {
    // Ctrl+G, when it bubbles up unconsumed, opens the fuzzy "go to file"
    // finder. Bubble-path (not an ApplicationShortcut) for the same reason as
    // Ctrl+E: the composer's reverse-i-search consumes Ctrl+G as its readline
    // abort while active, and must keep winning. Ctrl+P (the binding the GUI
    // explorer advertises) is the TUI command palette, so the finder takes the
    // adjacent "go to" chord. Tui delivers Ctrl+letter as a char event, so
    // match the text as well as the key.
    const bool finderKey = (event->modifiers() & Qt::ControlModifier) &&
                           (event->key() == Qt::Key_G ||
                            event->text().compare(QStringLiteral("g"), Qt::CaseInsensitive) == 0);
    if (finderKey) {
        openFileFinder();
        event->accept();
        return true;
    }
    return false;
}

bool RootWidget::handleExplorerToggle(Tui::ZKeyEvent* event) {
    // Ctrl+E, when it bubbles up unconsumed (i.e. the composer/list did not claim
    // it for end-of-line / export), toggles the file Explorer. Tui delivers
    // Ctrl+letter as a char event, so match the text as well as the key.
    const bool explorerToggleKey =
        (event->modifiers() & Qt::ControlModifier) &&
        (event->key() == Qt::Key_E ||
         event->text().compare(QStringLiteral("e"), Qt::CaseInsensitive) == 0);
    if (explorerToggleKey) {
        toggleExplorer();
        event->accept();
        return true;
    }
    return false;
}

bool RootWidget::handleEscapeQuit(Tui::ZKeyEvent* event) {
    // Final fallback for the context-sensitive Esc chain: when a pane leaves Esc
    // unhandled it bubbles up the parent chain to here (the ZRoot), where it
    // opens the quit confirmation. The composer consumes Esc itself; the quit
    // dialog consumes it while open, so this only fires from the panes.
    if (event->key() == Qt::Key_Escape && event->modifiers() == Qt::NoModifier) {
        // Distraction-free mode consumes the bare Esc first ("Esc to exit");
        // the quit prompt stays one further Esc away.
        if (m_distractionFree) {
            setDistractionFree(false);
        } else {
            promptQuit();
        }
        event->accept();
        return true;
    }
    return false;
}

bool RootWidget::hitTest(Tui::ZWidget* w, QPoint termPos, QPoint& local) const {
    // Hit-test: is the terminal point inside `w`? If so, report the widget-local
    // coordinate. mapFromTerminal walks the full ancestor chain, so this works for
    // the deeply nested panes regardless of column/layout offsets.
    if (w == nullptr) {
        return false;
    }
    local = w->mapFromTerminal(termPos);
    const QSize sz = w->geometry().size();
    return local.x() >= 0 && local.y() >= 0 && local.x() < sz.width() && local.y() < sz.height();
}

void RootWidget::handleMouse(QPoint termPos, MouseTerminal::MouseAction action, int button,
                             Qt::KeyboardModifiers modifiers) {
    using MA = MouseTerminal::MouseAction;
    // Click + wheel only: a primary-button press focuses + selects/opens, the wheel
    // scrolls the pane under the cursor. Release/move and middle/right are ignored.
    const bool isPress = action == MA::Press && button == 0;
    const bool isWheel = action == MA::WheelUp || action == MA::WheelDown;
    if (!isPress && !isWheel) {
        return;
    }
    if (isWheel) {
        routeWheel(action, termPos);
        return;
    }
    routeClick(termPos, modifiers);
}

void RootWidget::routeWheel(MouseTerminal::MouseAction action, QPoint termPos) {
    using MA = MouseTerminal::MouseAction;
    QPoint local;
    // Visibility-guarded like paneAt: a wheel never scrolls a pane hidden by
    // distraction-free mode / a File tab.
    const auto hit = [&](Tui::ZWidget* w) {
        return w != nullptr && w->isVisible() && hitTest(w, termPos, local);
    };
    const int delta = (action == MA::WheelUp) ? -3 : 3;
    if (hit(m_editorView)) {
        m_editorView->scrollByLines(delta);
    } else if (hit(m_fileTreeView)) {
        m_fileTreeView->scrollByLines(delta);
    } else if (hit(m_transcript)) {
        m_transcript->scrollByLines(delta);
    } else if (hit(m_listView)) {
        m_listView->scrollByLines(delta);
    } else if (hit(m_sidebarView)) {
        m_sidebarView->scrollByLines(delta);
    }
}

RootWidget::ClickPane RootWidget::paneAt(QPoint termPos, QPoint& local) {
    // Primary-button press: the first pane (in priority order) under the cursor.
    // The queue / attachments strips are only hit when they have non-zero height.
    // The left panes are visibility-guarded so a click never routes to a column
    // hidden by distraction-free mode (the reflowed transcript owns that area).
    const auto hit = [&](Tui::ZWidget* w) {
        return w != nullptr && w->isVisible() && hitTest(w, termPos, local);
    };
    if (hit(m_sidebarView)) {
        return ClickPane::Sidebar;
    }
    if (hit(m_listView)) {
        return ClickPane::List;
    }
    if (hit(m_search)) {
        return ClickPane::Search;
    }
    if (hit(m_transcript)) {
        return ClickPane::Transcript;
    }
    if (hit(m_queue) && m_queue->geometry().height() > 0) {
        return ClickPane::Queue;
    }
    if (hit(m_attachments) && m_attachments->geometry().height() > 0) {
        return ClickPane::Attachments;
    }
    if (hit(m_composer)) {
        return ClickPane::Composer;
    }
    return ClickPane::None;
}

void RootWidget::routeClick(QPoint termPos, Qt::KeyboardModifiers modifiers) {
    QPoint local;
    const auto hit = [&](Tui::ZWidget* w) { return hitTest(w, termPos, local); };

    // The modal quit dialog is topmost: a press activates its button (or is
    // swallowed) so clicks never leak to the panes behind it.
    if (m_overlays != nullptr && m_overlays->quitDialog() != nullptr) {
        const QList<Tui::ZButton*> buttons =
            m_overlays->quitDialog()->findChildren<Tui::ZButton*>();
        for (Tui::ZButton* b : buttons) {
            if (hit(b)) {
                b->click();
                break;
            }
        }
        return;
    }

    // The completion popup floats above the composer: a click on an item row
    // selects it and accepts (group-header lines return -1 and are ignored).
    if (m_completionPopup != nullptr && m_completionPopup->isLocallyVisible() &&
        hit(m_completionPopup)) {
        const int row = m_completionPopup->modelRowAt(local.y());
        if (row >= 0 && m_composerSession != nullptr) {
            m_composerSession->setActiveIndex(row);
            m_composerSession->acceptActive();
        }
        return;
    }

    // The tab strip is topmost in the session column: a click activates /
    // closes a tab or hits the "+" affordance (handled inside TabStripView).
    if (hit(m_tabStrip)) {
        m_tabStrip->setFocus(); // clicking the strip focuses it, like any pane
        m_tabStrip->clickAt(local);
        return;
    }

    // The file Explorer column (right side, shown via Ctrl+E): a click focuses it
    // and selects/opens the row under the cursor.
    if (m_fileTreeView != nullptr && m_fileTreeView->isVisible() && hit(m_fileTreeView)) {
        m_fileTreeView->setFocus();
        m_fileTreeView->clickAt(local);
        return;
    }
    // The code editor (shown in the session column for a File tab): a click
    // focuses it (caret placement is keyboard-driven for now).
    if (m_editorView != nullptr && m_editorView->isVisible() && hit(m_editorView)) {
        m_editorView->setFocus();
        m_editorView->clickAt(local, modifiers);
        return;
    }

    // Primary-button press: focus the clicked pane, then run its select/open.
    switch (paneAt(termPos, local)) {
    case ClickPane::Sidebar:
        m_sidebarView->setFocus();
        m_sidebarView->clickAt(local);
        break;
    case ClickPane::List:
        m_listView->setFocus();
        m_listView->activateAtLocalY(local.y());
        break;
    case ClickPane::Search:
        // The search box is not a focus stop; clicking it focuses the list (typed
        // characters are type-ahead routed through the list to the search box).
        m_listView->setFocus();
        break;
    case ClickPane::Transcript:
        m_transcript->setFocus();
        m_transcript->clickAt(local); // activate an approval/clarify control, if any
        break;
    case ClickPane::Queue:
        m_queue->setFocus();
        m_queue->clickAt(local);
        break;
    case ClickPane::Attachments:
        m_attachments->setFocus();
        m_attachments->clickAt(local);
        break;
    case ClickPane::Composer:
        m_composer->setFocus();
        break;
    case ClickPane::None:
        break;
    }
}

bool RootWidget::handlePageActionKey(Tui::ZKeyEvent* event) {
    const int kind = activePageKind();
    if (kind < 0 || m_pageHub == nullptr) {
        // The per-agent Profile tab is not an interactive hub (no action rows),
        // but 'e' on it opens the profile editor for the bound agent.
        if (m_active == nullptr && m_tabModel != nullptr && m_pageHub != nullptr &&
            event->modifiers() == Qt::NoModifier && event->text() == QStringLiteral("e")) {
            const int idx = m_tabModel->currentIndex();
            if (idx >= 0 && m_tabModel->kindAt(idx) == TabModel::Profile) {
                openProfileEditor(m_tabModel->agentRefAt(idx));
                event->accept();
                return true;
            }
        }
        return false;
    }
    if (m_pageHub->handlePageActionKey(kind, event)) {
        refreshActivePage();
        return true;
    }
    // Settings rows the hub declined (choice/text/zen): open the matching
    // picker / prompt / live toggle via the settings editor.
    if (kind == TabModel::Settings && m_settingsEditor != nullptr &&
        m_settingsEditor->handleKey(event)) {
        return true;
    }
    // Local model track: 'd' on the Models page opens the discover -> quant download flow.
    if (kind == TabModel::Models && event->modifiers() == Qt::NoModifier &&
        event->text() == QStringLiteral("d")) {
        openModelDownload();
        event->accept();
        return true;
    }
    // A5 (CON-13): 'q' on the Models page re-quantizes an installed model (source pick ->
    // target-quant pick -> ModelQuantize), the GUI installed row's "Re-quantize…" analog.
    if (kind == TabModel::Models && event->modifiers() == Qt::NoModifier &&
        event->text() == QStringLiteral("q") && m_overlays != nullptr) {
        m_overlays->openRequantize(m_services.modelCatalog, [this] { refreshActivePage(); });
        event->accept();
        return true;
    }
    // Accounts: 'a' opens the add-account wizard (provider pick -> credentials),
    // the GUI "Add account" button analog.
    if (kind == TabModel::Accounts && event->modifiers() == Qt::NoModifier &&
        event->text() == QStringLiteral("a")) {
        openAddAccount();
        event->accept();
        return true;
    }
    // Accounts: 'o' opens the interactive sign-in flow (SSO/OAuth begin -> browser URL ->
    // paste-callback), the GUI AuthFlowSheet analog over the shared AuthFlowController.
    if (kind == TabModel::Accounts && event->modifiers() == Qt::NoModifier &&
        event->text() == QStringLiteral("o")) {
        openAuthFlow();
        event->accept();
        return true;
    }
    // [acct-mgmt] Channels row-contextual keys. Every key here opens a dialog / palette (TuiPageHub
    // cannot host dialogs); 'd' (disconnect, non-destructive) stays in the hub key handler. The
    // acted-on verb depends on the selected row's kind (account vs room).
    if (kind == TabModel::Channels && event->modifiers() == Qt::NoModifier) {
        const QString text = event->text();
        const bool enter = event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return;
        const QList<QVariantMap> rows = pageActionRows(kind);
        if (rows.isEmpty()) {
            return false;
        }
        const int sel =
            qBound(0, m_pageHub->pageSelection(kind), static_cast<int>(rows.size()) - 1);
        const QVariantMap& row = rows.at(sel);
        const bool isAccount =
            row.value(QStringLiteral("rowKind")).toString() == QLatin1String("account");
        const QString transport = row.value(QStringLiteral("transport")).toString();
        bool handled = false;
        if (isAccount) {
            // [wave2:app-channels-liveness] B1: 'c' connects via the shared interactive-auth
            // launcher; 'x' removes the account (confirmed); 'g'/'n' open the room flows.
            if (text == QStringLiteral("c")) {
                openAuthFlow();
                handled = true;
            } else if (text == QStringLiteral("x")) {
                openChannelRemoveConfirm(transport,
                                         row.value(QStringLiteral("displayName")).toString());
                handled = true;
            } else if (text == QStringLiteral("g")) {
                openRoomJoinFlow(transport);
                handled = true;
            } else if (text == QStringLiteral("n")) {
                openRoomCreateFlow(transport);
                handled = true;
            }
        } else {
            // Room row: Enter members palette; 'i' invite; 'l' leave (confirm); 'x' delete
            // (confirm); 'p' pin to agent (session picker → bindChat).
            const QString conv = row.value(QStringLiteral("convId")).toString();
            const QString label = row.value(QStringLiteral("roomLabel")).toString();
            if (enter) {
                openRoomMembers(transport, conv);
                handled = true;
            } else if (text == QStringLiteral("i")) {
                openRoomInvite(transport, conv);
                handled = true;
            } else if (text == QStringLiteral("l")) {
                openRoomLeaveConfirm(transport, conv, label);
                handled = true;
            } else if (text == QStringLiteral("x")) {
                openRoomDeleteConfirm(transport, conv, label);
                handled = true;
            } else if (text == QStringLiteral("p")) {
                openRoomPin(transport, conv, row.value(QStringLiteral("kind")).toString());
                handled = true;
            }
        }
        if (handled) {
            event->accept();
            return true;
        }
    }
    // Fleet: 't' opens the steer prompt for the selected delegated child (F4/DEL-4) — a
    // RootWidget-level overlay because TuiPageHub cannot host dialogs.
    if (kind == TabModel::Fleet && event->modifiers() == Qt::NoModifier &&
        event->text() == QStringLiteral("t")) {
        const QList<QVariantMap> rows = pageActionRows(kind);
        if (!rows.isEmpty()) {
            const int sel =
                qBound(0, m_pageHub->pageSelection(kind), static_cast<int>(rows.size()) - 1);
            openFleetSteerPrompt(rows.at(sel));
        }
        event->accept();
        return true;
    }
    // [wave2:app-engines] Settings: 'g' opens the foreign-agent management dialog (C1) — the
    // register/remove parity counterpart of the GUI Settings -> Agents section.
    if (kind == TabModel::Settings && event->modifiers() == Qt::NoModifier &&
        event->text() == QStringLiteral("g") && m_services.agents != nullptr) {
        auto* dialog = new AgentsDialog(m_services.agents, this);
        dialog->setVisible(true);
        event->accept();
        return true;
    }
    // [wave2:app-delegation] Fleet: 'o' drills into the selected child's transcript (F1/DEL-1). The
    // GUI opens a read-only viewer; the TUI navigates to the child session tab (the story's
    // explicit reduced form). Enter stays pause/resume; operator steer/cancel stays on 't'/'c'.
    if (kind == TabModel::Fleet && event->modifiers() == Qt::NoModifier &&
        event->text() == QStringLiteral("o")) {
        const QList<QVariantMap> rows = pageActionRows(kind);
        if (!rows.isEmpty()) {
            const int sel =
                qBound(0, m_pageHub->pageSelection(kind), static_cast<int>(rows.size()) - 1);
            const QString childSession = rows.at(sel).value(QStringLiteral("sessionId")).toString();
            if (!childSession.isEmpty()) {
                openSessionPinnedTab(childSession);
            }
        }
        event->accept();
        return true;
    }
    // [wave2:app-approvals-safety] D3: Approvals 'D' opens a deny-with-reason prompt — a
    // RootWidget-level overlay (TuiPageHub cannot host dialogs) that threads the reason to the node
    // via ApprovalDecide.reason (wire v29). Plain 'd' (no reason) stays in the hub key handler.
    if (kind == TabModel::Approvals && event->text() == QStringLiteral("D")) {
        const QList<QVariantMap> rows = pageActionRows(kind);
        if (!rows.isEmpty()) {
            const int sel =
                qBound(0, m_pageHub->pageSelection(kind), static_cast<int>(rows.size()) - 1);
            openApprovalDenyReasonPrompt(rows.at(sel).value(QStringLiteral("id")).toString());
        }
        event->accept();
        return true;
    }
    if (kind == TabModel::Profiles && event->modifiers() == Qt::NoModifier) {
        // 'e' opens the interactive profile editor for the selected row.
        if (event->text() == QStringLiteral("e")) {
            const QList<QVariantMap> rows = pageActionRows(kind);
            if (!rows.isEmpty()) {
                const int sel =
                    qBound(0, m_pageHub->pageSelection(kind), static_cast<int>(rows.size()) - 1);
                openProfileEditor(rows.at(sel).value(QStringLiteral("id")).toString());
            }
            event->accept();
            return true;
        }
        // 'a' mints a NEW agent (name + engine list: daemon-core / ACP catalog)
        // over the shared create path - the sidebar shortcut, reachable from
        // the Profiles page too (engine choice is create-time).
        if (event->text() == QStringLiteral("a") && m_services.profiles != nullptr) {
            auto* dialog = new NewAgentDialog(m_services.profiles, m_services.agents, this);
            dialog->setVisible(true);
            event->accept();
            return true;
        }
    }
    return false;
}
