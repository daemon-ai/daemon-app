// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// tst_mock_both_modes (mirror A8) — both ingestor modes + the auto-replay gate exercised in a MOCK
// (Seeder-fed) stack, selected by the scenario's configurable api/<N> Hello (spec 09 §9 last
// bullet, §5.6, §6.8). The Seeder is the mock single Writer; the same ingestor reconnect algorithm
// and the same outbox gate run in mock as in daemon mode — parity by construction (§9).
//
// This mirrors exactly what the mock composition flip will wire from Scenario::apiVersion:
//   full (api/39):     onConnected → Bootstrap probe; auto-replay drain fires on reconnect
//   degraded (api/38): onConnected → staleness scan (no Bootstrap); lanes HOLD (manual drain only)

#include "local_database.h"
#include "mirror/ingestor.h"
#include "mirror/observe_coarse.h"
#include "mirror/seeder.h"
#include "outbox.h"
#include "verb_class.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using namespace mirror;

namespace {

constexpr int kBigCap = 64;

Conversation conv(const QString& t, const QString& id, const QString& title) {
    Conversation c;
    c.transport = t;
    c.id = id;
    c.title = title;
    return c;
}

Scenario scenarioForApi(int apiVersion) {
    Scenario s;
    s.apiVersion = apiVersion;
    s.seed.conversations = {conv("m", "!a", "Alpha")};
    return s;
}

} // namespace

class TstMockBothModes : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { qRegisterMetaType<mirror::PendingOp>("mirror::PendingOp"); }
    void init() {
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
    }
    void cleanup() { m_dir.reset(); }

    // api/39 scenario: the mock Hello advertises full mode → Bootstrap probe; auto-replay ON.
    void fullModeBootstrapAndAutoReplay() {
        CoarseObserve obs;
        Store store(obs);
        Seeder seeder(store);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);

        const Scenario scn = scenarioForApi(39);
        ScenarioPlayer player(seeder, scn);
        player.playSeed(); // the mock mirror is Seeder-fed
        QCOMPARE(store.snapshot().conversations.size(), std::size_t(1));

        // The scenario's api/<N> drives the ingestor's per-connection mode select (§5.6 full).
        ing.onConnected(player.apiVersion(), /*hasRevFields=*/true);
        QCOMPARE(exec.count(FetchOp::Bootstrap), 1);
        QVERIFY(!exec.has(FetchOp::SessionsQuery, QString())); // no degraded scan in full mode

        // The auto-replay gate (§6.8) keys off the SAME api/<N>: on at 39.
        LocalDatabase db(m_dir->filePath(QStringLiteral("local.db")));
        Outbox ob(&db);
        ob.setGate([](const QString&, VerbClass) { return true; });
        ob.setProvenanceCapable(Outbox::autoReplayEnabled(player.apiVersion()));
        QSignalSpy sent(&ob, &Outbox::sendRequested);
        const QString opId = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("m\037!a"),
                                        QByteArrayLiteral("p"), QStringLiteral("hi"));
        // Composition's reconnect handler: unattended drain iff the api/<N> allows it.
        QVERIFY(Outbox::autoReplayEnabled(player.apiVersion()));
        if (Outbox::autoReplayEnabled(player.apiVersion())) {
            ob.drain();
        }
        QCOMPARE(sent.count(), 1); // auto-replay fired
        QCOMPARE(ob.op(opId).state, OpState::Inflight);
    }

    // api/38 scenario: the mock Hello advertises degraded mode → staleness scan; auto-replay OFF.
    void degradedModeScanAndHold() {
        CoarseObserve obs;
        Store store(obs);
        Seeder seeder(store);
        RecordingExecutor exec;
        FetchScheduler sched(exec, kBigCap);
        Ingestor ing(store, sched);

        const Scenario scn = scenarioForApi(38);
        ScenarioPlayer player(seeder, scn);
        player.playSeed();

        ing.onConnected(player.apiVersion(), /*hasRevFields=*/false);
        QCOMPARE(exec.count(FetchOp::Bootstrap), 0);
        QVERIFY(
            exec.has(FetchOp::SessionsQuery, QString())); // degraded staleness scan re-baselines

        LocalDatabase db(m_dir->filePath(QStringLiteral("local.db")));
        Outbox ob(&db);
        ob.setGate([](const QString&, VerbClass) { return true; });
        ob.setProvenanceCapable(Outbox::autoReplayEnabled(player.apiVersion()));
        QSignalSpy sent(&ob, &Outbox::sendRequested);
        const QString opId = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("m\037!a"),
                                        QByteArrayLiteral("p"), QStringLiteral("hi"));
        QVERIFY(!Outbox::autoReplayEnabled(player.apiVersion()));
        if (Outbox::autoReplayEnabled(player.apiVersion())) {
            ob.drain(); // not taken at api/38
        }
        QCOMPARE(sent.count(), 0);                     // lanes HOLD — no blind resend at v38
        QCOMPARE(ob.op(opId).state, OpState::Pending); // awaiting a manual tap
        ob.drain();                                    // an explicit user tap always drains
        QCOMPARE(sent.count(), 1);
    }

private:
    // A recording executor: the mock composition installs a scripted pager here; the test only
    // needs to observe which fetch ops the reconnect algorithm issued.
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

    std::unique_ptr<QTemporaryDir> m_dir;
};

QTEST_GUILESS_MAIN(TstMockBothModes)
#include "tst_mock_both_modes.moc"
