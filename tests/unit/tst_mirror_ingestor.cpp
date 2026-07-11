// tst_mirror_ingestor — the single event-policy authority (spec 09 §5). Synthetic NodeEvent
// streams (E4, §12) + a recording FetchExecutor + a real mirror::Store exercise: policy dispatch,
// the ConvHistory cursor fix (§13 M1), chat windows v1 (§4.6), diff-before-write no-op suppression
// (§5.3), ConversationsChanged keyed patch (§5.2), TransportChanged/DownloadProgress
// patch-in-place, ResyncNeeded scoping (§5.7), reconnect both modes (§5.6), and visibility
// staleness (§5.8).

#include "mirror/ingestor.h"
#include "mirror/observe_coarse.h"

#include <QList>
#include <QObject>
#include <QSignalSpy>
#include <QTest>
#include <vector>

using namespace mirror;

namespace {
class RecordingExecutor : public FetchExecutor {
public:
    void execute(const FetchJob& job) override { executed.append(job); }
    [[nodiscard]] bool has(FetchOp op, const QString& scope) const {
        for (const FetchJob& j : executed) {
            if (j.op == op && j.scope == scope) {
                return true;
            }
        }
        return false;
    }
    [[nodiscard]] FetchJob last(FetchOp op) const {
        for (int i = executed.size() - 1; i >= 0; --i) {
            if (executed.at(i).op == op) {
                return executed.at(i);
            }
        }
        return {};
    }
    [[nodiscard]] int count(FetchOp op) const {
        int n = 0;
        for (const FetchJob& j : executed) {
            if (j.op == op) {
                ++n;
            }
        }
        return n;
    }
    QList<FetchJob> executed;
};

NodeEvent ev(NodeEventKind k) {
    NodeEvent e;
    e.kind = k;
    return e;
}

ChatMessage chat(const QString& t, const QString& c, quint64 cursor, const QString& text) {
    ChatMessage m;
    m.transport = t;
    m.conv = c;
    m.cursor = cursor;
    m.text = text;
    return m;
}

Conversation conv(const QString& t, const QString& id, const QString& title) {
    Conversation c;
    c.transport = t;
    c.id = id;
    c.title = title;
    return c;
}
} // namespace

class TstMirrorIngestor : public QObject {
    Q_OBJECT

private:
    static constexpr int kBigCap = 64; // so scan/enqueue assertions see every job executed

private slots:
    void sessionAdvancedNudgesNeverFetches() {
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        QSignalSpy spy(&ing, &Ingestor::nudgeFocused);
        NodeEvent e = ev(NodeEventKind::SessionAdvanced);
        e.session = QStringLiteral("s1");
        ing.ingest(e);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(exec.executed.size(), 0); // never a fetch by itself (§5.2)
    }

    void unknownArmIsCountedNoOp() {
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        ing.ingest(ev(NodeEventKind::Unknown));
        QCOMPARE(ing.unknownArmCount(), 1);
        QCOMPARE(exec.executed.size(), 0);
    }

    void messagesChangedPagesFromStoredCursorNotZero() {
        // The rung-0 cursor fix (§13 M1): ConvHistory pages from the stored per-conv cursor.
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);

        NodeEvent e = ev(NodeEventKind::MessagesChanged);
        e.transport = QStringLiteral("matrix-1");
        e.conv = QStringLiteral("!room");
        ing.ingest(e);
        QVERIFY(exec.has(FetchOp::ConvHistory, QStringLiteral("matrix-1\x1f!room")));
        QCOMPARE(exec.last(FetchOp::ConvHistory).afterCursor, quint64(0)); // cold: from 0

        // Deliver a page 1..5; the per-conv cursor advances to 5. The fetch then completes (the
        // bridge does this after decode) so the scheduler frees the coalesce-key slot.
        std::vector<ChatMessage> page = {chat("matrix-1", "!room", 1, "a"),
                                         chat("matrix-1", "!room", 5, "b")};
        ing.deliverChatPage(QStringLiteral("matrix-1"), QStringLiteral("!room"), page, 5, 5);
        ing.fetchCompleted(exec.last(FetchOp::ConvHistory));

