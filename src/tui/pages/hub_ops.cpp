// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Ops page projections: Dashboard, Fleet, Sessions, Approvals, Routing and
// Cron (the fleet/automation "ops" surfaces). Split out of tui_page_hub.cpp;
// the dispatch stays in TuiPageHub::pageMarkdownForKind.

#include "automation/icron_store.h"
#include "daemonnet/idaemonnet.h"
#include "daemonnet/routing_dtos.h"
#include "fleet/iapprovals_inbox.h"
#include "fleet/idashboard.h"
#include "fleet/ifleet_tree.h"
#include "fleet/isession_roster.h"
#include "tui_page_hub.h"
#include "uimodels/variant_list_model.h"

#include <QCoreApplication>

namespace {
// [wave2:app-delegation] F3: the TUI projection of the GUI fleet engine + lifetime chips, off the
// authoritative wire UnitNode fields (engine = "Core"/"Foreign" + engineAgent, lifetime). Local to
// the fleet row (the shared engineToken() reads the profile-row vocabulary the engines stream
// owns); integration 2 reconciles both onto the engines stream's EngineIdentity helper.
QString fleetEngineToken(const QVariantMap& row) {
    const QString engine = row.value(QStringLiteral("engine")).toString();
    if (engine == QStringLiteral("Foreign")) {
        const QString agent = row.value(QStringLiteral("engineAgent")).toString();
        return agent.isEmpty() ? QCoreApplication::translate("TuiPageHub", "Foreign") : agent;
    }
    return QCoreApplication::translate("TuiPageHub", "Native");
}

QString fleetLifetimeToken(const QVariantMap& row) {
    const QString lifetime = row.value(QStringLiteral("lifetime")).toString();
    if (lifetime == QStringLiteral("Persistent")) {
        return QCoreApplication::translate("TuiPageHub", "persistent");
    }
    if (lifetime == QStringLiteral("Ephemeral")) {
        return QCoreApplication::translate("TuiPageHub", "ephemeral");
    }
    return {};
}
} // namespace

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
             "**Space/Enter** pause/resume · **o** open transcript · **t** steer a child · "
             "**c** cancel its turn.\n\n");
    if (nodes != nullptr) {
        const auto rows = nodes->rows();
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap& n = rows.at(i);
            const int depth = n.value(QStringLiteral("depth")).toInt();
            md += QString(static_cast<qsizetype>(depth) * 2, QLatin1Char(' '));
            // [wave2:app-delegation] Engine + lifetime chips (C3/F3), off the wire UnitNode fields.
            const QString lifetime = fleetLifetimeToken(n);
            const QString lifetimeSuffix =
                lifetime.isEmpty() ? QString() : QStringLiteral(" · %1").arg(lifetime);
            md += tr("- %1%2 — %3 (`%4`) · %5%6\n")
                      .arg(mark(i), n.value(QStringLiteral("name")).toString(),
                           n.value(QStringLiteral("status")).toString(),
                           n.value(QStringLiteral("model")).toString(), fleetEngineToken(n),
                           lifetimeSuffix);
        }
    }
    return md;
}

QString TuiPageHub::buildSessionsMarkdown(int sel) const {
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_deps.roster->sessions());
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };
    const bool archived = m_deps.roster->scope() == QStringLiteral("archived");

    QString md;
    // The Active|Archived scope switcher (F6/DEL-6), mirroring the GUI SessionsPage toggle.
    md += archived ? tr("# Sessions — Archived\n\n") : tr("# Sessions\n\n");
    md += archived ? tr("**j/k** move · **r**/**Enter** restore · **v** back to active.\n\n")
                   : tr("**j/k** move · **s** suspend · **R**/**Enter** resume · "
                        "**x** close · **v** archived.\n\n");
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
        if (archived && rows.isEmpty()) {
            md += tr("_No archived sessions._\n");
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

// A human label for a pin row's origin scope ("#ops", "@bob", "api:key", "internal").
static QString routingScopeLabel(const QVariantMap& row) {
    const QString kind = row.value(QStringLiteral("scopeKind")).toString();
    if (kind == QStringLiteral("dm")) {
        return QStringLiteral("@") + row.value(QStringLiteral("user")).toString();
    }
    if (kind == QStringLiteral("group")) {
        const QString thread = row.value(QStringLiteral("thread")).toString();
        return row.value(QStringLiteral("chat")).toString() +
               (thread.isEmpty() ? QString() : QStringLiteral(" › ") + thread);
    }
    if (kind == QStringLiteral("api")) {
        return QStringLiteral("api:") + row.value(QStringLiteral("apiKey")).toString();
    }
    return QStringLiteral("internal");
}

QList<QVariantMap> TuiPageHub::routingPinRows() const {
    QList<QVariantMap> rows;
    if (m_deps.daemonNet == nullptr) {
        return rows;
    }
    const QList<daemonnet::RoutingPin> pins = m_deps.daemonNet->routes();
    rows.reserve(pins.size());
    for (const daemonnet::RoutingPin& pin : pins) {
        QVariantMap row;
        row[QStringLiteral("id")] = daemonnet::originKey(pin.origin);
        row[QStringLiteral("transport")] = pin.origin.transport.toString();
        switch (pin.origin.scope.kind) {
        case domain::OriginScopeKind::Dm:
            row[QStringLiteral("scopeKind")] = QStringLiteral("dm");
            row[QStringLiteral("user")] = pin.origin.scope.user;
            break;
        case domain::OriginScopeKind::Group:
            row[QStringLiteral("scopeKind")] = QStringLiteral("group");
            row[QStringLiteral("chat")] = pin.origin.scope.chat;
            row[QStringLiteral("thread")] = pin.origin.scope.thread;
            break;
        case domain::OriginScopeKind::Api:
            row[QStringLiteral("scopeKind")] = QStringLiteral("api");
            row[QStringLiteral("apiKey")] = pin.origin.scope.apiKey;
            break;
        case domain::OriginScopeKind::Internal:
            row[QStringLiteral("scopeKind")] = QStringLiteral("internal");
            break;
        }
        row[QStringLiteral("session")] = pin.session.toString();
        row[QStringLiteral("profile")] = pin.profile.toString();
        row[QStringLiteral("isolation")] = pin.isolation;
        rows.append(row);
    }
    return rows;
}

QString TuiPageHub::buildRoutingMarkdown(int sel) const {
    const auto mark = [sel](int i) { return i == sel ? QStringLiteral("▸ ") : QString(); };

    QString md;
    md += tr("# Routing\n\n");
    md += tr("Chat pins (origin → session), shared with the GUI routing manager. "
             "**j/k** move · **x** unbind.\n\n");
    const QList<QVariantMap> rows = routingPinRows();
    for (int i = 0; i < rows.size(); ++i) {
        const QVariantMap& r = rows.at(i);
        const QString profile = r.value(QStringLiteral("profile")).toString();
        md += tr("- %1**%2 · %3** ⇄ `%4`%5\n")
                  .arg(mark(i), r.value(QStringLiteral("transport")).toString(),
                       routingScopeLabel(r), r.value(QStringLiteral("session")).toString(),
                       profile.isEmpty() ? QString() : tr(" (agent `%1`)").arg(profile));
    }
    if (rows.isEmpty()) {
        md += tr("_No chat pins yet — pin a room/DM to a session from the GUI routing manager "
                 "or a room row._\n");
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
