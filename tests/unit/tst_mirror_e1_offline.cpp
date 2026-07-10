// tst_mirror_e1_offline (mirror A5) — the E1 cold-boot-offline scenario (spec 09 §12, §4.5, §6.6).
// A prior online session persisted node truth into mirror-<id>.db (disposable) AND enqueued a
// ConvSend into local-<id>.db (precious). A cold boot with NO connection must render the
// last-known state from mirror.db AND still carry the pending op from local.db (the two-file split:
// a mirror nuke can never destroy pending user intent). This is the integrations-vertical E1
// parity assertion: offline shows last-known + pending, with NO auto-replay.

#include "local_database.h"
#include "mirror/mirror_service.h"
#include "outbox.h"
#include "pending_ops_model.h"
#include "verb_class.h"

#include <QChar>
#include <QObject>
#include <QTemporaryDir>
#include <QTest>

using namespace mirror;

namespace {
Conversation conv(const QString& t, const QString& id, const QString& title) {
    Conversation c;
    c.transport = t;
    c.id = id;
    c.title = title;
    return c;
}
ChatMessage chat(const QString& t, const QString& c, quint64 cursor, const QString& text) {
    ChatMessage m;
    m.transport = t;
    m.conv = c;
    m.cursor = cursor;
    m.text = text;
    return m;
}
} // namespace

class TstMirrorE1Offline : public QObject {
    Q_OBJECT

private slots:
    void coldBootOfflineShowsLastKnownAndPending() {
        QTemporaryDir dir;
        const QString mirrorDb = dir.filePath(QStringLiteral("mirror.db"));
        const QString localDb = dir.filePath(QStringLiteral("local.db"));
        FixedDbPaths paths(mirrorDb, localDb);

        // ---- online session: the ingestor persisted node truth; a ConvSend was enqueued ----
        {
            MirrorService svc;
            QVERIFY(svc.open(paths));
            svc.ingestor().deliverConversations(QStringLiteral("m"),
                                                {conv("m", "!room", "The Room")},
                                                /*isFinalPage=*/true);
            svc.ingestor().deliverChatPage(
                QStringLiteral("m"), QStringLiteral("!room"),
                {chat("m", "!room", 1, "hello"), chat("m", "!room", 2, "there")},
                /*nextCursor=*/2, /*headCursor=*/2);

            LocalDatabase ldb(localDb);
            QVERIFY(ldb.isOpen());
            Outbox outbox(&ldb);
            outbox.load();
            const QString opId = outbox.enqueue(
                QStringLiteral("ConvSend"), QStringLiteral("m\x1f!room"),
                QByteArrayLiteral("{\"message\":\"unsent\"}"), QStringLiteral("unsent"));
            QVERIFY(!opId.isEmpty());
        }

        // ---- cold boot: NO connection, NO executor. Render from the two DBs. ----
        {
            MirrorService svc; // no setFetchExecutor → no feeder
            QVERIFY(svc.open(paths));
            // Last-known M-table surfaces (conversations/persons/contacts) render from the
            // mirror.db boot-load (§4.5) — the migrated M2 read surfaces work offline.
            QCOMPARE(svc.store().snapshot().conversations.size(), std::size_t(1));
            const Conversation* room = svc.store().snapshot().conversations.find(
                ConversationKey{QStringLiteral("m"), QStringLiteral("!room")});
            QVERIFY(room != nullptr);
            QCOMPARE(room->title, QStringLiteral("The Room"));
            // The chat WINDOW is the disposable class-W cache: A1 fills a cold window forward on
            // demand (§4.6 interim) rather than reloading rows, so an offline chat timeline is
            // empty until reconnect — a documented A1/BR seam, not an A5 regression.
            const ChatMessageScope scope{QStringLiteral("m"), QStringLiteral("!room")};
            QCOMPARE(svc.store().snapshot().chat.count(scope), std::size_t(0));

            // The precious local.db is never dropped: the pending ConvSend survived the restart
            // (booted inflight→pending; §6.6) and renders in the pending strip — NOT as a mirror
            // row.
            LocalDatabase ldb(localDb);
            Outbox outbox(&ldb);
            outbox.load();
            QCOMPARE(outbox.laneDepth(buildLane(VerbClass::ChatSend, QStringLiteral("m\x1f!room"))),
                     1);
            PendingOpsModel strip;
            strip.setOutbox(&outbox);
            strip.setScopeFilter(QStringLiteral("m\x1f!room"));
            QCOMPARE(strip.count(), 1);

            // Auto-replay stays OFF: the op is present but was NOT sent on boot (manual drain
            // only).
            QVERIFY(!Outbox::autoReplayEnabled(38));
            QVERIFY(!Outbox::autoReplayEnabled(39));
        }
    }
};

QTEST_GUILESS_MAIN(TstMirrorE1Offline)
#include "tst_mirror_e1_offline.moc"