        // A second MessagesChanged must page from cursor 5 — NEVER a full refetch from 0.
        ing.ingest(e);
        QCOMPARE(exec.last(FetchOp::ConvHistory).afterCursor, quint64(5));
    }

    void beginObservingChatTopsUpFromStoredCursor() {
        // §5.8 + §4.6: a visible chat scope declares itself and the INGESTOR issues the top-up —
        // forward from the stored per-conv cursor (never a lens-driven refetch, never from 0 once
        // a cursor exists).
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);

        ing.beginObserving(QStringLiteral("chat"), QStringLiteral("m\x1f!r"));
        QVERIFY(exec.has(FetchOp::ConvHistory, QStringLiteral("m\x1f!r")));
        QCOMPARE(exec.last(FetchOp::ConvHistory).afterCursor, quint64(0)); // cold scope

        std::vector<ChatMessage> page = {chat("m", "!r", 1, "x"), chat("m", "!r", 2, "y")};
        ing.deliverChatPage(QStringLiteral("m"), QStringLiteral("!r"), page, 2, 2);
        ing.fetchCompleted(exec.last(FetchOp::ConvHistory));

        ing.endObserving(QStringLiteral("chat"), QStringLiteral("m\x1f!r"));
        ing.beginObserving(QStringLiteral("chat"), QStringLiteral("m\x1f!r"));
        QCOMPARE(exec.last(FetchOp::ConvHistory).afterCursor, quint64(2)); // tail read, not 0
    }

    void requestOlderColdWindowFillsForwardThenEnds() {
        // §4.6 demand paging against v38: a COLD window forward-fills from 0 (true); once the
        // window holds the reachable tail, older history is not expressible (false) until BR's
        // before_cursor — the mediator surfaces end-of-history.
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);

        QVERIFY(ing.requestOlder(QStringLiteral("m\x1f!r"), 64)); // cold: a fill is coming
        QVERIFY(exec.has(FetchOp::ConvHistory, QStringLiteral("m\x1f!r")));
        QCOMPARE(exec.last(FetchOp::ConvHistory).afterCursor, quint64(0));

        std::vector<ChatMessage> page = {chat("m", "!r", 1, "x")};
        ing.deliverChatPage(QStringLiteral("m"), QStringLiteral("!r"), page, 1, 1);
        QVERIFY(!ing.requestOlder(QStringLiteral("m\x1f!r"), 64)); // tail held: end-of-history
    }

    void chatWindowAppendsWithMeta() {
        // Windows v1 (§4.6): window items + window_meta accounting.
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        std::vector<ChatMessage> page = {chat("m", "!r", 1, "x"), chat("m", "!r", 2, "y"),
                                         chat("m", "!r", 3, "z")};
        ing.deliverChatPage(QStringLiteral("m"), QStringLiteral("!r"), page, 3, 3);
        const ChatMessageScope scope{QStringLiteral("m"), QStringLiteral("!r")};
        QVERIFY(store.snapshot().chat.count(scope));
        const auto& win = store.snapshot().chat[scope];
        QCOMPARE(win.items.size(), std::size_t(3));
        QCOMPARE(win.meta.newest_cursor, quint64(3));
        QVERIFY(win.meta.byte_count > 0);
    }

    void convListDeliveryUpsertsAndPrunes() {
        // Full-list replace-and-prune (PageLoop semantics, §5.3).
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        ing.deliverConversations(QStringLiteral("m"), {conv("m", "!a", "A"), conv("m", "!b", "B")},
                                 true);
        QCOMPARE(store.snapshot().conversations.size(), std::size_t(2));
        // A later full list without !b prunes it (tombstone).
        ing.deliverConversations(QStringLiteral("m"), {conv("m", "!a", "A")}, true);
        QCOMPARE(store.snapshot().conversations.size(), std::size_t(1));
        QVERIFY(store.snapshot().conversations.find(
                    ConversationKey{QStringLiteral("m"), QStringLiteral("!a")}) != nullptr);
    }

    void onConnectedFullModeIssuesBootstrapProbe() {
        // §5.6 full (BR): an api/39 connect issues ONE Bootstrap probe — not a blind full scan of
        // every collection. The executor decodes the probe and calls onBootstrap for the deltas.
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        ing.onConnected(39, /*hasRevFields=*/true);
        QCOMPARE(exec.count(FetchOp::Bootstrap), 1);
        QVERIFY(!exec.has(FetchOp::SessionsQuery, QString())); // no degraded staleness scan
    }

    void onConnectedDegradedScansStaleNotBootstrap() {
        // §5.6 interim (api/38): no Bootstrap; the degraded staleness scan re-baselines instead.
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        ing.onConnected(38, /*hasRevFields=*/false);
        QCOMPARE(exec.count(FetchOp::Bootstrap), 0);
        QVERIFY(exec.has(FetchOp::SessionsQuery, QString()));
    }

    void deliverConversationsDeltaUpsertsRemovedOnlyNoPrune() {
        // §5.6 full / §10.2: a since_rev delta carries ONLY changed items + a `removed` list.
        // Absent keys are UNCHANGED (never pruned — the essential difference from the full-list
        // path), and the collection rev is recorded on markFresh for the next rev-gate.
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        ing.deliverConversations(QStringLiteral("m"), {conv("m", "!a", "A"), conv("m", "!b", "B")},
                                 true);
        QCOMPARE(store.snapshot().conversations.size(), std::size_t(2));

        // Delta: change !a, remove nothing. !b is absent from the page but MUST survive (a
        // full-list deliver would prune it here).
        ing.deliverConversationsDelta(QStringLiteral("m"), {conv("m", "!a", "A2")}, {}, 5, true);
        QCOMPARE(store.snapshot().conversations.size(), std::size_t(2));
        QCOMPARE(ing.syncState().collection(QStringLiteral("conversations")).nodeRev, quint64(5));

        // Delta: remove !b via the tombstone list.
        ing.deliverConversationsDelta(QStringLiteral("m"), {}, {QStringLiteral("!b")}, 6, true);
        QCOMPARE(store.snapshot().conversations.size(), std::size_t(1));
        QVERIFY(store.snapshot().conversations.find(
                    ConversationKey{QStringLiteral("m"), QStringLiteral("!a")}) != nullptr);
        QCOMPARE(ing.syncState().collection(QStringLiteral("conversations")).nodeRev, quint64(6));
    }

    void diffBeforeWriteSuppressesNoOpDelivery() {
        // §5.3: re-delivering an identical list stamps nothing (no journal growth, no churn).
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        ing.deliverConversations(QStringLiteral("m"), {conv("m", "!a", "A")}, true);
        const quint64 head = store.journal().headRev();
        ing.deliverConversations(QStringLiteral("m"), {conv("m", "!a", "A")}, true); // identical
        QCOMPARE(store.journal().headRev(), head);                                   // no new rev
    }

    void conversationsChangedRemovedTombstonesLocally() {
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        ing.deliverConversation(conv("m", "!a", "A"));
        QCOMPARE(store.snapshot().conversations.size(), std::size_t(1));
        NodeEvent e = ev(NodeEventKind::ConversationsChanged);
        e.transport = QStringLiteral("m");
        e.conv = QStringLiteral("!a");
        e.convChange = QStringLiteral("removed");
        ing.ingest(e);
        QCOMPARE(store.snapshot().conversations.size(), std::size_t(0)); // tombstoned locally
    }

    void conversationsChangedAddedEnqueuesConvGetViaLane() {
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        NodeEvent e = ev(NodeEventKind::ConversationsChanged);
        e.transport = QStringLiteral("m");
        e.conv = QStringLiteral("!new");
        e.convChange = QStringLiteral("added");
        ing.ingest(e);
        // Per-transport-conv lane debounces 200 ms; wait for the flush.
        QTRY_VERIFY_WITH_TIMEOUT(exec.has(FetchOp::ConvGet, QStringLiteral("m\x1f!new")), 2000);
    }

    void transportChangedPatchesAndFetchesOnConnect() {
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        NodeEvent e = ev(NodeEventKind::TransportChanged);
        e.transport = QStringLiteral("matrix-1");
        e.connection = QStringLiteral("connected");
        e.presence = QStringLiteral("available");
        e.hasPresence = true;
        ing.ingest(e);
        const TransportAccount* a = store.snapshot().transport_accounts.find(
            TransportAccountKey{QStringLiteral("matrix-1")});
        QVERIFY(a != nullptr);
        QCOMPARE(a->connection, QStringLiteral("connected"));
        QVERIFY(exec.has(FetchOp::ConvList, QStringLiteral("matrix-1"))); // →Connected refresh
    }

    void downloadProgressPatchesInPlace() {
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        NodeEvent e = ev(NodeEventKind::DownloadProgress);
        e.downloadId = 7;
        e.downloadState = QStringLiteral("downloading");
        e.downloadedBytes = 512;
        e.totalBytes = 1024;
        ing.ingest(e);
        const ModelDownload* d =
            store.snapshot().model_downloads.find(ModelDownloadKey{QStringLiteral("7")});
        QVERIFY(d != nullptr);
        QCOMPARE(d->downloaded_bytes, quint64(512));
        QCOMPARE(exec.executed.size(), 0); // patch-in-place is never a fetch
    }

    void resyncAllScopeScansAllStaleNotAStorm() {
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        QSignalSpy spy(&ing, &Ingestor::resyncScanStarted);
        NodeEvent e = ev(NodeEventKind::ResyncNeeded);
        e.scope = QStringLiteral("all");
        ing.ingest(e);
        QCOMPARE(spy.count(), 1);
        // Bounded, deduped, priority-banded scan of the stale collections (not a blind storm).
        QVERIFY(exec.has(FetchOp::SessionsQuery, QString()));
        QVERIFY(exec.has(FetchOp::Tree, QString()));
        QVERIFY(exec.has(FetchOp::ProfileList, QString()));
        QVERIFY(exec.has(FetchOp::PersonList, QString()));
    }

    void resyncProfilesScopeScansOnlyProfiles() {
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        NodeEvent e = ev(NodeEventKind::ResyncNeeded);
        e.scope = QStringLiteral("profiles");
        ing.ingest(e);
        QVERIFY(exec.has(FetchOp::ProfileList, QString()));
        QVERIFY(
            !exec.has(FetchOp::SessionsQuery, QString())); // never re-baselines unrelated (§5.7)
    }

    void reconnectDegradedScansStaleOnly() {
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        ing.syncState().markStale(QStringLiteral("sessions"));
        ing.syncState().markFresh(QStringLiteral("profiles"), 0, 1); // fresh ⇒ not scanned
        ing.onConnected(38, false);
        QVERIFY(exec.has(FetchOp::SessionsQuery, QString()));
        QVERIFY(!exec.has(FetchOp::ProfileList, QString()));
    }

    void reconnectFullBootstrapSkipsUnchangedRevs() {
        // §5.6 full: rev==stored ⇒ skip; else since_rev delta read.
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        ing.syncState().markFresh(QStringLiteral("profiles"), 42, 1); // stored rev 42
        QHash<QString, quint64> revs;
        revs.insert(QStringLiteral("profiles"), 42); // unchanged ⇒ skip
        revs.insert(QStringLiteral("sessions"), 99); // changed ⇒ delta read
        ing.onBootstrap(/*cursor=*/100, /*epoch=*/1, revs);
        QVERIFY(!exec.has(FetchOp::ProfileList, QString()));
        QVERIFY(exec.has(FetchOp::SessionsQuery, QString()));
        QVERIFY(exec.last(FetchOp::SessionsQuery).fullMode); // wire_delta intent
        QCOMPARE(ing.syncState().feedCursor(), quint64(100));
    }

    void bootstrapEpochChangeReBaselinesAll() {
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        ing.onBootstrap(10, 1, {}); // establish epoch 1
        ing.syncState().markFresh(QStringLiteral("profiles"), 42, 1);
        QHash<QString, quint64> revs;
        revs.insert(QStringLiteral("profiles"), 42); // same rev, but epoch changed ⇒ unservable
        ing.onBootstrap(20, 2, revs);                // epoch 1 → 2
        QVERIFY(exec.has(FetchOp::ProfileList, QString())); // full read despite rev match
        QCOMPARE(exec.last(FetchOp::ProfileList).sinceRev, quint64(0)); // 0 ⇒ full read
    }

    void visibilityFetchesWhenObservedCollectionIsStale() {
        // §5.8: a lens declares visibility; the ingestor decides the fetch (never a lens callsite).
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        ing.syncState().markStale(QStringLiteral("conversations"));
        ing.beginObserving(QStringLiteral("conversations"), QStringLiteral("matrix-1"));
        QVERIFY(exec.has(FetchOp::ConvList, QStringLiteral("matrix-1")));
        QCOMPARE(exec.last(FetchOp::ConvList).priority, Priority::VisibleSurface);
    }

    void visibilityDoesNotFetchWhenFresh() {
        CoarseObserve obs;
        Store store(obs);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);
        ing.setClock([] { return qint64(1000); });
        ing.syncState().markFresh(QStringLiteral("contacts"), 0, 1000);
        ing.setObservationMaxAgeMs(QStringLiteral("contacts"), 60000);
        ing.beginObserving(QStringLiteral("contacts"), QStringLiteral("matrix-1"));
        QCOMPARE(exec.count(FetchOp::RosterList), 0); // fresh + within max-age ⇒ no fetch
    }
};

QTEST_GUILESS_MAIN(TstMirrorIngestor)
#include "tst_mirror_ingestor.moc"
