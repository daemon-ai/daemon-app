// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

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
// [wave2:app-delegation] F3: optional `lifetime` ("Persistent"/"Ephemeral") and `foreignAgent`
// (engine-selector Foreign{agent}) append the v29 enrichment keys in CDDL order (after the required
// keys) so the decode path is exercised end-to-end.
void unitNode(QByteArray& b, const char* id, const char* kind, const QList<const char*>& children,
              const char* lifetime = nullptr, const char* foreignAgent = nullptr) {
    int keys = 6;
    if (lifetime != nullptr) {
        ++keys;
    }
    if (foreignAgent != nullptr) {
        ++keys;
    }
    mapHdr(b, keys);
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
    if (lifetime != nullptr) {
        cborText(b, "lifetime");
        cborText(b, lifetime);
    }
    if (foreignAgent != nullptr) {
        cborText(b, "engine");
        mapHdr(b, 1);
        cborText(b, "Foreign");
        mapHdr(b, 1);
        cborText(b, "agent");
        cborText(b, foreignAgent);
    }
}

// [waveB:app-v30] stretch: a Finished unit node carrying a terminal end_reason (state =
// {"Finished": {"end_reason": <reason>}}). Only the required keys, in CDDL order.
void finishedUnitNode(QByteArray& b, const char* id, const char* kind, const char* endReason) {
    mapHdr(b, 6);
    cborText(b, "id");
    cborText(b, id);
    cborText(b, "kind");
    cborText(b, kind);
    cborText(b, "state");
    mapHdr(b, 1);
    cborText(b, "Finished");
    mapHdr(b, 1);
    cborText(b, "end_reason");
    cborText(b, endReason);
    cborText(b, "work");
    b.append(static_cast<char>(0xF6)); // null
    cborText(b, "usage");
    zeroUsage(b);
    cborText(b, "children");
    arrHdr(b, 0);
}

