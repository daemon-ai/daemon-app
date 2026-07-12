// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "app/code_editor_controller.h"
#include "app/transcript_log.h"
#include "command_registry.h"
#include "composer_session_controller.h"
#include "daemon/daemon_connection_service.h" // complete type for the managed-daemon shutdown hook
#include "daemonnet/idaemonnet.h"             // complete type for setDaemonNet(QObject*)
#include "dialogs/add_account_flow.h"
#include "dialogs/auth_flow_dialog.h"
#include "dialogs/contact_flow.h" // [acct-mgmt] Channels transport-contacts flows (wire v34)
#include "dialogs/profile_editor_dialog.h"
#include "dialogs/room_flow.h" // [acct-mgmt] Channels room lifecycle + member flows
#include "display_role_adapter.h"
#include "domain/origin.h"          // [acct-mgmt] bindChat origin for the room pin
#include "feedback/ifeedback.h"     // app-feedback flow submit
#include "fleet/iapprovals_inbox.h" // [wave2:app-approvals-safety] D3: deny-with-reason prompt
#include "fleet/isession_roster.h"  // [acct-mgmt] session picker for the room pin
#include "fs/ifs_service.h"
#include "fs_explorer_model.h"
#include "memory/imemory_service.h"
#include "memory_graph_model.h"
#include "memory_list_model.h"
#include "memory_stats_model.h"
#include "memory_timeline_model.h"
#include "palette_dialog.h" // [acct-mgmt] session picker for the room pin
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
#include "transports/icontacts_service.h"   // [acct-mgmt] Channels contacts (wire v34)
#include "transports/itransport_registry.h" // [waveB:app-v30] D1: Channels remove-confirm
#include "tui_dialogs.h"
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
#include <memory>
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

namespace {

// The next theme in the fixed Light -> Dark -> Sepia -> Midnight -> Light cycle.
theme::ThemeName nextTheme(theme::ThemeName current) {
    using theme::ThemeName;
    switch (current) {
    case ThemeName::Light:
        return ThemeName::Dark;
    case ThemeName::Dark:
        return ThemeName::Sepia;
    case ThemeName::Sepia:
        return ThemeName::Midnight;
    case ThemeName::Midnight:
        return ThemeName::Light;
    }
    return ThemeName::Light;
}

} // namespace

QString RootWidget::pageMarkdownForKind(int kind) const {
    if (m_pageHub == nullptr) {
        return rwdetail::pageMarkdown(kind);
    }
    const QString profileRef =
        m_tabModel != nullptr ? m_tabModel->agentRefAt(m_tabModel->currentIndex()) : QString();
    const QString markdown = m_pageHub->pageMarkdownForKind(kind, profileRef);
    return markdown.isEmpty() ? rwdetail::pageMarkdown(kind) : markdown;
}

bool RootWidget::openManagerPage(const QString& id) {
    return m_pageHub != nullptr && m_pageHub->openManagerPage(id);
}

int RootWidget::activePageKind() const {
    return m_pageHub != nullptr ? m_pageHub->activePageKind(m_active != nullptr) : -1;
}

QList<QVariantMap> RootWidget::pageActionRows(int kind) const {
    return m_pageHub != nullptr ? m_pageHub->pageActionRows(kind) : QList<QVariantMap>{};
}

void RootWidget::refreshPageIfActive(int kind) {
    // Re-render `kind`'s page doc in place if it is the active page tab. Works for
    // both interactive hubs and the read-only Dashboard projection (so a live
    // roster/approvals change repaints the counters).
    if (m_active != nullptr || m_tabModel == nullptr) {
        return;
    }
    const int idx = m_tabModel->currentIndex();
    if (idx < 0 || m_tabModel->kindAt(idx) != kind) {
        return;
    }
    m_pageHub->clampSelection(kind);
    m_pageDoc.loadMarkdown(pageMarkdownForKind(kind));
    if (m_transcript != nullptr) {
        m_transcript->reload();
        // Keep the ▸ selection marker on screen: hub pages taller than the
        // viewport would otherwise re-render pinned to the bottom while j/k
        // walk rows scrolled out of view. No-op for marker-less pages.
        m_transcript->scrollLineWithTextIntoView(QStringLiteral("▸"));
    }
}

void RootWidget::refreshActivePage() {
    refreshPageIfActive(activePageKind());
}

void RootWidget::movePageSelection(int delta) {
    const int kind = activePageKind();
    if (kind < 0) {
        return;
    }
    m_pageHub->moveSelection(kind, delta);
    refreshActivePage();
}

