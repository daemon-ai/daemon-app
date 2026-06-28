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

namespace fleet {

// Daemon-backed, offline-first fleet/subagent tree (PRO-9 view + PRO-10 control). Projects the
// FleetRepository's cached daemon_fleet_units (Tree query) into the IFleetTree VariantListModel
// rows
// ({ depth, id, name, kind, status, model }); seeds from the cache in the ctor so the tree renders
// last-known with no connection, and rebuilds on each treeRefreshed. pause/resume/scale issue the
// wire control op via the repository.
//
// The wire UnitState has no "paused" (Running/Finished/Unknown only), so pause/resume keep a
// client-local paused overlay (m_paused) that survives a refresh until resumed - the visible
// pause state. A control rejection (Unsupported on an engine leaf) reverts the optimistic overlay.
class DaemonFleetTree : public IFleetTree {
    Q_OBJECT

public:
    explicit DaemonFleetTree(daemonapp::daemon::FleetRepository* repo, QObject* parent = nullptr);

    [[nodiscard]] QObject* nodes() const override;
    void pause(const QString& id) override;
    void resume(const QString& id) override;
    void scale(const QString& id, int n) override;
    void refresh() override;

private:
    void rebuild();
    void onControlRejected(const QString& message);

    daemonapp::daemon::FleetRepository* m_repo = nullptr;
    uimodels::VariantListModel* m_nodes = nullptr;
    QSet<QString> m_paused;  // client-local paused overlay (the wire has no Paused state)
    QString m_lastControlId; // the unit of the in-flight pause/resume (to revert on rejection)
};

} // namespace fleet
