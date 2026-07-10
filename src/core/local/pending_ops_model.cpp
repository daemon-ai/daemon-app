// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "pending_ops_model.h"

// RED stub: real implementation lands in the GREEN commit.
namespace mirror {

PendingOpsModel::PendingOpsModel(QObject* parent) : QAbstractListModel(parent) {}

void PendingOpsModel::setOutbox(Outbox*) {}
void PendingOpsModel::setLaneFilter(const QString&) {}
void PendingOpsModel::setScopeFilter(const QString&) {}

int PendingOpsModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}
QVariant PendingOpsModel::data(const QModelIndex&, int) const {
    return {};
}
QHash<int, QByteArray> PendingOpsModel::roleNames() const {
    return {};
}

void PendingOpsModel::repopulate() {}
bool PendingOpsModel::matches(const PendingOp&) const {
    return false;
}
int PendingOpsModel::indexOfOpId(const QString&) const {
    return -1;
}
void PendingOpsModel::onOpAdded(const QString&) {}
void PendingOpsModel::onOpChanged(const QString&) {}
void PendingOpsModel::onOpRemoved(const QString&) {}

} // namespace mirror
