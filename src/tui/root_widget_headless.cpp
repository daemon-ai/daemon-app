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

void RootWidget::dumpGeometry() const {
    const auto g = [](const char* n, const Tui::ZWidget* w) {
        if (w == nullptr) {
            return;
        }
        const QRect r = w->geometry();
        fprintf(stderr, "%-10s x=%d y=%d w=%d h=%d\n", n, r.x(), r.y(), r.width(), r.height());
    };
    g("window", m_window);
    g("sidebar", m_sidebarView);
    g("list", m_listView);
    g("transcript", m_transcript);
    g("chrome", m_composerChrome);
    g("composer", m_composer);
    g("filetree", m_fileTreeView);
    g("editor", m_editorView);
    g("footer", m_footer);
}

void RootWidget::focusComposer() const {
    if (m_composer != nullptr) {
        m_composer->setFocus();
    }
}

void RootWidget::focusTranscript() const {
    if (m_transcript != nullptr) {
        m_transcript->setFocus();
    }
}

bool RootWidget::awaitConnectionReady(int timeoutMs) const {
    auto* conn = m_services.connection;
    if (conn == nullptr) {
        return false;
    }
    if (conn->ready()) {
        return true;
    }
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(conn, &connection::IConnectionService::stateChanged, &loop, [&] {
        if (conn->ready()) {
            loop.quit();
        }
    });
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(timeoutMs);
    loop.exec();
    return conn->ready();
}

void RootWidget::driveFirstRunConnect() const {
    if (m_services.firstRun == nullptr || !m_services.firstRun->active()) {
        return; // returning users already auto-connected in the constructor
    }
    const QString target = m_services.settings->resolvedConnectionTarget();
    m_services.settings->setLastConnection(QStringLiteral("local"), target);
    m_services.connection->connectTo(QStringLiteral("local"), target);
}

void RootWidget::settleFirstRunGate(int ms) const {
    firstrun::FirstRunModel* firstRun = m_services.firstRun;
    if (firstRun == nullptr || firstRun->phase() == QStringLiteral("done")) {
        return;
    }
    QEventLoop loop;
    const auto conn =
        connect(firstRun, &firstrun::FirstRunModel::phaseChanged, &loop, [firstRun, &loop] {
            if (firstRun->phase() == QStringLiteral("done")) {
                loop.quit();
            }
        });
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
    disconnect(conn);
}

QString RootWidget::runHeadlessChat(const QString& prompt, int timeoutMs) const {
    const auto settle = [](int ms) {
        QEventLoop loop;
        QTimer::singleShot(ms, &loop, &QEventLoop::quit);
        loop.exec();
    };
    driveFirstRunConnect();
    if (!awaitConnectionReady(timeoutMs)) {
        return {};
    }
    settle(600); // drain the on-ready refreshes first
    auto* engine = new DaemonTurnEngine(m_services.nodeApi, m_services.cache,
                                        m_services.subscriptions, const_cast<RootWidget*>(this));
    engine->setSessionId(QStringLiteral("s-") + QUuid::createUuid().toString(QUuid::WithoutBraces));
    QString answer;
    QEventLoop loop;
    QObject::connect(
        engine, &ITurnEngine::eventsEmitted, engine, [&answer](const QVariantList& events) {
            for (const QVariant& item : events) {
                const QVariantMap map = item.toMap();
                if (map.value(QStringLiteral("type")).toString() == QStringLiteral("text")) {
                    answer += map.value(QStringLiteral("text")).toString();
                }
            }
        });
    QObject::connect(engine, &ITurnEngine::turnFinished, &loop, &QEventLoop::quit);
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    engine->start(prompt);
    loop.exec();
    engine->cancel();
    engine->deleteLater();
    return answer;
}

bool RootWidget::runHeadlessOnboarding(const QString& provider, const QString& key,
                                       int timeoutMs) const {
    const auto settle = [](int ms) {
        QEventLoop loop;
        QTimer::singleShot(ms, &loop, &QEventLoop::quit);
        loop.exec();
    };
    driveFirstRunConnect();
    if (!awaitConnectionReady(timeoutMs)) {
        return false;
    }
    settle(1000); // let the on-ready ProfileList/CredentialList/Models/ModelCurrent flush
    if (!key.isEmpty() && m_services.accounts != nullptr) {
        m_services.accounts->addApiKey(provider, QString(), key, QString());
    }
    if (m_services.modelCatalog != nullptr) {
        const QStringList ids = m_services.modelCatalog->installedIds();
        if (!ids.isEmpty()) {
            m_services.modelCatalog->activate(ids.first());
        }
    }
    if (m_services.firstRun != nullptr) {
        m_services.firstRun->completeInference();
    }
    settle(1000); // flush the CredentialSet (+ its re-list) round-trip
    return true;
}

void RootWidget::promptQuit() {
    if (m_overlays == nullptr) {
        return;
    }
    m_overlays->promptQuit([this] {
        if (m_sidebarView != nullptr) {
            m_sidebarView->setFocus(); // restore keyboard focus to the panes
        }
    });
}
