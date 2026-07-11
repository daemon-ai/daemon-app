// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// The real routing manager backend (B6/ROU, wire v28; M3 read path):
//  - RoutingRepository lists the chat pins (RoutingListChats page loop), binds/unbinds
//    (RoutingBindChat/RoutingUnbindChat -> Ok -> authoritative re-list), and lists a transport's
//    bindable rooms (TransportRooms). The decoded pins/rooms are the SAME wire shapes the mirror
//    decode-to-mirror bridge maps into route_pins/rooms (tst_routing_projection), so the wire
//    round-trip is exercised here and the mirror landing is exercised there.

#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "daemon_api_client_decode.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "wire_mux_fixture.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::DecodedChatRoute;
using daemonapp::daemon::DecodedOrigin;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;
using daemonapp::daemon::RoutingRepository;
using daemonapp::test::WireMuxServer;

namespace {

void setStr(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

// {"ChatRoutes": {items: [<one group pin>, <one dm pin>], next: null-ish}} via the generated
// encoder. Backing strings are static so they outlive the encode.
QByteArray chatRoutesResponse() {
    static const QByteArray t = QByteArrayLiteral("matrix/@bot:hs.org");
    static const QByteArray chat = QByteArrayLiteral("#secops");
    static const QByteArray user = QByteArrayLiteral("@bob:hs.org");
    static const QByteArray s1 = QByteArrayLiteral("s-secops");
    static const QByteArray s2 = QByteArrayLiteral("s-bob");
    static const QByteArray prof = QByteArrayLiteral("prof-2");

    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_chat_routes_m_c;
    chat_route_page& page =
        resp.api_response_response_chat_routes_m.response_chat_routes_ChatRoutes;
    page.chat_route_page_items_chat_route_m_count = 2;

    chat_route& r1 = page.chat_route_page_items_chat_route_m[0];
    setStr(r1.chat_route_origin.origin_transport, t);
    r1.chat_route_origin.origin_scope.origin_scope_t_choice =
        origin_scope_t_r::origin_scope_t_origin_scope_group_m_c;
    setStr(r1.chat_route_origin.origin_scope.origin_scope_t_origin_scope_group_m.Group_chat, chat);
    r1.chat_route_origin.origin_scope.origin_scope_t_origin_scope_group_m.Group_thread_choice =
        origin_scope_group::Group_thread_null_m_c;
    r1.chat_route_origin.origin_sender_present = false;
    setStr(r1.chat_route_session, s1);
    r1.chat_route_profile_present = true;
    r1.chat_route_profile.chat_route_profile_choice =
        chat_route_profile_r::chat_route_profile_profile_ref_m_c;
    setStr(r1.chat_route_profile.chat_route_profile_profile_ref_m, prof);
    r1.chat_route_isolation_present = false;

    chat_route& r2 = page.chat_route_page_items_chat_route_m[1];
    setStr(r2.chat_route_origin.origin_transport, t);
    r2.chat_route_origin.origin_scope.origin_scope_t_choice =
        origin_scope_t_r::origin_scope_t_origin_scope_dm_m_c;
    setStr(r2.chat_route_origin.origin_scope.origin_scope_t_origin_scope_dm_m.Dm_user, user);
    r2.chat_route_origin.origin_sender_present = false;
    setStr(r2.chat_route_session, s2);
    r2.chat_route_profile_present = false;
    r2.chat_route_isolation_present = false;

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

// {"Rooms": {items: [#ops (pinned to s-ops), #dev (unpinned)]}}
QByteArray roomsResponse() {
    static const QByteArray t = QByteArrayLiteral("matrix/@bot:hs.org");
    static const QByteArray r1 = QByteArrayLiteral("#ops");
    static const QByteArray r1name = QByteArrayLiteral("Ops room");
    static const QByteArray r1sess = QByteArrayLiteral("s-ops");
    static const QByteArray r2 = QByteArrayLiteral("#dev");

    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_rooms_m_c;
    room_page& page = resp.api_response_response_rooms_m.response_rooms_Rooms;
    page.room_page_items_room_info_m_count = 2;

    room_info& a = page.room_page_items_room_info_m[0];
    setStr(a.room_info_transport, t);
    setStr(a.room_info_room, r1);
    a.room_info_name_present = true;
    a.room_info_name.room_info_name_choice = room_info_name_r::room_info_name_tstr_c;
    setStr(a.room_info_name.room_info_name_tstr, r1name);
    a.room_info_session_present = true;
    a.room_info_session.room_info_session_choice =
        room_info_session_r::room_info_session_session_id_m_c;
    setStr(a.room_info_session.room_info_session_session_id_m, r1sess);

    room_info& b = page.room_page_items_room_info_m[1];
    setStr(b.room_info_transport, t);
    setStr(b.room_info_room, r2);
    b.room_info_name_present = false;
    b.room_info_session_present = false;

    page.room_page_next_present = false;

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

// ApiResponse::Ok (the bare CBOR text string).
QByteArray okResponse() {
    QByteArray b;
    daemonapp::test::cborText(b, "Ok");
    return b;
}

DecodedOrigin groupOrigin(const QString& transport, const QString& chat) {
    DecodedOrigin o;
    o.transport = transport;
    o.scopeKind = QStringLiteral("group");
    o.chat = chat;
    return o;
}

} // namespace

namespace {
domain::Origin domainGroupOrigin(const QString& transport, const QString& chat) {
    domain::Origin o;
    o.transport = domain::TransportId(transport);
    o.scope.kind = domain::OriginScopeKind::Group;
    o.scope.chat = chat;
    return o;
}
} // namespace

// AD: the routing repo is the MUTATION seam only — reads live on the mirror pin table
// (RoutingListChats → route_pins via the ingestor; the dead in-memory routes/rooms cache died).
class TestRoutingRepository : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;

private slots:
    // A bind goes out as RoutingBindChat (byte-identical to the encoder) and, on Ok,
    // mutationApplied fires — the graph's hook for the mirror pin-table refetch.
    void bindChatAcksViaMutationApplied() {
        const QString sock = m_tmp.filePath(QStringLiteral("rt-bind.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(okResponse());
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("rt-bind.db")));
        RoutingRepository repo(&client, &cache);

        QSignalSpy applied(&repo, &RoutingRepository::mutationApplied);
        repo.routingBindChat(
            domainGroupOrigin(QStringLiteral("matrix/@bot:hs.org"), QStringLiteral("#ops")),
            domain::SessionId(QStringLiteral("s-ops")),
            domain::ProfileRef(QStringLiteral("prof-1")));
        QTRY_COMPARE_WITH_TIMEOUT(applied.count(), 1, 3000);
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 1); // the mutation itself; the refetch is the ingestor's
        QCOMPARE(calls.at(0),
                 NodeApiCodec::encodeRoutingBindChatRequest(
                     groupOrigin(QStringLiteral("matrix/@bot:hs.org"), QStringLiteral("#ops")),
                     QStringLiteral("s-ops"), QStringLiteral("prof-1")));
    }

    // An unbind rejection (Error) surfaces via operationFailed — never a silent no-op.
    void unbindErrorSurfaces() {
        const QString sock = m_tmp.filePath(QStringLiteral("rt-err.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        QByteArray err;
        err.append(static_cast<char>(0xA1));
        daemonapp::test::cborText(err, "Error");
        err.append(static_cast<char>(0xA1));
        daemonapp::test::cborText(err, "Unsupported");
        daemonapp::test::cborText(err, "no such pin");
        fake.setReplyPayload(err);
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("rt-err.db")));
        RoutingRepository repo(&client, &cache);

        QSignalSpy failed(&repo, &RoutingRepository::operationFailed);
        repo.routingUnbindChat(
            domainGroupOrigin(QStringLiteral("matrix/@x"), QStringLiteral("#gone")));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
        QCOMPARE(failed.at(0).at(0).toString(), QStringLiteral("no such pin"));
    }
};

QTEST_GUILESS_MAIN(TestRoutingRepository)
#include "tst_routing_repository.moc"
