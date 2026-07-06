// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "fleet/ifleet_tree.h"

#include <QHash>
#include <QSet>
#include <QString>

namespace uimodels {
class VariantListModel;
}
namespace daemonapp::daemon {
class FleetRepository;
}
namespace profiles {
class IProfileStore;
}

namespace fleet {

// Daemon-backed, offline-first fleet/subagent tree (PRO-9 view + PRO-10 control). Projects the
// FleetRepository's cached daemon_fleet_units (Tree query) into the IFleetTree VariantListModel
// rows ({ depth, id, name, kind, status, model, engine, acpAgent, sessionId, role }); seeds from
// the cache in the ctor so the tree renders last-known with no connection, and rebuilds on each
// treeRefreshed. pause/resume/scale issue the wire control op via the repository; the operator
// Steer/Cancel row actions (F4) go through ISessionRoster with the row's sessionId.
//
// Engine identity (C3/ENG-7): each row joins its profileRef against the profile store's rows
// (engine = "Core" | "Acp", acpAgent = the foreign agent name), so the fleet tree can render the
// same engine chip as the profile list. The join re-runs when the profile store changes (specs
// hydrate async via ProfileGet).
//
// The wire UnitState has no "paused" (Running/Finished/Unknown only), so pause/resume keep a
// client-local paused overlay (m_paused) that survives a refresh until resumed - the visible
// pause state. A control rejection (Unsupported on an engine leaf) reverts the optimistic overlay.
class DaemonFleetTree : public IFleetTree {
    Q_OBJECT

public:
    explicit DaemonFleetTree(daemonapp::daemon::FleetRepository* repo,
                             profiles::IProfileStore* profiles = nullptr,
                             QObject* parent = nullptr);

    [[nodiscard]] QObject* nodes() const override;
    void pause(const QString& id) override;
    void resume(const QString& id) override;
    void scale(const QString& id, int n) override;
    void refresh() override;

private:
    void rebuild();
    void onControlRejected(const QString& message);

    daemonapp::daemon::FleetRepository* m_repo = nullptr;
    profiles::IProfileStore* m_profiles = nullptr; // engine-identity join source (may be null)
    uimodels::VariantListModel* m_nodes = nullptr;
    QSet<QString> m_paused;  // client-local paused overlay (the wire has no Paused state)
    QString m_lastControlId; // the unit of the in-flight pause/resume (to revert on rejection)
};

} // namespace fleet
