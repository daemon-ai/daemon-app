#pragma once
// mirror::Journal — the store's total change order and row-level delta feed (spec 09 §4.3/§4.4;
// ADR-001, journal ACTIVE from wave 0). Degraded mode: revs are client-minted in memory at
// commit time (origin = refetch_diff until the node's rung-2 wire_delta reads land, §5.6).
//
// Owns: the store-wide monotonic rev counter (seeded at boot from persisted MAX(rev)); an
// in-memory deque of the retained tail; the per-consumer watermark registry (§4.4, "every
// consumer is a cursor"); the compaction floor = min(all live watermarks) with the §4.3
// retention tail (4096 rows or 24 h, whichever is larger). Persistence of the rows lives in
// mirror::Persistence (§4.5); this class is pure in-memory bookkeeping.

#include "mirror/generated/entities_gen.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <QHash>
#include <QString>
#include <vector>

namespace mirror {

enum class JournalOp : quint8 { Upsert = 0, Tombstone = 1 };

// Stamping origin (§4.3). refetch_diff = degraded interim (coarse event -> refetch -> diff ->
// stamp); wire_delta = full mode (rung 2 since_rev reads); seeder = mock mode (§9).
enum class JournalOrigin : quint8 { WireDelta = 0, RefetchDiff = 1, Seeder = 2 };

// One journal record (§4.3 row shape). origin_op is nullable provenance (empty QString = null),
// carried from the causing operation's client op-id (rung 3, §10.3) — drives the outbox confirm
// contract (§6.6). at_ms is wall-clock capture for retention + crash forensics.
struct JournalRecord {
    quint64 rev = 0;
    EntityKind kind{};
    QString key;
    JournalOp op = JournalOp::Upsert;
    JournalOrigin origin = JournalOrigin::RefetchDiff;
    QString origin_op;
    qint64 at_ms = 0;
};

class Journal {
public:
    // §4.3 retention default: 4096 rows OR 24 h, whichever is LARGER, kept below the floor.
    static constexpr std::size_t kDefaultRetentionRows = 4096;
    static constexpr qint64 kDefaultRetentionMs = 24LL * 60 * 60 * 1000;

    Journal() = default;

    // Boot: seed the counter from the persisted MAX(rev) so minting stays monotonic across
    // restarts (§4.3). Safe only before any stamp().
    void seedHead(quint64 persistedMaxRev) noexcept { head_rev_ = persistedMaxRev; }
    [[nodiscard]] quint64 headRev() const noexcept { return head_rev_; }
    [[nodiscard]] std::size_t size() const noexcept { return records_.size(); }
    [[nodiscard]] const std::deque<JournalRecord>& records() const noexcept { return records_; }

    // Mint the next rev and append a record (called by the committer, one batch at a time).
    const JournalRecord& stamp(EntityKind kind, const QString& key, JournalOp op,
                               JournalOrigin origin, const QString& originOp, qint64 atMs);

    // Append a record whose rev was already assigned (boot reload of the persisted tail). The
    // counter is advanced to at least this rev.
    void appendPersisted(const JournalRecord& rec);

    // --- watermark consumer contract (§4.4) ---
    void registerConsumer(const QString& name, quint64 at = 0);
    void unregisterConsumer(const QString& name);
    void advanceWatermark(const QString& name, quint64 rev);
    [[nodiscard]] bool hasConsumer(const QString& name) const noexcept;
    [[nodiscard]] quint64 watermark(const QString& name) const noexcept;

    // Compaction floor = min over all live watermarks; head when there are no consumers (all
    // caught up). Re-expresses Sink's lowerBoundRevision behavior (listener.cpp:308-320,
    // genericresource.cpp:163-173) — behavior only, no borrowed text.
    [[nodiscard]] quint64 compactionFloor() const noexcept;

    // Ordered deltas strictly above `sinceRev` (a consumer draining from its watermark, §8.1).
    [[nodiscard]] std::vector<JournalRecord> since(quint64 sinceRev) const;
    [[nodiscard]] std::vector<JournalRecord> since(quint64 sinceRev, EntityKind kind) const;

    // Compaction sweep (§4.4): drop rows strictly below the floor that are ALSO past both
    // retention bounds (keeps >= retentionRows and >= retentionMs). Returns the count dropped.
    std::size_t compact(qint64 nowMs);

    void setRetention(std::size_t rows, qint64 ms) noexcept {
        retention_rows_ = rows;
        retention_ms_ = ms;
    }

private:
    std::deque<JournalRecord> records_;
    QHash<QString, quint64> watermarks_;
    quint64 head_rev_ = 0;
    std::size_t retention_rows_ = kDefaultRetentionRows;
    qint64 retention_ms_ = kDefaultRetentionMs;
};

} // namespace mirror
