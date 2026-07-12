// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// tst_provenance_landing (mirror A8) — the provenance-landing seam deferred to A8 by BR/A7
// (spec 09 §5.1/§6.6). The single-writer commit path doubles as the outbox's confirmation feed:
// MirrorService::provenanceStamped re-emits every journal record's non-empty origin_op, which the
// graph wires to Outbox::onProvenanceStamped. This suite exercises the END-TO-END state machine
// across the substrate↔outbox seam (the outbox's own transitions are covered by tst_outbox):
//
//   enqueue → drain → inflight → onAck → accepted (api/39) → provenance echo → landed (deleted)
//   enqueue → drain → onRejection → lane paused (§6.5)
//   enqueue → drain → onAck → accepted → ack-without-echo → expireAcceptedOps → disposed (§6.6)
//   api/38: an ack is terminal success (no provenance contract) — deleted on ack
//
// The provenance stamp originates from a REAL ingestor apply carrying origin_op (a conversation
// delta and a chat page — BR landed chat origin_op), proving the coupling is live, not simulated.

#include "local_database.h"
#include "mirror/mirror_service.h"
#include "outbox.h"
#include "outbox_types.h"
#include "verb_class.h"

#include <QObject>
#include <QTemporaryDir>
#include <QtTest>
#include <vector>

using namespace mirror;

namespace {

// Conversation provenance rides the deliverConversation() parameter (the journal record's
// origin_op), not an entity field — only class-W ChatMessage carries origin_op inline.
Conversation conv(const QString& t, const QString& id, const QString& title) {
    Conversation c;
    c.transport = t;
    c.id = id;
    c.title = title;
    return c;
}

ChatMessage chatMsg(const QString& t, const QString& cv, quint64 cursor, const QString& text,
                    const QString& originOp) {
    ChatMessage m;
    m.transport = t;
    m.conv = cv;
    m.cursor = cursor;
    m.text = text;
    m.origin_op = originOp;
    return m;
}

} // namespace

