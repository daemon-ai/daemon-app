#include "mirror/table_model.h"

namespace mirror {

quint64 TableModelBase::s_serial_ = 0;

TableModelBase::TableModelBase(Store& store, EntityKind kind, QObject* parent)
    : QAbstractListModel(parent), store_(&store), kind_(kind) {
    // A unique journal-consumer name so this lens participates in the compaction floor (§4.4):
    // its unread tail is protected while it lives, and teardown lifts it out of the floor.
    consumerName_ = QStringLiteral("lens/%1/%2")
                        .arg(QString::fromLatin1(entityKindName(kind_)))
                        .arg(++s_serial_);
    watermark_ = store_->journal().headRev();
    store_->journal().registerConsumer(consumerName_, watermark_);

    // Collection projections consume journal deltas regardless of the observe seam (§4.2/§8.1);
    // Store::committed is emitted once per non-empty commit, after the snapshot swap.
    connect(store_, &Store::committed, this, &TableModelBase::onStoreCommitted);
}

TableModelBase::~TableModelBase() {
    if (store_ != nullptr) {
        store_->journal().unregisterConsumer(consumerName_);
    }
}

const MirrorModel& TableModelBase::snapshot() const {
    return store_->snapshot();
}

void TableModelBase::setWatermark(quint64 rev) {
    watermark_ = rev;
    store_->journal().advanceWatermark(consumerName_, rev);
}

void TableModelBase::onStoreCommitted(quint64 revFrom, quint64 revTo) {
    applyCommit(revFrom, revTo);
}

void TableModelBase::requestOlder(int count) {
    Q_EMIT olderRequested(scopeKey_, count > 0 ? count : defaultPageSize_);
}

qint64 TableModelBase::stableIdAt(int row) const {
    return stableIdAtImpl(row);
}

int TableModelBase::rowForStableId(qint64 id) const {
    return rowForStableIdImpl(id);
}

} // namespace mirror
