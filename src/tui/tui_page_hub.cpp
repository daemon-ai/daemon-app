#include "tui_page_hub.h"

#include "accounts/iaccounts_service.h"
#include "automation/icron_store.h"
#include "automation/irouting_store.h"
#include "config/idaemon_config.h"
#include "connection/iconnection_service.h"
#include "fleet/iapprovals_inbox.h"
#include "fleet/idashboard.h"
#include "fleet/ifleet_tree.h"
#include "fleet/isession_roster.h"
#include "memory/imemory_service.h"
#include "memory_graph_model.h"
#include "memory_list_model.h"
#include "memory_stats_model.h"
#include "memory_timeline_model.h"
#include "models/imodel_catalog.h"
#include "profiles/iprofile_store.h"
#include "settings/isettings_store.h"
#include "tab_model.h"
#include "uimodels/variant_list_model.h"

#include <Tui/ZEvent.h>

namespace {

QList<QVariantMap> rowsOfModel(QObject* m) {
    auto* vlm = qobject_cast<uimodels::VariantListModel*>(m);
    return vlm ? vlm->rows() : QList<QVariantMap>{};
}

} // namespace

TuiPageHub::TuiPageHub(Dependencies deps) : m_deps(deps) {}

QString TuiPageHub::pageMarkdownForKind(int kind, const QString& profileRef) const {
    const int sel = m_pageSel.value(kind, 0);
    switch (kind) {
    case TabModel::Settings:
        return buildSettingsMarkdown();
    case TabModel::Models:
        return buildModelsMarkdown(sel);
    case TabModel::Accounts:
        return buildAccountsMarkdown(sel);
    case TabModel::Profiles:
        return buildProfilesMarkdown(sel);
    case TabModel::Dashboard:
        return buildDashboardMarkdown();
    case TabModel::Fleet:
        return buildFleetMarkdown(sel);
    case TabModel::Sessions:
        return buildSessionsMarkdown(sel);
    case TabModel::Approvals:
        return buildApprovalsMarkdown(sel);
    case TabModel::Routing:
        return buildRoutingMarkdown(sel);
    case TabModel::Cron:
        return buildCronMarkdown(sel);
    case TabModel::Memory:
        return buildMemoryMarkdown();
    case TabModel::Profile:
        return buildProfileMarkdown(profileRef);
    default:
        return {};
    }
}

bool TuiPageHub::openManagerPage(const QString& id) const {
    const QHash<QString, QPair<int, QString>> kPageRoutes = {
        {QStringLiteral("settings"), {TabModel::Settings, tr("Settings")}},
        {QStringLiteral("dashboard"), {TabModel::Dashboard, tr("Dashboard")}},
        {QStringLiteral("models"), {TabModel::Models, tr("Models")}},
        {QStringLiteral("accounts"), {TabModel::Accounts, tr("Accounts")}},
        {QStringLiteral("profiles"), {TabModel::Profiles, tr("Profiles")}},
        {QStringLiteral("fleet"), {TabModel::Fleet, tr("Fleet")}},
        {QStringLiteral("sessions"), {TabModel::Sessions, tr("Sessions")}},
        {QStringLiteral("approvals"), {TabModel::Approvals, tr("Approvals")}},
        {QStringLiteral("routing"), {TabModel::Routing, tr("Routing")}},
        {QStringLiteral("cron"), {TabModel::Cron, tr("Scheduled jobs")}},
    };
    const auto route = kPageRoutes.constFind(id);
    if (route == kPageRoutes.constEnd()) {
        return false;
    }
    if (m_deps.tabModel != nullptr) {
        m_deps.tabModel->openPage(route->first, route->second);
    }
    return true;
}

int TuiPageHub::activePageKind(bool transcriptActive) const {
    if (transcriptActive || m_deps.tabModel == nullptr) {
        return -1;
    }
    const int idx = m_deps.tabModel->currentIndex();
    if (idx < 0) {
        return -1;
    }
    switch (m_deps.tabModel->kindAt(idx)) {
    case TabModel::Models:
    case TabModel::Accounts:
    case TabModel::Profiles:
    case TabModel::Sessions:
    case TabModel::Fleet:
    case TabModel::Approvals:
    case TabModel::Routing:
    case TabModel::Cron:
        return m_deps.tabModel->kindAt(idx);
    default:
        return -1;
    }
}

