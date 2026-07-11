// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// G2 (M5): the fleet TREE ported onto the mirror — MirrorFleetTree projects the ONE generated
// mirror::FleetUnit entity (child_ids edge → pre-order depth reconstruction, §3.5 unitsByParent)
// into the flattened IFleetTree rows the FleetPage/ops-hub bind. Replaces the deleted
// tst_daemon_fleet_tree, re-expressing its still-relevant behaviors on the mirror path:
//  - offline/seeded render (rows appear from the mirror table with no wire),
//  - the v29 engine/lifetime/role enrichment projected off the mirror entity,
//  - live re-derivation on a FleetUnit journal delta (replace-and-prune reaches the consumer),
//  - refresh()/control-ack driving the ingestor's Tree refetch (single writer, §5.1),
//  - the PRO-10 control rejection reverting the optimistic paused overlay (WireMuxServer).

#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_transport.h"
#include "daemon/mirror_fleet_tree.h"
#include "daemon/node_api_client.h"
#include "daemon/repositories.h"
#include "mirror/mirror_service.h"
#include "mirror/store.h"
#include "uimodels/variant_list_model.h"
#include "wire_mux_fixture.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <vector>

using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::FleetRepository;
using daemonapp::daemon::NodeApiClient;
using daemonapp::test::cborText;
using daemonapp::test::WireMuxServer;
using fleet::MirrorFleetTree;

namespace {

mirror::FleetUnit unit(const QString& id, const QString& kind, const QStringList& children,
                       const QString& title = {}) {
    mirror::FleetUnit u;
    u.id = id;
    u.kind = kind;
    u.state = QStringLiteral("Running");
    u.title = title;
    u.child_ids = children.join(QChar(0x1f));
    u.child_count = static_cast<int>(children.size());
    return u;
}

uimodels::VariantListModel* model(MirrorFleetTree& tree) {
    return qobject_cast<uimodels::VariantListModel*>(tree.nodes());
}

class RecordingExecutor : public mirror::FetchExecutor {
public:
    void execute(const mirror::FetchJob& job) override { executed.append(job); }
    [[nodiscard]] int countOf(mirror::FetchOp op) const {
        int n = 0;
        for (const mirror::FetchJob& j : executed) {
            if (j.op == op) {
                ++n;
            }
        }
        return n;
    }
    QList<mirror::FetchJob> executed;
};

// {"Error": {"Unsupported": "engine leaf"}} — the PRO-10 control rejection.
QByteArray unsupportedResp() {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "Error");
    b.append(static_cast<char>(0xA1));
    cborText(b, "Unsupported");
    cborText(b, "engine leaf");
    return b;
}

} // namespace

class TestMirrorFleetTree : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;

