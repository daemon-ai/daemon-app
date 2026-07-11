// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// The M4 sessions/fleet fetch-ingest vertical on the MIRROR store (spec 09 §13 wave M4; §3.3, §5.1,
// §5.3, §5.5). The parity infrastructure that unblocks the session/fleet consumer sub-gates:
//  - the decode-to-mirror bridge (map_session / map_fleet_unit): a wire Sessions / Tree /
//    SessionDetail reply decodes into Session / FleetUnit entities with the typed fields, enum
//    strings, nullable-optional handling and the SessionGet hydration merge (§3.3);
//  - the ingestor's deliverSessions (GLOBAL replace-and-prune over sessions), deliverFleetUnits
//    (replace-and-prune over fleet_units + rev record) and deliverSession (keyed hydration upsert),
//    the single-writer store apply (§5.1/§5.3/§5.5).

#include "daemon/mirror_decode.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "mirror/mirror_service.h"
#include "mirror/store.h"

#include <QtTest/QtTest>
#include <vector>

using daemonapp::daemon::decodeFleetUnitsToMirror;
using daemonapp::daemon::decodeSessionDetailToMirror;
using daemonapp::daemon::decodeSessionsToMirror;

namespace {

void setStr(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

QByteArray encodeResponse(const api_response_r& resp) {
    QByteArray out(64 * 1024, Qt::Uninitialized);
    size_t written = 0;
    if (cbor_encode_api_response(reinterpret_cast<uint8_t*>(out.data()),
                                 static_cast<size_t>(out.size()), &resp,
                                 &written) != ZCBOR_SUCCESS) {
        return {};
    }
    out.resize(static_cast<qsizetype>(written));
    return out;
}

// {"Sessions": [<active pinned session with a title>, <ready session, no title>]}.
QByteArray sessionsResponse() {
    static const QByteArray s1 = QByteArrayLiteral("s-alpha");
    static const QByteArray t1 = QByteArrayLiteral("Alpha thread");
    static const QByteArray p1 = QByteArrayLiteral("prof-1");
    static const QByteArray s2 = QByteArrayLiteral("s-beta");

    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_sessions_m_c;
    response_sessions& rs = resp.api_response_response_sessions_m;
    rs.response_sessions_Sessions_session_info_m_count = 2;

    session_info& a = rs.response_sessions_Sessions_session_info_m[0];
    setStr(a.session_info_session, s1);
    a.session_info_state.session_state_choice = session_state_r::session_state_Active_tstr_c;
    a.session_info_title_present = true;
    a.session_info_title.session_info_title_choice =
        session_info_title_r::session_info_title_tstr_c;
    setStr(a.session_info_title.session_info_title_tstr, t1);
    a.session_info_bound_profile_present = true;
    a.session_info_bound_profile.session_info_bound_profile_choice =
        session_info_bound_profile_r::session_info_bound_profile_profile_ref_m_c;
    setStr(a.session_info_bound_profile.session_info_bound_profile_profile_ref_m, p1);
    a.session_info_pinned_present = true;
    a.session_info_pinned.session_info_pinned = true;
    a.session_info_role_present = true;
    a.session_info_role.session_info_role.session_role_choice =
        session_role_r::session_role_Primary_tstr_c;

    session_info& b = rs.response_sessions_Sessions_session_info_m[1];
    setStr(b.session_info_session, s2);
    b.session_info_state.session_state_choice = session_state_r::session_state_Ready_tstr_c;
    b.session_info_title_present = false;

    return encodeResponse(resp);
}

// {"Tree": {root: null, nodes: [<engine unit, 2 children>], rev: 7}}.
QByteArray treeResponse() {
    static const QByteArray u1 = QByteArrayLiteral("u-root");
    static const QByteArray title = QByteArrayLiteral("Root orchestrator");
    static const QByteArray prof = QByteArrayLiteral("prof-9");
    static const QByteArray childA = QByteArrayLiteral("u-child-a");
    static const QByteArray childB = QByteArrayLiteral("u-child-b");

    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_tree_m_c;
    tree_report& tree = resp.api_response_response_tree_m.response_tree_Tree;
    tree.tree_report_root_choice = tree_report::tree_report_root_null_m_c;
    tree.tree_report_nodes_unit_node_m_count = 1;
    tree.tree_report_next_present = false;
    tree.tree_report_rev = 7;

    unit_node& u = tree.tree_report_nodes_unit_node_m[0];
    setStr(u.unit_node_id, u1);
    u.unit_node_kind.unit_kind_choice = unit_kind_r::unit_kind_Orchestrator_tstr_c;
    u.unit_node_state.unit_state_choice = unit_state_r::unit_state_Running_tstr_c;
    u.unit_node_work_choice = unit_node::unit_node_work_null_m_c;
    u.unit_node_children_unit_id_m_count = 2;
    setStr(u.unit_node_children_unit_id_m[0], childA);
    setStr(u.unit_node_children_unit_id_m[1], childB);
    u.unit_node_profile_present = true;
    u.unit_node_profile.unit_node_profile_choice =
        unit_node_profile_r::unit_node_profile_profile_ref_m_c;
    setStr(u.unit_node_profile.unit_node_profile_profile_ref_m, prof);
    u.unit_node_title_present = true;
    u.unit_node_title.unit_node_title_choice = unit_node_title_r::unit_node_title_tstr_c;
    setStr(u.unit_node_title.unit_node_title_tstr, title);
    u.unit_node_session_present = false;
    u.unit_node_role_present = false; // null role => canonical Primary
    u.unit_node_lifetime_present = false;
    u.unit_node_engine_present = false;

    return encodeResponse(resp);
}

// {"SessionDetail": {info: <s-alpha>, model: "gpt-x", checkpoints: 3}}.
QByteArray sessionDetailResponse() {
    static const QByteArray sid = QByteArrayLiteral("s-alpha");
    static const QByteArray model = QByteArrayLiteral("gpt-x");

    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_session_detail_m_c;
    response_session_detail& rsd = resp.api_response_response_session_detail_m;
    rsd.response_session_detail_SessionDetail_choice =
        response_session_detail::response_session_detail_SessionDetail_session_detail_m_c;
    session_detail& d = rsd.response_session_detail_SessionDetail_session_detail_m;

    setStr(d.session_detail_info.session_info_session, sid);
    d.session_detail_info.session_info_state.session_state_choice =
        session_state_r::session_state_Active_tstr_c;
    d.session_detail_info.session_info_title_present = false;

    d.session_detail_model_present = true;
    d.session_detail_model.session_detail_model_choice =
        session_detail_model_r::session_detail_model_tstr_c;
    setStr(d.session_detail_model.session_detail_model_tstr, model);
    d.session_detail_checkpoints_present = true;
    d.session_detail_checkpoints.session_detail_checkpoints = 3;
    d.session_detail_overlay_present = false;
    d.session_detail_delivery_targets_present = false;
    d.session_detail_children_present = false;
    d.session_detail_engine_present = false;
    d.session_detail_foreign_backend_present = false;
    d.session_detail_model_selector_present = false;

    return encodeResponse(resp);
}

// {"SessionDetail": null} — the node does not know this session.
QByteArray sessionDetailNullResponse() {
    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_session_detail_m_c;
    resp.api_response_response_session_detail_m.response_session_detail_SessionDetail_choice =
        response_session_detail::response_session_detail_SessionDetail_null_m_c;
    return encodeResponse(resp);
}

mirror::Session session(const QString& id, const QString& title) {
    mirror::Session s;
    s.session = id;
    s.title = title;
    return s;
}

mirror::FleetUnit unit(const QString& id, const QString& title) {
    mirror::FleetUnit u;
    u.id = id;
    u.title = title;
    return u;
}

int sessionCount(mirror::MirrorService& svc) {
    int n = 0;
    for (const mirror::Session& s : svc.store().snapshot().sessions) {
        (void)s;
        ++n;
    }
    return n;
}

const mirror::Session* findSession(mirror::MirrorService& svc, const QString& id) {
    return svc.store().snapshot().sessions.find(mirror::SessionKey{id});
}

int fleetCount(mirror::MirrorService& svc) {
    int n = 0;
    for (const mirror::FleetUnit& u : svc.store().snapshot().fleet_units) {
        (void)u;
        ++n;
    }
    return n;
}

} // namespace

class TestMirrorSessionsFleet : public QObject {
    Q_OBJECT

private slots:
    // map_session: a wire Sessions page decodes into Session entities with typed fields, enum
    // strings and nullable-optional handling (an absent title stays empty).
    void decodesSessionsWithTypedFields() {
        std::vector<mirror::Session> out;
        QVERIFY(decodeSessionsToMirror(sessionsResponse(), &out));
        QCOMPARE(out.size(), std::size_t{2});
        QCOMPARE(out[0].session, QStringLiteral("s-alpha"));
        QCOMPARE(out[0].state, QStringLiteral("Active"));
        QCOMPARE(out[0].title, QStringLiteral("Alpha thread"));
        QCOMPARE(out[0].bound_profile, QStringLiteral("prof-1"));
        QVERIFY(out[0].pinned);
        QCOMPARE(out[0].role, QStringLiteral("Primary"));
        QCOMPARE(out[1].session, QStringLiteral("s-beta"));
        QCOMPARE(out[1].state, QStringLiteral("Ready"));
        QVERIFY(out[1].title.isEmpty());
        QVERIFY(!out[1].pinned);
    }

