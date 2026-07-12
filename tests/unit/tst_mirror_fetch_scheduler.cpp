// tst_mirror_fetch_scheduler — the bounded, deduplicating, band-ordered fetch queue (spec 09
// §5.4). Behaviors re-expressed (LGPL, behaviors only) from Akonadi's bounded pipeline + burst
// compression (references/architecture/akonadi/src/core/monitor_p.cpp:959-971,648-751,856-936):
// max-in-flight cap, one in-flight per coalesce key, newest-reason re-queue-once, band ordering.

#include "mirror/fetch_scheduler.h"

#include <QList>
#include <QObject>
#include <QTest>

using namespace mirror;

namespace {
// Records executed jobs; does NOT auto-complete (the test drives complete() to inspect state).
class RecordingExecutor : public FetchExecutor {
public:
    void execute(const FetchJob& job) override { executed.append(job); }
    QList<FetchJob> executed;
};

FetchJob job(FetchOp op, const QString& scope, Priority prio = Priority::Prefetch,
             quint64 reason = 1) {
    FetchJob j;
    j.op = op;
    j.scope = scope;
    j.priority = prio;
    j.reason = reason;
    return j;
}
} // namespace

class TstMirrorFetchScheduler : public QObject {
    Q_OBJECT

private slots:
    void dedupOneInFlightPerCoalesceKey() {
        // §5.4: a re-request while in flight is ABSORBED (no second execute for the same key).
        RecordingExecutor exec;
        FetchScheduler sched(exec);
        sched.enqueue(job(FetchOp::ConvList, QStringLiteral("matrix-1"), Priority::Prefetch, 1));
        QCOMPARE(exec.executed.size(), 1);
        QVERIFY(sched.isInflight(job(FetchOp::ConvList, QStringLiteral("matrix-1")).coalesceKey()));
        // Same coalesce key, same-or-older reason ⇒ absorbed.
        sched.enqueue(job(FetchOp::ConvList, QStringLiteral("matrix-1"), Priority::Prefetch, 1));
        QCOMPARE(exec.executed.size(), 1);
        QCOMPARE(sched.queuedCount(), 0);
    }

    void newerReasonReQueuesOnceAfterComplete() {
        // §5.4: a re-request with a NEWER reason re-queues exactly once (burst compression).
        RecordingExecutor exec;
        FetchScheduler sched(exec);
        const QString key = job(FetchOp::ConvList, QStringLiteral("m")).coalesceKey();
        sched.enqueue(job(FetchOp::ConvList, QStringLiteral("m"), Priority::Prefetch, 1));
        sched.enqueue(job(FetchOp::ConvList, QStringLiteral("m"), Priority::Prefetch, 2)); // newer
        sched.enqueue(job(FetchOp::ConvList, QStringLiteral("m"), Priority::Prefetch, 3)); // newer
        QCOMPARE(exec.executed.size(), 1); // still one in flight; newest pending collapses
        sched.complete(key);
        QCOMPARE(exec.executed.size(), 2);           // re-queued exactly once
        QCOMPARE(exec.executed.last().reason, 3ULL); // the NEWEST reason won
        sched.complete(key);
        QCOMPARE(exec.executed.size(), 2); // no further re-queue
    }

    void capsInFlightAtFour() {
        // §5.4 SPEC-DECISION: at most 4 concurrent jobs; the 5th waits.
        RecordingExecutor exec;
        FetchScheduler sched(exec); // default max 4
        for (int i = 0; i < 6; ++i) {
            sched.enqueue(job(FetchOp::ConvList, QStringLiteral("t%1").arg(i)));
        }
        QCOMPARE(sched.inflightCount(), 4);
        QCOMPARE(exec.executed.size(), 4);
        QCOMPARE(sched.queuedCount(), 2);
        // Completing one frees a slot and pumps the next.
        sched.complete(exec.executed.first().coalesceKey());
        QCOMPARE(exec.executed.size(), 5);
        QCOMPARE(sched.inflightCount(), 4);
    }

    void bandOrderingVisibleBeforePrefetchBeforeReconcile() {
        // §5.4: band-ordered FIFO — higher band drains first when the cap frees.
        RecordingExecutor exec;
        FetchScheduler sched(exec, /*maxInflight=*/1);
        sched.enqueue(job(FetchOp::Tree, QStringLiteral("a"), Priority::Reconcile,
                          1)); // executes now (cap 1)
        sched.enqueue(job(FetchOp::ConvList, QStringLiteral("b"), Priority::Reconcile, 2));
        sched.enqueue(job(FetchOp::ConvList, QStringLiteral("c"), Priority::VisibleSurface, 3));
        sched.enqueue(job(FetchOp::ConvList, QStringLiteral("d"), Priority::Prefetch, 4));
        QCOMPARE(exec.executed.size(), 1); // only the first (cap 1)
        sched.complete(exec.executed.first().coalesceKey());
        // Next should be the VisibleSurface job (c), not the older Reconcile (b).
        QCOMPARE(exec.executed.last().scope, QStringLiteral("c"));
        sched.complete(exec.executed.last().coalesceKey());
        QCOMPARE(exec.executed.last().scope, QStringLiteral("d")); // then Prefetch
        sched.complete(exec.executed.last().coalesceKey());
        QCOMPARE(exec.executed.last().scope, QStringLiteral("b")); // Reconcile last
    }

    void fifoWithinABand() {
        RecordingExecutor exec;
        FetchScheduler sched(exec, /*maxInflight=*/1);
        sched.enqueue(job(FetchOp::ConvList, QStringLiteral("first"), Priority::Prefetch, 1));
        sched.enqueue(job(FetchOp::ConvList, QStringLiteral("second"), Priority::Prefetch, 2));
        sched.enqueue(job(FetchOp::ConvList, QStringLiteral("third"), Priority::Prefetch, 3));
        QCOMPARE(exec.executed.size(), 1);
        QCOMPARE(exec.executed.at(0).scope, QStringLiteral("first"));
        sched.complete(exec.executed.last().coalesceKey());
        QCOMPARE(exec.executed.at(1).scope, QStringLiteral("second"));
        sched.complete(exec.executed.last().coalesceKey());
        QCOMPARE(exec.executed.at(2).scope, QStringLiteral("third"));
    }
};

QTEST_APPLESS_MAIN(TstMirrorFetchScheduler)
#include "tst_mirror_fetch_scheduler.moc"
