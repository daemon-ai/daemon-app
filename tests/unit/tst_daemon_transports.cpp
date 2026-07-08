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
#include "transports/mock_transport_registry.h"
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
    // [waveB:app-v30] D3: one node-labeled policy row {key,label,value}.
    static const QByteArray pk = QByteArrayLiteral("auto_accept");
    static const QByteArray pl = QByteArrayLiteral("Auto-accept invites");
    static const QByteArray pv = QByteArrayLiteral("trusted only");
    a.adapter_info_policies_present = true;
    a.adapter_info_policies.adapter_info_policies_policy_entry_m_count = 1;
    policy_entry& pe = a.adapter_info_policies.adapter_info_policies_policy_entry_m[0];
    setZ(pe.policy_entry_key, pk);
    setZ(pe.policy_entry_label, pl);
    setZ(pe.policy_entry_value, pv);
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

// [waveB:app-v30] D1: a Disconnecting instance carrying reason (AuthenticationFailed) + a verbatim
// message + fatal=true — the re-auth-gating shape.
QByteArray instancesResponseFatal() {
    const QByteArray t = QByteArrayLiteral("matrix/@bot:hs");
    const QByteArray fam = QByteArrayLiteral("matrix");
    const QByteArray disp = QByteArrayLiteral("@bot:hs");
    const QByteArray msg = QByteArrayLiteral("token expired");
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
        .connection_state_choice = connection_state_r::connection_state_Disconnecting_tstr_c;
    ti.transport_instance_info_presence_present = false;
    ti.transport_instance_info_bound_profile_present = false;
    ti.transport_instance_info_reason_present = true;
    ti.transport_instance_info_reason.transport_instance_info_reason_choice =
        transport_instance_info_reason_r::transport_instance_info_reason_disconnect_reason_m_c;
    ti.transport_instance_info_reason.transport_instance_info_reason_disconnect_reason_m
        .disconnect_reason_choice =
        disconnect_reason_r::disconnect_reason_AuthenticationFailed_tstr_c;
    ti.transport_instance_info_message_present = true;
    ti.transport_instance_info_message.transport_instance_info_message_choice =
        transport_instance_info_message_r::transport_instance_info_message_tstr_c;
    setZ(ti.transport_instance_info_message.transport_instance_info_message_tstr, msg);
    ti.transport_instance_info_fatal_present = true;
    ti.transport_instance_info_fatal.transport_instance_info_fatal = true;
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

// [acct-mgmt] ApiResponse::Ok — the externally-tagged unit variant serializes as the bare text
// "Ok" (matching the node), reused for the member-op Ok replies.
QByteArray okResponse() {
    QByteArray b;
    daemonapp::test::cborText(b, "Ok");
    return b;
}

// [acct-mgmt] {"Conversation": Some(ConversationInfo{ channel "!room:hs" + one member Bob@Op })}.
QByteArray convGetResponse() {
    static const QByteArray t = QByteArrayLiteral("matrix/@bot:hs");
    static const QByteArray id = QByteArrayLiteral("!room:hs");
    static const QByteArray title = QByteArrayLiteral("General");
    static const QByteArray bob = QByteArrayLiteral("@bob:matrix.org");
    static const QByteArray bobName = QByteArrayLiteral("Bob");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_conversation_m_c;
    response_conversation& rc = resp->api_response_response_conversation_m;
    rc.response_conversation_Conversation_choice =
        response_conversation::response_conversation_Conversation_conversation_info_m_c;
    conversation_info& cv = rc.response_conversation_Conversation_conversation_info_m;
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
    cv.conversation_info_members_present = true;
    cv.conversation_info_members.conversation_info_members_conversation_member_m_count = 1;
    conversation_member& m =
        cv.conversation_info_members.conversation_info_members_conversation_member_m[0];
    setZ(m.conversation_member_contact.contact_info_id, bob);
    m.conversation_member_contact.contact_info_display_name_present = true;
    m.conversation_member_contact.contact_info_display_name.contact_info_display_name_choice =
        contact_info_display_name_r::contact_info_display_name_tstr_c;
    setZ(m.conversation_member_contact.contact_info_display_name.contact_info_display_name_tstr,
         bobName);
    m.conversation_member_contact.contact_info_presence_present = true;
    m.conversation_member_contact.contact_info_presence.contact_info_presence.presence_primitive
        .presence_primitive_t_choice =
        presence_primitive_t_r::presence_primitive_t_Available_tstr_c;
    m.conversation_member_contact.contact_info_presence.contact_info_presence
        .presence_message_present = false;
    m.conversation_member_contact.contact_info_presence.contact_info_presence
        .presence_emoji_present = false;
    m.conversation_member_contact.contact_info_presence.contact_info_presence
        .presence_mobile_present = false;
    m.conversation_member_contact.contact_info_presence.contact_info_presence
        .presence_idle_since_present = false;
    m.conversation_member_contact.contact_info_permission_present = false;
    m.conversation_member_alias_present = false;
    m.conversation_member_nickname_present = false;
    m.conversation_member_typing_present = false;
    m.conversation_member_role_present = true;
    m.conversation_member_role.conversation_member_role.member_role_choice =
        member_role_r::member_role_Op_tstr_c;
    m.conversation_member_session_present = false;
    return encodeResponse(*resp);
}

// [acct-mgmt] {"ConvJoinDetails": ChannelJoinDetails{ nickname+password supported, one extra }}.
QByteArray joinDetailsResponse() {
    static const QByteArray fkey = QByteArrayLiteral("floor_policy");
    static const QByteArray flabel = QByteArrayLiteral("Floor policy");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_conv_join_details_m_c;
    channel_join_details& d =
        resp->api_response_response_conv_join_details_m.response_conv_join_details_ConvJoinDetails;
    d.channel_join_details_name_present = false;
    d.channel_join_details_name_max_length_present = true;
    d.channel_join_details_name_max_length.channel_join_details_name_max_length = 64;
    d.channel_join_details_nickname_present = false;
    d.channel_join_details_nickname_supported_present = true;
    d.channel_join_details_nickname_supported.channel_join_details_nickname_supported = true;
    d.channel_join_details_nickname_max_length_present = false;
    d.channel_join_details_password_present = false;
    d.channel_join_details_password_supported_present = true;
    d.channel_join_details_password_supported.channel_join_details_password_supported = true;
    d.channel_join_details_password_max_length_present = false;
    d.channel_join_details_extras_schema_present = true;
    account_settings_schema& s =
        d.channel_join_details_extras_schema.channel_join_details_extras_schema;
    s.account_settings_schema_fields_present = true;
    s.account_settings_schema_fields.account_settings_schema_fields_auth_param_field_m_count = 1;
    auth_param_field& f =
        s.account_settings_schema_fields.account_settings_schema_fields_auth_param_field_m[0];
    setZ(f.auth_param_field_key, fkey);
    setZ(f.auth_param_field_label, flabel);
    f.auth_param_field_required = false;
    d.channel_join_details_extras_present = false;
    return encodeResponse(*resp);
}

// [acct-mgmt] {"ConvCreateDetails": CreateConversationDetails{ max=0, one extra }}.
QByteArray createDetailsResponse() {
    static const QByteArray fkey = QByteArrayLiteral("room_name");
    static const QByteArray flabel = QByteArrayLiteral("Room name");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_conv_create_details_m_c;
    create_conversation_details& d = resp->api_response_response_conv_create_details_m
                                         .response_conv_create_details_ConvCreateDetails;
    d.create_conversation_details_max_participants_present = true;
    d.create_conversation_details_max_participants.create_conversation_details_max_participants = 0;
    d.create_conversation_details_participants_present = false;
    d.create_conversation_details_extras_schema_present = true;
    account_settings_schema& s =
        d.create_conversation_details_extras_schema.create_conversation_details_extras_schema;
    s.account_settings_schema_fields_present = true;
    s.account_settings_schema_fields.account_settings_schema_fields_auth_param_field_m_count = 1;
    auth_param_field& f =
        s.account_settings_schema_fields.account_settings_schema_fields_auth_param_field_m[0];
    setZ(f.auth_param_field_key, fkey);
    setZ(f.auth_param_field_label, flabel);
    f.auth_param_field_required = true;
    d.create_conversation_details_extras_present = false;
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
        // [waveB:app-v30] D3: the node-labeled policy row decodes verbatim.
        QCOMPARE(out.at(0).policies.size(), 1);
        QCOMPARE(out.at(0).policies.at(0).value(QStringLiteral("key")).toString(),
                 QStringLiteral("auto_accept"));
        QCOMPARE(out.at(0).policies.at(0).value(QStringLiteral("label")).toString(),
                 QStringLiteral("Auto-accept invites"));
        QCOMPARE(out.at(0).policies.at(0).value(QStringLiteral("value")).toString(),
                 QStringLiteral("trusted only"));
    }

    void instancesRoundTrip() {
        QList<DecodedTransportInstance> out;
        QVERIFY(NodeApiCodec::decodeTransportInstances(instancesResponse(), &out));
        QCOMPARE(out.size(), 1);
        QCOMPARE(out.at(0).transport, QStringLiteral("matrix/@bot:hs"));
        QCOMPARE(out.at(0).connection, QStringLiteral("connected"));
        QCOMPARE(out.at(0).presence, QStringLiteral("unknown"));
        // No provenance on a healthy instance.
        QVERIFY(out.at(0).reason.isEmpty());
        QVERIFY(out.at(0).message.isEmpty());
        QVERIFY(!out.at(0).fatal);
    }

    // [waveB:app-v30] D1: the disconnecting state + reason/message/fatal decode.
    void instancesReasonMessageFatalRoundTrip() {
        QList<DecodedTransportInstance> out;
        QVERIFY(NodeApiCodec::decodeTransportInstances(instancesResponseFatal(), &out));
        QCOMPARE(out.size(), 1);
        QCOMPARE(out.at(0).connection, QStringLiteral("disconnecting"));
        QCOMPARE(out.at(0).reason, QStringLiteral("authentication_failed"));
        QCOMPARE(out.at(0).message, QStringLiteral("token expired"));
        QVERIFY(out.at(0).fatal);
    }

    void conversationsRoundTrip() {
        QList<DecodedConversation> out;
        QVERIFY(NodeApiCodec::decodeConversations(conversationsResponse(), &out));
        QCOMPARE(out.size(), 1);
        QCOMPARE(out.at(0).id, QStringLiteral("!room:hs"));
        QCOMPARE(out.at(0).kind, QStringLiteral("channel"));
        QCOMPARE(out.at(0).title, QStringLiteral("General"));
    }

    // [acct-mgmt] ConvGet decodes a single conversation carrying members (contact + role/presence).
    void convGetMembersRoundTrip() {
        DecodedConversation out;
        bool found = false;
        QVERIFY(NodeApiCodec::decodeConversation(convGetResponse(), &out, &found));
        QVERIFY(found);
        QCOMPARE(out.id, QStringLiteral("!room:hs"));
        QVERIFY(out.hasMembers);
        QCOMPARE(out.members.size(), 1);
        QCOMPARE(out.members.at(0).contactId, QStringLiteral("@bob:matrix.org"));
        QCOMPARE(out.members.at(0).displayName, QStringLiteral("Bob"));
        QCOMPARE(out.members.at(0).presence, QStringLiteral("available"));
        QCOMPARE(out.members.at(0).role, QStringLiteral("Op"));
    }

    // [acct-mgmt] The two-phase form details decode (honoring *_supported + *_max_length + extras).
    void joinDetailsRoundTrip() {
        daemonapp::daemon::DecodedChannelJoinDetails d;
        QVERIFY(NodeApiCodec::decodeConvJoinDetails(joinDetailsResponse(), &d));
        QVERIFY(d.hasNameMaxLength);
        QCOMPARE(d.nameMaxLength, 64u);
        QVERIFY(d.nicknameSupported);
        QVERIFY(d.passwordSupported);
        QCOMPARE(d.extrasSchema.size(), 1);
        QCOMPARE(d.extrasSchema.at(0).key, QStringLiteral("floor_policy"));
        QCOMPARE(d.extrasSchema.at(0).label, QStringLiteral("Floor policy"));
    }

    void createDetailsRoundTrip() {
        daemonapp::daemon::DecodedCreateConversationDetails d;
        QVERIFY(NodeApiCodec::decodeConvCreateDetails(createDetailsResponse(), &d));
        QVERIFY(d.hasMaxParticipants);
        QCOMPARE(d.maxParticipants, 0u);
        QCOMPARE(d.extrasSchema.size(), 1);
        QCOMPARE(d.extrasSchema.at(0).key, QStringLiteral("room_name"));
        QVERIFY(d.extrasSchema.at(0).required);
    }

    // [acct-mgmt] The room-lifecycle + member request encoders each select a distinct, non-empty
    // request arm (so a reply never crosses wires).
    void roomAndMemberRequestsEncodeDistinctArms() {
        const QString t = QStringLiteral("matrix/@bot:hs");
        const QString c = QStringLiteral("!room:hs");
        const QString who = QStringLiteral("@bob:matrix.org");
        daemonapp::daemon::ConvJoinForm jf;
        jf.name = QStringLiteral("#daemon-dev");
        jf.hasNickname = true;
        jf.nickname = QStringLiteral("ali");
        daemonapp::daemon::ConvCreateForm cf;
        cf.participants = QStringList{who};
        const QList<QByteArray> encs = {
            NodeApiCodec::encodeConvJoinDetailsRequest(t),
            NodeApiCodec::encodeConvCreateDetailsRequest(t),
            NodeApiCodec::encodeConvJoinRequest(t, jf),
            NodeApiCodec::encodeConvCreateRequest(t, cf),
            NodeApiCodec::encodeConvLeaveRequest(t, c),
            NodeApiCodec::encodeConvDeleteRequest(t, c),
            NodeApiCodec::encodeConvGetRequest(t, c),
            NodeApiCodec::encodeMemberInviteRequest(t, c, who),
            NodeApiCodec::encodeMemberRemoveRequest(t, c, who),
            NodeApiCodec::encodeMemberBanRequest(t, c, who),
            NodeApiCodec::encodeMemberSetRoleRequest(t, c, who, QStringLiteral("Op")),
        };
        for (const QByteArray& e : encs) {
            QVERIFY(!e.isEmpty());
        }
        // All distinct (no two encoders collapse to the same bytes).
        for (int i = 0; i < encs.size(); ++i) {
            for (int j = i + 1; j < encs.size(); ++j) {
                QVERIFY2(encs.at(i) != encs.at(j),
                         qPrintable(QStringLiteral("encoders %1 and %2 collided").arg(i).arg(j)));
            }
        }
    }

    // [acct-mgmt] conversationJoinDetails over the mux → the seam's joinDetailsReady form
    // descriptor (honoring the node's supported/schema fields).
    void joinDetailsPopulatesOverMux() {
        const QString sock = m_tmp.filePath(QStringLiteral("jd.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(joinDetailsResponse());
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("jd.db")));
        TransportRepository repo(&tx.client, &cache);
        DaemonTransportRegistry registry(&repo);

        QSignalSpy ready(&registry, &transports::ITransportRegistry::joinDetailsReady);
        registry.conversationJoinDetails(QStringLiteral("matrix/@bot:hs"));
        QTRY_COMPARE_WITH_TIMEOUT(ready.count(), 1, 3000);
        const QVariantMap form = ready.at(0).at(1).toMap();
        QVERIFY(form.value(QStringLiteral("nicknameSupported")).toBool());
        QVERIFY(form.value(QStringLiteral("passwordSupported")).toBool());
        QCOMPARE(form.value(QStringLiteral("nameMaxLength")).toInt(), 64);
        QCOMPARE(form.value(QStringLiteral("extras")).toList().size(), 1);
    }

    // [acct-mgmt] conversationMembers (ConvGet) over the mux → membersChanged; then a MemberSetRole
    // Ok re-issues ConvGet (the palette re-hydrates) — the continuation is the codec's own bytes.
    void membersAndMemberOpOverMux() {
        const QString sock = m_tmp.filePath(QStringLiteral("mem.sock"));
        const QString t = QStringLiteral("matrix/@bot:hs");
        const QString c = QStringLiteral("!room:hs");
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(convGetResponse());
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("mem.db")));
        TransportRepository repo(&tx.client, &cache);
        DaemonTransportRegistry registry(&repo);

        QSignalSpy members(&registry, &transports::ITransportRegistry::membersChanged);
        registry.conversationMembers(t, c);
        QTRY_COMPARE_WITH_TIMEOUT(members.count(), 1, 3000);
        const QVariantList rows = members.at(0).at(2).toList();
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows.at(0).toMap().value(QStringLiteral("role")).toString(), QStringLiteral("Op"));

        // A member op replies Ok, then the repo re-issues ConvGet (a second membersChanged).
        fake.setReplySequence({okResponse(), convGetResponse()});
        registry.memberSetRole(t, c, QStringLiteral("@bob:matrix.org"), QStringLiteral("Voice"));
        QTRY_COMPARE_WITH_TIMEOUT(members.count(), 2, 3000);
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 3); // ConvGet, MemberSetRole, ConvGet
        QCOMPARE(calls.at(1),
                 NodeApiCodec::encodeMemberSetRoleRequest(t, c, QStringLiteral("@bob:matrix.org"),
                                                          QStringLiteral("Voice")));
        QCOMPARE(calls.at(2), NodeApiCodec::encodeConvGetRequest(t, c));
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

    // [wave2:app-channels-liveness] B5: applyTransportChanged patches a cached row's
    // connection/presence IN PLACE, preserving family/displayName/boundProfile, and emits
    // instancesRefreshed. An unknown transport does NOT invent a partial row (it refetches).
    void applyTransportChangedPatchesInPlace() {
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("apply.db")));
        QVERIFY(cache.isOpen());
        CachedTransportInstanceRow row;
        row.transport = QStringLiteral("matrix/@bot:hs");
        row.family = QStringLiteral("matrix");
        row.displayName = QStringLiteral("@bot:hs");
        row.connection = QStringLiteral("offline");
        row.presence = QStringLiteral("unknown");
        row.boundProfile = QStringLiteral("p1");
        row.updatedAtMs = 1;
        QVERIFY(cache.upsertTransportInstance(row));

        DaemonTransport transport; // unconnected
        NodeApiClient client(&transport);
        TransportRepository repo(&client, &cache);
        QSignalSpy refreshed(&repo, &TransportRepository::instancesRefreshed);

        repo.applyTransportChanged(QStringLiteral("matrix/@bot:hs"), QStringLiteral("connected"),
                                   QStringLiteral("available"), true, QString(), false, QString(),
                                   false, false);
        QCOMPARE(refreshed.count(), 1);
        const auto rows = cache.transportInstances();
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows.at(0).connection, QStringLiteral("connected"));
        QCOMPARE(rows.at(0).presence, QStringLiteral("available"));
        QCOMPARE(rows.at(0).family, QStringLiteral("matrix"));
        QCOMPARE(rows.at(0).boundProfile, QStringLiteral("p1"));

        // Absent presence leaves the previous presence untouched (patches connection only).
        // [waveB:app-v30] D1: a fatal error carries a reason + verbatim message.
        repo.applyTransportChanged(QStringLiteral("matrix/@bot:hs"), QStringLiteral("error"),
                                   QString(), false, QStringLiteral("authentication_failed"), true,
                                   QStringLiteral("token expired"), true, true);
        QCOMPARE(cache.transportInstances().at(0).connection, QStringLiteral("error"));
        QCOMPARE(cache.transportInstances().at(0).presence, QStringLiteral("available"));
        QCOMPARE(cache.transportInstances().at(0).connectionReason,
                 QStringLiteral("authentication_failed"));
        QCOMPARE(cache.transportInstances().at(0).connectionMessage,
                 QStringLiteral("token expired"));
        QVERIFY(cache.transportInstances().at(0).fatal);

        // A subsequent non-error transition clears reason/message and fatal.
        repo.applyTransportChanged(QStringLiteral("matrix/@bot:hs"), QStringLiteral("connected"),
                                   QString(), false, QString(), false, QString(), false, false);
        QVERIFY(cache.transportInstances().at(0).connectionReason.isEmpty());
        QVERIFY(cache.transportInstances().at(0).connectionMessage.isEmpty());
        QVERIFY(!cache.transportInstances().at(0).fatal);

        // Unknown transport: no partial row invented (the fallback is a refetch).
        repo.applyTransportChanged(QStringLiteral("nope"), QStringLiteral("connected"), QString(),
                                   false, QString(), false, QString(), false, false);
        QCOMPARE(cache.transportInstances().size(), 1);
    }

    // [acct-mgmt] The Mock registry carries canned rooms/members so the room-lifecycle UI works
    // offline (UI-first): it reports an account with rooms, ConvGet-style members, and the verbs
    // mutate the canned state + emit the same change signals the daemon registry does.
    void mockRegistryOfflineRoomFlows() {
        transports::MockTransportRegistry mock;
        const QVariantList accounts = mock.instances();
        QCOMPARE(accounts.size(), 1);
        const QString t = accounts.at(0).toMap().value(QStringLiteral("transport")).toString();
        QVERIFY(!t.isEmpty());
        QVERIFY(!mock.conversations(t).isEmpty());
        const QString conv =
            mock.conversations(t).at(0).toMap().value(QStringLiteral("id")).toString();

        // conversationMembers → membersChanged with the canned roster.
        QSignalSpy members(&mock, &transports::ITransportRegistry::membersChanged);
        mock.conversationMembers(t, conv);
        QCOMPARE(members.count(), 1);
        QVERIFY(!members.at(0).at(2).toList().isEmpty());

        // Join adds a room → conversationsChanged; the new room is present.
        QSignalSpy convs(&mock, &transports::ITransportRegistry::conversationsChanged);
        mock.joinRoom(t, QVariantMap{{QStringLiteral("name"), QStringLiteral("newbie")}});
        QCOMPARE(convs.count(), 1);
        bool found = false;
        for (const QVariant& rv : mock.conversations(t)) {
            if (rv.toMap().value(QStringLiteral("title")).toString() == QLatin1String("newbie")) {
                found = true;
            }
        }
        QVERIFY(found);

        // setRole mutates the member in place.
        mock.memberSetRole(t, conv,
                           members.at(0)
                               .at(2)
                               .toList()
                               .at(0)
                               .toMap()
                               .value(QStringLiteral("contactId"))
                               .toString(),
                           QStringLiteral("Voice"));
        QVERIFY(members.count() >= 2);
    }

    // [waveB:app-v30] D1: the two teardown intents encode to the right request arms.
    void disconnectRemoveEncodeDistinctIntents() {
        const QByteArray disc =
            NodeApiCodec::encodeTransportDisconnectRequest(QStringLiteral("matrix/@bot:hs"));
        const QByteArray rem =
            NodeApiCodec::encodeTransportRemoveRequest(QStringLiteral("matrix/@bot:hs"));
        QVERIFY(!disc.isEmpty());
        QVERIFY(!rem.isEmpty());
        QVERIFY(disc != rem);
    }

    // [wave2:app-channels-liveness] B2: the first refresh establishes the baseline (no badges); a
    // room that appears on a later refresh is badged "new" until marked seen.
    void newConversationBadgeTracksLaterArrivals() {
        const QString sock = m_tmp.filePath(QStringLiteral("newconv.sock"));
        const QString t = QStringLiteral("matrix/@bot:hs");
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("newconv.db")));
        TransportRepository repo(&tx.client, &cache);
        QSignalSpy convs(&repo, &TransportRepository::conversationsRefreshed);

        // First refresh: one room -> baseline, nothing badged.
        fake.setReplyPayload(conversationsPageResponse(QByteArrayLiteral("!a:hs")));
        repo.refreshConversations(t);
        QTRY_COMPARE_WITH_TIMEOUT(convs.count(), 1, 3000);
        QVERIFY(!repo.isNewConversation(t, QStringLiteral("!a:hs")));

        // Second refresh: a new room arrives -> badged new; the pre-existing one is not.
        fake.setReplySequence(
            {conversationsPageResponse(QByteArrayLiteral("!a:hs"), QByteArrayLiteral("!a:hs")),
             conversationsPageResponse(QByteArrayLiteral("!b:hs"))});
        repo.refreshConversations(t);
        QTRY_COMPARE_WITH_TIMEOUT(convs.count(), 2, 3000);
        QVERIFY(repo.isNewConversation(t, QStringLiteral("!b:hs")));
        QVERIFY(!repo.isNewConversation(t, QStringLiteral("!a:hs")));

        // Viewing clears the badge.
        repo.markConversationSeen(t, QStringLiteral("!b:hs"));
        QVERIFY(!repo.isNewConversation(t, QStringLiteral("!b:hs")));
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
