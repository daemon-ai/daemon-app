// tst_conv_send_lane (mirror A5) — ConvSend routed through the durable outbox lane (spec 09 §7,
// §6.4/§6.5/§6.8, §8.4). Asserts: enqueue lands a per-conversation pending row (not a mirror row);
// the strip filters per conversation; drain is MANUAL (no auto-replay for api/38 OR api/39); ack
// clears the op; a rejection pauses the lane and surfaces retry/edit/discard.

#include "conv_send_controller.h"
#include "local_database.h"
#include "outbox.h"
#include "pending_ops_model.h"
#include "verb_class.h"

#include <QChar>
#include <QObject>
#include <QSignalSpy>
#include <QTest>

using namespace mirror;

namespace {
QString scope(const QString& t, const QString& c) {
    return t + QChar(0x1f) + c;
}
} // namespace

class TstConvSendLane : public QObject {
    Q_OBJECT

    LocalDatabase* db = nullptr;
    Outbox* outbox = nullptr;

private slots:
    void init() {
        db = new LocalDatabase(QStringLiteral(":memory:"), this);
        QVERIFY(db->isOpen());
        outbox = new Outbox(db, this);
        outbox->setGate([](const QString&, VerbClass) { return true; }); // connected+authed
        outbox->load();
    }
    void cleanup() {
        delete outbox;
        outbox = nullptr;
        delete db;
        db = nullptr;
    }

    void enqueuePendingRowNoMirrorRow() {
        ConvSendController c;
        c.setOutbox(outbox);
        c.open(QStringLiteral("m"), QStringLiteral("!room"));
        QSignalSpy enq(&c, &ConvSendController::enqueued);

        const QString opId = c.send(QStringLiteral("hello world"));
        QVERIFY(!opId.isEmpty());
        QCOMPARE(enq.count(), 1);
        // The pending strip (outbox lens) shows exactly this op — pending, distinct from any
        // ChatMessage (R2: no fake mirror row exists; the outbox is the ONLY pending surface).
        QCOMPARE(c.pendingCount(), 1);
        auto* model = qobject_cast<PendingOpsModel*>(c.pending());
        QVERIFY(model != nullptr);
        QCOMPARE(model->rowCount(), 1);
        const QModelIndex idx = model->index(0, 0);
        QCOMPARE(model->data(idx, PendingOpsModel::VerbRole).toString(),
                 QStringLiteral("ConvSend"));
        QCOMPARE(model->data(idx, PendingOpsModel::StateRole).toInt(),
                 static_cast<int>(OpState::Pending));
        // The lane is the chat-send lane for this conversation's scope (§6.4).
        QCOMPARE(model->data(idx, PendingOpsModel::LaneRole).toString(),
                 buildLane(VerbClass::ChatSend, scope("m", "!room")));
    }

    void pendingStripFiltersPerConversation() {
        ConvSendController a;
        a.setOutbox(outbox);
        a.open(QStringLiteral("m"), QStringLiteral("!a"));
        ConvSendController b;
        b.setOutbox(outbox);
        b.open(QStringLiteral("m"), QStringLiteral("!b"));

        a.send(QStringLiteral("to a"));
        a.send(QStringLiteral("to a again"));
        b.send(QStringLiteral("to b"));

        QCOMPARE(a.pendingCount(), 2); // only conversation !a's ops
        QCOMPARE(b.pendingCount(), 1); // only conversation !b's ops
    }

    void drainIsManualAckClears() {
        ConvSendController c;
        c.setOutbox(outbox);
        c.open(QStringLiteral("m"), QStringLiteral("!room"));
        QSignalSpy sent(outbox, &Outbox::sendRequested);

        const QString opId = c.send(QStringLiteral("hi"));
        QCOMPARE(sent.count(), 0); // NOTHING sent on enqueue — durable-first, drain is explicit

        c.drain();
        QCOMPARE(sent.count(), 1); // the manual tap sent the head

        // The owning seam performs the wire ConvSend and reports the ack; provenance-keyed confirm
        // (or v38 ack-terminal) clears the op — the pending row disappears.
        outbox->onAck(opId);
        QCOMPARE(c.pendingCount(), 0);
    }

    void rejectionPausesLaneAndAffords() {
        ConvSendController c;
        c.setOutbox(outbox);
        c.open(QStringLiteral("m"), QStringLiteral("!room"));

        const QString opId = c.send(QStringLiteral("bad"));
        c.drain();
        outbox->onRejection(opId, QStringLiteral("Forbidden"), QStringLiteral("nope"));

        QVERIFY(c.lanePaused());
        auto* model = qobject_cast<PendingOpsModel*>(c.pending());
        QCOMPARE(model->data(model->index(0, 0), PendingOpsModel::StateRole).toInt(),
                 static_cast<int>(OpState::Rejected));

        // retry unpauses + returns the op to pending (same op-id, dedup-safe).
        c.retry(opId);
        QVERIFY(!c.lanePaused());
        QCOMPARE(model->data(model->index(0, 0), PendingOpsModel::StateRole).toInt(),
                 static_cast<int>(OpState::Pending));

        // edit reopens the text and discards the old op (a new op-id is minted on re-send).
        const QString again = c.send(QStringLiteral("worse"));
        outbox->onRejection(again, QStringLiteral("Forbidden"), QStringLiteral("nope"));
        const QString text = c.takeForEdit(again);
        QCOMPARE(text, QStringLiteral("worse"));
    }

    void autoReplayStaysOff() {
        // The core M2 invariant (§6.8): the outbox never auto-replays — not against v38, and NOT
        // against api/39 either (BR flips it later, per connection). A5 asserts it stays OFF.
        QVERIFY(!ConvSendController::autoReplayEnabled(38));
        QVERIFY(!ConvSendController::autoReplayEnabled(39));
        QVERIFY(!ConvSendController::autoReplayEnabled(40));
    }
};

QTEST_GUILESS_MAIN(TstConvSendLane)
#include "tst_conv_send_lane.moc"
