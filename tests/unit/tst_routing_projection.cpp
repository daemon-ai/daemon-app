// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// The M3 routing read path on the MIRROR store (spec 09 §5.4/§8; wave M3). Ported from the deleted
// mock routing-model tests: routing precedence is now node-owned (the client reads the store's pin
// table), so these assert the mirror landing instead —
//  - the decode-to-mirror bridge (map_route_pin): a wire ChatRoutes page decodes into RoutePin
//    entities with the canonical origin_key derivation;
//  - the ingestor's deliverRoutePins (GLOBAL replace-and-prune over route_pins) and deliverRooms
//    (PER-TRANSPORT replace-and-prune over rooms), the single-writer store apply (§5.1/§5.3).

#include "daemon/mirror_decode.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "mirror/mirror_service.h"
#include "mirror/store.h"

#include <QtTest/QtTest>
#include <vector>

using daemonapp::daemon::decodeRoutePinsToMirror;

namespace {

void setStr(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

// {"ChatRoutes": {items: [<one group pin with a profile>], next: null}} via the generated encoder.
QByteArray chatRoutesResponse() {
    static const QByteArray t = QByteArrayLiteral("matrix/@bot:hs.org");
    static const QByteArray chat = QByteArrayLiteral("#secops");
    static const QByteArray sess = QByteArrayLiteral("s-secops");
    static const QByteArray prof = QByteArrayLiteral("prof-2");

    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_chat_routes_m_c;
    chat_route_page& page =
        resp.api_response_response_chat_routes_m.response_chat_routes_ChatRoutes;
    page.chat_route_page_items_chat_route_m_count = 1;
    chat_route& r = page.chat_route_page_items_chat_route_m[0];
    setStr(r.chat_route_origin.origin_transport, t);
    r.chat_route_origin.origin_scope.origin_scope_t_choice =
        origin_scope_t_r::origin_scope_t_origin_scope_group_m_c;
    setStr(r.chat_route_origin.origin_scope.origin_scope_t_origin_scope_group_m.Group_chat, chat);
    r.chat_route_origin.origin_scope.origin_scope_t_origin_scope_group_m.Group_thread_choice =
        origin_scope_group::Group_thread_null_m_c;
    r.chat_route_origin.origin_sender_present = false;
    setStr(r.chat_route_session, sess);
    r.chat_route_profile_present = true;
    r.chat_route_profile.chat_route_profile_choice =
        chat_route_profile_r::chat_route_profile_profile_ref_m_c;
    setStr(r.chat_route_profile.chat_route_profile_profile_ref_m, prof);
    r.chat_route_isolation_present = false;
    page.chat_route_page_next_present = false;

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

mirror::RoutePin pin(const QString& key, const QString& transport, const QString& session) {
    mirror::RoutePin p;
    p.origin_key = key;
    p.transport = transport;
    p.session = session;
    return p;
}

mirror::Room room(const QString& transport, const QString& id, const QString& session) {
    mirror::Room r;
    r.transport = transport;
    r.room = id;
    r.session = session;
    return r;
}

int routePinCount(mirror::MirrorService& svc) {
    int n = 0;
    for (const mirror::RoutePin& p : svc.store().snapshot().route_pins) {
        (void)p;
        ++n;
    }
    return n;
}

int roomCount(mirror::MirrorService& svc, const QString& transport) {
    int n = 0;
    for (const mirror::Room& r : svc.store().snapshot().rooms) {
        if (r.transport == transport) {
            ++n;
        }
    }
    return n;
}

} // namespace

class TestRoutingProjection : public QObject {
    Q_OBJECT

private slots:
    // map_route_pin: a wire ChatRoutes page decodes into RoutePin entities, deriving the canonical
    // origin_key (`transport|group|chat|thread`, empty thread appended) + the typed fields.
    void decodesPinsWithCanonicalKey() {
        std::vector<mirror::RoutePin> pins;
        QString next;
        QVERIFY(decodeRoutePinsToMirror(chatRoutesResponse(), &pins, &next));
        QCOMPARE(pins.size(), std::size_t{1});
        QCOMPARE(pins[0].origin_key, QStringLiteral("matrix/@bot:hs.org|group|#secops|"));
        QCOMPARE(pins[0].transport, QStringLiteral("matrix/@bot:hs.org"));
        QCOMPARE(pins[0].session, QStringLiteral("s-secops"));
        QCOMPARE(pins[0].profile, QStringLiteral("prof-2"));
        QVERIFY(next.isEmpty());
    }

    // deliverRoutePins is a GLOBAL replace-and-prune: a later page that drops a key tombstones it.
    void routePinsReplaceAndPrune() {
        mirror::MirrorService svc;
        svc.openInMemory();
        svc.ingestor().deliverRoutePins(
            {pin(QStringLiteral("t|group|a|"), QStringLiteral("t"), QStringLiteral("s-a")),
             pin(QStringLiteral("t|dm|bob"), QStringLiteral("t"), QStringLiteral("s-bob"))},
            /*isFinalPage=*/true);
        QCOMPARE(routePinCount(svc), 2);

        // A later full list without the dm pin prunes it; the surviving pin updates in place.
        svc.ingestor().deliverRoutePins(
            {pin(QStringLiteral("t|group|a|"), QStringLiteral("t"), QStringLiteral("s-a2"))},
            /*isFinalPage=*/true);
        QCOMPARE(routePinCount(svc), 1);
        const mirror::RoutePin* survivor = svc.store().snapshot().route_pins.find(
            mirror::RoutePinKey{QStringLiteral("t|group|a|")});
        QVERIFY(survivor != nullptr);
        QCOMPARE(survivor->session, QStringLiteral("s-a2"));
        QVERIFY(svc.store().snapshot().route_pins.find(
                    mirror::RoutePinKey{QStringLiteral("t|dm|bob")}) == nullptr);
    }

    // deliverRooms is a PER-TRANSPORT replace-and-prune: another transport's rooms are untouched.
    void roomsPruneScopedToTransport() {
        mirror::MirrorService svc;
        svc.openInMemory();
        svc.ingestor().deliverRooms(
            QStringLiteral("tx1"),
            {room(QStringLiteral("tx1"), QStringLiteral("#a"), QString()),
             room(QStringLiteral("tx1"), QStringLiteral("#b"), QStringLiteral("s-b"))},
            /*isFinalPage=*/true);
        svc.ingestor().deliverRooms(QStringLiteral("tx2"),
                                    {room(QStringLiteral("tx2"), QStringLiteral("#c"), QString())},
                                    /*isFinalPage=*/true);
        QCOMPARE(roomCount(svc, QStringLiteral("tx1")), 2);
        QCOMPARE(roomCount(svc, QStringLiteral("tx2")), 1);

        // Re-list tx1 without #b: it is pruned; tx2 is unaffected.
        svc.ingestor().deliverRooms(QStringLiteral("tx1"),
                                    {room(QStringLiteral("tx1"), QStringLiteral("#a"), QString())},
                                    /*isFinalPage=*/true);
        QCOMPARE(roomCount(svc, QStringLiteral("tx1")), 1);
        QCOMPARE(roomCount(svc, QStringLiteral("tx2")), 1);
    }
};

QTEST_GUILESS_MAIN(TestRoutingProjection)
#include "tst_routing_projection.moc"
