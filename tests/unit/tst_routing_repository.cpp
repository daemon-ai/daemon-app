// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// The real routing manager backend (B6/ROU, wire v28):
//  - RoutingRepository lists the chat pins (RoutingListChats page loop), binds/unbinds
//    (RoutingBindChat/RoutingUnbindChat -> Ok -> authoritative re-list), and lists a transport's
//    bindable rooms (TransportRooms);
//  - DaemonDaemonNet projects the repo through the IDaemonNet seam the RoutingPage binds:
//    routes() as RoutingPins, resolve() answering from the pin table only (Pin vs Default),
//    accountsAgents() from the transport instances, and the QML chip helpers.

#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_daemonnet.h"
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
using daemonapp::daemon::DaemonDaemonNet;
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

class TestRoutingRepository : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;

private slots:
    // RoutingListChats round-trip: routes() populates with the decoded origins/sessions/profiles.
    void refreshChatsPopulates() {
        const QString sock = m_tmp.filePath(QStringLiteral("rt.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(chatRoutesResponse());
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("rt.db")));
        RoutingRepository repo(&client, &cache);

        QSignalSpy refreshed(&repo, &RoutingRepository::routesRefreshed);
        repo.refreshChats();
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        QCOMPARE(repo.routes().size(), 2);
        const DecodedChatRoute& pin = repo.routes().at(0);
        QCOMPARE(pin.origin.transport, QStringLiteral("matrix/@bot:hs.org"));
        QCOMPARE(pin.origin.scopeKind, QStringLiteral("group"));
        QCOMPARE(pin.origin.chat, QStringLiteral("#secops"));
        QCOMPARE(pin.session, QStringLiteral("s-secops"));
        QCOMPARE(pin.profile, QStringLiteral("prof-2"));
        QCOMPARE(repo.routes().at(1).origin.scopeKind, QStringLiteral("dm"));
        QCOMPARE(repo.routes().at(1).origin.user, QStringLiteral("@bob:hs.org"));
        // The outgoing request was the canonical encoder output.
        QCOMPARE(fake.callPayloads().first(), NodeApiCodec::encodeRoutingListChatsRequest());
    }

    // A bind goes out as RoutingBindChat and, on Ok, triggers the authoritative re-list.
    void bindChatRelistsOnOk() {
        const QString sock = m_tmp.filePath(QStringLiteral("rt-bind.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplySequence({okResponse(), chatRoutesResponse()});
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("rt-bind.db")));
        RoutingRepository repo(&client, &cache);

        QSignalSpy refreshed(&repo, &RoutingRepository::routesRefreshed);
        repo.bindChat(groupOrigin(QStringLiteral("matrix/@bot:hs.org"), QStringLiteral("#ops")),
                      QStringLiteral("s-ops"), QStringLiteral("prof-1"));
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        QCOMPARE(repo.routes().size(), 2); // the re-listed table
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.at(0),
                 NodeApiCodec::encodeRoutingBindChatRequest(
                     groupOrigin(QStringLiteral("matrix/@bot:hs.org"), QStringLiteral("#ops")),
                     QStringLiteral("s-ops"), QStringLiteral("prof-1")));
        QCOMPARE(calls.at(1), NodeApiCodec::encodeRoutingListChatsRequest());
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
        repo.unbindChat(groupOrigin(QStringLiteral("matrix/@x"), QStringLiteral("#gone")));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
        QCOMPARE(failed.at(0).at(0).toString(), QStringLiteral("no such pin"));
    }

    // TransportRooms round-trip: roomsFor(transport) populates (name fallback + pinned session).
    void roomsPopulate() {
        const QString sock = m_tmp.filePath(QStringLiteral("rt-rooms.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(roomsResponse());
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("rt-rooms.db")));
        RoutingRepository repo(&client, &cache);

        QSignalSpy refreshed(&repo, &RoutingRepository::roomsRefreshed);
        repo.refreshRooms(QStringLiteral("matrix/@bot:hs.org"));
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        const auto rooms = repo.roomsFor(QStringLiteral("matrix/@bot:hs.org"));
        QCOMPARE(rooms.size(), 2);
        QCOMPARE(rooms.at(0).room, QStringLiteral("#ops"));
        QCOMPARE(rooms.at(0).name, QStringLiteral("Ops room"));
        QCOMPARE(rooms.at(0).session, QStringLiteral("s-ops"));
        QVERIFY(rooms.at(1).name.isEmpty());
        QVERIFY(rooms.at(1).session.isEmpty());
    }

    // DaemonDaemonNet projects the repo through the IDaemonNet seam: pins, pin-table resolve
    // (Pin vs honest Default), and the QML chip helpers.
    void daemonNetProjectsPins() {
        const QString sock = m_tmp.filePath(QStringLiteral("rt-net.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(chatRoutesResponse());
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("rt-net.db")));
        RoutingRepository repo(&client, &cache);
        DaemonDaemonNet net(&repo, nullptr, nullptr);

        QSignalSpy changed(&net, &daemonnet::IDaemonNet::changed);
        repo.refreshChats();
        QTRY_VERIFY_WITH_TIMEOUT(changed.count() >= 1, 3000);

        const auto pins = net.routes();
        QCOMPARE(pins.size(), 2);
        QCOMPARE(pins.at(0).origin.transport.toString(), QStringLiteral("matrix/@bot:hs.org"));
        QCOMPARE(pins.at(0).origin.scope.chat, QStringLiteral("#secops"));
        QCOMPARE(pins.at(0).session.toString(), QStringLiteral("s-secops"));

        // resolve(): a pinned origin resolves by the pin; an unpinned one is an honest Default
        // (the client never re-derives the node's lower precedence rungs).
        domain::Origin pinned;
        pinned.transport = domain::TransportId(QStringLiteral("matrix/@bot:hs.org"));
        pinned.scope.kind = domain::OriginScopeKind::Group;
        pinned.scope.chat = QStringLiteral("#secops");
        QCOMPARE(net.resolve(pinned).decidedBy, daemonnet::DecidedBy::Pin);
        QCOMPARE(net.resolve(pinned).session.toString(), QStringLiteral("s-secops"));
        domain::Origin unpinned = pinned;
        unpinned.scope.chat = QStringLiteral("#not-pinned");
        QCOMPARE(net.resolve(unpinned).decidedBy, daemonnet::DecidedBy::Default);
        QVERIFY(net.resolve(unpinned).session.isEmpty());

        // The QML chip helpers (session header + room rows).
        QCOMPARE(net.pinsForSession(QStringLiteral("s-secops")).size(), 1);
        QCOMPARE(net.pinsForSession(QStringLiteral("s-secops"))
                     .first()
                     .toMap()
                     .value(QStringLiteral("label"))
                     .toString(),
                 QStringLiteral("#secops"));
        QVERIFY(net.pinsForSession(QStringLiteral("nope")).isEmpty());
        QCOMPARE(
            net.pinnedSessionFor(QStringLiteral("matrix/@bot:hs.org"), QStringLiteral("#secops")),
            QStringLiteral("s-secops"));
        QVERIFY(net.pinnedSessionFor(QStringLiteral("matrix/@bot:hs.org"), QStringLiteral("#dev"))
                    .isEmpty());

        // bindingRules() is honestly empty (config-time; no wire read op).
        QVERIFY(net.bindingRules().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestRoutingRepository)
#include "tst_routing_repository.moc"