class TstProvenanceLanding : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { qRegisterMetaType<mirror::PendingOp>("mirror::PendingOp"); }

    void init() {
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
    }
    void cleanup() { m_dir.reset(); }

    // A conversation delta carrying origin_op == op_id lands the accepted op (ack → echo → landed).
    void ackThenConversationEchoLands() {
        MirrorService svc;
        svc.openInMemory();
        LocalDatabase db(m_dir->filePath(QStringLiteral("local.db")));
        Outbox ob(&db);
        ob.setProvenanceCapable(true); // api/39
        ob.setGate([](const QString&, VerbClass) { return true; });
        QObject::connect(&svc, &MirrorService::provenanceStamped, &ob,
                         &Outbox::onProvenanceStamped);

        const QString opId = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("m\037!r"),
                                        QByteArrayLiteral("payload"), QStringLiteral("hi"));
        QVERIFY(!opId.isEmpty());
        ob.drain();
        ob.onAck(opId);
        QCOMPARE(ob.op(opId).state, OpState::Accepted); // capable: awaits provenance, not deleted

        // The node's read path echoes the change stamped with the causing op-id (§10.3). The
        // ingestor applies it; the commit re-emits the stamp; the outbox lands the op.
        svc.ingestor().deliverConversation(
            conv(QStringLiteral("m"), QStringLiteral("!r"), QStringLiteral("Room")), opId);
        QVERIFY(!ob.contains(opId)); // landed → terminal success → row deleted
    }

    // The same contract via a chat-window apply (BR's chat origin_op on the ChatMessage entity).
    void chatPageEchoLands() {
        MirrorService svc;
        svc.openInMemory();
        LocalDatabase db(m_dir->filePath(QStringLiteral("local.db")));
        Outbox ob(&db);
        ob.setProvenanceCapable(true);
        ob.setGate([](const QString&, VerbClass) { return true; });
        QObject::connect(&svc, &MirrorService::provenanceStamped, &ob,
                         &Outbox::onProvenanceStamped);

        const QString opId = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("m\037!r"),
                                        QByteArrayLiteral("payload"), QStringLiteral("hey"));
        ob.drain();
        ob.onAck(opId);
        QCOMPARE(ob.op(opId).state, OpState::Accepted);

        std::vector<ChatMessage> page = {
            chatMsg(QStringLiteral("m"), QStringLiteral("!r"), 1, QStringLiteral("hey"), opId)};
        svc.ingestor().deliverChatPage(QStringLiteral("m"), QStringLiteral("!r"), page,
                                       /*nextCursor=*/2, /*headCursor=*/2);
        QVERIFY(!ob.contains(opId));
    }

    // An unrelated echo (a different op's provenance) never lands my op.
    void unrelatedEchoDoesNotLand() {
        MirrorService svc;
        svc.openInMemory();
        LocalDatabase db(m_dir->filePath(QStringLiteral("local.db")));
        Outbox ob(&db);
        ob.setProvenanceCapable(true);
        ob.setGate([](const QString&, VerbClass) { return true; });
        QObject::connect(&svc, &MirrorService::provenanceStamped, &ob,
                         &Outbox::onProvenanceStamped);

        const QString opId = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("m\037!r"),
                                        QByteArrayLiteral("payload"), QStringLiteral("hi"));
        ob.drain();
        ob.onAck(opId);
        svc.ingestor().deliverConversation(
            conv(QStringLiteral("m"), QStringLiteral("!r"), QStringLiteral("Room")),
            QStringLiteral("some-other-op"));
        QVERIFY(ob.contains(opId)); // still accepted; my op was not the cause
        QCOMPARE(ob.op(opId).state, OpState::Accepted);
    }

    // A node rejection pauses the lane (§6.5) — the mandatory rejection path.
    void rejectionPausesLane() {
        MirrorService svc;
        svc.openInMemory();
        LocalDatabase db(m_dir->filePath(QStringLiteral("local.db")));
        Outbox ob(&db);
        ob.setProvenanceCapable(true);
        ob.setGate([](const QString&, VerbClass) { return true; });
        QObject::connect(&svc, &MirrorService::provenanceStamped, &ob,
                         &Outbox::onProvenanceStamped);

        const QString opId = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("m\037!r"),
                                        QByteArrayLiteral("payload"), QStringLiteral("boom"));
        ob.drain();
        ob.onRejection(opId, QStringLiteral("Forbidden"), QStringLiteral("no"));
        QVERIFY(ob.contains(opId));
        QCOMPARE(ob.op(opId).state, OpState::Rejected);
        QVERIFY(ob.lanePaused(ob.op(opId).lane));
    }

    // Ack without an ever-arriving echo: the accepted op is disposed after the grace window (§6.6).
    void ackWithoutEchoExpires() {
        MirrorService svc;
        svc.openInMemory();
        LocalDatabase db(m_dir->filePath(QStringLiteral("local.db")));
        Outbox ob(&db);
        ob.setProvenanceCapable(true);
        qint64 clock = 1'000'000;
        ob.setClock([&clock] { return clock; });
        ob.setGate([](const QString&, VerbClass) { return true; });
        QObject::connect(&svc, &MirrorService::provenanceStamped, &ob,
                         &Outbox::onProvenanceStamped);

        const QString opId = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("m\037!r"),
                                        QByteArrayLiteral("payload"), QStringLiteral("hi"));
        ob.drain();
        ob.onAck(opId);
        clock += 11 * 60 * 1000; // past the 10-minute grace
        ob.expireAcceptedOps(clock, 10 * 60 * 1000);
        QVERIFY(!ob.contains(opId));
    }

    // api/38 (not provenance-capable): an ack is terminal success — deleted on ack, no lingering.
    void apiV38AckIsTerminal() {
        MirrorService svc;
        svc.openInMemory();
        LocalDatabase db(m_dir->filePath(QStringLiteral("local.db")));
        Outbox ob(&db); // default: NOT provenance-capable
        ob.setGate([](const QString&, VerbClass) { return true; });
        QObject::connect(&svc, &MirrorService::provenanceStamped, &ob,
                         &Outbox::onProvenanceStamped);

        const QString opId = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("m\037!r"),
                                        QByteArrayLiteral("payload"), QStringLiteral("hi"));
        ob.drain();
        ob.onAck(opId);
        QVERIFY(!ob.contains(opId));
    }

private:
    std::unique_ptr<QTemporaryDir> m_dir;
};

QTEST_GUILESS_MAIN(TstProvenanceLanding)
#include "tst_provenance_landing.moc"