QList<QVariantMap> TuiPageHub::pageActionRows(int kind) const {
    switch (kind) {
    case TabModel::Models:
        return rowsOfModel(m_deps.modelCatalog->installed());
    case TabModel::Accounts:
        return rowsOfModel(m_deps.accounts->accounts());
    case TabModel::Profiles:
        return rowsOfModel(m_deps.profiles->profiles());
    case TabModel::Sessions:
        return rowsOfModel(m_deps.roster->sessions());
    case TabModel::Fleet:
        return rowsOfModel(m_deps.fleetTree->nodes());
    case TabModel::Approvals:
        return rowsOfModel(m_deps.approvals->pending());
    case TabModel::Routing:
        return rowsOfModel(m_deps.routing->rules());
    case TabModel::Cron:
        return rowsOfModel(m_deps.cron->jobs());
    default:
        return {};
    }
}

void TuiPageHub::clampSelection(int kind) {
    const int rows = static_cast<int>(pageActionRows(kind).size());
    if (rows > 0) {
        int& sel = m_pageSel[kind];
        sel = qBound(0, sel, rows - 1);
    }
}

void TuiPageHub::moveSelection(int kind, int delta) {
    const int rows = static_cast<int>(pageActionRows(kind).size());
    if (rows == 0) {
        return;
    }
    int& sel = m_pageSel[kind];
    sel = qBound(0, sel + delta, rows - 1);
}

