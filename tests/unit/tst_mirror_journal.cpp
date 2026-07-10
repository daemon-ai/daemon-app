// tst_mirror_journal — the ACTIVE degraded journal (spec 09 §4.3/§4.4; ADR-001).
//
// Invariants: store-wide monotonic in-memory rev minting seeded from the persisted MAX(rev);
// ordered delta tail above a watermark (§8.1 consumer feed); the watermark consumer contract
// with compaction floor = min(all live watermarks) and the 4096-row / 24h retention tail (§4.3).
//
// Behaviors around the watermark floor + old-revision cleanup are re-expressed (NOT copied) from
// Sink's LGPL resource bookkeeping — common/listener.cpp:308-320 (lowerBoundRevision = min over
// clients) and common/genericresource.cpp:163-173 (cleanupRevisions below that bound).

#include "mirror/journal.h"

#include <QObject>
#include <QTest>

using namespace mirror;

class TstMirrorJournal : public QObject {
    Q_OBJECT

private slots:
    void revMintingIsMonotonicFromZero() {
        Journal j;
        QCOMPARE(j.headRev(), quint64(0));
        const auto& r1 = j.stamp(EntityKind::Conversation,
                                 QStringLiteral("t\x1f"
                                                "a"),
                                 JournalOp::Upsert, JournalOrigin::RefetchDiff, QString(), 1000);
        const auto& r2 = j.stamp(EntityKind::Conversation,
                                 QStringLiteral("t\x1f"
                                                "b"),
                                 JournalOp::Upsert, JournalOrigin::RefetchDiff, QString(), 1000);
        QCOMPARE(r1.rev, quint64(1));
        QCOMPARE(r2.rev, quint64(2));
        QCOMPARE(j.headRev(), quint64(2));
    }

    void revCounterSeedsFromPersistedMax() {
        // Boot path (§4.3): the counter resumes above the persisted head so revs never collide.
        Journal j;
        j.seedHead(100);
        const auto& r = j.stamp(EntityKind::Session, QStringLiteral("s1"), JournalOp::Upsert,
                                JournalOrigin::RefetchDiff, QString(), 0);
        QCOMPARE(r.rev, quint64(101));
    }

    void originAndProvenanceAreRecorded() {
        Journal j;
        const auto& r =
            j.stamp(EntityKind::ChatMessage,
                    QStringLiteral("scope\x1f"
                                   "7"),
                    JournalOp::Upsert, JournalOrigin::RefetchDiff, QStringLiteral("op-abc"), 4242);
        QCOMPARE(r.origin, JournalOrigin::RefetchDiff); // degraded interim mode (§4.3)
        QCOMPARE(r.origin_op, QStringLiteral("op-abc"));
        QCOMPARE(r.at_ms, qint64(4242));
        QCOMPARE(r.op, JournalOp::Upsert);
    }

    void sinceReturnsOrderedTailAboveWatermark() {
        Journal j;
        for (int i = 0; i < 5; ++i) {
            j.stamp(EntityKind::Person, QStringLiteral("p%1").arg(i), JournalOp::Upsert,
                    JournalOrigin::RefetchDiff, QString(), 0);
        }
        const auto tail = j.since(2);
        QCOMPARE(int(tail.size()), 3);
        QCOMPARE(tail.front().rev, quint64(3));
        QCOMPARE(tail.back().rev, quint64(5));
        // revs are strictly ascending
        for (std::size_t i = 1; i < tail.size(); ++i) {
            QVERIFY(tail[i].rev > tail[i - 1].rev);
        }
    }

    void sinceFiltersByKind() {
        Journal j;
        j.stamp(EntityKind::Person, QStringLiteral("p"), JournalOp::Upsert,
                JournalOrigin::RefetchDiff, QString(), 0);
        j.stamp(EntityKind::Session, QStringLiteral("s"), JournalOp::Upsert,
                JournalOrigin::RefetchDiff, QString(), 0);
        j.stamp(EntityKind::Person, QStringLiteral("q"), JournalOp::Tombstone,
                JournalOrigin::RefetchDiff, QString(), 0);
        const auto persons = j.since(0, EntityKind::Person);
        QCOMPARE(int(persons.size()), 2);
        QCOMPARE(persons[0].key, QStringLiteral("p"));
        QCOMPARE(persons[1].op, JournalOp::Tombstone);
    }

