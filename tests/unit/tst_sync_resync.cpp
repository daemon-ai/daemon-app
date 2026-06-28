#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon_turn_engine.h"
#include "i_turn_engine.h"
#include "wire_mux_fixture.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::NodeApiClient;
using daemonapp::test::WireMuxServer;

namespace {

// An empty LogPage carrying an explicit epoch: {"LogPage":{"entries":[],next_seq,head_seq,epoch}}.
QByteArray logPage(quint64 nextSeq, quint64 headSeq, quint64 epoch) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "LogPage");
    b.append(static_cast<char>(0xA4));
    daemonapp::test::cborText(b, "entries");
    b.append(static_cast<char>(0x80)); // empty array
    daemonapp::test::cborText(b, "next_seq");
    daemonapp::test::cborUint(b, nextSeq);
    daemonapp::test::cborText(b, "head_seq");
    daemonapp::test::cborUint(b, headSeq);
    daemonapp::test::cborText(b, "epoch");
    daemonapp::test::cborUint(b, epoch);
    return b;
}

// An empty durable Journal: {"Journal":{entries:[],next_cursor:0,head_cursor:0,sealed_after:null}}.
QByteArray emptyJournal() {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "Journal");
    b.append(static_cast<char>(0xA4));
    daemonapp::test::cborText(b, "entries");
    b.append(static_cast<char>(0x80));
    daemonapp::test::cborText(b, "next_cursor");
    daemonapp::test::cborUint(b, 0);
    daemonapp::test::cborText(b, "head_cursor");
    daemonapp::test::cborUint(b, 0);
    daemonapp::test::cborText(b, "sealed_after");
    b.append(static_cast<char>(0xF6)); // null
    return b;
}

} // namespace

// Phase 2 (L2) client resync: the DaemonTurnEngine consumes a push Subscribe stream, tracks the
// session-activation epoch, and re-baselines from the durable journal on a generation change (an
// epoch bump or a Reset) - the fix for the daemon-restart desync - instead of silently stalling.
class TestSyncResync : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;

    QString sock(const QString& name) { return m_tmp.filePath(name); }

private slots:
    // An epoch bump on the streamed LogPage triggers a journal re-baseline (a SessionHistory call
    // crosses) and finishes the turn cleanly rather than hanging.
    void epochBumpReBaselinesFromJournal() {
        const QString path = sock(QStringLiteral("epoch.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(emptyJournal()); // Submit-ack (non-error) + the SessionHistory reply
        QVERIFY2(fake.start(path), "listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonTurnEngine engine(&client);
        engine.setSessionId(QStringLiteral("s1"));
        QSignalSpy finished(&engine, &ITurnEngine::turnFinished);

        engine.start(QStringLiteral("hi"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000); // push stream opened
        const int callsBefore = fake.requestCount();

        fake.pushItem(logPage(0, 0, 0)); // establish epoch 0
        fake.pushItem(logPage(0, 0, 1)); // epoch 1 > 0 -> re-baseline

        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 3000);
        // The re-baseline issued a SessionHistory one-shot (a new Call beyond Submit).
        QVERIFY(fake.requestCount() > callsBefore);
    }

    // A Reset frame for the active stream triggers the same journal re-baseline.
    void resetFrameReBaselines() {
        const QString path = sock(QStringLiteral("reset.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(emptyJournal());
        QVERIFY2(fake.start(path), "listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonTurnEngine engine(&client);
        engine.setSessionId(QStringLiteral("s1"));
        QSignalSpy finished(&engine, &ITurnEngine::turnFinished);

        engine.start(QStringLiteral("hi"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        fake.pushItem(logPage(0, 0, 0)); // establish epoch 0
        fake.pushReset(1, 0);            // generation change -> re-baseline

        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 3000);
    }

    // A mid-turn transport drop (daemon alive) is recoverable: the stream ends with an error, the
    // engine suspends into "stalled" (still active, not finished), and reopens when the socket
    // returns - it does not re-baseline (same epoch) and does not die.
    void streamDropSuspendsAndReopens() {
        const QString path = sock(QStringLiteral("drop.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(emptyJournal());
        QVERIFY2(fake.start(path), "listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonTurnEngine engine(&client);
        engine.setSessionId(QStringLiteral("s1"));
        QSignalSpy finished(&engine, &ITurnEngine::turnFinished);

        engine.start(QStringLiteral("hi"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        fake.pushItem(logPage(0, 0, 0));
        QTRY_VERIFY_WITH_TIMEOUT(engine.active() && engine.turnState() == QLatin1String("thinking"),
                                 3000);
        const int opensBefore = fake.openCount();

        fake.stop(); // transport drop -> streamEnded(error) -> stalled
        QTRY_COMPARE_WITH_TIMEOUT(engine.turnState(), QString("stalled"), 5000);
        QVERIFY(engine.active());
        QCOMPARE(finished.count(), 0);

        QVERIFY2(fake.start(path), "relisten");
        // The backoff reopen issues a fresh Open on the new connection.
        QTRY_VERIFY_WITH_TIMEOUT(fake.openCount() > opensBefore, 8000);
        fake.pushItem(logPage(0, 0, 0)); // a good page lifts "stalled"
        QTRY_VERIFY_WITH_TIMEOUT(engine.active() && engine.turnState() == QLatin1String("thinking"),
                                 5000);
        QCOMPARE(finished.count(), 0);
        engine.cancel();
    }
};

QTEST_GUILESS_MAIN(TestSyncResync)
#include "tst_sync_resync.moc"
