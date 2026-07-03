// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "app/code_editor_controller.h"
#include "app/transcript_log.h"
#include "command_registry.h"
#include "composer_session_controller.h"
#include "daemon/daemon_connection_service.h" // complete type for the managed-daemon shutdown hook
#include "daemonnet/idaemonnet.h"             // complete type for setDaemonNet(QObject*)
#include "dialogs/add_account_flow.h"
#include "dialogs/profile_editor_dialog.h"
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
    auto* fastBtn = new Tui::ZButton(QString(), dlg);
    auto* verboseBtn = new Tui::ZButton(QString(), dlg);
    layout->addWidget(profileBtn);
    layout->addWidget(effortBtn);
    layout->addWidget(fastBtn);
    layout->addWidget(verboseBtn);
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
    const auto sync = [ss, profileBtn, effortBtn, fastBtn, verboseBtn, nameForId] {
        profileBtn->setText(tr("Profile: %1").arg(nameForId(ss->profile())));
        effortBtn->setText(tr("Effort:  %1").arg(ss->effort()));
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

    dlg->setGeometry(QRect(0, 0, 48, 13));
    effortBtn->setFocus();
}

void RootWidget::buildCheckpointDisplay(const QList<QVariantMap>& rows, QStringList& display,
                                        QStringList& ids) const {
    for (const QVariantMap& c : rows) {
        ids << c.value(QStringLiteral("id")).toString();
        display << tr("%1  ·  %2  ·  %3 tok%4")
                       .arg(c.value(QStringLiteral("label")).toString(),
                            c.value(QStringLiteral("time")).toString(),
                            c.value(QStringLiteral("tokens")).toString(),
                            c.value(QStringLiteral("current")).toBool() ? tr("  (current)")
                                                                        : QString());
    }
    if (display.isEmpty()) {
        display << tr("(no checkpoints)");
    }
}

void RootWidget::openCheckpointsOverlay() {
    if (m_active == nullptr || m_services.checkpoints == nullptr) {
        return;
    }
    m_services.checkpoints->setSessionId(m_active->sessionId);
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_services.checkpoints->checkpoints());
    const QList<QVariantMap> rows = model != nullptr ? model->rows() : QList<QVariantMap>{};

    auto* dlg = new Tui::ZDialog(this);
    dlg->setOptions(Tui::ZWindow::DeleteOnClose);
    dlg->setWindowTitle(tr("Checkpoints"));
    dlg->setContentsMargins({2, 1, 2, 1});
    auto* layout = new Tui::ZVBoxLayout();
    dlg->setLayout(layout);

    auto* hint = new Tui::ZLabel(tr("Enter restores the selected checkpoint · Esc closes"), dlg);
    layout->addWidget(hint);

    auto* list = new Tui::ZListView(dlg);
    QStringList display;
    QStringList ids;
    buildCheckpointDisplay(rows, display, ids);
    list->setItems(display);
    if (list->model() != nullptr && !rows.isEmpty()) {
        list->setCurrentIndex(list->model()->index(0, 0));
    }
    layout->addWidget(list);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* restoreBtn = new Tui::ZButton(tr("Restore"), dlg);
    buttons->addWidget(restoreBtn);
    auto* closeBtn = new Tui::ZButton(tr("Close"), dlg);
    closeBtn->setDefault(true);
    buttons->addWidget(closeBtn);

    const auto doRestore = [this, dlg, list, ids] {
        const int row = list->currentIndex().row();
        if (row >= 0 && row < ids.size()) {
            m_services.checkpoints->restore(ids.at(row));
        }
        dlg->close();
    };
    connect(restoreBtn, &Tui::ZButton::clicked, dlg, doRestore);
    connect(list, &Tui::ZListView::enterPressed, dlg, [doRestore](int) { doRestore(); });
    connect(closeBtn, &Tui::ZButton::clicked, dlg, &Tui::ZDialog::close);

    dlg->setGeometry(QRect(0, 0, 62, qBound(9, static_cast<int>(rows.size()) + 8, 20)));
    list->setFocus();
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

void RootWidget::openProfileEditor(const QString& profileId) {
    if (m_services.profiles == nullptr || profileId.isEmpty()) {
        return;
    }
    auto* dlg =
        new ProfileEditorDialog(m_services.profiles, m_services.providerCatalog, profileId, this);
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
        {m_window, m_sidebarView, m_composerChrome, m_queue, m_attachments, m_footer,
         m_completionPopup, m_search, m_composer, m_tabStrip, m_todos, m_subagents, m_fileTreeView,
         m_editorView});
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