void RootWidget::openSessionSettingsOverlay() {
    // Only meaningful over a transcript tab; bind the per-session overrides to
    // the focused chat first (parity with ComposerControls setting sessionId).
    if (m_active == nullptr || m_services.sessionSettings == nullptr) {
        return;
    }
    m_services.sessionSettings->setSessionId(m_active->sessionId);
    session::ISessionSettings* ss = m_services.sessionSettings;

    auto* dlg = new Tui::ZDialog(this);
    dlg->setOptions(Tui::ZWindow::DeleteOnClose);
    dlg->setWindowTitle(tr("Session settings"));
    dlg->setContentsMargins({2, 1, 2, 1});
    auto* layout = new Tui::ZVBoxLayout();
    dlg->setLayout(layout);

    auto* hint =
        new Tui::ZLabel(tr("Enter / Space on a row cycles or toggles it · Esc closes"), dlg);
    layout->addWidget(hint);
    layout->addSpacing(1);

    auto* profileBtn = new Tui::ZButton(QString(), dlg);
    auto* effortBtn = new Tui::ZButton(QString(), dlg);
    auto* approvalBtn = new Tui::ZButton(QString(), dlg);
    auto* fastBtn = new Tui::ZButton(QString(), dlg);
    auto* verboseBtn = new Tui::ZButton(QString(), dlg);
    layout->addWidget(profileBtn);
    layout->addWidget(effortBtn);
    layout->addWidget(approvalBtn);
    layout->addWidget(fastBtn);
    layout->addWidget(verboseBtn);
    // [wave2:app-approvals-safety] D4: remembered exec-approval fingerprints for this session
    // (list + revoke) live in their own overlay, opened from here (parity with the GUI popover's
    // "Remembered approvals" subsection).
    auto* rememberedBtn = new Tui::ZButton(tr("Remembered approvals…"), dlg);
    layout->addWidget(rememberedBtn);
    connect(rememberedBtn, &Tui::ZButton::clicked, dlg,
            [this] { openRememberedApprovalsOverlay(); });
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* closeBtn = new Tui::ZButton(tr("Close"), dlg);
    closeBtn->setDefault(true);
    buttons->addWidget(closeBtn);

    // The per-session profile stores the profile ID (parity with the GUI popover, so session->turn
    // is a strict pass-through); display the human name for that id.
    auto* profileModel = qobject_cast<uimodels::VariantListModel*>(m_services.profiles->profiles());
    const auto nameForId = [profileModel](const QString& id) {
        if (profileModel != nullptr) {
            for (const QVariantMap& row : profileModel->rows()) {
                if (row.value(QStringLiteral("id")).toString() == id) {
                    return row.value(QStringLiteral("name")).toString();
                }
            }
        }
        return id;
    };
    const auto sync = [ss, profileBtn, effortBtn, approvalBtn, fastBtn, verboseBtn, nameForId] {
        profileBtn->setText(tr("Profile: %1").arg(nameForId(ss->profile())));
        effortBtn->setText(tr("Effort:  %1").arg(ss->effort()));
        approvalBtn->setText(tr("Approval: %1").arg(ss->approvalMode()));
        fastBtn->setText(tr("Fast:    %1").arg(ss->fast() ? tr("on") : tr("off")));
        verboseBtn->setText(tr("Verbose: %1").arg(ss->verbose() ? tr("on") : tr("off")));
    };
    sync();

    connect(effortBtn, &Tui::ZButton::clicked, dlg, [ss, sync] {
        const QStringList opts = ss->effortOptions();
        if (!opts.isEmpty()) {
            const int idx = static_cast<int>(qMax<qsizetype>(0, opts.indexOf(ss->effort())));
            ss->setEffort(opts.at((idx + 1) % static_cast<int>(opts.size())));
        }
        sync();
    });
    // Approval mode (E1/TOOL-7, CHA-4): cycle ask -> accept_edits -> auto_allow -> deny; the
    // daemon seam pushes SetSessionMode so the node parks/auto-resolves accordingly.
    connect(approvalBtn, &Tui::ZButton::clicked, dlg, [ss, sync] {
        const QStringList opts = ss->approvalModeOptions();
        if (!opts.isEmpty()) {
            const int idx = static_cast<int>(qMax<qsizetype>(0, opts.indexOf(ss->approvalMode())));
            ss->setApprovalMode(opts.at((idx + 1) % static_cast<int>(opts.size())));
        }
        sync();
    });
    connect(profileBtn, &Tui::ZButton::clicked, dlg, [ss, sync, profileModel] {
        if (profileModel == nullptr || profileModel->rows().isEmpty()) {
            return;
        }
        const QList<QVariantMap> rows = profileModel->rows();
        int idx = -1;
        for (int i = 0; i < rows.size(); ++i) {
            if (rows.at(i).value(QStringLiteral("id")).toString() == ss->profile()) {
                idx = i;
                break;
            }
        }
        // Cycle to the next profile by ID (idx == -1 -> the first entry).
        ss->setProfile(rows.at((idx + 1) % rows.size()).value(QStringLiteral("id")).toString());
        sync();
    });
    connect(fastBtn, &Tui::ZButton::clicked, dlg, [ss, sync] {
        ss->setFast(!ss->fast());
        sync();
    });
    connect(verboseBtn, &Tui::ZButton::clicked, dlg, [ss, sync] {
        ss->setVerbose(!ss->verbose());
        sync();
    });
    connect(closeBtn, &Tui::ZButton::clicked, dlg, &Tui::ZDialog::close);

    dlg->setGeometry(QRect(0, 0, 48, 16));
    effortBtn->setFocus();
}