bool TuiPageHub::handlePageActionKey(int kind, Tui::ZKeyEvent* event) {
    if (event->modifiers() != Qt::NoModifier || kind < 0) {
        return false;
    }
    const QList<QVariantMap> rows = pageActionRows(kind);
    const QString text = event->text();
    const int key = event->key();

    if (key == Qt::Key_Up || text == QStringLiteral("k")) {
        moveSelection(kind, -1);
        event->accept();
        return true;
    }
    if (key == Qt::Key_Down || text == QStringLiteral("j")) {
        moveSelection(kind, 1);
        event->accept();
        return true;
    }

    if (rows.isEmpty()) {
        return false;
    }
    const int sel = qBound(0, m_pageSel.value(kind, 0), static_cast<int>(rows.size()) - 1);
    const QVariantMap& row = rows.at(sel);
    const QString id = row.value(QStringLiteral("id")).toString();
    const bool enter = key == Qt::Key_Enter || key == Qt::Key_Return;
    bool acted = false;

    switch (kind) {
    case TabModel::Models:
        if (enter) {
            m_deps.modelCatalog->activate(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_deps.modelCatalog->remove(id);
            acted = true;
        }
        break;
    case TabModel::Accounts:
        if (enter || text == QStringLiteral("R")) {
            m_deps.accounts->reauth(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_deps.accounts->remove(id);
            acted = true;
        }
        break;
    case TabModel::Profiles:
        if (enter) {
            m_deps.profiles->setDefault(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_deps.profiles->remove(id);
            acted = true;
        }
        break;
    case TabModel::Sessions:
        if (text == QStringLiteral("s")) {
            m_deps.roster->suspend(id);
            acted = true;
        } else if (text == QStringLiteral("R") || enter) {
            m_deps.roster->resume(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_deps.roster->close(id);
            acted = true;
        }
        break;
    case TabModel::Fleet:
        if (key == Qt::Key_Space || enter || text == QStringLiteral("p")) {
            const bool paused =
                row.value(QStringLiteral("status")).toString() == QLatin1String("paused");
            if (paused) {
                m_deps.fleetTree->resume(id);
            } else {
                m_deps.fleetTree->pause(id);
            }
            acted = true;
        }
        break;
    case TabModel::Approvals:
        if (text == QStringLiteral("a") || enter) {
            m_deps.approvals->approve(id);
            acted = true;
        } else if (text == QStringLiteral("d")) {
            m_deps.approvals->deny(id);
            acted = true;
        }
        break;
    case TabModel::Routing:
        if (key == Qt::Key_Space || enter) {
            m_deps.routing->setEnabled(id, !row.value(QStringLiteral("enabled")).toBool());
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_deps.routing->remove(id);
            acted = true;
        }
        break;
    case TabModel::Cron:
        if (key == Qt::Key_Space || enter) {
            m_deps.cron->setEnabled(id, !row.value(QStringLiteral("enabled")).toBool());
            acted = true;
        } else if (text == QStringLiteral("t")) {
            m_deps.cron->runNow(id);
            acted = true;
        } else if (text == QStringLiteral("x")) {
            m_deps.cron->remove(id);
            acted = true;
        }
        break;
    default:
        break;
    }

    if (acted) {
        event->accept();
        return true;
    }
    return false;
}

QString TuiPageHub::buildModelsMarkdown(int sel) const {
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Models\n\n");
    md += tr("Installed models, shared with the GUI. **j/k** move · **Enter** "
             "activates · **x** removes.\n\n");

    md += tr("## Installed\n\n");
    const auto installed = rowsOfModel(m_deps.modelCatalog->installed());
    if (installed.isEmpty()) {
        md += tr("_None installed._\n\n");
    } else {
        for (int i = 0; i < installed.size(); ++i) {
            const QVariantMap& m = installed.at(i);
            md += tr("- %1**%2** (%3, %4 GiB)%5\n")
                      .arg(mark(i), m.value(QStringLiteral("name")).toString(),
                           m.value(QStringLiteral("params")).toString(),
                           m.value(QStringLiteral("sizeGiB")).toString(),
                           m.value(QStringLiteral("active")).toBool() ? tr(" — **active**")
                                                                      : QString());
        }
        md += QLatin1Char('\n');
    }

    md += tr("## Discover\n\n");
    for (const QVariantMap& m : rowsOfModel(m_deps.modelCatalog->discover())) {
        md += tr("- %1 — %2 · %3 GiB · %4%5\n")
                  .arg(m.value(QStringLiteral("name")).toString(),
                       m.value(QStringLiteral("params")).toString(),
                       m.value(QStringLiteral("sizeGiB")).toString(),
                       m.value(QStringLiteral("provider")).toString(),
                       m.value(QStringLiteral("installed")).toBool() ? tr(" (installed)")
                                                                     : QString());
    }

    md += tr("\n## Providers\n\n");
    const QVariantList provs = m_deps.modelCatalog->providers();
    for (const QVariant& v : provs) {
        const QVariantMap m = v.toMap();
        md += tr("- %1 (%2)%3\n")
                  .arg(m.value(QStringLiteral("name")).toString(),
                       m.value(QStringLiteral("kind")).toString(),
                       m.value(QStringLiteral("configured")).toBool() ? tr(" — configured")
                                                                      : QString());
    }

    return md;
}

QString TuiPageHub::buildRoutingMarkdown(int sel) const {
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_deps.routing->rules());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Routing\n\n");
    md += tr("Intent → model rules, shared with the GUI. **j/k** move · "
             "**Space/Enter** toggle · **x** delete.\n\n");
    if (model != nullptr) {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& r = rows.at(i);
            md += tr("- %1**%2** → `%3` (fallback `%4`)%5\n")
                      .arg(mark(i), r.value(QStringLiteral("intent")).toString(),
                           r.value(QStringLiteral("target")).toString(),
                           r.value(QStringLiteral("fallback")).toString(),
                           r.value(QStringLiteral("enabled")).toBool() ? QString()
                                                                       : tr(" — _disabled_"));
        }
    }
    return md;
}

QString TuiPageHub::buildCronMarkdown(int sel) const {
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_deps.cron->jobs());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Scheduled jobs\n\n");
    md += tr("**j/k** move · **Space/Enter** enable/disable · **t** run now · "
             "**x** delete.\n\n");
    if (model != nullptr) {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& j = rows.at(i);
            md += tr("## %1%2%3\n\n")
                      .arg(mark(i), j.value(QStringLiteral("name")).toString(),
                           j.value(QStringLiteral("enabled")).toBool() ? QString()
                                                                       : tr(" (disabled)"));
            md += tr("- Schedule: `%1`\n").arg(j.value(QStringLiteral("schedule")).toString());
            md += tr("- Profile: %1\n").arg(j.value(QStringLiteral("profile")).toString());
            md += tr("- Next: %1 · Last: %2\n\n")
                      .arg(j.value(QStringLiteral("nextRun")).toString(),
                           j.value(QStringLiteral("lastRun")).toString());
        }
    }
    return md;
}

