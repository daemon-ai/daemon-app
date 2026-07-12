// tst_mirror_observe — the FINALIZED observe seam (spec 09 §4.2; ADR-008). A3 re-ran gate 1 on a
// representative VM TU set (5 TUs incl. a QML-heavy and a TUI consumer): lager stayed decisively
// past the ≤15% clean / ≤10% incremental threshold (+50% clean / +53% incremental,
// ~+4.5 s/TU — see ADR-008-addendum-A3.md), confirming A1's default. LagerObserve was therefore
// DELETED; the coarse seam is the sole implementation.
//
// These tests re-target the invariants that used to justify lager (transactional visibility,
// glitch-freedom / exactly-one-notification-per-commit) at the surviving coarse seam: the Store
// publishes ONE atomic snapshot + ONE mirrorCommitted per non-empty batch, and per-reader
// equality gating is a cheap value compare over the immer snapshot (the coarse-seam equivalent of
// lager's has_changed gate).

#include "mirror/observe_coarse.h"
#include "mirror/store.h"

#include <QSignalSpy>
#include <QTest>

using namespace mirror;

namespace {
Conversation conv(const QString& id, const QString& title) {
    Conversation c;
    c.transport = QStringLiteral("m");
    c.id = id;
    c.title = title;
    return c;
}
} // namespace

class TstMirrorObserve : public QObject {
    Q_OBJECT

private slots:
    void emitsOncePerCommitWithRevRange() {
        CoarseObserve obs;
        Store store(obs);
        QSignalSpy spy(&obs, &CoarseObserve::mirrorCommitted);
        store.beginBatch().upsert(conv(QStringLiteral("!a"), QStringLiteral("A"))).commit();
        store.beginBatch().upsert(conv(QStringLiteral("!b"), QStringLiteral("B"))).commit();
        QCOMPARE(spy.count(), 2);
        const auto args = spy.at(1);
        QCOMPARE(args.at(0).toULongLong(), quint64(1)); // revFrom
        QCOMPARE(args.at(1).toULongLong(), quint64(2)); // revTo
        QCOMPARE(obs.seamName(), "coarse-signals");
    }

    void silentOnEmptyBatch() {
        CoarseObserve obs;
        Store store(obs);
        QSignalSpy spy(&obs, &CoarseObserve::mirrorCommitted);
        store.beginBatch().commit(); // nothing staged (§5.3 diff-before-write)
        QCOMPARE(spy.count(), 0);
        QCOMPARE(obs.commitCount(), 0);
    }

    void oneAtomicSnapshotAndNotificationPerBatch() {
        // The transactional-visibility guarantee lager gave, re-expressed at the coarse seam:
        // a whole multi-mutation batch publishes exactly ONE snapshot + ONE notification (§4.2),
        // and that snapshot is already the fully-applied new truth when the signal fires.
        CoarseObserve obs;
        Store store(obs);
        int seenSizeAtNotify = -1;
        int notifies = 0;
        QObject::connect(&obs, &CoarseObserve::mirrorCommitted, &obs, [&](quint64, quint64) {
            ++notifies;
            seenSizeAtNotify = static_cast<int>(store.snapshot().conversations.size());
        });
        auto b = store.beginBatch();
        b.upsert(conv(QStringLiteral("!a"), QStringLiteral("A")));
        b.upsert(conv(QStringLiteral("!b"), QStringLiteral("B")));
        b.upsert(conv(QStringLiteral("!c"), QStringLiteral("C")));
        b.commit();
        QCOMPARE(notifies, 1);         // one wave for the whole batch (no glitches / partials)
        QCOMPARE(seenSizeAtNotify, 3); // the published snapshot is the fully-applied truth
    }

    void valueCompareGatingIsTheCoarseEquivalentOfHasChanged() {
        // A scalar/derived reader under the coarse seam gates its own NOTIFY by value-comparing
        // the snapshot it cares about — cheap under immer structural sharing. Re-committing an
        // equal value stamps nothing, so the reader would see the SAME value and not re-notify.
        CoarseObserve obs;
        Store store(obs);
        const Conversation c = conv(QStringLiteral("!a"), QStringLiteral("A"));
        store.beginBatch().upsert(c).commit();
        const auto snap1 = store.snapshot().conversations;

        store.beginBatch().upsert(c).commit(); // identical -> diff-before-write suppresses it
        const auto snap2 = store.snapshot().conversations;

        // Structural sharing: the unchanged table compares equal by value (the gate a coarse
        // reader uses instead of lager's has_changed).
        QVERIFY(snap1 == snap2);
    }
};

QTEST_MAIN(TstMirrorObserve)
#include "tst_mirror_observe.moc"