    // map_fleet_unit: a wire Tree page decodes into FleetUnit entities; child_count = children
    // count, a null role canonicalizes to Primary, and the tree rev is surfaced.
    void decodesFleetUnitsWithChildCountAndRev() {
        std::vector<mirror::FleetUnit> out;
        QString next;
        quint64 rev = 0;
        QVERIFY(decodeFleetUnitsToMirror(treeResponse(), &out, &next, &rev));
        QCOMPARE(out.size(), std::size_t{1});
        QCOMPARE(out[0].id, QStringLiteral("u-root"));
        QCOMPARE(out[0].kind, QStringLiteral("Orchestrator"));
        QCOMPARE(out[0].state, QStringLiteral("Running"));
        QCOMPARE(out[0].title, QStringLiteral("Root orchestrator"));
        QCOMPARE(out[0].profile, QStringLiteral("prof-9"));
        QCOMPARE(out[0].role, QStringLiteral("Primary"));
        QCOMPARE(out[0].child_count, 2);
        QVERIFY(next.isEmpty());
        QCOMPARE(rev, quint64{7});
    }

    // decodeSessionDetailToMirror: SessionGet hydrates the detail-only model + checkpoints onto the
    // base session-info fields.
    void decodesSessionDetailHydration() {
        mirror::Session out;
        bool found = false;
        QVERIFY(decodeSessionDetailToMirror(sessionDetailResponse(), &out, &found));
        QVERIFY(found);
        QCOMPARE(out.session, QStringLiteral("s-alpha"));
        QCOMPARE(out.state, QStringLiteral("Active"));
        QCOMPARE(out.model, QStringLiteral("gpt-x"));
        QCOMPARE(out.checkpoints, 3);
    }