QString TuiPageHub::buildMemoryMarkdown() const {
    QString md;
    const QString agent = m_deps.memory != nullptr ? m_deps.memory->profile() : QString();
    md += tr("# Memory - %1\n\n").arg(agent.isEmpty() ? tr("agent") : agent);
    md += tr("Mnemosyne memory for the active scope, shared with the GUI. The "
             "knowledge graph renders as a node-link diagram in the GUI; here it is "
             "an adjacency listing.\n\n");

    if (m_deps.memStats != nullptr) {
        md += tr("## Overview\n\n");
        md += tr("- Working: **%1** · Episodic: **%2** · Scratchpad: **%3**\n")
                  .arg(m_deps.memStats->working())
                  .arg(m_deps.memStats->episodic())
                  .arg(m_deps.memStats->scratchpad());
        md += tr("- Facts: **%1** · Conflicts: **%2**\n\n")
                  .arg(m_deps.memStats->facts())
                  .arg(m_deps.memStats->conflicts());

        const auto gauges = [&md](const QString& title, const QVariantList& rows) {
            if (rows.isEmpty())
                return;
            md += QStringLiteral("**%1**\n\n").arg(title);
            for (const QVariant& v : rows) {
                const QVariantMap r = v.toMap();
                const double frac = r.value(QStringLiteral("fraction")).toDouble();
                const int filled = qBound(0, qRound(frac * 12.0), 12);
                const QString bar = QString(filled, QChar('#')) + QString(12 - filled, QChar('-'));
                md += QStringLiteral("- `%1` %2 %3\n")
                          .arg(bar, r.value(QStringLiteral("key")).toString())
                          .arg(r.value(QStringLiteral("count")).toInt());
            }
            md += QStringLiteral("\n");
        };
        gauges(tr("By source"), m_deps.memStats->bySource());
        gauges(tr("By veracity"), m_deps.memStats->byVeracity());
        gauges(tr("By lifecycle"), m_deps.memStats->byDegradation());
    }

    if (m_deps.memList != nullptr) {
        md += tr("## Memories\n\n");
        const int n = m_deps.memList->rowCount();
        if (n == 0)
            md += tr("_No memories in scope._\n\n");
        for (int i = 0; i < n; ++i) {
            const QVariantMap e = m_deps.memList->entryAt(i);
            md += tr("- **%1** _(%2 · %3 · imp %4)_\n")
                      .arg(e.value(QStringLiteral("content")).toString(),
                           e.value(QStringLiteral("tier")).toString(),
                           e.value(QStringLiteral("veracity")).toString())
                      .arg(e.value(QStringLiteral("importance")).toDouble(), 0, 'f', 2);
        }
        md += QStringLiteral("\n");
    }

    if (m_deps.memGraph != nullptr) {
        md += tr("## Graph adjacency\n\n");
        const QVariantList edges = m_deps.memGraph->edges();
        if (edges.isEmpty())
            md += tr("_No edges in scope._\n\n");
        for (const QVariant& v : edges) {
            const QVariantMap e = v.toMap();
            md += QStringLiteral("- `%1` --%2--> `%3`\n")
                      .arg(e.value(QStringLiteral("source")).toString(),
                           e.value(QStringLiteral("edgeType")).toString(),
                           e.value(QStringLiteral("target")).toString());
        }
        md += QStringLiteral("\n");
    }

    if (m_deps.memTimeline != nullptr) {
        md += tr("## Timeline\n\n");
        const int n = m_deps.memTimeline->rowCount();
        for (int i = 0; i < n; ++i) {
            const QModelIndex idx = m_deps.memTimeline->index(i);
            const bool header =
                m_deps.memTimeline->data(idx, memoryui::MemoryTimelineModel::IsHeaderRole).toBool();
            if (header) {
                md += QStringLiteral("\n**%1**\n\n")
                          .arg(m_deps.memTimeline
                                   ->data(idx, memoryui::MemoryTimelineModel::GroupKeyRole)
                                   .toString());
            } else {
                md +=
                    QStringLiteral("- _%1_ %2\n")
                        .arg(m_deps.memTimeline->data(idx, memoryui::MemoryTimelineModel::KindRole)
                                 .toString(),
                             m_deps.memTimeline
                                 ->data(idx, memoryui::MemoryTimelineModel::SummaryRole)
                                 .toString());
            }
        }
        md += QStringLiteral("\n");
    }

    return md;
}

