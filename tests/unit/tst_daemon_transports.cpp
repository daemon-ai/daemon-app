// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_presence_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/daemon_transport_registry.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "wire_mux_fixture.h"

#include <memory>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::CachedTransportInstanceRow;
using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::DecodedAdapterInfo;
using daemonapp::daemon::DecodedConversation;
using daemonapp::daemon::DecodedTransportInstance;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;
using daemonapp::daemon::TransportRepository;
using daemonapp::test::WireMuxServer;
using transports::DaemonPresenceService;
using transports::DaemonTransportRegistry;

namespace {

void setZ(zcbor_string& z, const QByteArray& b) {
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

// {"Adapters":[ AdapterInfo{ matrix } ]}
QByteArray adaptersResponse() {
    const QByteArray fam = QByteArrayLiteral("matrix");
    const QByteArray disp = QByteArrayLiteral("Matrix");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_adapters_m_c;
    response_adapters& ra = resp->api_response_response_adapters_m;
    ra.response_adapters_Adapters_adapter_info_m_count = 1;
    adapter_info& a = ra.response_adapters_Adapters_adapter_info_m[0];
    setZ(a.adapter_info_family, fam);
    setZ(a.adapter_info_display_name, disp);
    a.adapter_info_capabilities.adapter_capabilities_rooms = true;
    a.adapter_info_capabilities.adapter_capabilities_direct_messages = true;
    a.adapter_info_capabilities.adapter_capabilities_presence = true;
    a.adapter_info_capabilities.adapter_capabilities_room_enumeration = true;
    a.adapter_info_capabilities.adapter_capabilities_file_transfer = false;
    a.adapter_info_capabilities.adapter_capabilities_interactive_auth = true;
    a.adapter_info_account_schema_present = false;
    return encodeResponse(*resp);
}

// {"TransportInstances":[ TransportInstanceInfo{ matrix/@bot:hs, Connected, Unknown } ]}
QByteArray instancesResponse() {
    const QByteArray t = QByteArrayLiteral("matrix/@bot:hs");
    const QByteArray fam = QByteArrayLiteral("matrix");
    const QByteArray disp = QByteArrayLiteral("@bot:hs");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_transport_instances_m_c;
    response_transport_instances& ri = resp->api_response_response_transport_instances_m;
    ri.response_transport_instances_TransportInstances_transport_instance_info_m_count = 1;
    transport_instance_info& ti =
        ri.response_transport_instances_TransportInstances_transport_instance_info_m[0];
    setZ(ti.transport_instance_info_transport, t);
    setZ(ti.transport_instance_info_family, fam);
    setZ(ti.transport_instance_info_display_name, disp);
    ti.transport_instance_info_connection_present = true;
    ti.transport_instance_info_connection.transport_instance_info_connection
        .connection_state_choice = connection_state_r::connection_state_Connected_tstr_c;
    ti.transport_instance_info_presence_present = true;
    ti.transport_instance_info_presence.transport_instance_info_presence.presence_state_choice =
        presence_state_r::presence_state_Unknown_tstr_c;
    ti.transport_instance_info_bound_profile_present = false;
    return encodeResponse(*resp);
}

// {"Conversations": conv-page{ items: [ ConversationInfo{ channel, "General" } ] }} — the
// uniform WirePage envelope (wire v25), last page (no `next`).
QByteArray conversationsResponse() {
    const QByteArray t = QByteArrayLiteral("matrix/@bot:hs");
    const QByteArray id = QByteArrayLiteral("!room:hs");
    const QByteArray title = QByteArrayLiteral("General");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_conversations_m_c;
    conv_page& rc =
        resp->api_response_response_conversations_m.response_conversations_Conversations;
    rc.conv_page_items_conversation_info_m_count = 1;
    rc.conv_page_next_present = false;
    conversation_info& cv = rc.conv_page_items_conversation_info_m[0];
    setZ(cv.conversation_info_transport, t);
    setZ(cv.conversation_info_id, id);
    cv.conversation_info_kind.conversation_type_choice =
        conversation_type_r::conversation_type_Channel_tstr_c;
    cv.conversation_info_title_present = true;
    cv.conversation_info_title.conversation_info_title_choice =
        conversation_info_title_r::conversation_info_title_tstr_c;
    setZ(cv.conversation_info_title.conversation_info_title_tstr, title);
    cv.conversation_info_topic_present = false;
    cv.conversation_info_description_present = false;
    cv.conversation_info_members_present = false;
    return encodeResponse(*resp);
}

// One conv-page with a single minimal conversation (channel `id`, untitled) and an optional
// `next` cursor — the two-page loop fixture.
QByteArray conversationsPageResponse(const QByteArray& id, const QByteArray& next = {}) {
    const QByteArray t = QByteArrayLiteral("matrix/@bot:hs");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_conversations_m_c;
    conv_page& rc =
        resp->api_response_response_conversations_m.response_conversations_Conversations;
    rc.conv_page_items_conversation_info_m_count = 1;
    conversation_info& cv = rc.conv_page_items_conversation_info_m[0];
    setZ(cv.conversation_info_transport, t);
    setZ(cv.conversation_info_id, id);
    cv.conversation_info_kind.conversation_type_choice =
        conversation_type_r::conversation_type_Channel_tstr_c;
    cv.conversation_info_title_present = false;
    cv.conversation_info_topic_present = false;
    cv.conversation_info_description_present = false;
    cv.conversation_info_members_present = false;
    rc.conv_page_next_present = !next.isEmpty();
    if (!next.isEmpty()) {
        rc.conv_page_next.conv_page_next_choice = conv_page_next_r::conv_page_next_tstr_c;
        setZ(rc.conv_page_next.conv_page_next_tstr, next);
    }
    return encodeResponse(*resp);
}

struct DaemonTransportFixture {
    explicit DaemonTransportFixture(const QString& sock) : client(&transport) {
        transport.setSocketPath(sock);
    }
    DaemonTransport transport;
    NodeApiClient client;
};

} // namespace

// Phase 6a: the Channels read-surface codec facade round-trips, and the daemon-backed transport
// registry + presence render offline-first from the cache and populate over the mux.
class TestDaemonTransports : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;

private slots:
    void adaptersRoundTrip() {
        QList<DecodedAdapterInfo> out;
        QVERIFY(NodeApiCodec::decodeAdapters(adaptersResponse(), &out));
        QCOMPARE(out.size(), 1);
        QCOMPARE(out.at(0).family, QStringLiteral("matrix"));
        QCOMPARE(out.at(0).displayName, QStringLiteral("Matrix"));
        QVERIFY(out.at(0).capabilities.value(QStringLiteral("roomEnumeration")).toBool());
        QVERIFY(!out.at(0).capabilities.value(QStringLiteral("fileTransfer")).toBool());
    }