private slots:
    // The seeded mirror renders the supervision hierarchy: the root is the unit no child list
    // names; children flatten pre-order in their LISTED order with depth assigned; name falls
    // back to the id when the title is empty; kind/status map onto the display tokens.
    void rendersSeededMirrorTreePreOrder() {
        mirror::MirrorService svc;
        svc.openInMemory();
        // Delivered out of id order — the edge, not delivery order, decides the render.
        mirror::FleetUnit leafB = unit(QStringLiteral("u3"), QStringLiteral("Engine"), {});
        leafB.state = QStringLiteral("Finished");
        svc.ingestor().deliverFleetUnits(
            {leafB, unit(QStringLiteral("u2"), QStringLiteral("Engine"), {}),
             unit(QStringLiteral("u1"), QStringLiteral("Orchestrator"),
                  {QStringLiteral("u2"), QStringLiteral("u3")}, QStringLiteral("Acme"))},
            /*isFinalPage=*/true, /*rev=*/1);

        MirrorFleetTree tree(&svc.store(), &svc.ingestor(), nullptr);
        auto* m = model(tree);
        QVERIFY(m != nullptr);
        QCOMPARE(m->count(), 3);
        QCOMPARE(m->at(0).value(QStringLiteral("id")).toString(), QStringLiteral("u1"));
        QCOMPARE(m->at(0).value(QStringLiteral("name")).toString(), QStringLiteral("Acme"));
        QCOMPARE(m->at(0).value(QStringLiteral("kind")).toString(), QStringLiteral("orchestrator"));
        QCOMPARE(m->at(0).value(QStringLiteral("depth")).toInt(), 0);
        QCOMPARE(m->at(0).value(QStringLiteral("status")).toString(), QStringLiteral("running"));
        QCOMPARE(m->at(1).value(QStringLiteral("id")).toString(), QStringLiteral("u2"));
        QCOMPARE(m->at(1).value(QStringLiteral("name")).toString(),
                 QStringLiteral("u2")); // id fallback
        QCOMPARE(m->at(1).value(QStringLiteral("kind")).toString(), QStringLiteral("worker"));
        QCOMPARE(m->at(1).value(QStringLiteral("depth")).toInt(), 1);
        QCOMPARE(m->at(2).value(QStringLiteral("id")).toString(), QStringLiteral("u3"));
        QCOMPARE(m->at(2).value(QStringLiteral("status")).toString(),
                 QStringLiteral("idle")); // Finished -> idle (wire has no paused)
    }

    // The v29 enrichment projects straight off the mirror entity: engine ("Core"/"Foreign" + the
    // foreign agent), lifetime, the node-stamped role, and the bound session id (the F4
    // steer/cancel handle). An unenriched root leaves them empty.
    void wireEnrichmentProjectsOffMirrorEntity() {
        mirror::MirrorService svc;
        svc.openInMemory();
        mirror::FleetUnit child = unit(QStringLiteral("u2"), QStringLiteral("Engine"), {});
        child.engine = QStringLiteral("Foreign");
        child.engine_agent = QStringLiteral("gemini-cli");
        child.lifetime = QStringLiteral("Ephemeral");
        child.role = QStringLiteral("EphemeralSubagent");
        child.session = QStringLiteral("s-child");
        child.profile = QStringLiteral("prof-9");
        svc.ingestor().deliverFleetUnits(
            {unit(QStringLiteral("u1"), QStringLiteral("Orchestrator"), {QStringLiteral("u2")}),
             child},
            /*isFinalPage=*/true, /*rev=*/1);

        MirrorFleetTree tree(&svc.store(), &svc.ingestor(), nullptr);
        auto* m = model(tree);
        QCOMPARE(m->count(), 2);
        QCOMPARE(m->at(0).value(QStringLiteral("engine")).toString(), QString());
        QCOMPARE(m->at(0).value(QStringLiteral("lifetime")).toString(), QString());
        QCOMPARE(m->at(1).value(QStringLiteral("engine")).toString(), QStringLiteral("Foreign"));
        QCOMPARE(m->at(1).value(QStringLiteral("engineAgent")).toString(),
                 QStringLiteral("gemini-cli"));
        QCOMPARE(m->at(1).value(QStringLiteral("lifetime")).toString(),
                 QStringLiteral("Ephemeral"));
        QCOMPARE(m->at(1).value(QStringLiteral("role")).toString(),
                 QStringLiteral("EphemeralSubagent"));
        QCOMPARE(m->at(1).value(QStringLiteral("sessionId")).toString(), QStringLiteral("s-child"));
        QCOMPARE(m->at(1).value(QStringLiteral("model")).toString(), QStringLiteral("prof-9"));
    }

    // A FleetUnit journal delta re-derives the rows (a later replace-and-prune list shrinks the
    // render); an unrelated kind's delta leaves the model untouched (§8.1 watermark discipline).
    void liveUpdateOnFleetDeltaOnly() {
        mirror::MirrorService svc;
        svc.openInMemory();
        svc.ingestor().deliverFleetUnits(
            {unit(QStringLiteral("u1"), QStringLiteral("Orchestrator"), {QStringLiteral("u2")}),
             unit(QStringLiteral("u2"), QStringLiteral("Engine"), {})},
            /*isFinalPage=*/true, /*rev=*/1);
        MirrorFleetTree tree(&svc.store(), &svc.ingestor(), nullptr);
        QCOMPARE(model(tree)->count(), 2);

        QSignalSpy changed(&tree, &fleet::IFleetTree::changed);
        mirror::Session s;
        s.session = QStringLiteral("s-x");
        svc.ingestor().deliverSessions({s}, /*isFinalPage=*/true);
        QCOMPARE(changed.count(), 0); // unrelated kind: no tree re-derivation

        svc.ingestor().deliverFleetUnits(
            {unit(QStringLiteral("u1"), QStringLiteral("Orchestrator"), {})},
            /*isFinalPage=*/true, /*rev=*/2);
        QVERIFY(changed.count() > 0);
        QCOMPARE(model(tree)->count(), 1); // u2 pruned (the finished subagent left the tree)
    }

    // A pathological cycle in the edge terminates and renders each unit once (the DFS seen-set).
    void cycleGuardTerminates() {
        mirror::MirrorService svc;
        svc.openInMemory();
        // u1 -> u2 -> u1: no unit is root by the "never named as a child" rule, so both are
        // reachable only via the cycle guard's root fallback — the render must not hang and must
        // not duplicate. (Root fallback: every id named as a child of something in a cycle still
        // sorts deterministically; the guard just proves termination.)
        svc.ingestor().deliverFleetUnits(
            {unit(QStringLiteral("u1"), QStringLiteral("Orchestrator"), {QStringLiteral("u2")}),
             unit(QStringLiteral("u2"), QStringLiteral("Engine"), {QStringLiteral("u1")})},
            /*isFinalPage=*/true, /*rev=*/1);
        MirrorFleetTree tree(&svc.store(), &svc.ingestor(), nullptr);
        // Both units are named as children => no roots => an EMPTY render (never a hang). The
        // node never serves a cyclic tree; this pins the degenerate-input behavior.
        QCOMPARE(model(tree)->count(), 0);
    }

    // refresh() (the FleetPage/ops-hub action) and a control-ack treeRefreshed both drive the
    // ingestor's Tree refetch — the mirror twin of the legacy repo self-refresh.
    void refreshAndControlAckEnqueueMirrorTreeFetch() {
        mirror::MirrorService svc;
        svc.openInMemory();
        RecordingExecutor exec;
        svc.setFetchExecutor(&exec);
        FleetRepository repo(nullptr, nullptr);
        MirrorFleetTree tree(&svc.store(), &svc.ingestor(), &repo);

        tree.refresh();
        QCOMPARE(exec.countOf(mirror::FetchOp::Tree), 1);
        // Free the coalesce slot (the in-flight job would dedup a same-key enqueue), then the
        // control-Ok ack path (repo re-fetched its legacy tree) drives a fresh mirror refetch.
        svc.scheduler().complete(exec.executed.first().coalesceKey());
        emit repo.treeRefreshed();
        QCOMPARE(exec.countOf(mirror::FetchOp::Tree), 2);
        svc.setFetchExecutor(nullptr); // exec outlives this scope, the service does not
    }

    // PRO-10: pausing an engine leaf is Unsupported -> the optimistic paused overlay shows
    // immediately, controlRejected fires, and the overlay reverts (the row does not stay
    // "paused"). Control routes through the FleetRepository over the WireMuxServer fixture.
    void controlUnsupportedSurfacesRejection() {
        const QString sock = m_tmp.filePath(QStringLiteral("fleet-ctl.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(unsupportedResp());
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("fleet-ctl.db")));
        FleetRepository repo(&client, &cache);

        mirror::MirrorService svc;
        svc.openInMemory();
        svc.ingestor().deliverFleetUnits({unit(QStringLiteral("u2"), QStringLiteral("Engine"), {})},
                                         /*isFinalPage=*/true, /*rev=*/1);
        MirrorFleetTree tree(&svc.store(), &svc.ingestor(), &repo);

        QSignalSpy rejected(&tree, &fleet::IFleetTree::controlRejected);
        tree.pause(QStringLiteral("u2"));
        // Optimistic overlay is visible before the node's rejection lands (no event loop yet).
        QCOMPARE(model(tree)->at(0).value(QStringLiteral("status")).toString(),
                 QStringLiteral("paused"));
        QTRY_COMPARE_WITH_TIMEOUT(rejected.count(), 1, 3000);
        // The optimistic paused overlay was reverted -> the row reads "running", not "paused".
        QCOMPARE(model(tree)->at(0).value(QStringLiteral("status")).toString(),
                 QStringLiteral("running"));
    }
};

QTEST_GUILESS_MAIN(TestMirrorFleetTree)
#include "tst_mirror_fleet_tree.moc"
