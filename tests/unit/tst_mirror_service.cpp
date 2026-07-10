// tst_mirror_service (mirror A5) — the live-wiring composition root (spec 09 §2/§4/§5, wave M2).
// Asserts: (1) an ingested feed reaches the store through the ingestor and persists via
// write-behind flush-on-commit; (2) a cold boot renders migrated surfaces from mirror.db with NO
// connection (offline render, §12); (3) a fetch job triggered by an event is forwarded to the live
// FetchExecutor delegate (the scheduler→executor edge A5 wires); (4) dual-write parity holds
// between the mirror row-set and the (simulated) legacy row-set at quiesce (§13).

#include "mirror/fetch_job.h"
#include "mirror/fetch_scheduler.h"
#include "mirror/mirror_service.h"
#include "mirror/parity.h"

#include <QObject>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>
#include <vector>

using namespace mirror;

namespace {

Conversation conv(const QString& t, const QString& id, const QString& title) {
    Conversation c;
    c.transport = t;
    c.id = id;
    c.title = title;
    return c;
}

// Records every job the scheduler forwards through the service's delegate seam (the live edge).
class RecordingExecutor : public FetchExecutor {
public:
    void execute(const FetchJob& job) override { executed.push_back(job); }
    [[nodiscard]] bool sawOp(FetchOp op) const {
        for (const FetchJob& j : executed) {
            if (j.op == op) {
                return true;
            }
        }
        return false;
    }
    std::vector<FetchJob> executed;
};

} // namespace

class TstMirrorService : public QObject {
    Q_OBJECT

private slots:
    void ingestPersistsAndReloadsOffline() {
        QTemporaryDir dir;
        FixedDbPaths paths(dir.filePath(QStringLiteral("mirror.db")), QString());

        {
            MirrorService svc;
            QVERIFY(svc.open(paths));
            QVERIFY(svc.persistent());
            // A "live feed" arrives: a ConvList page for transport 'm' lands as the read path.
            svc.ingestor().deliverConversations(
                QStringLiteral("m"), {conv("m", "!a", "Alpha"), conv("m", "!b", "Bravo")},
                /*isFinalPage=*/true);
            // flush-on-commit persisted it; the in-memory snapshot has both rows.
            QCOMPARE(svc.store().snapshot().conversations.size(), std::size_t(2));
        }

        // Cold boot: a fresh service over the SAME db, with NO executor / NO connection, must
        // render the last-known state from mirror.db (offline render, §12 E1).
        {
            MirrorService svc;
            QVERIFY(svc.open(paths));
            QCOMPARE(svc.store().snapshot().conversations.size(), std::size_t(2));
            const Conversation* a = svc.store().snapshot().conversations.find(
                ConversationKey{QStringLiteral("m"), QStringLiteral("!a")});
            QVERIFY(a != nullptr);
            QCOMPARE(a->title, QStringLiteral("Alpha"));
        }
    }

    void eventTriggersForwardedFetch() {
        MirrorService svc;
        svc.openInMemory();
        RecordingExecutor exec;
        svc.setFetchExecutor(&exec);

        // A ConversationsChanged (added) event must schedule a ConvGet fetch job, forwarded to the
        // live executor delegate through the scheduler — the wiring A5 owns.
        NodeEvent e;
        e.kind = NodeEventKind::ConversationsChanged;
        e.transport = QStringLiteral("m");
        e.conv = QStringLiteral("!c");
        e.convChange = QStringLiteral("added");
        svc.ingest(e);
        // The per-transport ConversationsChanged lane is debounced (200 ms, §5.2); flush it.
        QTest::qWait(300);
        QVERIFY(!exec.executed.empty());
    }

    void dualWriteParityHolds() {
        MirrorService svc;
        svc.openInMemory();
        svc.ingestor().deliverConversations(QStringLiteral("m"),
                                            {conv("m", "!a", "A"), conv("m", "!b", "B")},
                                            /*isFinalPage=*/true);
        // The legacy TransportRegistry would hold the same two conversation keys for transport 'm'.
        const QSet<QString> legacy = {QStringLiteral("m\x1f!a"), QStringLiteral("m\x1f!b")};
        const parity::Result r = parity::compareKeys(
            parity::conversationKeys(svc.store().snapshot(), QStringLiteral("m")), legacy);
        QVERIFY(r.matches());
    }

    void inMemoryDoesNotPersist() {
        QTemporaryDir dir;
        FixedDbPaths paths(dir.filePath(QStringLiteral("mirror.db")), QString());
        {
            MirrorService svc;
            svc.openInMemory();
            svc.ingestor().deliverConversations(QStringLiteral("m"), {conv("m", "!a", "A")},
                                                /*isFinalPage=*/true);
        }
        // Nothing was written to the (never-opened) db path; a persistent boot finds an empty
        // store.
        MirrorService svc;
        QVERIFY(svc.open(paths));
        QCOMPARE(svc.store().snapshot().conversations.size(), std::size_t(0));
    }
};

QTEST_GUILESS_MAIN(TstMirrorService)
#include "tst_mirror_service.moc"