QString TuiPageHub::buildProfileMarkdown(const QString& profileRef) const {
    QString md;
    if (profileRef.isEmpty() || m_deps.profiles == nullptr) {
        md += tr("# Profile\n\n_No agent selected._\n");
        return md;
    }
    const QVariantMap p = m_deps.profiles->profile(profileRef);
    const auto val = [&p](const QString& key, const QString& fallback) {
        const QString v = p.value(key).toString();
        return v.isEmpty() ? fallback : v;
    };

    md += tr("# Profile - %1\n\n").arg(val(QStringLiteral("name"), profileRef));
    md += tr("Agent == profile. Memory lives in this agent's bank (`%1`).\n\n").arg(profileRef);

    md += tr("## Engine\n\n");
    md += tr("- Provider: **%1**\n").arg(val(QStringLiteral("provider"), QStringLiteral("-")));
    md += tr("- Model: **%1**\n").arg(val(QStringLiteral("model"), QStringLiteral("-")));
    md += tr("- Base URL: %1\n").arg(val(QStringLiteral("baseUrl"), tr("(provider default)")));
    md += tr("- Context engine: %1\n\n")
              .arg(val(QStringLiteral("contextEngine"), QStringLiteral("lcm")));

    md += tr("## Memory\n\n");
    md += tr("- Memory provider: **%1**\n\n")
              .arg(val(QStringLiteral("memoryProvider"), QStringLiteral("mnemosyne")));

    md += tr("## Persona\n\n");
    md += val(QStringLiteral("systemPrompt"), QStringLiteral("-")) + QStringLiteral("\n\n");

    const auto chips = [&md, &p](const QString& title, const QString& key) {
        const QStringList items = p.value(key).toStringList();
        md += QStringLiteral("## %1\n\n").arg(title);
        if (items.isEmpty()) {
            md += tr("_none_\n\n");
            return;
        }
        for (const QString& it : items)
            md += QStringLiteral("- `%1`\n").arg(it);
        md += QStringLiteral("\n");
    };
    chips(tr("Tool allowlist"), QStringLiteral("toolAllowlist"));
    chips(tr("Skills"), QStringLiteral("skills"));
    return md;
}

QString TuiPageHub::buildDashboardMarkdown() const {
    auto* activity = qobject_cast<uimodels::VariantListModel*>(m_deps.dashboard->activity());

    QString md;
    md += tr("# Dashboard\n\n");
    md += tr("- Active sessions: **%1**\n").arg(m_deps.dashboard->activeSessions());
    md += tr("- Running agents: **%1**\n").arg(m_deps.dashboard->runningAgents());
    md += tr("- Pending approvals: **%1**\n").arg(m_deps.dashboard->pendingApprovals());
    md += tr("- Tokens today: **%1**\n").arg(m_deps.dashboard->tokensToday());
    md += tr("- Daemon health: **%1**\n\n")
              .arg(m_deps.dashboard->healthy() ? tr("OK") : tr("degraded"));

    md += tr("## Recent activity\n\n");
    if (activity != nullptr) {
        for (const QVariantMap& a : activity->rows()) {
            md += tr("- _%1_ — %2\n")
                      .arg(a.value(QStringLiteral("time")).toString(),
                           a.value(QStringLiteral("text")).toString());
        }
    }
    return md;
}

