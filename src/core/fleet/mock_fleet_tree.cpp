// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "fleet/mock_fleet_tree.h"

#include "daemonnet/idaemonnet.h"

namespace fleet {

MockFleetTree::MockFleetTree(daemonnet::IDaemonNet* net, QObject* parent)
    : IFleetTree(parent), m_nodes(new uimodels::VariantListModel(this)) {
    // Single source: copy the DaemonNet's fleet projection (the delegation spanning tree) at
    // construction; pause/resume then mutate this local copy.
    if (net != nullptr) {
        if (auto* src = qobject_cast<uimodels::VariantListModel*>(net->fleet())) {
            m_nodes->setRows(src->rows());
        }
    }
}

QObject* MockFleetTree::nodes() const {
    return m_nodes;
}

void MockFleetTree::setStatus(const QString& id, const QString& status) {
    const int row = m_nodes->indexOfId(id);
    if (row < 0) {
        return;
    }
    QVariantMap n = m_nodes->at(row);
    n[QStringLiteral("status")] = status;
    m_nodes->upsert(n);
    emit changed();
}

void MockFleetTree::pause(const QString& id) {
    setStatus(id, QStringLiteral("paused"));
}
void MockFleetTree::resume(const QString& id) {
    setStatus(id, QStringLiteral("running"));
}

} // namespace fleet