    void instancesRoundTrip() {
        QList<DecodedTransportInstance> out;
        QVERIFY(NodeApiCodec::decodeTransportInstances(instancesResponse(), &out));
        QCOMPARE(out.size(), 1);
        QCOMPARE(out.at(0).transport, QStringLiteral("matrix/@bot:hs"));
        QCOMPARE(out.at(0).connection, QStringLiteral("connected"));
        QCOMPARE(out.at(0).presence, QStringLiteral("unknown"));
    }

    void conversationsRoundTrip() {
        QList<DecodedConversation> out;
        QVERIFY(NodeApiCodec::decodeConversations(conversationsResponse(), &out));
        QCOMPARE(out.size(), 1);
        QCOMPARE(out.at(0).id, QStringLiteral("!room:hs"));
        QCOMPARE(out.at(0).kind, QStringLiteral("channel"));
        QCOMPARE(out.at(0).title, QStringLiteral("General"));
    }

    // Offline-first: a cached account renders in the registry + presence with NO connection.
    void registryRendersCacheOffline() {
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("ch-off.db")));
        QVERIFY(cache.isOpen());
        CachedTransportInstanceRow row;
        row.transport = QStringLiteral("matrix/@bot:hs");
        row.family = QStringLiteral("matrix");
        row.displayName = QStringLiteral("@bot:hs");
        row.connection = QStringLiteral("connected");
        row.presence = QStringLiteral("available");
        row.updatedAtMs = 1;
        QVERIFY(cache.upsertTransportInstance(row));

        DaemonTransport transport; // unconnected
        NodeApiClient client(&transport);
        TransportRepository repo(&client, &cache);
        DaemonTransportRegistry registry(&repo);
        DaemonPresenceService presence(&repo);

        const QVariantList accounts = registry.instances();
        QCOMPARE(accounts.size(), 1);
        QCOMPARE(accounts.at(0).toMap().value(QStringLiteral("transport")).toString(),
                 QStringLiteral("matrix/@bot:hs"));
        QCOMPARE(presence.connectionState(QStringLiteral("matrix/@bot:hs")),
                 QStringLiteral("connected"));
        QCOMPARE(presence.presence(QStringLiteral("matrix/@bot:hs")), QStringLiteral("available"));
        // An unknown transport degrades to offline/unknown.
        QCOMPARE(presence.connectionState(QStringLiteral("nope")), QStringLiteral("offline"));
    }

    // Instances + conversations populate the cache + registry end-to-end over the mux.
    void refreshPopulatesOverMux() {
        const QString sock = m_tmp.filePath(QStringLiteral("ch.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(instancesResponse());
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("ch.db")));
        TransportRepository repo(&tx.client, &cache);
        DaemonTransportRegistry registry(&repo);
        DaemonPresenceService presence(&repo);

        QSignalSpy instances(&registry, &transports::ITransportRegistry::instancesChanged);
        repo.refreshInstances();
        QTRY_COMPARE_WITH_TIMEOUT(instances.count(), 1, 3000);
        QCOMPARE(cache.transportInstances().size(), 1);
        QCOMPARE(registry.instances().size(), 1);
        QCOMPARE(presence.connectionState(QStringLiteral("matrix/@bot:hs")),
                 QStringLiteral("connected"));

        // EIO-8: a live ConvList populates the per-account rooms.
        fake.setReplyPayload(conversationsResponse());
        QSignalSpy convs(&registry, &transports::ITransportRegistry::conversationsChanged);
        registry.refreshConversations(QStringLiteral("matrix/@bot:hs"));
        QTRY_COMPARE_WITH_TIMEOUT(convs.count(), 1, 3000);
        const QVariantList rooms = registry.conversations(QStringLiteral("matrix/@bot:hs"));
        QCOMPARE(rooms.size(), 1);
        QCOMPARE(rooms.at(0).toMap().value(QStringLiteral("title")).toString(),
                 QStringLiteral("General"));
    }

    // Wire v25 page loop: a conversations listing served in two pages accumulates into ONE
    // conversationsRefreshed (with both rows cached), and the continuation request carries the
    // `after` cursor.
    void conversationsPageAcrossTheCursorLoop() {
        const QString sock = m_tmp.filePath(QStringLiteral("ch-pg.sock"));
        const QString t = QStringLiteral("matrix/@bot:hs");
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplySequence(
            {conversationsPageResponse(QByteArrayLiteral("!a:hs"), QByteArrayLiteral("!a:hs")),
             conversationsPageResponse(QByteArrayLiteral("!b:hs"))});
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("ch-pg.db")));
        TransportRepository repo(&tx.client, &cache);

        QSignalSpy convs(&repo, &TransportRepository::conversationsRefreshed);
        repo.refreshConversations(t);
        QTRY_COMPARE_WITH_TIMEOUT(convs.count(), 1, 3000);
        QTest::qWait(100); // settle: a page-loop bug would emit again / issue extra requests
        QCOMPARE(convs.count(), 1);

        // Both pages landed in ONE sync (replace + prune over the whole listing).
        const auto cached = cache.conversations(t);
        QCOMPARE(cached.size(), 2);
        QCOMPARE(cached.at(0).convId, QStringLiteral("!a:hs"));
        QCOMPARE(cached.at(1).convId, QStringLiteral("!b:hs"));

        // The continuation carried the cursor: byte-identical to the codec's own encoding.
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 2);
        QCOMPARE(calls.at(0), NodeApiCodec::encodeConvListRequest(t));
        QCOMPARE(calls.at(1), NodeApiCodec::encodeConvListRequest(t, QStringLiteral("!a:hs")));
    }
};

QTEST_GUILESS_MAIN(TestDaemonTransports)
#include "tst_daemon_transports.moc"