// [wave2:app-approvals-safety] D4: the active session's remembered exec-approval fingerprints —
// list + revoke (FingerprintList/FingerprintRevoke, wire v29). Mirrors the openCheckpointsOverlay
// pattern: render the current rows and re-render when the facade announces fresh ones. Provenance
// is limited to the fingerprint hash (+ label when the node ever fills it) — the node stores only
// the hash, so no "what/when" is shown.
void RootWidget::openRememberedApprovalsOverlay() {
    if (m_active == nullptr || m_services.sessionSettings == nullptr) {
        return;
    }
    session::ISessionSettings* ss = m_services.sessionSettings;
    ss->setSessionId(m_active->sessionId);
    ss->refreshFingerprints();
    auto* model = qobject_cast<uimodels::VariantListModel*>(ss->rememberedFingerprints());

    auto* dlg = new Tui::ZDialog(this);
    dlg->setOptions(Tui::ZWindow::DeleteOnClose);
    dlg->setWindowTitle(tr("Remembered approvals"));
    dlg->setContentsMargins({2, 1, 2, 1});
    auto* layout = new Tui::ZVBoxLayout();
    dlg->setLayout(layout);

    layout->addWidget(new Tui::ZLabel(
        tr("Commands allowed permanently this session · Enter/Revoke drops one · Esc closes"),
        dlg));

    auto* list = new Tui::ZListView(dlg);
    auto fps = std::make_shared<QStringList>(); // full fingerprint hex per visible row
    const auto refreshRows = [ss, list, fps] {
        auto* m = qobject_cast<uimodels::VariantListModel*>(ss->rememberedFingerprints());
        const QList<QVariantMap> rows = m != nullptr ? m->rows() : QList<QVariantMap>{};
        QStringList display;
        fps->clear();
        for (const QVariantMap& r : rows) {
            fps->append(r.value(QStringLiteral("fingerprint")).toString());
            const QString label = r.value(QStringLiteral("label")).toString();
            QString line =
                label.isEmpty() ? r.value(QStringLiteral("shortFingerprint")).toString() : label;
            // [waveB:app-v30] D6: append the node's remembered-at timestamp when present (parity
            // with the GUI SessionSettingsPopover row).
            const QString when = r.value(QStringLiteral("rememberedAt")).toString();
            if (!when.isEmpty()) {
                line += QStringLiteral("  ·  ") + when;
            }
            display << line;
        }
        if (display.isEmpty()) {
            display << tr("(no remembered approvals)");
        }
        list->setItems(display);
        if (list->model() != nullptr && !rows.isEmpty()) {
            list->setCurrentIndex(list->model()->index(0, 0));
        }
    };
    refreshRows();
    if (model != nullptr) {
        connect(model, &QAbstractItemModel::modelReset, dlg, refreshRows);
        connect(model, &QAbstractItemModel::rowsInserted, dlg, refreshRows);
        connect(model, &QAbstractItemModel::rowsRemoved, dlg, refreshRows);
    }
    layout->addWidget(list);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* revokeBtn = new Tui::ZButton(tr("Revoke"), dlg);
    buttons->addWidget(revokeBtn);
    auto* closeBtn = new Tui::ZButton(tr("Close"), dlg);
    closeBtn->setDefault(true);
    buttons->addWidget(closeBtn);

    const auto revokeSelected = [ss, list, fps] {
        const int row = list->currentIndex().row();
        if (row < 0 || row >= fps->size()) {
            return;
        }
        ss->revokeFingerprint(fps->at(row));
    };
    connect(revokeBtn, &Tui::ZButton::clicked, dlg, revokeSelected);
    connect(list, &Tui::ZListView::enterPressed, dlg, [revokeSelected](int) { revokeSelected(); });
    connect(closeBtn, &Tui::ZButton::clicked, dlg, &Tui::ZDialog::close);

    dlg->setGeometry(QRect(0, 0, 60, 14));
    list->setFocus();
}

void RootWidget::buildCheckpointDisplay(const QList<QVariantMap>& rows, QStringList& display,
                                        QStringList& ids) const {
    for (const QVariantMap& c : rows) {
        ids << c.value(QStringLiteral("id")).toString();
        // tokens < 0 = unknown (the daemon wire carries no token counter): omit the badge.
        const int tokens = c.value(QStringLiteral("tokens")).toInt();
        const QString tokenBadge =
            tokens >= 0 ? tr("  ·  %1 tok").arg(c.value(QStringLiteral("tokens")).toString())
                        : QString();
        display << tr("%1  ·  %2%3%4")
                       .arg(c.value(QStringLiteral("label")).toString(),
                            c.value(QStringLiteral("time")).toString(), tokenBadge,
                            c.value(QStringLiteral("current")).toBool() ? tr("  (current)")
                                                                        : QString());
    }
    if (display.isEmpty()) {
        display << tr("(no checkpoints)");
    }
}

// Whether the ACTIVE tab's session runs on a foreign ACP engine (per-session profile override,
// falling back to the active default profile). Gates rewind affordances: a foreign session's
// checkpoints are the foreign agent's to manage (C4/E4).
bool RootWidget::activeSessionIsForeign() const {
    if (m_active == nullptr || m_services.sessionSettings == nullptr ||
        m_services.profiles == nullptr) {
        return false;
    }
    QString pid = m_services.sessionSettings->profileFor(m_active->sessionId);
    if (pid.isEmpty()) {
        pid = m_services.profiles->defaultProfileId();
    }
    const QVariantMap p = m_services.profiles->profile(pid);
    return p.value(QStringLiteral("engine")).toString() == QStringLiteral("Foreign");
}

