#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_fleet_tree.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/repositories.h"
#include "uimodels/variant_list_model.h"
#include "wire_mux_fixture.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::CachedFleetUnitRow;
using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::FleetRepository;
using daemonapp::daemon::NodeApiClient;
using daemonapp::test::cborText;
using daemonapp::test::cborUint;
using daemonapp::test::WireMuxServer;
using fleet::DaemonFleetTree;

namespace {

void mapHdr(QByteArray& b, int n) {
    b.append(static_cast<char>(0xA0 | n));
}
void arrHdr(QByteArray& b, int n) {
    b.append(static_cast<char>(0x80 | n));
}

void zeroUsage(QByteArray& b) {
    mapHdr(b, 7);
    for (const char* k : {"input_tokens", "output_tokens", "api_calls", "cache_read_tokens",
                          "cache_write_tokens", "reasoning_tokens", "cost_micros"}) {
        cborText(b, k);
        cborUint(b, 0);
    }
}

// A minimal unit-node map (the 6 required keys; optionals omitted). `kind`/state are bare tstrs.
void unitNode(QByteArray& b, const char* id, const char* kind, const QList<const char*>& children) {
    mapHdr(b, 6);
    cborText(b, "id");
    cborText(b, id);
    cborText(b, "kind");
    cborText(b, kind);
    cborText(b, "state");
    cborText(b, "Running");
    cborText(b, "work");
    b.append(static_cast<char>(0xF6)); // null
    cborText(b, "usage");
    zeroUsage(b);
    cborText(b, "children");
    arrHdr(b, children.size());
    for (const char* c : children) {
        cborText(b, c);
    }
}

// {"Tree": {"root": "u1", "nodes": [ orchestrator u1 -> child u2, engine u2 ]}}
QByteArray treeResp() {
    QByteArray b;
    mapHdr(b, 1);
    cborText(b, "Tree");
    mapHdr(b, 2);
    cborText(b, "root");
    cborText(b, "u1");
    cborText(b, "nodes");
    arrHdr(b, 2);
    unitNode(b, "u1", "Orchestrator", {"u2"});
    unitNode(b, "u2", "Engine", {});
    return b;
}

// {"Error": {"Unsupported": "engine leaf"}}
QByteArray unsupportedResp() {
    QByteArray b;
    mapHdr(b, 1);
    cborText(b, "Error");
    mapHdr(b, 1);
    cborText(b, "Unsupported");
    cborText(b, "engine leaf");
    return b;
}

uimodels::VariantListModel* model(DaemonFleetTree& tree) {
    return qobject_cast<uimodels::VariantListModel*>(tree.nodes());
}

} // namespace

// Phase 5b: the daemon-backed fleet tree renders offline from cache, populates from a Tree query,
// and surfaces a control rejection (PRO-10 Unsupported on an engine leaf) while reverting the
// optimistic pause overlay.
class TestDaemonFleetTree : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;

private slots:
    // Offline-first: a cached tree renders in the model with NO connection.
    void rendersCachedTreeOffline() {
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("fleet-off.db")));
        QVERIFY(cache.isOpen());
        CachedFleetUnitRow root;
        root.unitId = QStringLiteral("u1");
        root.depth = 0;
        root.ordinal = 0;
        root.name = QStringLiteral("Acme");
        root.kind = QStringLiteral("Orchestrator");
        root.state = QStringLiteral("Running");
        root.updatedAtMs = 1;
        QVERIFY(cache.upsertFleetUnit(root));
        CachedFleetUnitRow child;
        child.unitId = QStringLiteral("u2");
        child.parentId = QStringLiteral("u1");
        child.depth = 1;
        child.ordinal = 1;
        child.name = QStringLiteral("Coder");
        child.kind = QStringLiteral("Engine");
        child.state = QStringLiteral("Running");
        child.updatedAtMs = 1;
        QVERIFY(cache.upsertFleetUnit(child));

        DaemonTransport transport; // unconnected
        NodeApiClient client(&transport);
        FleetRepository repo(&client, &cache);
        DaemonFleetTree tree(&repo);

        auto* m = model(tree);
        QVERIFY(m != nullptr);
        QCOMPARE(m->count(), 2);
        QCOMPARE(m->at(0).value(QStringLiteral("id")).toString(), QStringLiteral("u1"));
        QCOMPARE(m->at(0).value(QStringLiteral("kind")).toString(), QStringLiteral("orchestrator"));
        QCOMPARE(m->at(0).value(QStringLiteral("status")).toString(), QStringLiteral("running"));
        QCOMPARE(m->at(1).value(QStringLiteral("kind")).toString(), QStringLiteral("worker"));
        QCOMPARE(m->at(1).value(QStringLiteral("depth")).toInt(), 1);
    }

    // A Tree query populates the cache + model end-to-end over the mux.
    void treeRefreshPopulates() {
        const QString sock = m_tmp.filePath(QStringLiteral("fleet.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(treeResp());
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("fleet.db")));
        FleetRepository repo(&client, &cache);
        DaemonFleetTree tree(&repo);

        QSignalSpy refreshed(&repo, &FleetRepository::treeRefreshed);
        repo.refreshTree();
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        QCOMPARE(cache.fleetUnits().size(), 2);
        auto* m = model(tree);
        QCOMPARE(m->count(), 2);
        QCOMPARE(m->at(0).value(QStringLiteral("id")).toString(), QStringLiteral("u1"));
    }

    // PRO-10: pausing an engine leaf is Unsupported -> controlRejected fires + the optimistic
    // paused overlay is reverted (the row does not stay "paused").
    void controlUnsupportedSurfacesRejection() {
        const QString sock = m_tmp.filePath(QStringLiteral("fleet-ctl.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(unsupportedResp());
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("fleet-ctl.db")));
        CachedFleetUnitRow leaf;
        leaf.unitId = QStringLiteral("u2");
        leaf.kind = QStringLiteral("Engine");
        leaf.state = QStringLiteral("Running");
        leaf.updatedAtMs = 1;
        QVERIFY(cache.upsertFleetUnit(leaf));
        FleetRepository repo(&client, &cache);
        DaemonFleetTree tree(&repo);

        QSignalSpy rejected(&tree, &fleet::IFleetTree::controlRejected);
        tree.pause(QStringLiteral("u2"));
        QTRY_COMPARE_WITH_TIMEOUT(rejected.count(), 1, 3000);
        // The optimistic paused overlay was reverted -> the row reads "running", not "paused".
        QCOMPARE(model(tree)->at(0).value(QStringLiteral("status")).toString(),
                 QStringLiteral("running"));
    }
};

QTEST_GUILESS_MAIN(TestDaemonFleetTree)
#include "tst_daemon_fleet_tree.moc"
