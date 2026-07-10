// tst_mirror_observe — the swappable observe seam (spec 09 §4.2; ADR-008 spike). Proves BOTH
// conforming implementations deliver exactly one notification wave per commit and expose the
// committed snapshot consistently: CoarseObserve (a per-commit signal) and LagerObserve
// (lager::state<transactional_tag> + lager::commit).
//
// The lager transactional-visibility + glitch-free two-phase behaviors are re-expressed
// (adapted, MIT, attribution) from lager's own suite:
//   - transactional set-then-commit visibility: references/architecture/lager/test/state.cpp:
//     51-74,102-141
//   - derived reader value tracking: references/architecture/lager/test/cursor.cpp:81-107
//   - single notification per commit / glitch-free: test/detail/nodes.cpp:36-88,138-164

#include "mirror/observe_coarse.h"
#include "mirror/observe_lager.h"
#include "mirror/store.h"

#include <lager/commit.hpp>
#include <lager/lenses/attr.hpp>
#include <lager/state.hpp>
#include <lager/watch.hpp>
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
    // ---- CoarseObserve -----------------------------------------------------------------------
    void coarseEmitsOncePerCommitWithRevRange() {
        CoarseObserve obs;
        Store store(obs);
        QSignalSpy spy(&obs, &CoarseObserve::mirrorCommitted);
        store.beginBatch().upsert(conv(QStringLiteral("!a"), QStringLiteral("A"))).commit();
        store.beginBatch().upsert(conv(QStringLiteral("!b"), QStringLiteral("B"))).commit();
        QCOMPARE(spy.count(), 2);
        const auto args = spy.at(1);
        QCOMPARE(args.at(0).toULongLong(), quint64(1)); // revFrom
        QCOMPARE(args.at(1).toULongLong(), quint64(2)); // revTo
    }

    void coarseSilentOnEmptyBatch() {
        CoarseObserve obs;
        Store store(obs);
        QSignalSpy spy(&obs, &CoarseObserve::mirrorCommitted);
        store.beginBatch().commit(); // nothing staged
        QCOMPARE(spy.count(), 0);
        QCOMPARE(obs.seamName(), "coarse-signals");
    }

    // ---- LagerObserve ------------------------------------------------------------------------
    void lagerNotifiesOncePerCommit() {
        LagerObserve obs;
        Store store(obs);
        store.beginBatch().upsert(conv(QStringLiteral("!a"), QStringLiteral("A"))).commit();
        store.beginBatch().upsert(conv(QStringLiteral("!b"), QStringLiteral("B"))).commit();
        // Exactly one root notification per commit (glitch-free two-phase; nodes.cpp behavior).
        QCOMPARE(obs.notifyCount(), 2);
        QCOMPARE(obs.seamName(), "lager-headers");
    }

    void lagerReaderReflectsCommittedSnapshot() {
        LagerObserve obs;
        Store store(obs);
        store.beginBatch()
            .upsert(conv(QStringLiteral("!a"), QStringLiteral("A")))
            .upsert(conv(QStringLiteral("!b"), QStringLiteral("B")))
            .commit();
        // The reader's committed value is the published snapshot.
        QCOMPARE(obs.current().conversations.size(), std::size_t(2));
        lager::reader<MirrorModel> r = obs.reader();
        QCOMPARE(r.get().conversations.size(), std::size_t(2));
    }

    // ---- lager reactive-core ports (the seam mechanics candidate (a) relies on) --------------
    void lagerTransactionalSetIsInvisibleUntilCommit() {
        auto s = lager::make_state(0, lager::transactional_tag{});
        int notifies = 0;
        int lastSeen = -1;
        lager::watch(s, [&](int v) {
            ++notifies;
            lastSeen = v;
        });
        s.set(5);
        QCOMPARE(notifies, 0); // transactional: staged, not yet visible
        QCOMPARE(s.get(), 0);  // committed value unchanged
        lager::commit(s);
        QCOMPARE(notifies, 1); // exactly one notification on commit
        QCOMPARE(lastSeen, 5);
        QCOMPARE(s.get(), 5);
    }

    void lagerBatchedSetsNotifyOnceOnCommit() {
        auto s = lager::make_state(0, lager::transactional_tag{});
        int notifies = 0;
        lager::watch(s, [&](int) { ++notifies; });
        s.set(1);
        s.set(2);
        s.set(3);
        lager::commit(s);
        QCOMPARE(notifies, 1); // a whole batch => one notification wave (§4.2 mechanism)
        QCOMPARE(s.get(), 3);
    }

    void lagerDerivedReaderTracksCommittedValue() {
        auto s = lager::make_state(2, lager::transactional_tag{});
        lager::reader<int> doubled = s.map([](int x) { return x * 2; });
        QCOMPARE(doubled.get(), 4);
        s.set(10);
        lager::commit(s);
        QCOMPARE(doubled.get(), 20); // derived reader reflects the new committed value
    }
};

QTEST_MAIN(TstMirrorObserve)
#include "tst_mirror_observe.moc"
