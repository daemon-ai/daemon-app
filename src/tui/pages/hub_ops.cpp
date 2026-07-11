// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Ops page projections: Dashboard, Fleet, Sessions, Approvals, Routing and
// Cron (the fleet/automation "ops" surfaces). Split out of tui_page_hub.cpp;
// the dispatch stays in TuiPageHub::pageMarkdownForKind.

#include "automation/icron_store.h"
#include "daemon/engine_identity.h" // [wave2:integration] C5 approval origin
#include "fleet/iapprovals_inbox.h"
#include "fleet/idashboard.h"
#include "fleet/ifleet_tree.h"
#include "fleet/isession_roster.h"
#include "tools/itool_inventory.h" // [wave2:app-approvals-safety] D2
#include "tui_page_hub.h"
#include "uimodels/variant_list_model.h"

#include <QCoreApplication>

#ifdef DAEMON_APP_HAVE_MIRROR_SUBSTRATE
#include "mirror/mirror_service.h"
#include "mirror/store.h"
#endif

namespace {
// [wave2:integration] F3 + C3/ENG-7: the TUI projection of the GUI fleet engine chip, off the
// authoritative wire UnitNode fields (engine = "Core"/"Foreign" + engineAgent). The foreign label
// is single-sourced through the shared EngineIdentity facade (labelFor) so the TUI fleet row, the
// GUI Fleet chip and every composer/approval surface read identically ("<agent> · ACP" / bare
// agent when the catalog is cold); native falls back to "Native".
QString fleetEngineToken(const QVariantMap& row,
                         const daemonapp::daemon::EngineIdentity* engineIdentity) {
    const QString engine = row.value(QStringLiteral("engine")).toString();
    if (engine == QStringLiteral("Foreign")) {
        const QString agent = row.value(QStringLiteral("engineAgent")).toString();
        if (engineIdentity != nullptr) {
            const QString label =
                engineIdentity->labelFor(engine, agent, engineIdentity->protocolForAgent(agent));
            if (!label.isEmpty()) {
                return label;
            }
        }
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

// [phase H] The TUI projection of the GUI Fleet role badge, off the node-stamped UnitNode `role`
// (Primary / ManagedChild / EphemeralSubagent) rendered verbatim — never inverted from lifetime.
QString fleetRoleToken(const QVariantMap& row) {
    const QString role = row.value(QStringLiteral("role")).toString();
    if (role == QStringLiteral("ManagedChild")) {
        return QCoreApplication::translate("TuiPageHub", "managed child");
    }
    if (role == QStringLiteral("EphemeralSubagent")) {
        return QCoreApplication::translate("TuiPageHub", "subagent");
    }
    if (role == QStringLiteral("Primary")) {
        return QCoreApplication::translate("TuiPageHub", "primary");
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
            // Role badge (phase H) mirrors the GUI Fleet role chip.
            const QString lifetime = fleetLifetimeToken(n);
            const QString role = fleetRoleToken(n);
            QString suffix;
            if (!role.isEmpty()) {
                suffix += QStringLiteral(" · %1").arg(role);
            }
            if (!lifetime.isEmpty()) {
                suffix += QStringLiteral(" · %1").arg(lifetime);
            }
            md += tr("- %1%2 — %3 (`%4`) · %5%6\n")
                      .arg(mark(i), n.value(QStringLiteral("name")).toString(),
                           n.value(QStringLiteral("status")).toString(),
                           n.value(QStringLiteral("model")).toString(),
                           fleetEngineToken(n, m_deps.engineIdentity), suffix);
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
    // [wave2:app-approvals-safety] D3: **D** opens a deny-reason prompt (RootWidget-level),
    // threaded to the model via ApprovalDecide.reason (wire v29).
    md += tr("**j/k** move · **a**/**Enter** approve · **p** allow permanently · **d** deny · "
             "**D** deny with reason.\n\n");
    const auto rows = model->rows();
    for (int i = 0; i < rows.size(); ++i) {
        const QVariantMap& a = rows.at(i);
        // [wave2:app-approvals-safety] Q4: no risk tier — the wire sends none, so none is shown.
        md += tr("## %1%2\n\n").arg(mark(i), a.value(QStringLiteral("tool")).toString());
        // [wave2:integration] C5 origin line: resolve the requesting engine from the row's session
        // through the shared EngineIdentity facade (parity with the GUI EngineOriginChip). Shown
        // only for a foreign engine; native sessions add no line.
        if (m_deps.engineIdentity != nullptr) {
            const QVariantMap origin = m_deps.engineIdentity->engineForSession(
                a.value(QStringLiteral("session")).toString());
            if (origin.value(QStringLiteral("engine")).toString() == QStringLiteral("Foreign")) {
                const QString label = origin.value(QStringLiteral("label")).toString();
                if (!label.isEmpty()) {
                    md += tr("- Requested by: %1\n").arg(label);
                }
            }
        }
        md += tr("- Session: %1\n").arg(a.value(QStringLiteral("session")).toString());
        // [wave2:app-approvals-safety] D3: render the node's honest prompt faithfully; fall back to
        // the compact command line when no multi-line prompt is present.
        const QString prompt = a.value(QStringLiteral("prompt")).toString();
        md += tr("- Command: `%1`\n")
                  .arg(prompt.isEmpty() ? a.value(QStringLiteral("command")).toString() : prompt);
        const QString path = a.value(QStringLiteral("path")).toString();
        if (!path.isEmpty()) {
            md += tr("- Path: `%1`\n").arg(path);
        }
        // [waveB:app-v30] D5: render an fs.diff detail as a fenced unified diff (parity with the
        // GUI DiffBlock; the raw +/- lines carry their own coloring cue). Unknown kinds degrade.
        if (a.value(QStringLiteral("detailKind")).toString() == QLatin1String("fs.diff")) {
            const QString diffPath = a.value(QStringLiteral("diffPath")).toString();
            if (!diffPath.isEmpty()) {
                md += tr("- Diff: `%1`\n").arg(diffPath);
            }
            const QString diff = a.value(QStringLiteral("diff")).toString();
            if (!diff.isEmpty()) {
                md += QStringLiteral("\n```diff\n") + diff + QStringLiteral("\n```\n");
            }
        }
        // [wave2:app-approvals-safety] D3: fingerprint chip equivalent — the digest the "allow
        // permanently" scope remembers.
        const QString shortFp = a.value(QStringLiteral("shortFingerprint")).toString();
        if (!shortFp.isEmpty()) {
            md += tr("- Fingerprint: `%1`\n").arg(shortFp);
        }
        // Wire v28: only fingerprinted approvals can be remembered permanently; surface the offer
        // so the **p** keybind is discoverable per-row (matches the GUI's conditional button).
        if (a.value(QStringLiteral("canAllowPermanent")).toBool()) {
            md += tr("- _Can be allowed permanently (**p**)._\n");
        }
        md += QStringLiteral("\n");
    }
    return md;
}

// [wave2:app-approvals-safety] D2: read-only tool inventory (mirrors the GUI Settings -> Tools
// section). Rendered as a subsection of the TUI Settings page (the TUI folds GUI settings-sections
// into its one Settings page). No mutation keys — the node owns tool gating; a disabled tool names
// its requirement.
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

namespace {
// Decompose a canonical origin key (see the RoutePin origin_key derivation / daemonnet::originKey)
// into the flattened scope fields the routing page + its 'x' unbind need. Formats:
// `t|dm|user`, `t|group|chat|thread`, `t|api|key`, `t|internal`.
void packOriginKey(const QString& key, QVariantMap& row) {
    const QStringList parts = key.split(QLatin1Char('|'));
    row[QStringLiteral("transport")] = parts.value(0);
    const QString kind = parts.value(1);
    if (kind == QStringLiteral("dm")) {
        row[QStringLiteral("scopeKind")] = QStringLiteral("dm");
        row[QStringLiteral("user")] = parts.value(2);
    } else if (kind == QStringLiteral("group")) {
        row[QStringLiteral("scopeKind")] = QStringLiteral("group");
        row[QStringLiteral("chat")] = parts.value(2);
        row[QStringLiteral("thread")] = parts.value(3);
    } else if (kind == QStringLiteral("api")) {
        row[QStringLiteral("scopeKind")] = QStringLiteral("api");
        row[QStringLiteral("apiKey")] = parts.value(2);
    } else {
        row[QStringLiteral("scopeKind")] = QStringLiteral("internal");
    }
}
} // namespace

QList<QVariantMap> TuiPageHub::routingPinRows() const {
    QList<QVariantMap> rows;
#ifdef DAEMON_APP_HAVE_MIRROR_SUBSTRATE
    if (m_deps.mirror == nullptr) {
        return rows;
    }
    // The pin table IS the mirror store's route_pins projection (shared with the GUI routing
    // manager); the origin_key carries the flattened scope, unpacked here for display + unbind.
    for (const mirror::RoutePin& pin : m_deps.mirror->store().snapshot().route_pins) {
        QVariantMap row;
        row[QStringLiteral("id")] = pin.origin_key;
        packOriginKey(pin.origin_key, row);
        if (!pin.transport.isEmpty()) {
            row[QStringLiteral("transport")] = pin.transport;
        }
        row[QStringLiteral("session")] = pin.session;
        row[QStringLiteral("profile")] = pin.profile;
        row[QStringLiteral("isolation")] = pin.isolation;
        rows.append(row);
    }
#endif
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