void RootWidget::openCheckpointsOverlay() {
    if (m_active == nullptr || m_services.checkpoints == nullptr) {
        return;
    }
    m_services.checkpoints->setSessionId(m_active->sessionId);
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_services.checkpoints->checkpoints());
    const bool foreign = activeSessionIsForeign();

    auto* dlg = new Tui::ZDialog(this);
    dlg->setOptions(Tui::ZWindow::DeleteOnClose);
    dlg->setWindowTitle(tr("Checkpoints"));
    dlg->setContentsMargins({2, 1, 2, 1});
    auto* layout = new Tui::ZVBoxLayout();
    dlg->setLayout(layout);

    auto* hint =
        new Tui::ZLabel(foreign ? tr("Rewind is managed by the foreign agent · Esc closes")
                                : tr("Enter rewinds to the selected checkpoint (confirmed) "
                                     "· Esc closes"),
                        dlg);
    layout->addWidget(hint);

    auto* list = new Tui::ZListView(dlg);
    // The daemon timeline re-fetches async on setSessionId: render the current rows now and
    // re-render when the facade announces fresh ones while the overlay is open. `ids` lives on
    // the heap and refreshes with the list so Enter always targets the row it shows.
    auto ids = std::make_shared<QStringList>();
    const auto refreshRows = [this, list, ids] {
        auto* m = qobject_cast<uimodels::VariantListModel*>(m_services.checkpoints->checkpoints());
        const QList<QVariantMap> rows = m != nullptr ? m->rows() : QList<QVariantMap>{};
        QStringList display;
        ids->clear();
        buildCheckpointDisplay(rows, display, *ids);
        list->setItems(display);
        if (list->model() != nullptr && !rows.isEmpty()) {
            list->setCurrentIndex(list->model()->index(0, 0));
        }
    };
    refreshRows();
    connect(m_services.checkpoints, &session::ICheckpointTimeline::changed, dlg, refreshRows);
    layout->addWidget(list);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    Tui::ZButton* restoreBtn = nullptr;
    if (!foreign) {
        restoreBtn = new Tui::ZButton(tr("Rewind…"), dlg);
        buttons->addWidget(restoreBtn);
    }
    auto* closeBtn = new Tui::ZButton(tr("Close"), dlg);
    closeBtn->setDefault(true);
    buttons->addWidget(closeBtn);

    // Rewind is destructive (drops the session's turns after the point): confirm before issuing.
    const auto confirmRestore = [this, dlg, list, ids, foreign] {
        const int row = list->currentIndex().row();
        if (foreign || row < 0 || row >= ids->size()) {
            return;
        }
        const QString id = ids->at(row);
        auto* confirm = new Tui::ZDialog(this);
        confirm->setOptions(Tui::ZWindow::DeleteOnClose);
        confirm->setWindowTitle(tr("Rewind to this checkpoint?"));
        confirm->setContentsMargins({2, 1, 2, 1});
        auto* confirmLayout = new Tui::ZVBoxLayout();
        confirm->setLayout(confirmLayout);
        confirmLayout->addWidget(new Tui::ZLabel(
            tr("This drops the session's turns after the selected point."), confirm));
        confirmLayout->addSpacing(1);
        auto* confirmButtons = new Tui::ZHBoxLayout();
        confirmLayout->add(confirmButtons);
        confirmButtons->addStretch();
        auto* rewindBtn = new Tui::ZButton(tr("Rewind"), confirm);
        confirmButtons->addWidget(rewindBtn);
        auto* cancelBtn = new Tui::ZButton(tr("Cancel"), confirm);
        cancelBtn->setDefault(true);
        confirmButtons->addWidget(cancelBtn);
        connect(rewindBtn, &Tui::ZButton::clicked, confirm, [this, confirm, dlg, id] {
            m_services.checkpoints->restore(id);
            confirm->close();
            dlg->close();
        });
        connect(cancelBtn, &Tui::ZButton::clicked, confirm, &Tui::ZDialog::close);
        confirm->setGeometry(QRect(0, 0, 58, 7));
        cancelBtn->setFocus();
    };
    if (restoreBtn != nullptr) {
        connect(restoreBtn, &Tui::ZButton::clicked, dlg, confirmRestore);
        connect(list, &Tui::ZListView::enterPressed, dlg,
                [confirmRestore](int) { confirmRestore(); });
    }
    connect(closeBtn, &Tui::ZButton::clicked, dlg, &Tui::ZDialog::close);

    dlg->setGeometry(QRect(0, 0, 62, qBound(9, m_services.checkpoints->count() + 8, 20)));
    list->setFocus();
}

void RootWidget::openInlineDenyReasonPrompt(const QString& callId) {
    if (m_active == nullptr) {
        return;
    }
    // Capture the active session's host + engine by value: a modal prompt could outlive a tab
    // switch, and the gate belongs to the session that was focused when Deny-with-reason was hit.
    auto* host = m_active->host;
    auto* turn = m_active->turn;
    auto* dlg = new TextPromptDialog(tr("Deny with reason (the agent will hear it)"), QString(),
                                     /*masked=*/false, this);
    connect(dlg, &TextPromptDialog::submitted, this, [host, turn, callId](const QString& text) {
        // Clear the local gate (shared toolApprovalPatch contract) and resolve over the wire; the
        // reason rides Approved.reason (wire v29) so the node relays it to the model.
        if (host != nullptr) {
            host->onApprovalDecided(callId, QStringLiteral("denied"), false);
        }
        if (turn != nullptr) {
            turn->respondApproval(callId, /*allow=*/false, /*allowPermanent=*/false,
                                  text.trimmed());
        }
    });
}

void RootWidget::openApprovalDenyReasonPrompt(const QString& id) {
    if (m_services.approvals == nullptr || id.isEmpty()) {
        return;
    }
    auto* dlg = new TextPromptDialog(tr("Deny with reason (the agent will hear it)"), QString(),
                                     /*masked=*/false, this);
    connect(dlg, &TextPromptDialog::submitted, this,
            [this, id](const QString& text) { m_services.approvals->deny(id, text.trimmed()); });
}

