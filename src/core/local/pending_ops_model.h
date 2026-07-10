// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "outbox_types.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QPointer>
#include <QString>

namespace mirror {

class Outbox;

// A read-only lens over the outbox (spec 09 §6.5/§8.4): the pending strip / queue panel both
// front ends render. It is GUI-free (no QtQuick) and translates the outbox's add/change/remove
// signals into EXACT row ops (insert/remove/dataChanged) — never `beginResetModel` outside the
// initial population (guardrail §14.8). Per-lane / per-scope filter hooks serve A5's per-
// conversation pending strip.
class PendingOpsModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        OpIdRole = Qt::UserRole + 1,
        LaneRole,
        VerbRole,
        DisplayRole,
        StateRole,
        AttemptsRole,
        EnqueuedMsRole,
        LastErrorRole,
        RejectedRole, // convenience: state == Rejected
    };

    explicit PendingOpsModel(QObject* parent = nullptr);

    // Bind the source outbox. Safe to call once; re-population is a single reset (initial only).
    void setOutbox(Outbox* outbox);

    // Filter hooks (A5's pending strip). An empty filter matches everything. `laneFilter` matches
    // the full lane id; `scopeFilter` matches the lane's scope tail (the part after the class
    // separator) so a per-conversation strip can filter by 'transport\x1fconv'.
    void setLaneFilter(const QString& lane);
    void setScopeFilter(const QString& scope);

    [[nodiscard]] int count() const { return static_cast<int>(m_rows.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void countChanged();

private:
    void repopulate(); // initial population only (single reset)
    [[nodiscard]] bool matches(const PendingOp& op) const;
    [[nodiscard]] int indexOfOpId(const QString& opId) const;
    void onOpAdded(const QString& opId);
    void onOpChanged(const QString& opId);
    void onOpRemoved(const QString& opId);

    QPointer<Outbox> m_outbox;
    QList<PendingOp> m_rows; // filtered, in outbox FIFO order
    QString m_laneFilter;
    QString m_scopeFilter;
};

} // namespace mirror