    // The null arm: the node does not know the session; found=false, no throw.
    void decodesSessionDetailNullArm() {
        mirror::Session out;
        bool found = true;
        QVERIFY(decodeSessionDetailToMirror(sessionDetailNullResponse(), &out, &found));
        QVERIFY(!found);
    }

    // deliverSessions is a GLOBAL replace-and-prune: a later list dropping a key tombstones it
    // (§5.3 PageLoop).
    void sessionsReplaceAndPrune() {
        mirror::MirrorService svc;
        svc.openInMemory();
        svc.ingestor().deliverSessions({session(QStringLiteral("s-a"), QStringLiteral("A")),
                                        session(QStringLiteral("s-b"), QStringLiteral("B"))},
                                       /*isFinalPage=*/true);
        QCOMPARE(sessionCount(svc), 2);
        svc.ingestor().deliverSessions({session(QStringLiteral("s-a"), QStringLiteral("A"))},
                                       /*isFinalPage=*/true);
        QCOMPARE(sessionCount(svc), 1);
        QVERIFY(findSession(svc, QStringLiteral("s-a")) != nullptr);
        QVERIFY(findSession(svc, QStringLiteral("s-b")) == nullptr);
    }

    // deliverSession is a keyed hydration upsert: it updates one row without pruning siblings.
    void sessionHydrationUpsertDoesNotPrune() {
        mirror::MirrorService svc;
        svc.openInMemory();
        svc.ingestor().deliverSessions({session(QStringLiteral("s-a"), QStringLiteral("A")),
                                        session(QStringLiteral("s-b"), QStringLiteral("B"))},
                                       /*isFinalPage=*/true);
        mirror::Session hydrated = session(QStringLiteral("s-a"), QStringLiteral("A"));
        hydrated.model = QStringLiteral("gpt-x");
        hydrated.checkpoints = 4;
        svc.ingestor().deliverSession(hydrated);
        QCOMPARE(sessionCount(svc), 2); // s-b survives the keyed patch
        const mirror::Session* a = findSession(svc, QStringLiteral("s-a"));
        QVERIFY(a != nullptr);
        QCOMPARE(a->model, QStringLiteral("gpt-x"));
        QCOMPARE(a->checkpoints, 4);
    }

    // deliverFleetUnits replace-and-prune + rev record: a later list dropping a key tombstones it,
    // and the tree rev lands in sync-state for the FleetChanged rung-1 skip-gate (§5.5).
    void fleetUnitsReplaceAndPruneRecordsRev() {
        mirror::MirrorService svc;
        svc.openInMemory();
        svc.ingestor().deliverFleetUnits({unit(QStringLiteral("u-a"), QStringLiteral("A")),
                                          unit(QStringLiteral("u-b"), QStringLiteral("B"))},
                                         /*isFinalPage=*/true, /*rev=*/11);
        QCOMPARE(fleetCount(svc), 2);
        QCOMPARE(svc.ingestor().syncState().collection(QStringLiteral("fleet")).nodeRev,
                 quint64{11});
        svc.ingestor().deliverFleetUnits({unit(QStringLiteral("u-a"), QStringLiteral("A"))},
                                         /*isFinalPage=*/true, /*rev=*/12);
        QCOMPARE(fleetCount(svc), 1);
        QCOMPARE(svc.ingestor().syncState().collection(QStringLiteral("fleet")).nodeRev,
                 quint64{12});
    }

    // §5.3 diff-before-write: re-delivering an identical session list stamps nothing.
    void identicalSessionListStampsNothing() {
        mirror::MirrorService svc;
        svc.openInMemory();
        svc.ingestor().deliverSessions({session(QStringLiteral("s-a"), QStringLiteral("A"))},
                                       /*isFinalPage=*/true);
        const quint64 head = svc.store().journal().headRev();
        svc.ingestor().deliverSessions({session(QStringLiteral("s-a"), QStringLiteral("A"))},
                                       /*isFinalPage=*/true);
        QCOMPARE(svc.store().journal().headRev(), head);
    }
};

QTEST_MAIN(TestMirrorSessionsFleet)
#include "tst_mirror_sessions_fleet.moc"
