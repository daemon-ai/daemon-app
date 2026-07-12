// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// tst_roster_edit_lane (known-debt D3): the four roster verbs (RosterAdd / RosterUpdate /
// RosterRemove / ContactSetAlias) routed through the durable `roster-edit` outbox lane
// (spec 09 §6.4/§6.5/§6.6), scoped per transport, instead of the historical DIRECT sends.
// Asserts: an edit made offline persists as a pending op (nothing sent — the gate holds the
// lane); the durable rows survive a restart and replay in FIFO order to the send seam with
// their op-ids intact (§6.6/§6.8); a node rejection pauses the lane and surfaces through the
// seam's contactOperationFailed relay with retry re-queuing the SAME op-id (§6.5); alias
// set/clear and remove are covered, not just add. No fake mirror rows anywhere: the roster
// rows refresh only via the node's ContactsChanged → refetch (law 8).

#include "daemon/daemon_contacts_service.h"
#include "daemon/node_api_codec.h"
#include "local_database.h"
#include "outbox.h"
#include "verb_class.h"

#include <memory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using namespace mirror;

namespace {

QString rosterLane(const QString& transport) {
    return buildLane(VerbClass::RosterEdit, transport);
}

QJsonObject payloadOf(const PendingOp& op) {
    return QJsonDocument::fromJson(op.payload).object();
}

} // namespace

class TstRosterEditLane : public QObject {
    Q_OBJECT

    std::unique_ptr<QTemporaryDir> m_dir;