void RootWidget::openAppFeedbackPrompt() {
    if (m_services.feedback == nullptr) {
        return;
    }
    // Stable wire keys for the category (parity with the GUI dialog's combo); the
    // list shows localized labels.
    const QStringList keys{QStringLiteral("bug"), QStringLiteral("idea"), QStringLiteral("other")};
    auto* dlg = new Tui::ZDialog(this);
    dlg->setOptions(Tui::ZWindow::DeleteOnClose);
    dlg->setWindowTitle(tr("Send feedback"));
    dlg->setContentsMargins({2, 1, 2, 1});
    auto* layout = new Tui::ZVBoxLayout();
    dlg->setLayout(layout);
    layout->addWidget(new Tui::ZLabel(tr("What kind of feedback?"), dlg));
    auto* list = new Tui::ZListView(dlg);
    list->setItems({tr("Bug"), tr("Idea"), tr("Other")});
    if (list->model() != nullptr) {
        list->setCurrentIndex(list->model()->index(0, 0));
    }
    layout->addWidget(list);
    layout->addSpacing(1);
    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* nextBtn = new Tui::ZButton(tr("Next\u2026"), dlg);
    buttons->addWidget(nextBtn);
    auto* cancelBtn = new Tui::ZButton(tr("Cancel"), dlg);
    buttons->addWidget(cancelBtn);
    connect(cancelBtn, &Tui::ZButton::clicked, dlg, &Tui::ZDialog::close);

    auto* feedback = m_services.feedback;
    const auto pickCategory = [this, dlg, list, keys, feedback] {
        const int row = list->currentIndex().row();
        if (row < 0 || row >= keys.size()) {
            return;
        }
        const QString& category = keys.at(row);
        dlg->close();
        // Then the free-text note. Diagnostics ride along by default; the telemetry
        // opt-in lives on the Settings consent row (a text prompt cannot host a
        // checkbox), so this flow never flips telemetry silently.
        auto* note = new TextPromptDialog(tr("Your feedback (Enter to send)"), QString(),
                                          /*masked=*/false, this);
        connect(note, &TextPromptDialog::submitted, this,
                [feedback, category](const QString& text) {
                    feedback->submitAppFeedback(category, text.trimmed(),
                                                /*includeDiagnostics=*/true,
                                                /*alsoEnableTelemetry=*/false);
                });
    };
    connect(nextBtn, &Tui::ZButton::clicked, dlg, pickCategory);
    connect(list, &Tui::ZListView::enterPressed, dlg, [pickCategory](int) { pickCategory(); });
    dlg->setGeometry(QRect(0, 0, 52, 11));
    list->setFocus();
}

// [waveB:app-v30] D1: destructive confirm before TransportRemove. The node sequences the full
// teardown (disconnect + conv close + routing unbind + credential drop + config drop); we send one
// intent and re-read the reported state. Mirrors the GUI ChannelsPage removeAccountDialog.
void RootWidget::openChannelRemoveConfirm(const QString& transport, const QString& label) {
    if (m_services.transportRegistry == nullptr || transport.isEmpty()) {
        return;
    }
    auto* confirm = new Tui::ZDialog(this);
    confirm->setOptions(Tui::ZWindow::DeleteOnClose);
    confirm->setWindowTitle(tr("Remove account?"));
    confirm->setContentsMargins({2, 1, 2, 1});
    auto* layout = new Tui::ZVBoxLayout();
    confirm->setLayout(layout);
    layout->addWidget(new Tui::ZLabel(
        tr("Remove “%1”? The node disconnects it, closes its conversations, unbinds its "
           "routes, and drops the stored credential. This cannot be undone.")
            .arg(label.isEmpty() ? transport : label),
        confirm));
    layout->addSpacing(1);
    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* removeBtn = new Tui::ZButton(tr("Remove account"), confirm);
    buttons->addWidget(removeBtn);
    auto* cancelBtn = new Tui::ZButton(tr("Cancel"), confirm);
    buttons->addWidget(cancelBtn);
    connect(removeBtn, &Tui::ZButton::clicked, confirm, [this, confirm, transport] {
        m_services.transportRegistry->remove(transport);
        confirm->close();
    });
    connect(cancelBtn, &Tui::ZButton::clicked, confirm, &Tui::ZDialog::close);
    confirm->setGeometry(QRect(0, 0, 62, 8));
    removeBtn->setFocus();
}

// [acct-mgmt] wire v35: set/clear the node-persisted account label (TransportSetLabel). A text
// prompt seeded with the current label; an empty value clears it node-side. Node-decides — the row
// re-renders off the refetched instances. Mirrors the GUI ChannelsPage renameAccountDialog.
void RootWidget::openChannelRename(const QString& transport, const QString& currentLabel) {
    if (m_services.transportRegistry == nullptr || transport.isEmpty()) {
        return;
    }
    auto* dialog = new TextPromptDialog(tr("Rename account (empty clears the label)"), currentLabel,
                                        /*masked=*/false, this);
    connect(dialog, &TextPromptDialog::submitted, this, [this, transport](const QString& text) {
        if (m_services.transportRegistry != nullptr) {
            m_services.transportRegistry->setLabel(transport, text.trimmed());
        }
    });
}

// [acct-mgmt] wire v35: rename a credential (CredentialSetLabel via the accounts seam; an empty
// value clears the label node-side). A text prompt seeded with the current label. GUI AccountsPage
// inline-rename parity — the node owns the label, the row re-renders off the refetched list.
void RootWidget::openAccountRename(const QString& accountId, const QString& currentLabel) {
    if (m_services.accounts == nullptr || accountId.isEmpty()) {
        return;
    }
    auto* dialog = new TextPromptDialog(tr("Rename account (empty clears the label)"), currentLabel,
                                        /*masked=*/false, this);
    connect(dialog, &TextPromptDialog::submitted, this, [this, accountId](const QString& text) {
        if (m_services.accounts != nullptr) {
            m_services.accounts->rename(accountId, text.trimmed());
        }
    });
}

// [acct-mgmt] Lazily build the shared room flow (join/create/invite/members chained dialogs) the
// way openAddAccount builds AddAccountFlow: refresh the page on a seam mutation, restore page focus
// when the dialogs close.
void RootWidget::openRoomJoinFlow(const QString& transport) {
    if (m_services.transportRegistry == nullptr || transport.isEmpty()) {
        return;
    }
    if (m_roomFlow == nullptr) {
        m_roomFlow = new RoomFlow(m_services.transportRegistry, this);
        connect(m_roomFlow, &RoomFlow::changed, this, [this] { refreshActivePage(); });
        connect(m_roomFlow, &RoomFlow::finished, this, [this] {
            if (m_transcript != nullptr && activePageKind() >= 0) {
                m_transcript->setFocus();
            }
        });
    }
    m_roomFlow->startJoin(transport);
}