QString TuiPageHub::buildFleetMarkdown(int sel) const {
    auto* nodes = qobject_cast<uimodels::VariantListModel*>(m_deps.fleetTree->nodes());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Fleet\n\n");
    md += tr("Orchestrator/worker tree, shared with the GUI. **j/k** move · "
             "**Space/Enter** pause/resume.\n\n");
    if (nodes != nullptr) {
        const auto rows = nodes->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& n = rows.at(i);
            const int depth = n.value(QStringLiteral("depth")).toInt();
            md += QString(static_cast<qsizetype>(depth) * 2, QLatin1Char(' '));
            md += tr("- %1%2 — %3 (`%4`)\n")
                      .arg(mark(i), n.value(QStringLiteral("name")).toString(),
                           n.value(QStringLiteral("status")).toString(),
                           n.value(QStringLiteral("model")).toString());
        }
    }
    return md;
}

QString TuiPageHub::buildSessionsMarkdown(int sel) const {
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_deps.roster->sessions());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Sessions\n\n");
    md += tr("**j/k** move · **s** suspend · **R**/**Enter** resume · "
             "**x** close.\n\n");
    if (model != nullptr) {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& s = rows.at(i);
            md += tr("- %1**%2** — %3 · %4 · %5 · %6 tok\n")
                      .arg(mark(i), s.value(QStringLiteral("title")).toString(),
                           s.value(QStringLiteral("state")).toString(),
                           s.value(QStringLiteral("lifecycle")).toString(),
                           s.value(QStringLiteral("profile")).toString(),
                           s.value(QStringLiteral("tokens")).toString());
        }
    }
    return md;
}

QString TuiPageHub::buildApprovalsMarkdown(int sel) const {
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_deps.approvals->pending());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Approvals\n\n");
    if (model == nullptr || model->count() == 0) {
        md += tr("_Inbox zero — no pending approvals._\n");
        return md;
    }
    md += tr("**j/k** move · **a**/**Enter** approve · **d** deny.\n\n");
    const auto rows = model->rows();
    for (int i = 0; i < rows.size(); ++i) {
        const QVariantMap& a = rows.at(i);
        md += tr("## %1%2 (%3 risk)\n\n")
                  .arg(mark(i), a.value(QStringLiteral("tool")).toString(),
                       a.value(QStringLiteral("risk")).toString());
        md += tr("- Session: %1\n").arg(a.value(QStringLiteral("session")).toString());
        md += tr("- Command: `%1`\n\n").arg(a.value(QStringLiteral("command")).toString());
    }
    return md;
}

QString TuiPageHub::buildProfilesMarkdown(int sel) const {
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_deps.profiles->profiles());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Profiles\n\n");
    md += tr("Agent profiles, shared with the GUI. **j/k** move · **Enter** "
             "set default · **x** delete. Use the GUI editor for model / "
             "prompt / skills.\n\n");

    if (model != nullptr) {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& p = rows.at(i);
            md += tr("## %1%2%3\n\n")
                      .arg(mark(i), p.value(QStringLiteral("name")).toString(),
                           p.value(QStringLiteral("isDefault")).toBool() ? tr(" (default)")
                                                                         : QString());
            md += tr("- Model: `%1`\n").arg(p.value(QStringLiteral("model")).toString());
            const QString desc = p.value(QStringLiteral("description")).toString();
            if (!desc.isEmpty()) {
                md += QStringLiteral("- %1\n").arg(desc);
            }
            md += tr("- Skills: %1\n")
                      .arg(p.value(QStringLiteral("skills"))
                               .toStringList()
                               .join(QStringLiteral(", ")));
            md +=
                tr("- Tools: %1\n\n")
                    .arg(
                        p.value(QStringLiteral("tools")).toStringList().join(QStringLiteral(", ")));
        }
    }

    return md;
}

