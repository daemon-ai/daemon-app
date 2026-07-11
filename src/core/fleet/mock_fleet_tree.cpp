// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "fleet/mock_fleet_tree.h"

#include "daemonnet/mock_fleet_source.h"

namespace fleet {

MockFleetTree::MockFleetTree(daemonnet::MockFleetSource* src, QObject* parent)
    : IFleetTree(parent), m_nodes(new uimodels::VariantListModel(this)) {
    // Single source: copy the mock seed's fleet projection (the delegation spanning tree) at
    // construction; pause/resume then mutate this local copy.
    if (src != nullptr) {
        if (auto* rows = qobject_cast<uimodels::VariantListModel*>(src->fleet())) {
            m_nodes->setRows(rows->rows());
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