    QString dbPath(const QString& name = QStringLiteral("local.db")) const {
        return m_dir->filePath(name);
    }

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        qRegisterMetaType<mirror::PendingOp>("mirror::PendingOp");
    }

    void init() {
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
    }

    void cleanup() { m_dir.reset(); }

    // An edit made OFFLINE is durable, not lost: the op persists in the per-transport
    // `roster-edit` lane and NOTHING reaches the wire while the drain gate holds (§6.3/§6.6).
    void enqueueOfflinePersistsAndHolds() {
        LocalDatabase db(dbPath());
        QVERIFY(db.isOpen());
        Outbox outbox(&db);
        outbox.setGate([](const QString&, VerbClass) { return false; }); // offline
        outbox.load();
        transports::DaemonContactsService svc(/*repo=*/nullptr);
        svc.setOutbox(&outbox);
        QSignalSpy sent(&outbox, &Outbox::sendRequested);

        const QString t = QStringLiteral("matrix/@bot:hs");
        svc.addContact(t, QStringLiteral("@zoe:matrix.org"));

        QCOMPARE(sent.count(), 0); // held by the gate — durable-first, no send
        QCOMPARE(outbox.laneDepth(rosterLane(t)), 1);
        const QList<PendingOp> ops = outbox.ops();
        QCOMPARE(ops.size(), 1);
        QCOMPARE(ops.first().verb, QStringLiteral("RosterAdd"));
        QCOMPARE(ops.first().state, OpState::Pending);
        QCOMPARE(ops.first().lane, rosterLane(t));
        const QJsonObject args = payloadOf(ops.first());
        QCOMPARE(args.value(QStringLiteral("transport")).toString(), t);
        QCOMPARE(args.value(QStringLiteral("id")).toString(), QStringLiteral("@zoe:matrix.org"));
    }

    // Lanes are scoped PER TRANSPORT (§6.4): edits against two accounts are independent lanes.
    void lanesScopePerTransport() {
        LocalDatabase db(dbPath());
        Outbox outbox(&db);
        outbox.setGate([](const QString&, VerbClass) { return false; });
        outbox.load();
        transports::DaemonContactsService svc(nullptr);
        svc.setOutbox(&outbox);

        svc.addContact(QStringLiteral("matrix/@a:hs"), QStringLiteral("@x:hs"));
        svc.removeContact(QStringLiteral("xmpp/b@srv"), QStringLiteral("y@srv"));

        QCOMPARE(outbox.laneDepth(rosterLane(QStringLiteral("matrix/@a:hs"))), 1);
        QCOMPARE(outbox.laneDepth(rosterLane(QStringLiteral("xmpp/b@srv"))), 1);
    }

    // Offline edits survive a RESTART and replay in FIFO order on reconnect: the encoded
    // requests reach the send seam (sendRequested) with their original op-ids (§6.6/§6.8).
    // Covers all four verbs — add, update, alias, remove.
    void replayAfterRestartInOrder() {
        const QString path = dbPath();
        const QString t = QStringLiteral("matrix/@bot:hs");
        const QString id = QStringLiteral("@alice:matrix.org");
        QStringList ids;
        {
            LocalDatabase db(path);
            Outbox outbox(&db);
            outbox.setGate([](const QString&, VerbClass) { return false; }); // offline
            outbox.load();
            transports::DaemonContactsService svc(nullptr);
            svc.setOutbox(&outbox);
            QSignalSpy added(&outbox, &Outbox::opAdded);

            svc.addContact(t, id);
            svc.updateContact(
                t, QVariantMap{{QStringLiteral("id"), id},
                               {QStringLiteral("displayName"), QStringLiteral("Alice")}});
            svc.setAlias(t, id, QStringLiteral("Ali"));
            svc.removeContact(t, id);
            QCOMPARE(added.count(), 4);
            for (const auto& sig : added) {
                ids << sig.at(0).toString();
            }
            // App exits with the node unreachable — the rows are already durable.
        }

        LocalDatabase db2(path);
        Outbox outbox2(&db2);
        outbox2.setGate([](const QString&, VerbClass) { return true; }); // reconnected
        outbox2.load();
        QCOMPARE(outbox2.laneDepth(rosterLane(t)), 4);

        // Drive the drain→ack loop the service graph runs: one in-flight per lane, FIFO.
        QSignalSpy sent(&outbox2, &Outbox::sendRequested);
        QList<PendingOp> replayed;
        for (int i = 0; i < 4; ++i) {
            outbox2.drain();
            QCOMPARE(sent.count(), i + 1);
            const auto op = qvariant_cast<mirror::PendingOp>(sent.at(i).at(0));
            replayed << op;
            outbox2.onAck(op.opId); // v38-style terminal ack frees the lane head
        }
        QCOMPARE(replayed.at(0).verb, QStringLiteral("RosterAdd"));
        QCOMPARE(replayed.at(1).verb, QStringLiteral("RosterUpdate"));
        QCOMPARE(replayed.at(2).verb, QStringLiteral("ContactSetAlias"));
        QCOMPARE(replayed.at(3).verb, QStringLiteral("RosterRemove"));
        for (int i = 0; i < 4; ++i) {
            QCOMPARE(replayed.at(i).opId, ids.at(i)); // original op-ids, dedup-safe resend
        }
        QCOMPARE(payloadOf(replayed.at(1)).value(QStringLiteral("displayName")).toString(),
                 QStringLiteral("Alice"));
        QCOMPARE(outbox2.laneDepth(rosterLane(t)), 0);
    }

    // §6.5: a node rejection pauses the lane, surfaces through the seam's typed failure relay
    // (the SAME contactOperationFailed the GUI toast + TUI notice bind), and retry re-queues
    // the SAME op-id and unpauses.
    void rejectionSurfacesPausesAndRetries() {
        LocalDatabase db(dbPath());
        Outbox outbox(&db);
        outbox.setGate([](const QString&, VerbClass) { return true; });
        outbox.load();
        transports::DaemonContactsService svc(nullptr);
        svc.setOutbox(&outbox);
        QSignalSpy failed(&svc, &transports::IContactsService::contactOperationFailed);
        QSignalSpy added(&outbox, &Outbox::opAdded);

        const QString t = QStringLiteral("matrix/@bot:hs");
        svc.addContact(t, QStringLiteral("@evil:matrix.org"));
        QCOMPARE(added.count(), 1);
        const QString opId = added.at(0).at(0).toString();
        outbox.drain();
        outbox.onRejection(opId, QStringLiteral("Forbidden"), QStringLiteral("nope"));

        QVERIFY(outbox.lanePaused(rosterLane(t)));
        QCOMPARE(outbox.op(opId).state, OpState::Rejected);
        QCOMPARE(failed.count(), 1);
        const QString message = failed.at(0).at(0).toString();
        QVERIFY(message.contains(QStringLiteral("Forbidden")));
        QVERIFY(message.contains(QStringLiteral("nope")));

        outbox.retry(opId); // same op-id back to pending; the lane resumes (§6.5)
        QVERIFY(!outbox.lanePaused(rosterLane(t)));
        QCOMPARE(outbox.op(opId).state, OpState::Pending);
    }

    // Alias set vs clear ride the ONE ContactSetAlias verb with the optional encoded in the
    // payload (empty alias ⇒ hasAlias=false, clearing node-side), mirroring the repo/codec
    // convention.
    void aliasSetAndClearPayloads() {
        LocalDatabase db(dbPath());
        Outbox outbox(&db);
        outbox.setGate([](const QString&, VerbClass) { return false; });
        outbox.load();
        transports::DaemonContactsService svc(nullptr);
        svc.setOutbox(&outbox);

        const QString t = QStringLiteral("matrix/@bot:hs");
        const QString id = QStringLiteral("@alice:matrix.org");
        svc.setAlias(t, id, QStringLiteral("Ali"));
        svc.setAlias(t, id, QString());

        const QList<PendingOp> ops = outbox.ops();
        QCOMPARE(ops.size(), 2); // roster-edit does NOT coalesce (§6.4: only conv-meta does)
        QCOMPARE(ops.at(0).verb, QStringLiteral("ContactSetAlias"));
        const QJsonObject set = payloadOf(ops.at(0));
        QVERIFY(set.value(QStringLiteral("hasAlias")).toBool());
        QCOMPARE(set.value(QStringLiteral("alias")).toString(), QStringLiteral("Ali"));
        const QJsonObject clear = payloadOf(ops.at(1));
        QVERIFY(!clear.value(QStringLiteral("hasAlias")).toBool());
    }

    // The roster wire encoders now carry the client-minted op-id (§10.3): a non-empty opId
    // changes the encoded request (the optional op_id field is present); the same opId encodes
    // identically (the outbox reuses it across retries so a resend is dedup-safe).
    void encodersCarryOpId() {
        using daemonapp::daemon::DecodedContact;
        using daemonapp::daemon::NodeApiCodec;
        const QString t = QStringLiteral("matrix/@bot:hs");
        DecodedContact c;
        c.id = QStringLiteral("@zoe:matrix.org");
        const QString op = QStringLiteral("018f-op");

        QVERIFY(NodeApiCodec::encodeRosterAddRequest(t, c, op) !=
                NodeApiCodec::encodeRosterAddRequest(t, c));
        QCOMPARE(NodeApiCodec::encodeRosterAddRequest(t, c, op),
                 NodeApiCodec::encodeRosterAddRequest(t, c, op));
        QVERIFY(NodeApiCodec::encodeRosterUpdateRequest(t, c, op) !=
                NodeApiCodec::encodeRosterUpdateRequest(t, c));
        QVERIFY(NodeApiCodec::encodeRosterRemoveRequest(t, c, op) !=
                NodeApiCodec::encodeRosterRemoveRequest(t, c));
        QVERIFY(
            NodeApiCodec::encodeContactSetAliasRequest(t, c.id, true, QStringLiteral("A"), op) !=
            NodeApiCodec::encodeContactSetAliasRequest(t, c.id, true, QStringLiteral("A")));
        // The no-op-id encodings are byte-identical to the historical shape (v38 parity).
        QCOMPARE(NodeApiCodec::encodeRosterAddRequest(t, c),
                 NodeApiCodec::encodeRosterAddRequest(t, c, QString()));
    }

    // Without an outbox the daemon seam's mutations are inert no-ops (no legacy direct-send
    // fallback — the MirrorSessionStore precedent), and never crash.
    void withoutOutboxMutationsNoop() {
        transports::DaemonContactsService svc(nullptr);
        svc.addContact(QStringLiteral("t"), QStringLiteral("x"));
        svc.updateContact(QStringLiteral("t"),
                          QVariantMap{{QStringLiteral("id"), QStringLiteral("x")}});
        svc.removeContact(QStringLiteral("t"), QStringLiteral("x"));
        svc.setAlias(QStringLiteral("t"), QStringLiteral("x"), QStringLiteral("a"));
        // reaching here without a crash is the assertion
        QVERIFY(true);
    }
};

QTEST_GUILESS_MAIN(TstRosterEditLane)
#include "tst_roster_edit_lane.moc"
