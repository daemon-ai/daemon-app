// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Ops page projections: Dashboard, Fleet, Sessions, Approvals, Routing and
// Cron (the fleet/automation "ops" surfaces). Split out of tui_page_hub.cpp;
// the dispatch stays in TuiPageHub::pageMarkdownForKind.

#include "automation/icron_store.h"
#include "automation/irouting_store.h"
#include "fleet/iapprovals_inbox.h"
#include "fleet/idashboard.h"
#include "fleet/ifleet_tree.h"
#include "fleet/isession_roster.h"
#include "tui_page_hub.h"
#include "uimodels/variant_list_model.h"

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
            // Engine identity (C3/ENG-7): the same chip the GUI fleet rows render.
            md += tr("- %1%2 — %3 (`%4`) · %5\n")
                      .arg(mark(i), n.value(QStringLiteral("name")).toString(),
                           n.value(QStringLiteral("status")).toString(),
                           n.value(QStringLiteral("model")).toString(), engineToken(n));
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
    md += tr("**j/k** move · **a**/**Enter** approve · **p** allow permanently · **d** deny.\n\n");
    const auto rows = model->rows();
    for (int i = 0; i < rows.size(); ++i) {
        const QVariantMap& a = rows.at(i);
        md += tr("## %1%2 (%3 risk)\n\n")
                  .arg(mark(i), a.value(QStringLiteral("tool")).toString(),
                       a.value(QStringLiteral("risk")).toString());
        md += tr("- Session: %1\n").arg(a.value(QStringLiteral("session")).toString());
        md += tr("- Command: `%1`\n").arg(a.value(QStringLiteral("command")).toString());
        // Wire v28: only fingerprinted approvals can be remembered permanently; surface the offer
        // so the **p** keybind is discoverable per-row (matches the GUI's conditional button).
        if (a.value(QStringLiteral("canAllowPermanent")).toBool()) {
            md += tr("- _Can be allowed permanently (**p**)._\n");
        }
        md += QStringLiteral("\n");
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
