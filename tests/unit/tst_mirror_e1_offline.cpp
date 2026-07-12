// tst_mirror_e1_offline (mirror A5) — the E1 cold-boot-offline scenario (spec 09 §12, §4.5, §4.6,
// §6.6). A prior online session persisted node truth into mirror-<id>.db (disposable) AND enqueued
// a ConvSend into local-<id>.db (precious). A cold boot with NO connection must render the
// last-known state from mirror.db — the M tables AND the chat window's persisted contiguous tail —
// and still carry the pending op from local.db (the two-file split: a mirror nuke can never
// destroy pending user intent). Offline shows last-known + pending, with NO auto-replay.

#include "local_database.h"
#include "mirror/chat_window_model.h"
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
            // The chat window's persisted contiguous tail reloads too (§4.6 + E1 "last-known
            // state"): the OFFLINE TIMELINE IS NON-EMPTY. Forward-fill-on-demand remains the
            // reconnect top-up path, resuming after the seeded per-conv cursor.
            const ChatMessageScope scope{QStringLiteral("m"), QStringLiteral("!room")};
            QVERIFY(svc.store().snapshot().chat.count(scope) != 0U);
            const Window<ChatMessage>& win = svc.store().snapshot().chat[scope];
            QCOMPARE(static_cast<int>(win.items.size()), 2);
            QCOMPARE(win.items[0].get().text, QStringLiteral("hello"));
            QCOMPARE(win.items[1].get().text, QStringLiteral("there"));
            QCOMPARE(win.meta.newest_cursor, quint64(2));
            // The ChatWindowModel lens renders the reloaded tail (the surface-level assert).
            ChatWindowModel timeline(svc.store(), scope);
            QCOMPARE(timeline.rowCount(), 2);
            QCOMPARE(timeline.data(timeline.index(0, 0), ChatWindowModel::TextRole).toString(),
                     QStringLiteral("hello"));
            // The reconnect top-up resumes AFTER the persisted tail: the per-conv cursor was
            // seeded from the reloaded window, so the next MessagesChanged pages from cursor 2 —
            // never re-appending (or disordering) what boot restored.
            QCOMPARE(svc.ingestor().syncState().convCursor(QStringLiteral("m\x1f!room")),
                     quint64(2));

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

            // Offline boot never blindly resends: the op is present but was NOT sent (there is no
            // connection to drain to). The auto-replay gate (§6.8) is api-version-based — OFF at
            // api/38, ON from api/39 — but it is only consulted on an authenticated reconnect,
            // which has not happened here; the outbox never drained on boot.
            QVERIFY(!Outbox::autoReplayEnabled(38));
            QVERIFY(Outbox::autoReplayEnabled(39));
            QCOMPARE(outbox.op(outbox.ops().first().opId).state, OpState::Pending);
        }
    }
};

QTEST_GUILESS_MAIN(TstMirrorE1Offline)
#include "tst_mirror_e1_offline.moc"
