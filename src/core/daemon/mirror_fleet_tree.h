// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "fleet/ifleet_tree.h"

#include <QSet>
#include <QString>

namespace uimodels {
class VariantListModel;
}
namespace mirror {
class Store;
class Ingestor;
} // namespace mirror
namespace daemonapp::daemon {
class FleetRepository;
}

namespace fleet {

// G2 (M5): the fleet/subagent TREE projected from the ONE generated mirror::FleetUnit (the 6→1
// unification's tree half; the sidebar fleet MEMBERSHIP was already mirror-served in A7). Replaces
// DaemonFleetTree, which read the legacy daemon_fleet_units cache. The supervision hierarchy is
// reconstructed from the entity's `child_ids` edge (§3.5 unitsByParent): the root is a unit no
// child list names, and a pre-order DFS assigns each row its depth — the same flattened
// VariantListModel shape ({ depth, id, name, kind, status, model, engine, engineAgent, lifetime,
// sessionId, role }) the FleetPage/ops-hub already bind, so no GUI/TUI change is needed.
//
// Reads are pure projections of the mirror fleet_units table, re-derived on a FleetUnit journal
// delta. MUTATIONS stay node-authoritative: pause/resume/scale and the on-demand refresh route
// through the wire (FleetRepository control ops + the ingestor Tree refetch) — the mirror is the
// read model, never the mutation path (§5.1). The wire UnitState has no "paused"; that overlay is
// client-local (m_paused) and reverts on a control rejection, exactly as the legacy tree did.
class MirrorFleetTree : public IFleetTree {
    Q_OBJECT

public:
    MirrorFleetTree(mirror::Store* store, mirror::Ingestor* ingestor,
                    daemonapp::daemon::FleetRepository* control, QObject* parent = nullptr);

    [[nodiscard]] QObject* nodes() const override;
    void pause(const QString& id) override;
    void resume(const QString& id) override;
    void scale(const QString& id, int n) override;
    void refresh() override;

private:
    void onCommitted();
    void rebuild();
    void onControlRejected(const QString& message);

    mirror::Store* m_mirror = nullptr;
    mirror::Ingestor* m_ingestor = nullptr;
    daemonapp::daemon::FleetRepository* m_control = nullptr;
    uimodels::VariantListModel* m_nodes = nullptr;
    quint64 m_watermark = 0;
    QSet<QString> m_paused;  // client-local paused overlay (the wire has no Paused state)
    QString m_lastControlId; // the unit of the in-flight pause/resume (to revert on rejection)
};

} // namespace fleet
