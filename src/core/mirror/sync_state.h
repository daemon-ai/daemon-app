#pragma once
// mirror::SyncState — the ingestor's durable sync bookkeeping (spec 09 §5.5). Owns the two
// revision spaces that meet only here: NODE revs (per collection, minted by the node) and the
// feed cursor/epoch; plus per-conversation ConvHistory cursors (the §13 M1 fix) and per-collection
// freshness state (E5). Two never mix with the client-local mirror journal revs (§4.3).
//
// This is the authoritative in-memory copy; the read-only projection lives in MirrorModel.sync
// (§4.1) and the durable copy in sync_cursors/node_revs (§4.5), advanced by the write-behind
// consumer in the same transaction as the rows (B7).

#include "mirror/model.h" // CollectionState, StampingMode

#include <QHash>
#include <QString>
#include <QStringList>

namespace mirror {

class SyncState {
public:
    // --- collection freshness (§5.5/E5) ---
    struct Collection {
        quint64 nodeRev = 0;
        qint64 fetchedAtMs = 0;
        CollectionState state = CollectionState::Stale;
        StampingMode mode = StampingMode::RefetchDiff;
        QString lastError;
    };

    [[nodiscard]] Collection collection(const QString& name) const {
        return collections_.value(name);
    }
    void markStale(const QString& name);
    void markSyncing(const QString& name);
    void markFresh(const QString& name, quint64 nodeRev, qint64 atMs);
    void markError(const QString& name, const QString& err);
    void setMode(const QString& name, StampingMode mode);
    [[nodiscard]] QStringList staleCollections() const;

    // Rung-1 skip-gate (§5.2 "full"): a rev-gated arm skips its fetch when the event's rev equals
    // the stored node rev for the collection. In degraded mode (no wire revs) this never skips.
    [[nodiscard]] bool revUnchanged(const QString& name, quint64 eventRev, bool hasRev) const;

    // --- feed cursor + epoch (§5.5/§5.6) ---
    [[nodiscard]] quint64 feedCursor() const noexcept { return feed_cursor_; }
    void setFeedCursor(quint64 c) noexcept {
        if (c > feed_cursor_) {
            feed_cursor_ = c;
        }
    }
    [[nodiscard]] quint64 feedEpoch() const noexcept { return feed_epoch_; }
    // Returns true when the epoch changed (node restart ⇒ re-baseline, §5.6 full step 1).
    bool setFeedEpoch(quint64 e);

    // --- per-conversation ConvHistory cursor (the §13 M1 fix) ---
    [[nodiscard]] quint64 convCursor(const QString& scope) const {
        return conv_cursors_.value(scope, 0);
    }
    void setConvCursor(const QString& scope, quint64 cursor) {
        if (cursor > conv_cursors_.value(scope, 0)) {
            conv_cursors_.insert(scope, cursor);
        }
    }

    // --- ResyncNeeded scope → collection set (§5.7) ---
    // v38 scopes: ""/"all"/"roster"/"profiles" (07§5.1). Rung 1 widens to collection names.
    // Never re-baselines unrelated collections; an unknown scope maps to nothing (no blind storm).
    [[nodiscard]] static QStringList scopeToCollections(const QString& scope);
    [[nodiscard]] static const QStringList& allCollections();

    // --- mode selection (§5.6) ---
    // Full (wire_delta) iff the connection advertises api ≥ 39 AND the rev/removed fields are
    // present in the codec; otherwise degraded (refetch_diff). ACTIVE from BR: the v39 codec is
    // vendored (hasRevFields is compile-time true) and onConnected drives the FULL path for an
    // api/39 node — deployed v38 nodes stay first-class on the degraded path.
    static constexpr int kFullModeApiFloor = 39;
    [[nodiscard]] static StampingMode selectMode(int apiVersion, bool hasRevFields) {
        return (apiVersion >= kFullModeApiFloor && hasRevFields) ? StampingMode::WireDelta
                                                                 : StampingMode::RefetchDiff;
    }

private:
    QHash<QString, Collection> collections_;
    QHash<QString, quint64> conv_cursors_;
    quint64 feed_cursor_ = 0;
    quint64 feed_epoch_ = 0;
};

} // namespace mirror
