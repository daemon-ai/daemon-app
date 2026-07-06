// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_fleet_tree.h"

#include "daemon/repositories.h"
#include "profiles/iprofile_store.h"
#include "uimodels/variant_list_model.h"

namespace fleet {
namespace {

// Map the wire UnitKind onto the FleetPage's display discriminator (orchestrator icon vs worker).
QString kindFor(const QString& wireKind) {
    return wireKind == QStringLiteral("Orchestrator") ? QStringLiteral("orchestrator")
                                                      : QStringLiteral("worker");
}

// Map the wire UnitState onto the FleetPage status (the wire has no "paused"; that overlay is
// applied client-side in rebuild()).
QString statusFor(const QString& wireState) {
    return wireState == QStringLiteral("Running") ? QStringLiteral("running")
                                                  : QStringLiteral("idle");
}

} // namespace

DaemonFleetTree::DaemonFleetTree(daemonapp::daemon::FleetRepository* repo,
                                 profiles::IProfileStore* profiles, QObject* parent)
    : IFleetTree(parent), m_repo(repo), m_profiles(profiles),
      m_nodes(new uimodels::VariantListModel(this)) {
    if (m_repo != nullptr) {
        connect(m_repo, &daemonapp::daemon::FleetRepository::treeRefreshed, this,
                &DaemonFleetTree::rebuild);
        connect(m_repo, &daemonapp::daemon::FleetRepository::controlFailed, this,
                &DaemonFleetTree::onControlRejected);
        if (m_profiles != nullptr) {
            // Profile specs hydrate async (ProfileGet per row); re-join engine identity when they
            // land so rows flip from the optimistic "Core" to the real engine.
            connect(m_profiles, &profiles::IProfileStore::changed, this, &DaemonFleetTree::rebuild);
        }
        // Offline-first: render the last-known tree from cache immediately, before any connect.
        rebuild();
    }
}

QObject* DaemonFleetTree::nodes() const {
    return m_nodes;
}

void DaemonFleetTree::rebuild() {
    if (m_repo == nullptr) {
        return;
    }
    QList<QVariantMap> rows;
    QSet<QString> live;
    for (const daemonapp::daemon::CachedFleetUnitRow& u : m_repo->cachedUnits()) {
        live.insert(u.unitId);
        QVariantMap row;
        row[QStringLiteral("id")] = u.unitId;
        row[QStringLiteral("depth")] = u.depth;
        row[QStringLiteral("name")] = u.name.isEmpty() ? u.unitId : u.name;
        row[QStringLiteral("kind")] = kindFor(u.kind);
        row[QStringLiteral("status")] =
            m_paused.contains(u.unitId) ? QStringLiteral("paused") : statusFor(u.state);
        // No model field on a unit; the bound profile is the closest "what is this running as".
        row[QStringLiteral("model")] = u.profileRef;
        // Engine identity (C3): join profileRef -> profile store row. Absent store / unknown
        // profile degrades to "Core" (the optimistic default the profile store itself uses
        // before its spec hydrates).
        QString engine = QStringLiteral("Core");
        QString acpAgent;
        if (m_profiles != nullptr && !u.profileRef.isEmpty()) {
            const QVariantMap p = m_profiles->profile(u.profileRef);
            if (!p.isEmpty()) {
                engine = p.value(QStringLiteral("engine"), engine).toString();
                acpAgent = p.value(QStringLiteral("acpAgent")).toString();
            }
        }
        row[QStringLiteral("engine")] = engine;
        row[QStringLiteral("acpAgent")] = acpAgent;
        // F4: the unit's bound session id + wire role, so delegated-child rows can offer the
        // operator Steer/Cancel actions (a child IS a session; Submit is session-addressable).
        row[QStringLiteral("sessionId")] = u.sessionId;
        row[QStringLiteral("role")] = u.role; // "Primary" | "ManagedChild" | "EphemeralSubagent"
        rows.append(row);
    }
    // Drop paused-overlay entries for units that have left the tree (finished), so the set does not
    // grow unbounded.
    m_paused.intersect(live);
    m_nodes->setRows(rows);
    emit changed();
}

void DaemonFleetTree::pause(const QString& id) {
    if (m_repo == nullptr) {
        return;
    }
    m_lastControlId = id;
    m_paused.insert(id); // optimistic; reverted on controlFailed
    rebuild();
    m_repo->pause(id);
}

void DaemonFleetTree::resume(const QString& id) {
    if (m_repo == nullptr) {
        return;
    }
    m_lastControlId = id;
    m_paused.remove(id);
    rebuild();
    m_repo->resume(id);
}

void DaemonFleetTree::scale(const QString& id, int n) {
    if (m_repo == nullptr || n < 0) {
        return;
    }
    m_lastControlId = id;
    m_repo->scale(id, static_cast<quint32>(n));
}

void DaemonFleetTree::refresh() {
    if (m_repo != nullptr) {
        m_repo->refreshTree();
    }
}

void DaemonFleetTree::onControlRejected(const QString& message) {
    // The optimistic pause overlay was wrong (e.g. Unsupported on an engine leaf); revert it.
    if (!m_lastControlId.isEmpty()) {
        m_paused.remove(m_lastControlId);
        rebuild();
    }
    emit controlRejected(message);
}

} // namespace fleet