    void compactionFloorIsMinAcrossLiveConsumers() {
        Journal j;
        for (int i = 0; i < 10; ++i) {
            j.stamp(EntityKind::Contact, QString::number(i), JournalOp::Upsert,
                    JournalOrigin::RefetchDiff, QString(), 0);
        }
        j.registerConsumer(QStringLiteral("persistence"));
        j.registerConsumer(QStringLiteral("chat-lens"));
        j.advanceWatermark(QStringLiteral("persistence"), 10);
        j.advanceWatermark(QStringLiteral("chat-lens"), 4);
        // Re-expressed Sink lowerBoundRevision: the slowest consumer bounds the floor.
        QCOMPARE(j.compactionFloor(), quint64(4));
    }

    void compactionFloorIsHeadWhenNoConsumers() {
        Journal j;
        j.stamp(EntityKind::Contact, QStringLiteral("x"), JournalOp::Upsert,
                JournalOrigin::RefetchDiff, QString(), 0);
        QCOMPARE(j.compactionFloor(), j.headRev());
    }

    void watermarkNeverRewinds() {
        Journal j;
        j.registerConsumer(QStringLiteral("c"));
        j.advanceWatermark(QStringLiteral("c"), 7);
        j.advanceWatermark(QStringLiteral("c"), 3); // stale advance ignored
        QCOMPARE(j.watermark(QStringLiteral("c")), quint64(7));
    }

    void teardownLiftsConsumerOutOfFloor() {
        // A lens teardown removes its watermark so a stalled surface cannot pin the floor
        // (§4.4: the Sink stalled-external-reader failure mode cannot occur in-process).
        Journal j;
        for (int i = 0; i < 6; ++i) {
            j.stamp(EntityKind::Room, QString::number(i), JournalOp::Upsert,
                    JournalOrigin::RefetchDiff, QString(), 0);
        }
        j.registerConsumer(QStringLiteral("persistence"));
        j.registerConsumer(QStringLiteral("stalled-lens"));
        j.advanceWatermark(QStringLiteral("persistence"), 6);
        j.advanceWatermark(QStringLiteral("stalled-lens"), 1);
        QCOMPARE(j.compactionFloor(), quint64(1));
        j.unregisterConsumer(QStringLiteral("stalled-lens"));
        QCOMPARE(j.compactionFloor(), quint64(6));
    }

    void compactionKeepsLargerOfRowAndAgeTail() {
        // §4.3 retention: keep 4096 rows OR 24h, whichever is LARGER. With a tiny row tail and a
        // zero age window, only rows below the floor AND past both tails are dropped.
        Journal j;
        j.setRetention(/*rows=*/2, /*ms=*/0);
        for (int i = 0; i < 10; ++i) {
            j.stamp(EntityKind::Contact, QString::number(i), JournalOp::Upsert,
                    JournalOrigin::RefetchDiff, QString(), /*at_ms=*/1000);
        }
        j.registerConsumer(QStringLiteral("persistence"));
        j.advanceWatermark(QStringLiteral("persistence"), 10); // floor = 10
        const std::size_t dropped = j.compact(/*nowMs=*/1'000'000);
        // Deletable iff rev<10 AND (head-rev)>=2 AND age past => keep the 2 newest (rev 9,10).
        QCOMPARE(dropped, std::size_t(8));
        QCOMPARE(j.size(), std::size_t(2));
        QCOMPARE(j.records().front().rev, quint64(9));
    }

    void compactionRespectsAgeWindow() {
        // With a generous age window, rows younger than it survive even below the floor.
        Journal j;
        j.setRetention(/*rows=*/0, /*ms=*/24LL * 60 * 60 * 1000);
        for (int i = 0; i < 5; ++i) {
            j.stamp(EntityKind::Contact, QString::number(i), JournalOp::Upsert,
                    JournalOrigin::RefetchDiff, QString(), /*at_ms=*/1'000'000);
        }
        j.registerConsumer(QStringLiteral("p"));
        j.advanceWatermark(QStringLiteral("p"), 5);
        // now only 1h after the rows => within the 24h window => nothing dropped.
        const std::size_t dropped = j.compact(/*nowMs=*/1'000'000 + 60LL * 60 * 1000);
        QCOMPARE(dropped, std::size_t(0));
        QCOMPARE(j.size(), std::size_t(5));
    }
};

QTEST_MAIN(TstMirrorJournal)
#include "tst_mirror_journal.moc"