void RootWidget::openRoomCreateFlow(const QString& transport) {
    if (m_services.transportRegistry == nullptr || transport.isEmpty()) {
        return;
    }
    openRoomJoinFlow(transport); // ensure m_roomFlow exists + is wired
    m_roomFlow->startCreate(transport);
}

void RootWidget::openRoomInvite(const QString& transport, const QString& conversation) {
    if (m_services.transportRegistry == nullptr || transport.isEmpty() || conversation.isEmpty()) {
        return;
    }
    openRoomJoinFlow(transport); // ensure m_roomFlow exists + is wired (does not start a join here)
    m_roomFlow->startInvite(transport, conversation);
}

void RootWidget::openRoomMembers(const QString& transport, const QString& conversation) {
    if (m_services.transportRegistry == nullptr || transport.isEmpty() || conversation.isEmpty()) {
        return;
    }
    openRoomJoinFlow(transport); // ensure m_roomFlow exists + is wired
    m_roomFlow->openMembers(transport, conversation);
}

void RootWidget::openRoomLeaveConfirm(const QString& transport, const QString& conversation,
                                      const QString& label) {
    if (m_services.transportRegistry == nullptr || transport.isEmpty() || conversation.isEmpty()) {
        return;
    }
    auto* confirm = new ConfirmDialog(tr("Leave room"),
                                      tr("Leave “%1”? You can re-join later if the room allows it.")
                                          .arg(label.isEmpty() ? conversation : label),
                                      this);
    connect(confirm, &ConfirmDialog::confirmed, this, [this, transport, conversation] {
        m_services.transportRegistry->leaveRoom(transport, conversation);
    });
}

void RootWidget::openRoomDeleteConfirm(const QString& transport, const QString& conversation,
                                       const QString& label) {
    if (m_services.transportRegistry == nullptr || transport.isEmpty() || conversation.isEmpty()) {
        return;
    }
    auto* confirm = new ConfirmDialog(tr("Delete room"),
                                      tr("Delete “%1” on the node? This cannot be undone.")
                                          .arg(label.isEmpty() ? conversation : label),
                                      this);
    connect(confirm, &ConfirmDialog::confirmed, this, [this, transport, conversation] {
        m_services.transportRegistry->deleteRoom(transport, conversation);
    });
}

// [acct-mgmt] Pin a room to a session: a PaletteDialog session picker, then IDaemonNet::bindChat
// on the canonical origin (Group{chat} for a room, Dm{user} for a dm — honoring the
// pinnedDmSessionFor precedent). Reuses the routing seam directly (no RoutingManagerController).
void RootWidget::openRoomPin(const QString& transport, const QString& conversation,
                             const QString& kind) {
    if (m_services.daemonNet == nullptr || m_services.roster == nullptr || transport.isEmpty() ||
        conversation.isEmpty()) {
        return;
    }
    auto* picker = new PaletteDialog(tr("Pin room to session"), this);
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_services.roster->sessions());
    QVector<PaletteDialog::Item> items;
    if (model != nullptr) {
        for (const QVariantMap& s : model->rows()) {
            const QString id = s.value(QStringLiteral("id")).toString();
            if (id.isEmpty()) {
                continue;
            }
            items.push_back({id, s.value(QStringLiteral("title")).toString(), id});
        }
    }
    picker->setItems(items);
    const bool isDm = kind == QLatin1String("dm");
    connect(picker, &PaletteDialog::activated, this,
            [this, transport, conversation, isDm](const QString& sessionId) {
                domain::Origin origin;
                origin.transport = domain::TransportId(transport);
                if (isDm) {
                    origin.scope.kind = domain::OriginScopeKind::Dm;
                    origin.scope.user = conversation;
                } else {
                    origin.scope.kind = domain::OriginScopeKind::Group;
                    origin.scope.chat = conversation;
                }
                m_services.daemonNet->bindChat(origin, domain::SessionId(sessionId),
                                               domain::ProfileRef());
                refreshActivePage();
            });
    connect(picker, &PaletteDialog::canceled, this, [this] {
        if (m_transcript != nullptr && activePageKind() >= 0) {
            m_transcript->setFocus();
        }
    });
    picker->openCentered();
}

// [acct-mgmt] Lazily build the shared contact flow (add/find/alias/profile chained dialogs +
// palettes), mirroring openRoomJoinFlow: refresh the page on a seam mutation, restore page focus
// when the dialogs close.
void RootWidget::openContactAdd(const QString& transport) {
    if (m_services.contacts == nullptr || transport.isEmpty()) {
        return;
    }
    if (m_contactFlow == nullptr) {
        m_contactFlow = new ContactFlow(m_services.contacts, m_services.transportRegistry, this);
        connect(m_contactFlow, &ContactFlow::changed, this, [this] { refreshActivePage(); });
        connect(m_contactFlow, &ContactFlow::finished, this, [this] {
            if (m_transcript != nullptr && activePageKind() >= 0) {
                m_transcript->setFocus();
            }
        });
    }
    m_contactFlow->startAdd(transport);
}

void RootWidget::openContactFind(const QString& transport) {
    if (m_services.contacts == nullptr || transport.isEmpty()) {
        return;
    }
    openContactAdd(transport); // ensure m_contactFlow exists + is wired (does not start an add)
    m_contactFlow->startFind(transport);
}

void RootWidget::openContactAlias(const QString& transport, const QString& contactId,
                                  const QString& currentName) {
    if (m_services.contacts == nullptr || transport.isEmpty() || contactId.isEmpty()) {
        return;
    }
    openContactAdd(transport); // ensure m_contactFlow exists + is wired
    m_contactFlow->startAlias(transport, contactId, currentName);
}