// [waveB:app-v30] stretch: a tree whose child unit finished with end_reason "Failed".
QByteArray treeRespFinishedFailed() {
    QByteArray b;
    mapHdr(b, 1);
    cborText(b, "Tree");
    mapHdr(b, 2);
    cborText(b, "root");
    cborText(b, "u1");
    cborText(b, "nodes");
    arrHdr(b, 2);
    unitNode(b, "u1", "Orchestrator", {"u2"});
    finishedUnitNode(b, "u2", "Engine", "Failed");
    return b;
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

// [wave2:app-delegation] F3: a tree whose child carries the v29 lifetime + foreign-engine wire
// enrichment (root u1 -> child u2, ephemeral, running on a foreign agent "gemini-cli").
QByteArray treeRespEnriched() {
    QByteArray b;
    mapHdr(b, 1);
    cborText(b, "Tree");
    mapHdr(b, 2);
    cborText(b, "root");
    cborText(b, "u1");
    cborText(b, "nodes");
    arrHdr(b, 2);
    unitNode(b, "u1", "Orchestrator", {"u2"});
    unitNode(b, "u2", "Engine", {}, "Ephemeral", "gemini-cli");
    return b;
}

// One tree page (wire v25): {"Tree": {"root": "u1", "nodes": [<one node>], ?"next": <cursor>}}.
// `root` rides every page; a set `next` chains the node pages.
QByteArray treePageResp(const char* nodeId, const char* kind, const QList<const char*>& children,
                        const char* next = nullptr) {
    QByteArray b;
    mapHdr(b, 1);
    cborText(b, "Tree");
    mapHdr(b, next != nullptr ? 3 : 2);
    cborText(b, "root");
    cborText(b, "u1");
    cborText(b, "nodes");
    arrHdr(b, 1);
    unitNode(b, nodeId, kind, children);
    if (next != nullptr) {
        cborText(b, "next");
        cborText(b, next);
    }
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

    // Wire v25 page loop: a tree served in two node pages accumulates into ONE treeRefreshed with
    // the full reassembled structure (root from the first page; the child arrives on page two),
    // and the continuation request carries the `after` cursor.
    void treePagesReassembleAcrossTheCursorLoop() {
        const QString sock = m_tmp.filePath(QStringLiteral("fleet-pg.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplySequence(
            {treePageResp("u1", "Orchestrator", {"u2"}, "u1"), treePageResp("u2", "Engine", {})});
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("fleet-pg.db")));
        FleetRepository repo(&client, &cache);
        DaemonFleetTree tree(&repo);

        QSignalSpy refreshed(&repo, &FleetRepository::treeRefreshed);
        repo.refreshTree();
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        QTest::qWait(100); // settle: a page-loop bug would emit again / issue extra requests
        QCOMPARE(refreshed.count(), 1);

        // The whole (two-page) tree reassembled: root at depth 0, the page-two child under it.
        QCOMPARE(cache.fleetUnits().size(), 2);
        auto* m = model(tree);
        QCOMPARE(m->count(), 2);
        QCOMPARE(m->at(0).value(QStringLiteral("id")).toString(), QStringLiteral("u1"));
        QCOMPARE(m->at(1).value(QStringLiteral("id")).toString(), QStringLiteral("u2"));
        QCOMPARE(m->at(1).value(QStringLiteral("depth")).toInt(), 1);

        // The continuation request carried the cursor: byte-identical to encodeTreeRequest("u1").
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 2);
        QCOMPARE(calls.at(0), daemonapp::daemon::NodeApiCodec::encodeTreeRequest());
        QCOMPARE(calls.at(1),
                 daemonapp::daemon::NodeApiCodec::encodeTreeRequest(QStringLiteral("u1")));
    }

    // [wave2:app-delegation] F3: lifetime + engine are decoded straight off the wire UnitNode (v29)
    // — no client-side profile-join. The child node carries lifetime="Ephemeral" and a foreign
    // engine ("gemini-cli"); the row surfaces engine="Foreign" + engineAgent + lifetime, the root
    // (unenriched) leaves them empty.
    void wireLifetimeAndEngineDecode() {
        const QString sock = m_tmp.filePath(QStringLiteral("fleet-enr.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(treeRespEnriched());
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("fleet-enr.db")));
        FleetRepository repo(&client, &cache);
        DaemonFleetTree tree(&repo);

        QSignalSpy refreshed(&repo, &FleetRepository::treeRefreshed);
        repo.refreshTree();
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);

        auto* m = model(tree);
        QCOMPARE(m->count(), 2);
        // Root: no enrichment -> empty engine/lifetime (chip hidden).
        QCOMPARE(m->at(0).value(QStringLiteral("engine")).toString(), QString());
        QCOMPARE(m->at(0).value(QStringLiteral("lifetime")).toString(), QString());
        // Child: foreign engine + ephemeral, decoded from the wire.
        QCOMPARE(m->at(1).value(QStringLiteral("engine")).toString(), QStringLiteral("Foreign"));
        QCOMPARE(m->at(1).value(QStringLiteral("engineAgent")).toString(),
                 QStringLiteral("gemini-cli"));
        QCOMPARE(m->at(1).value(QStringLiteral("lifetime")).toString(),
                 QStringLiteral("Ephemeral"));

        // The enrichment also survives offline: it round-trips through the v7 cache columns.
        const QList<CachedFleetUnitRow> cached = cache.fleetUnits();
        QCOMPARE(cached.size(), 2);
        QCOMPARE(cached.at(1).engineKind, QStringLiteral("Foreign"));
        QCOMPARE(cached.at(1).engineAgent, QStringLiteral("gemini-cli"));
        QCOMPARE(cached.at(1).lifetime, QStringLiteral("Ephemeral"));
    }

    // [waveB:app-v30] stretch: a Finished unit's node-reported end_reason decodes off the wire and
    // round-trips through the v8 cache column (the subagent strip reads it to render an error).
    void wireEndReasonDecodesAndCaches() {
        const QString sock = m_tmp.filePath(QStringLiteral("fleet-end.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(treeRespFinishedFailed());
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("fleet-end.db")));
        FleetRepository repo(&client, &cache);
        DaemonFleetTree tree(&repo);

        QSignalSpy refreshed(&repo, &FleetRepository::treeRefreshed);
        repo.refreshTree();
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);

        const QList<CachedFleetUnitRow> cached = cache.fleetUnits();
        QCOMPARE(cached.size(), 2);
        // The finished child carries the node-reported terminal reason; the running root does not.
        const CachedFleetUnitRow* child = nullptr;
        const CachedFleetUnitRow* root = nullptr;
        for (const CachedFleetUnitRow& r : cached) {
            if (r.unitId == QStringLiteral("u2")) {
                child = &r;
            } else if (r.unitId == QStringLiteral("u1")) {
                root = &r;
            }
        }
        QVERIFY(child != nullptr && root != nullptr);
        QCOMPARE(child->state, QStringLiteral("Finished"));
        QCOMPARE(child->endReason, QStringLiteral("Failed"));
        QVERIFY(root->endReason.isEmpty());
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
