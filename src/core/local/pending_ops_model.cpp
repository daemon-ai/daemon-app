// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "pending_ops_model.h"

#include "outbox.h"
#include "verb_class.h"

#include <algorithm>
#include <QChar>

namespace mirror {
namespace {
constexpr char16_t kSep = u'\x1f';

// The scope tail of a lane id '<class>\x1f<scope>' — everything after the FIRST separator (the
// scope itself may contain separators, e.g. 'transport\x1fconv').
QString scopeOf(const QString& lane) {
    const qsizetype idx = lane.indexOf(QChar(kSep));
    return idx < 0 ? QString() : lane.mid(idx + 1);
}
} // namespace

PendingOpsModel::PendingOpsModel(QObject* parent) : QAbstractListModel(parent) {}

void PendingOpsModel::setOutbox(Outbox* outbox) {
    if (m_outbox == outbox) {
        return;
    }
    if (m_outbox != nullptr) {
        m_outbox->disconnect(this);
    }
    m_outbox = outbox;
    if (m_outbox != nullptr) {
        connect(m_outbox, &Outbox::opAdded, this, &PendingOpsModel::onOpAdded);
        connect(m_outbox, &Outbox::opChanged, this, &PendingOpsModel::onOpChanged);
        connect(m_outbox, &Outbox::opRemoved, this, &PendingOpsModel::onOpRemoved);
    }
    repopulate();
}

void PendingOpsModel::setLaneFilter(const QString& lane) {
    if (m_laneFilter == lane) {
        return;
    }
    m_laneFilter = lane;
    repopulate();
}

void PendingOpsModel::setScopeFilter(const QString& scope) {
    if (m_scopeFilter == scope) {
        return;
    }
    m_scopeFilter = scope;
    repopulate();
}

bool PendingOpsModel::matches(const PendingOp& op) const {
    if (!m_laneFilter.isEmpty() && op.lane != m_laneFilter) {
        return false;
    }
    if (!m_scopeFilter.isEmpty() && scopeOf(op.lane) != m_scopeFilter) {
        return false;
    }
    return true;
}

int PendingOpsModel::indexOfOpId(const QString& opId) const {
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).opId == opId) {
            return i;
        }
    }
    return -1;
}

void PendingOpsModel::repopulate() {
    // Initial population / filter change: the ONE place a reset is allowed (guardrail §14.8).
    beginResetModel();
    m_rows.clear();
    if (m_outbox != nullptr) {
        for (const PendingOp& op : m_outbox->ops()) {
            if (matches(op)) {
                m_rows.append(op);
            }
        }
    }
    endResetModel();
    emit countChanged();
}

void PendingOpsModel::onOpAdded(const QString& opId) {
    if (m_outbox == nullptr || indexOfOpId(opId) >= 0) {
        return;
    }
    const PendingOp op = m_outbox->op(opId);
    if (!matches(op)) {
        return;
    }
    // Insert in FIFO (enqueued_ms) order so the strip shows lane order.
    int pos = 0;
    while (pos < m_rows.size() && m_rows.at(pos).enqueuedMs <= op.enqueuedMs) {
        ++pos;
    }
    beginInsertRows({}, pos, pos);
    m_rows.insert(pos, op);
    endInsertRows();
    emit countChanged();
}

void PendingOpsModel::onOpChanged(const QString& opId) {
    if (m_outbox == nullptr) {
        return;
    }
    const PendingOp op = m_outbox->op(opId);
    const int idx = indexOfOpId(opId);
    const bool present = idx >= 0;
    const bool nowMatches = op.opId == opId && matches(op);
    if (present && !nowMatches) {
        onOpRemoved(opId); // filtered out now
        return;
    }
    if (!present && nowMatches) {
        onOpAdded(opId); // newly matches (e.g. filter edge)
        return;
    }
    if (present && nowMatches) {
        m_rows[idx] = op;
        const QModelIndex mi = index(idx);
        emit dataChanged(mi, mi);
    }
}

void PendingOpsModel::onOpRemoved(const QString& opId) {
    const int idx = indexOfOpId(opId);
    if (idx < 0) {
        return;
    }
    beginRemoveRows({}, idx, idx);
    m_rows.removeAt(idx);
    endRemoveRows();
    emit countChanged();
}

int PendingOpsModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant PendingOpsModel::data(const QModelIndex& index, int role) const {
    if (index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const PendingOp& op = m_rows.at(index.row());
    switch (role) {
    case OpIdRole:
        return op.opId;
    case LaneRole:
        return op.lane;
    case VerbRole:
        return op.verb;
    case DisplayRole:
        return op.display;
    case StateRole:
        return static_cast<int>(op.state);
    case AttemptsRole:
        return op.attempts;
    case EnqueuedMsRole:
        return op.enqueuedMs;
    case LastErrorRole:
        return op.lastError;
    case RejectedRole:
        return op.state == OpState::Rejected;
    default:
        return {};
    }
}

QHash<int, QByteArray> PendingOpsModel::roleNames() const {
    return {
        {OpIdRole, "opId"},
        {LaneRole, "lane"},
        {VerbRole, "verb"},
        {DisplayRole, "display"},
        {StateRole, "state"},
        {AttemptsRole, "attempts"},
        {EnqueuedMsRole, "enqueuedMs"},
        {LastErrorRole, "lastError"},
        {RejectedRole, "rejected"},
    };
}

} // namespace mirror