void RootWidget::openContactProfile(const QString& transport, const QString& contactId) {
    if (m_services.contacts == nullptr || transport.isEmpty() || contactId.isEmpty()) {
        return;
    }
    openContactAdd(transport); // ensure m_contactFlow exists + is wired
    m_contactFlow->openProfile(transport, contactId);
}

void RootWidget::openContactRemoveConfirm(const QString& transport, const QString& contactId) {
    if (m_services.contacts == nullptr || transport.isEmpty() || contactId.isEmpty()) {
        return;
    }
    auto* confirm = new ConfirmDialog(tr("Remove contact"),
                                      tr("Remove %1 from your contacts?").arg(contactId), this);
    connect(confirm, &ConfirmDialog::confirmed, this, [this, transport, contactId] {
        m_services.contacts->removeContact(transport, contactId);
    });
}

void RootWidget::openContactDm(const QString& transport, const QString& contactId) const {
    // The conv-create seam with the contact as participant (no new wire op).
    if (m_services.transportRegistry == nullptr || transport.isEmpty() || contactId.isEmpty()) {
        return;
    }
    QVariantMap form;
    form[QStringLiteral("participants")] = QVariantList{contactId};
    m_services.transportRegistry->createRoom(transport, form);
}

void RootWidget::openFleetSteerPrompt(const QVariantMap& row) {
    if (m_services.roster == nullptr) {
        return;
    }
    const QString sessionId = row.value(QStringLiteral("sessionId")).toString();
    const QString role = row.value(QStringLiteral("role")).toString();
    const bool child = role == QLatin1String("ManagedChild") ||
                       role == QLatin1String("EphemeralSubagent") ||
                       (role.isEmpty() && row.value(QStringLiteral("depth")).toInt() > 0);
    if (sessionId.isEmpty() || !child) {
        return; // roots/primaries keep their own composer; only children are steerable rows
    }
    const bool running = row.value(QStringLiteral("status")).toString() == QLatin1String("running");
    auto* dlg = new TextPromptDialog(running ? tr("Steer this agent")
                                             : tr("Steer this agent (idle — starts a turn)"),
                                     QString(), /*masked=*/false, this);
    connect(dlg, &TextPromptDialog::submitted, this,
            [this, sessionId, running](const QString& text) {
                const QString trimmed = text.trimmed();
                if (trimmed.isEmpty()) {
                    return;
                }
                // Steer nudges a RUNNING turn; an idle child needs StartTurn instead.
                if (running) {
                    m_services.roster->steer(sessionId, trimmed);
                } else {
                    m_services.roster->startTurn(sessionId, trimmed);
                }
            });
}

void RootWidget::openModelPicker() {
    if (m_overlays == nullptr) {
        return;
    }
    m_overlays->openModelPicker(m_composerSession, [this] {
        if (m_composer != nullptr)
            m_composer->setFocus();
    });
}

void RootWidget::openModelDownload() {
    if (m_overlays == nullptr) {
        return;
    }
    m_overlays->openModelDownload(m_services.modelCatalog, [this] { refreshActivePage(); });
}

void RootWidget::openAddAccount() {
    if (m_addAccounts == nullptr) {
        m_addAccounts = new AddAccountFlow(m_services.accounts, this);
        connect(m_addAccounts, &AddAccountFlow::changed, this, [this] { refreshActivePage(); });
        // When the wizard's dialogs go away, return focus to the page doc so
        // j/k + action keys keep working without a click.
        connect(m_addAccounts, &AddAccountFlow::finished, this, [this] {
            if (m_transcript != nullptr && activePageKind() >= 0) {
                m_transcript->setFocus();
            }
        });
    }
    m_addAccounts->open();
}

void RootWidget::openAuthFlow() {
    if (m_services.authFlowController == nullptr) {
        return;
    }
    if (m_authFlow == nullptr) {
        m_authFlow = new AuthFlowLauncher(m_services.authFlowController, this);
        connect(m_authFlow, &AuthFlowLauncher::finished, this, [this] {
            refreshActivePage();
            if (m_transcript != nullptr && activePageKind() >= 0) {
                m_transcript->setFocus();
            }
        });
    }
    m_authFlow->open();
}

// [waveB:app-v30] CON-15: open the shared auth launcher narrowed to a provider's node-advertised
// sign_in family (the AgentInferencePicker "Sign in" button's TUI analog).
void RootWidget::openAuthFlowForFamily(const QString& family) {
    if (m_services.authFlowController == nullptr || family.isEmpty()) {
        return;
    }
    if (m_authFlow == nullptr) {
        m_authFlow = new AuthFlowLauncher(m_services.authFlowController, this);
        connect(m_authFlow, &AuthFlowLauncher::finished, this, [this] {
            refreshActivePage();
            if (m_transcript != nullptr && activePageKind() >= 0) {
                m_transcript->setFocus();
            }
        });
    }
    m_authFlow->openForFamily(family);
}

void RootWidget::openProfileEditor(const QString& profileId) {
    if (m_services.profiles == nullptr || profileId.isEmpty()) {
        return;
    }
    auto* dlg =
        new ProfileEditorDialog(m_services.profiles, m_services.providerCatalog, profileId, this);
    // [waveB:app-v30] CON-15: the editor's provider-row sign-in opens the shared auth launcher.
    connect(dlg, &ProfileEditorDialog::signInRequested, this, &RootWidget::openAuthFlowForFamily);
    // The editor's local "Discover More Models" row routes to the shared
    // download flow (the GUI editor's Nav.open("models","discover") analog).
    connect(dlg, &ProfileEditorDialog::modelDiscoverRequested, this,
            &RootWidget::openModelDownload);
    // Repaint the underlying page immediately on save (the row-model churn
    // refresh also covers the async daemon reflection).
    connect(dlg, &ProfileEditorDialog::saved, this, [this](const QString&) {
        refreshActivePage();
        refreshPageIfActive(TabModel::Profile);
    });
    // Tui does not restore focus for us: hand it back to the page view.
    connect(dlg, &QObject::destroyed, this, [this] {
        if (m_active == nullptr && m_transcript != nullptr) {
            m_transcript->setFocus();
        }
    });
}