QString TuiPageHub::buildAccountsMarkdown(int sel) const {
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_deps.accounts->accounts());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Accounts\n\n");
    md += tr("Connected provider accounts, shared with the GUI. **j/k** move · "
             "**R**/**Enter** re-auth · **x** remove. Use the GUI wizard to add "
             "accounts.\n\n");

    md += tr("## Connected\n\n");
    if (model == nullptr || model->count() == 0) {
        md += tr("_No accounts._\n\n");
    } else {
        const auto rows = model->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& a = rows.at(i);
            md += tr("- %1**%2** — %3 (%4) · %5\n")
                      .arg(mark(i), a.value(QStringLiteral("label")).toString(),
                           a.value(QStringLiteral("kind")).toString() == QLatin1String("oauth")
                               ? tr("OAuth")
                               : tr("API key"),
                           a.value(QStringLiteral("status")).toString(),
                           a.value(QStringLiteral("detail")).toString());
        }
        md += QLatin1Char('\n');
    }

    md += tr("## Available providers\n\n");
    for (const QVariant& v : m_deps.accounts->availableProviders()) {
        const QVariantMap p = v.toMap();
        md += tr("- %1 (%2)\n")
                  .arg(p.value(QStringLiteral("name")).toString(),
                       p.value(QStringLiteral("kinds")).toStringList().join(QStringLiteral(", ")));
    }

    return md;
}

QString TuiPageHub::buildSettingsMarkdown() const {
    const auto cfg = [this](const char* key) {
        return m_deps.daemonConfig->value(QString::fromLatin1(key)).toString();
    };
    const auto onoff = [this](const char* key) {
        return m_deps.daemonConfig->value(QString::fromLatin1(key)).toBool() ? tr("on") : tr("off");
    };

    QString md;
    md += tr("# Settings\n\n");
    md += tr("The settings page, shared with the GUI. Edit values in the GUI; "
             "the TUI reflects the same daemon-config + app prefs. **F8** cycles "
             "the theme.\n\n");

    md += tr("## Connection\n\n");
    md += tr("- State: **%1**\n").arg(m_deps.connection->state());
    md += tr("- Mode: %1\n").arg(m_deps.connection->mode());
    md += tr("- Target: `%1`\n").arg(m_deps.connection->target());
    if (m_deps.settings != nullptr) {
        md += tr("- Start a local daemon automatically: %1\n")
                  .arg(m_deps.settings->managedLocalDaemon() ? tr("on") : tr("off"));
        md += tr("- Stop the managed daemon on exit: %1\n")
                  .arg(m_deps.settings->managedDaemonShutdownOnExit() ? tr("on") : tr("off"));
    }
    md += QStringLiteral("\n");

    md += tr("## Model\n\n");
    md += tr("- Default: `%1`\n").arg(cfg("model/default"));
    md += tr("- Reasoning effort: %1\n").arg(cfg("model/effort"));
    md += tr("- Fast mode: %1\n\n").arg(onoff("model/fast"));

    md += tr("## Chat\n\n");
    md += tr("- Stream responses: %1\n").arg(onoff("chat/streaming"));
    md += tr("- Send on Enter: %1\n").arg(onoff("chat/sendOnEnter"));
    md += tr("- Show token counts: %1\n\n").arg(onoff("chat/showTokenCounts"));

    md += tr("## Safety\n\n");
    md += tr("- Approval policy: %1\n").arg(cfg("safety/approvalPolicy"));
    md += tr("- Filesystem access: %1\n").arg(cfg("safety/sandbox"));
    md += tr("- Allow network: %1\n\n").arg(onoff("safety/allowNetwork"));

    md += tr("## Memory & Context\n\n");
    md += tr("- Max context tokens: %1\n")
              .arg(m_deps.daemonConfig->value(QStringLiteral("memory/contextWindow")).toInt());
    md += tr("- Auto-compact: %1\n").arg(onoff("memory/autoCompact"));
    md += tr("- Persist memory: %1\n\n").arg(onoff("memory/persistMemory"));

    md += tr("## Workspace\n\n");
    md += tr("- Root: `%1`\n").arg(cfg("workspace/root"));
    md += tr("- Respect .gitignore: %1\n\n").arg(onoff("workspace/followGitignore"));

    md += tr("## Voice\n\n");
    md += tr("- Enabled: %1\n").arg(onoff("voice/enabled"));
    md += tr("- Transcription model: %1\n\n").arg(cfg("voice/model"));

    md += tr("## Advanced\n\n");
    md += tr("- Log level: %1\n").arg(cfg("advanced/logLevel"));
    md += tr("- Telemetry: %1\n").arg(onoff("advanced/telemetry"));
    md += tr("- Experimental tools: %1\n").arg(onoff("advanced/experimentalTools"));

    return md;
}