void RootWidget::openCommandPalette() {
    if (m_overlays == nullptr) {
        return;
    }
    TuiOverlayHost::CommandCallbacks callbacks;
    callbacks.openManagerPage = [this](const QString& id) { return openManagerPage(id); };
    callbacks.newTranscriptTab = [this] { newTranscriptTab(); };
    callbacks.cycleTheme = [this] { cycleTheme(); };
    callbacks.openModelPicker = [this] { openModelPicker(); };
    callbacks.focusSearch = [this] {
        if (m_search != nullptr)
            m_listView->setFocus();
    };
    callbacks.toggleExplorer = [this] { toggleExplorer(); };
    callbacks.openTranscriptSearch = [this] { openTranscriptSearch(); };
    callbacks.invokeActiveCommand = [this](const QString& id) {
        if (id == QStringLiteral("reasoning")) {
            m_composerSession->cycleReasoningEffort();
        } else if (id == QStringLiteral("fast")) {
            m_composerSession->toggleFastMode();
        } else if (id == QStringLiteral("verbose")) {
            m_composerSession->toggleVerbose();
        } else if (id == QStringLiteral("distraction")) {
            // Window-level like theme: works with a page tab active too.
            toggleDistractionFree();
        } else if (m_active != nullptr) {
            m_composerSession->invokeCommand(id);
        }
    };
    callbacks.focusComposer = [this] {
        if (m_composer != nullptr)
            m_composer->setFocus();
    };
    m_overlays->openCommandPalette(m_commands, callbacks);
}

void RootWidget::openFileFinder() {
    if (m_overlays == nullptr || m_fileFinder == nullptr || m_tabModel == nullptr) {
        return;
    }
    m_overlays->openFileFinder(m_fileFinder,
                               [this](const QString& rootId, const QString& path, bool pinned) {
                                   // Enter -> preview (transient, VSCode-style), Shift+Enter ->
                                   // pinned: the explorer's single- vs double-click semantics on
                                   // keys. An empty title lets TabModel derive the basename, like
                                   // Session.qml openFile.
                                   if (pinned) {
                                       m_tabModel->openFilePinned(rootId, path, QString());
                                   } else {
                                       m_tabModel->previewFile(rootId, path, QString());
                                   }
                               });
}

void RootWidget::openAttachmentPicker() {
    if (m_overlays == nullptr || m_fileFinder == nullptr) {
        return;
    }
    m_overlays->openAttachPicker(
        m_fileFinder,
        [this](const QString& /*rootId*/, const QString& path) {
            // GUI parity (Composer.qml workspaceFilePicker.onPicked): the chip
            // carries the root-relative path; buildRefs() renders it as a
            // @file:/@image: ref on submit. Kind via the GUI's drop heuristic.
            m_composerSession->addAttachment(path, rwdetail::attachmentKindForName(path));
        },
        [this] {
            // The picker was invoked from the composer; return focus there on
            // both accept and cancel.
            if (m_composer != nullptr) {
                m_composer->setFocus();
            }
        });
}

void RootWidget::repaintForTheme() {
    // The session list and transcript bake tpal::* colors into cached span
    // lists at build time, so a bare update() would repaint stale colors: rebuild
    // them. (Selection + scroll survive the rebuild.)
    if (m_listView != nullptr) {
        m_listView->relayout();
    }
    if (m_transcript != nullptr) {
        m_transcript->reload();
    }
    // The remaining custom-painted views sample tpal::* live at paint, so a plain
    // repaint suffices.
    const auto views = std::to_array<Tui::ZWidget*>(
        {m_window, m_sidebarView, m_integrationsView, m_composerChrome, m_queue, m_attachments,
         m_footer, m_completionPopup, m_search, m_composer, m_tabStrip, m_todos, m_subagents,
         m_fileTreeView, m_editorView});
    for (Tui::ZWidget* w : views) {
        if (w != nullptr) {
            w->update();
        }
    }
}

void RootWidget::cycleTheme() {
    // Advance through the four themes in a fixed order.
    applyTheme(nextTheme(tpal::activeTheme()));
}

void RootWidget::applyTheme(theme::ThemeName name) {
    using theme::ThemeName;

    tpal::setActiveTheme(name);
    // Recolor stock widgets (window/dialog/lists/inputs) via the palette...
    setPalette(daemonPalette(name));
    // Keep the accent-tinted text caret in sync with the new theme.
    if (terminal() != nullptr) {
        const Tui::ZColor accent = tpal::accent();
        terminal()->setCursorColor(accent.red(), accent.green(), accent.blue());
    }
    repaintForTheme();
    // The editor's syntax colors are baked by KSyntaxHighlighting per the theme;
    // re-pick the light/dark definition theme for every open file controller.
    const bool dark = name != ThemeName::Light && name != ThemeName::Sepia;
    if (m_fileTabs != nullptr)
        m_fileTabs->setDarkTheme(dark);

    // Persist to the GUI-shared key so both front ends honor the same choice.
    QSettings settings(QStringLiteral("daemon-app"), QStringLiteral("daemon-app"));
    settings.setValue(QStringLiteral("ui/theme"), theme::ThemePalette::toString(name));
}
