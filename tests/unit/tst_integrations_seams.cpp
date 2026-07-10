// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [integrations wire v38] Work package A1 (app/core-seams): the non-UI foundation the later app
// packages (tree UI, auth component, chat tabs) consume. Covers the v38 codec wrappers
// (enriched auth-param-field, AuthFlowKind::UserPassword, ConversationInfo.parent + Space,
// AdapterInfo.account_schema, ConvSend/ConvHistory, PersonList, TransportSettings/Configure,
// MessagesChanged/PersonsChanged events), the daemon repositories + services + mocks, the
// SubscriptionManager fan-out, the IAuthFlowService DTO plumbing for the enriched fields (incl. the
// mock's full challenge matrix), and the conversation parent/Space cache round-trip — all against
// the vendored zcbor codec + the WireMuxServer fixture.

#include "auth/iauth_flow_service.h"
#include "auth/mock_auth_flow_service.h"
#include "daemon/daemon_auth_flow_service.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_chat_service.h"
#include "daemon/daemon_persons_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/daemon_transport_registry.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "daemon/subscription_manager.h"
#include "daemon_api_client_decode.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "transports/ichat_service.h"
#include "transports/ipersons_service.h"
#include "transports/mock_chat_service.h"
#include "transports/mock_persons_service.h"
#include "wire_mux_fixture.h"

#include <memory>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::ApprovalRepository;
using daemonapp::daemon::AuthRepository;
using daemonapp::daemon::ChatRepository;
using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::DecodedAdapterInfo;
using daemonapp::daemon::DecodedAuthBeginResponse;
using daemonapp::daemon::DecodedAuthProviderInfo;
using daemonapp::daemon::DecodedChatMessage;
using daemonapp::daemon::DecodedConversation;
using daemonapp::daemon::DecodedEventsPage;
using daemonapp::daemon::DecodedNodeEvent;
using daemonapp::daemon::DecodedPerson;
using daemonapp::daemon::ModelRepository;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;
using daemonapp::daemon::PersonsRepository;
using daemonapp::daemon::SessionRepository;
using daemonapp::daemon::SubscriptionManager;
using daemonapp::daemon::TransportRepository;
using daemonapp::test::WireMuxServer;

namespace {

void setZ(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

QString zstr(const zcbor_string& z) {
    return QString::fromUtf8(reinterpret_cast<const char*>(z.value), static_cast<qsizetype>(z.len));
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

// ApiResponse::Ok (the bare externally-tagged unit variant serializes as "Ok").
QByteArray okResponse() {
    QByteArray b;
    daemonapp::test::cborText(b, "Ok");
    return b;
}

// Fill an enriched auth-param-field: kind=Password, a default + placeholder, and two choices, so
// the decode proves every v38 optional lands. `key`/`label` are borrowed (keep them alive).
void putEnrichedField(auth_param_field& f, const QByteArray& key, const QByteArray& label,
                      const QByteArray& def, const QByteArray& ph, const QByteArray& c0,
                      const QByteArray& c1) {
    setZ(f.auth_param_field_key, key);
    setZ(f.auth_param_field_label, label);
    f.auth_param_field_required = true;
    f.auth_param_field_kind_present = true;
    f.auth_param_field_kind.auth_param_field_kind.auth_field_kind_choice =
        auth_field_kind_r::auth_field_kind_Password_tstr_c;
    f.auth_param_field_default_present = true;
    f.auth_param_field_default.auth_param_field_default_choice =
        auth_param_field_default_r::auth_param_field_default_tstr_c;
    setZ(f.auth_param_field_default.auth_param_field_default_tstr, def);
    f.auth_param_field_placeholder_present = true;
    f.auth_param_field_placeholder.auth_param_field_placeholder_choice =
        auth_param_field_placeholder_r::auth_param_field_placeholder_tstr_c;
    setZ(f.auth_param_field_placeholder.auth_param_field_placeholder_tstr, ph);
    f.auth_param_field_choices_present = true;
    f.auth_param_field_choices.auth_param_field_choices_tstr_count = 2;
    setZ(f.auth_param_field_choices.auth_param_field_choices_tstr[0], c0);
    setZ(f.auth_param_field_choices.auth_param_field_choices_tstr[1], c1);
}

// {"AuthProviders":[ AuthProviderInfo{ family, flow_kind=UserPassword, one enriched field } ]}.
QByteArray authProvidersEnriched() {
    static const QByteArray fam = QByteArrayLiteral("demo");
    static const QByteArray disp = QByteArrayLiteral("Demo");
    static const QByteArray key = QByteArrayLiteral("password");
    static const QByteArray label = QByteArrayLiteral("Password");
    static const QByteArray def = QByteArrayLiteral("hunter2");
    static const QByteArray ph = QByteArrayLiteral("your password");
    static const QByteArray c0 = QByteArrayLiteral("a");
    static const QByteArray c1 = QByteArrayLiteral("b");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_auth_providers_m_c;
    response_auth_providers& p = resp->api_response_response_auth_providers_m;
    p.response_auth_providers_AuthProviders_auth_provider_info_m_count = 1;
    auth_provider_info& info = p.response_auth_providers_AuthProviders_auth_provider_info_m[0];
    setZ(info.auth_provider_info_family, fam);
    info.auth_provider_info_flow_kind.auth_flow_kind_choice =
        auth_flow_kind_r::auth_flow_kind_UserPassword_tstr_c;
    setZ(info.auth_provider_info_display_name, disp);
    info.auth_provider_info_params_schema_auth_param_field_m_count = 1;
    putEnrichedField(info.auth_provider_info_params_schema_auth_param_field_m[0], key, label, def,
                     ph, c0, c1);
    return encodeResponse(*resp);
}

// {"AuthBegun": AuthBeginResponse{ flow, Form challenge with one enriched field, expires }}.
QByteArray authBegunFormEnriched() {
    static const QByteArray flow = QByteArrayLiteral("flow-1");
    static const QByteArray title = QByteArrayLiteral("Sign in");
    static const QByteArray key = QByteArrayLiteral("password");
    static const QByteArray label = QByteArrayLiteral("Password");
    static const QByteArray def = QByteArrayLiteral("hunter2");
    static const QByteArray ph = QByteArrayLiteral("your password");
    static const QByteArray c0 = QByteArrayLiteral("a");
    static const QByteArray c1 = QByteArrayLiteral("b");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_auth_begun_m_c;
    auth_begin_response& b = resp->api_response_response_auth_begun_m.response_auth_begun_AuthBegun;
    setZ(b.auth_begin_response_flow_id, flow);
    b.auth_begin_response_challenge.auth_challenge_choice =
        auth_challenge_r::auth_challenge_form_m_c;
    auth_challenge_form& form = b.auth_begin_response_challenge.auth_challenge_form_m;
    setZ(form.Form_title, title);
    form.Form_fields_auth_param_field_m_count = 1;
    putEnrichedField(form.Form_fields_auth_param_field_m[0], key, label, def, ph, c0, c1);
    b.auth_begin_response_expires_at = 9999;
    return encodeResponse(*resp);
}

// {"Adapters":[ AdapterInfo{ demo + an account_schema with one enriched field } ]}.
QByteArray adaptersWithAccountSchema() {
    static const QByteArray fam = QByteArrayLiteral("demo");
    static const QByteArray disp = QByteArrayLiteral("Demo");
    static const QByteArray key = QByteArrayLiteral("server_password");
    static const QByteArray label = QByteArrayLiteral("Server password");
    static const QByteArray def = QByteArrayLiteral("");
    static const QByteArray ph = QByteArrayLiteral("optional");
    static const QByteArray c0 = QByteArrayLiteral("a");
    static const QByteArray c1 = QByteArrayLiteral("b");
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
    a.adapter_info_account_schema_present = true;
    account_settings_schema& s = a.adapter_info_account_schema.adapter_info_account_schema;
    s.account_settings_schema_fields_present = true;
    s.account_settings_schema_fields.account_settings_schema_fields_auth_param_field_m_count = 1;
    putEnrichedField(
        s.account_settings_schema_fields.account_settings_schema_fields_auth_param_field_m[0], key,
        label, def, ph, c0, c1);
    return encodeResponse(*resp);
}

// {"Conversations": conv-page{ [ Space "!space", child "!room" parent="!space" ], next: null }}.
QByteArray conversationsWithSpaceAndParent() {
    static const QByteArray t = QByteArrayLiteral("demo/acct");
    static const QByteArray spaceId = QByteArrayLiteral("!space:demo");
    static const QByteArray roomId = QByteArrayLiteral("!room:demo");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_conversations_m_c;
    conv_page& rc =
        resp->api_response_response_conversations_m.response_conversations_Conversations;
    rc.conv_page_items_conversation_info_m_count = 2;
    conversation_info& space = rc.conv_page_items_conversation_info_m[0];
    setZ(space.conversation_info_transport, t);
    setZ(space.conversation_info_id, spaceId);
    space.conversation_info_kind.conversation_type_choice =
        conversation_type_r::conversation_type_Space_tstr_c;
    space.conversation_info_title_present = false;
    space.conversation_info_topic_present = false;
    space.conversation_info_description_present = false;
    space.conversation_info_members_present = false;
    space.conversation_info_parent_present = false;
    conversation_info& room = rc.conv_page_items_conversation_info_m[1];
    setZ(room.conversation_info_transport, t);
    setZ(room.conversation_info_id, roomId);
    room.conversation_info_kind.conversation_type_choice =
        conversation_type_r::conversation_type_Channel_tstr_c;
    room.conversation_info_title_present = false;
    room.conversation_info_topic_present = false;
    room.conversation_info_description_present = false;
    room.conversation_info_members_present = false;
    room.conversation_info_parent_present = true;
    room.conversation_info_parent.conversation_info_parent_choice =
        conversation_info_parent_r::conversation_info_parent_tstr_c;
    setZ(room.conversation_info_parent.conversation_info_parent_tstr, spaceId);
    rc.conv_page_next_present = false;
    return encodeResponse(*resp);
}

// {"Persons":[ Person{ id, alias, endpoints:[ demo/acct -> @bob ] } ]}.
QByteArray personsResponse() {
    static const QByteArray pid = QByteArrayLiteral("person-1");
    static const QByteArray alias = QByteArrayLiteral("Bob (all accounts)");
    static const QByteArray t = QByteArrayLiteral("demo/acct");
    static const QByteArray cid = QByteArrayLiteral("@bob:demo");
    static const QByteArray cname = QByteArrayLiteral("Bob");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_persons_m_c;
    response_persons& rp = resp->api_response_response_persons_m;
    rp.response_persons_Persons_person_m_count = 1;
    person& p = rp.response_persons_Persons_person_m[0];
    setZ(p.person_id, pid);
    p.person_alias_present = true;
    p.person_alias.person_alias_choice = person_alias_r::person_alias_tstr_c;
    setZ(p.person_alias.person_alias_tstr, alias);
    p.person_avatar_present = false;
    p.person_endpoints_present = true;
    p.person_endpoints.person_endpoints_person_endpoint_m_count = 1;
    person_endpoint& ep = p.person_endpoints.person_endpoints_person_endpoint_m[0];
    setZ(ep.person_endpoint_transport, t);
    setZ(ep.person_endpoint_contact.contact_info_id, cid);
    ep.person_endpoint_contact.contact_info_display_name_present = true;
    ep.person_endpoint_contact.contact_info_display_name.contact_info_display_name_choice =
        contact_info_display_name_r::contact_info_display_name_tstr_c;
    setZ(ep.person_endpoint_contact.contact_info_display_name.contact_info_display_name_tstr,
         cname);
    ep.person_endpoint_contact.contact_info_presence_present = false;
    ep.person_endpoint_contact.contact_info_permission_present = false;
    return encodeResponse(*resp);
}

// Fill one Chat journal record: cursor + a chat-message {id, author=@bob, text, timestamp, system}.
void putChatRecord(journal_record& rec, quint64 cursor, const QByteArray& msgId,
                   const QByteArray& authorId, const QByteArray& authorName, const QByteArray& text,
                   quint64 ts) {
    rec.journal_record_cursor = cursor;
    rec.journal_record_segment = 0;
    rec.journal_record_seq = cursor;
    rec.journal_record_epoch = 1;
    rec.journal_record_trace = 0;
    static const QByteArray kind = QByteArrayLiteral("Chat");
    setZ(rec.journal_record_kind, kind);
    rec.journal_record_timestamp_ms = ts;
    rec.journal_record_verified = true;
    rec.journal_record_payload.journal_record_payload_t_choice =
        journal_record_payload_t_r::journal_record_payload_t_journal_record_payload_chat_m_c;
    chat_message& m = rec.journal_record_payload
                          .journal_record_payload_t_journal_record_payload_chat_m.Chat_message;
    m.chat_message_id_present = true;
    m.chat_message_id.chat_message_id_choice = chat_message_id_r::chat_message_id_tstr_c;
    setZ(m.chat_message_id.chat_message_id_tstr, msgId);
    m.chat_message_author_present = true;
    m.chat_message_author.chat_message_author_choice =
        chat_message_author_r::chat_message_author_participant_m_c;
    participant_r& who = m.chat_message_author.chat_message_author_participant_m;
    who.participant_choice = participant_r::participant_contact_m_c;
    contact_info& c = who.participant_contact_m.participant_contact_Contact;
    setZ(c.contact_info_id, authorId);
    c.contact_info_display_name_present = true;
    c.contact_info_display_name.contact_info_display_name_choice =
        contact_info_display_name_r::contact_info_display_name_tstr_c;
    setZ(c.contact_info_display_name.contact_info_display_name_tstr, authorName);
    c.contact_info_presence_present = false;
    c.contact_info_permission_present = false;
    m.chat_message_replying_to_present = false;
    setZ(m.chat_message_text, text);
    m.chat_message_attachments_present = false;
    m.chat_message_timestamp_present = true;
    m.chat_message_timestamp.chat_message_timestamp_choice =
        chat_message_timestamp_r::chat_message_timestamp_uint64_m_c;
    m.chat_message_timestamp.chat_message_timestamp_uint64_m = ts;
    m.chat_message_delivered_at_present = false;
    m.chat_message_edited_at_present = false;
    m.chat_message_error_present = false;
    m.chat_message_title_present = false;
    m.chat_message_highlight_color_present = false;
    m.chat_message_action_present = false;
    m.chat_message_event_present = false;
    m.chat_message_notice_present = false;
    m.chat_message_system_present = false;
    m.chat_message_highlighted_present = false;
}

// {"Journal": JournalPageView{ [chat@10 "hi", chat@11 "yo"], next=11, head=11, sealed=null }} —
// the ConvHistory reply shape (a per-conversation Chat journal).
QByteArray convHistoryResponse() {
    static const QByteArray m0 = QByteArrayLiteral("m0");
    static const QByteArray m1 = QByteArrayLiteral("m1");
    static const QByteArray bob = QByteArrayLiteral("@bob:demo");
    static const QByteArray bobName = QByteArrayLiteral("Bob");
    static const QByteArray t0 = QByteArrayLiteral("hi");
    static const QByteArray t1 = QByteArrayLiteral("yo");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_journal_m_c;
    journal_page_view& page = resp->api_response_response_journal_m.response_journal_Journal;
    page.journal_page_view_entries_journal_record_m_count = 2;
    putChatRecord(page.journal_page_view_entries_journal_record_m[0], 10, m0, bob, bobName, t0,
                  1000);
    putChatRecord(page.journal_page_view_entries_journal_record_m[1], 11, m1, bob, bobName, t1,
                  2000);
    page.journal_page_view_next_cursor = 11;
    page.journal_page_view_head_cursor = 11;
    page.journal_page_view_sealed_after_choice =
        journal_page_view::journal_page_view_sealed_after_null_m_c;
    return encodeResponse(*resp);
}

// {"TransportSettings": account-settings-values{ homeserver -> https://demo, room_limit -> 50 }}.
QByteArray transportSettingsResponse() {
    static const QByteArray k0 = QByteArrayLiteral("homeserver");
    static const QByteArray v0 = QByteArrayLiteral("https://demo");
    static const QByteArray k1 = QByteArrayLiteral("room_limit");
    static const QByteArray v1 = QByteArrayLiteral("50");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_transport_settings_m_c;
    account_settings_values& vals = resp->api_response_response_transport_settings_m
                                        .response_transport_settings_TransportSettings;
    vals.account_settings_values_values_present = true;
    vals.account_settings_values_values.values_tstrtstr_count = 2;
    values_tstrtstr& kv0 = vals.account_settings_values_values.values_tstrtstr[0];
    setZ(kv0.account_settings_values_values_tstrtstr_key, k0);
    setZ(kv0.values_tstrtstr, v0);
    values_tstrtstr& kv1 = vals.account_settings_values_values.values_tstrtstr[1];
    setZ(kv1.account_settings_values_values_tstrtstr_key, k1);
    setZ(kv1.values_tstrtstr, v1);
    return encodeResponse(*resp);
}

// {"MessagesChanged":{"transport":t,"conv":c}} — the granular transcript-grew feed pointer.
QByteArray neMessagesChanged(const char* transport, const char* conv) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "MessagesChanged");
    b.append(static_cast<char>(0xA2));
    daemonapp::test::cborText(b, "transport");
    daemonapp::test::cborText(b, transport);
    daemonapp::test::cborText(b, "conv");
    daemonapp::test::cborText(b, conv);
    return b;
}

// "PersonsChanged" — the payload-free person-registry invalidation pointer (a bare tstr variant).
QByteArray nePersonsChanged() {
    QByteArray b;
    daemonapp::test::cborText(b, "PersonsChanged");
    return b;
}

// Decode a captured request payload into api_request_r (assert what an encoder produced).
bool decodeReq(const QByteArray& cbor, api_request_r* out) {
    return cbor_decode_api_request(reinterpret_cast<const uint8_t*>(cbor.constData()),
                                   static_cast<size_t>(cbor.size()), out, nullptr) == ZCBOR_SUCCESS;
}

} // namespace

class TestIntegrationsSeams : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;
    [[nodiscard]] QString sock(const QString& n) { return m_tmp.filePath(n); }
    [[nodiscard]] QString db(const QString& n) { return m_tmp.filePath(n); }

private slots:
    // ----- codec: enriched auth-param-field + UserPassword flow kind -----
    void authParamFieldEnrichmentRoundTrip() {
        QList<DecodedAuthProviderInfo> out;
        QVERIFY(NodeApiCodec::decodeAuthProviders(authProvidersEnriched(), &out));
        QCOMPARE(out.size(), 1);
        // wire v38: UserPassword must decode as itself (not silently fold to MatrixSso).
        QCOMPARE(out.at(0).flowKind, QStringLiteral("UserPassword"));
        QCOMPARE(out.at(0).paramsSchema.size(), 1);
        const auto& f = out.at(0).paramsSchema.at(0);
        QCOMPARE(f.key, QStringLiteral("password"));
        QCOMPARE(f.kind, QStringLiteral("Password"));
        QVERIFY(f.hasDefault);
        QCOMPARE(f.defaultValue, QStringLiteral("hunter2"));
        QVERIFY(f.hasPlaceholder);
        QCOMPARE(f.placeholder, QStringLiteral("your password"));
        QCOMPARE(f.choices, QStringList({QStringLiteral("a"), QStringLiteral("b")}));
    }

    // The enriched metadata also lands on a Form challenge's fields (the auth component renders
    // it).
    void authChallengeFormEnrichmentRoundTrip() {
        DecodedAuthBeginResponse out;
        QVERIFY(NodeApiCodec::decodeAuthBegun(authBegunFormEnriched(), &out));
        QCOMPARE(out.challenge.formFields.size(), 1);
        const auto& f = out.challenge.formFields.at(0);
        QCOMPARE(f.kind, QStringLiteral("Password"));
        QVERIFY(f.hasDefault);
        QCOMPARE(f.defaultValue, QStringLiteral("hunter2"));
        QVERIFY(f.hasPlaceholder);
        QCOMPARE(f.choices.size(), 2);
    }

    // The adapter's account_schema (wire v38) decodes with the enriched field metadata.
    void adapterAccountSchemaRoundTrip() {
        QList<DecodedAdapterInfo> out;
        QVERIFY(NodeApiCodec::decodeAdapters(adaptersWithAccountSchema(), &out));
        QCOMPARE(out.size(), 1);
        QCOMPARE(out.at(0).accountSchema.size(), 1);
        const auto& f = out.at(0).accountSchema.at(0);
        QCOMPARE(f.key, QStringLiteral("server_password"));
        QCOMPARE(f.kind, QStringLiteral("Password"));
        QVERIFY(f.hasPlaceholder);
        QCOMPARE(f.placeholder, QStringLiteral("optional"));
    }

    // ----- codec: ConversationInfo.parent + ConversationType::Space -----
    void conversationParentAndSpaceRoundTrip() {
        QList<DecodedConversation> out;
        QVERIFY(NodeApiCodec::decodeConversations(conversationsWithSpaceAndParent(), &out));
        QCOMPARE(out.size(), 2);
        QCOMPARE(out.at(0).kind, QStringLiteral("space"));
        QVERIFY(!out.at(0).hasParent);
        QCOMPARE(out.at(1).kind, QStringLiteral("channel"));
        QVERIFY(out.at(1).hasParent);
        QCOMPARE(out.at(1).parent, QStringLiteral("!space:demo"));
    }

    // ----- codec: ConvSend / ConvHistory encode + decode -----
    void convSendEncodeRoundTrip() {
        const QByteArray req = NodeApiCodec::encodeConvSendRequest(
            QStringLiteral("demo/acct"), QStringLiteral("!room:demo"), QStringLiteral("hi"));
        QVERIFY(!req.isEmpty());
        auto decoded = std::make_unique<api_request_r>();
        QVERIFY(decodeReq(req, decoded.get()));
        QCOMPARE(decoded->api_request_choice, api_request_r::api_request_request_conv_send_m_c);
        const conv_send_args& args =
            decoded->api_request_request_conv_send_m.request_conv_send_ConvSend;
        QCOMPARE(zstr(args.conv_send_args_transport), QStringLiteral("demo/acct"));
        QCOMPARE(zstr(args.conv_send_args_conv), QStringLiteral("!room:demo"));
        QCOMPARE(zstr(args.conv_send_args_message.user_msg_text), QStringLiteral("hi"));
    }

    void convHistoryEncodeRoundTrip() {
        const QByteArray req = NodeApiCodec::encodeConvHistoryRequest(
            QStringLiteral("demo/acct"), QStringLiteral("!room:demo"), true, 7, true, 20);
        QVERIFY(!req.isEmpty());
        auto decoded = std::make_unique<api_request_r>();
        QVERIFY(decodeReq(req, decoded.get()));
        QCOMPARE(decoded->api_request_choice, api_request_r::api_request_request_conv_history_m_c);
        const conv_history_args& args =
            decoded->api_request_request_conv_history_m.request_conv_history_ConvHistory;
        QCOMPARE(zstr(args.conv_history_args_conv), QStringLiteral("!room:demo"));
        QVERIFY(args.conv_history_args_after_cursor_present);
        QCOMPARE(quint64(args.conv_history_args_after_cursor.conv_history_args_after_cursor),
                 quint64(7));
        QVERIFY(args.conv_history_args_max_present);
        QCOMPARE(quint32(args.conv_history_args_max.conv_history_args_max), quint32(20));
    }

    void convHistoryDecodeRoundTrip() {
        QList<DecodedChatMessage> msgs;
        quint64 next = 0;
        quint64 head = 0;
        QVERIFY(NodeApiCodec::decodeConvHistory(convHistoryResponse(), &msgs, &next, &head));
        QCOMPARE(msgs.size(), 2);
        QCOMPARE(msgs.at(0).cursor, quint64(10));
        QCOMPARE(msgs.at(0).id, QStringLiteral("m0"));
        QCOMPARE(msgs.at(0).authorId, QStringLiteral("@bob:demo"));
        QCOMPARE(msgs.at(0).authorName, QStringLiteral("Bob"));
        QCOMPARE(msgs.at(0).text, QStringLiteral("hi"));
        QVERIFY(msgs.at(0).hasTimestamp);
        QCOMPARE(msgs.at(0).timestamp, quint64(1000));
        QCOMPARE(msgs.at(1).text, QStringLiteral("yo"));
        QCOMPARE(next, quint64(11));
        QCOMPARE(head, quint64(11));
    }

    // ----- codec: PersonList -----
    void personListEncodeDecodeRoundTrip() {
        const QByteArray req = NodeApiCodec::encodePersonListRequest();
        QVERIFY(!req.isEmpty());
        auto decoded = std::make_unique<api_request_r>();
        QVERIFY(decodeReq(req, decoded.get()));
        QCOMPARE(decoded->api_request_choice, api_request_r::api_request_request_person_list_m_c);

        QList<DecodedPerson> out;
        QVERIFY(NodeApiCodec::decodePersons(personsResponse(), &out));
        QCOMPARE(out.size(), 1);
        QCOMPARE(out.at(0).id, QStringLiteral("person-1"));
        QVERIFY(out.at(0).hasAlias);
        QCOMPARE(out.at(0).alias, QStringLiteral("Bob (all accounts)"));
        QCOMPARE(out.at(0).endpoints.size(), 1);
        QCOMPARE(out.at(0).endpoints.at(0).transport, QStringLiteral("demo/acct"));
        QCOMPARE(out.at(0).endpoints.at(0).contact.id, QStringLiteral("@bob:demo"));
        QCOMPARE(out.at(0).endpoints.at(0).contact.displayName, QStringLiteral("Bob"));
    }

    // ----- codec: TransportSettings read + TransportConfigure -----
    void transportSettingsDecodeRoundTrip() {
        const QByteArray req =
            NodeApiCodec::encodeTransportSettingsRequest(QStringLiteral("demo/acct"));
        QVERIFY(!req.isEmpty());
        auto decoded = std::make_unique<api_request_r>();
        QVERIFY(decodeReq(req, decoded.get()));
        QCOMPARE(decoded->api_request_choice,
                 api_request_r::api_request_request_transport_settings_m_c);

        QMap<QString, QString> values;
        QVERIFY(NodeApiCodec::decodeTransportSettings(transportSettingsResponse(), &values));
        QCOMPARE(values.value(QStringLiteral("homeserver")), QStringLiteral("https://demo"));
        QCOMPARE(values.value(QStringLiteral("room_limit")), QStringLiteral("50"));
    }

    void transportConfigureEncodeRoundTrip() {
        QMap<QString, QString> settings;
        settings.insert(QStringLiteral("homeserver"), QStringLiteral("https://new"));
        const QByteArray req =
            NodeApiCodec::encodeTransportConfigureRequest(QStringLiteral("demo/acct"), settings);
        QVERIFY(!req.isEmpty());
        auto decoded = std::make_unique<api_request_r>();
        QVERIFY(decodeReq(req, decoded.get()));
        QCOMPARE(decoded->api_request_choice,
                 api_request_r::api_request_request_transport_configure_m_c);
        const request_transport_configure& c = decoded->api_request_request_transport_configure_m;
        QCOMPARE(zstr(c.TransportConfigure_transport), QStringLiteral("demo/acct"));
        QVERIFY(c.TransportConfigure_settings.account_settings_values_values_present);
        QCOMPARE(
            int(c.TransportConfigure_settings.account_settings_values_values.values_tstrtstr_count),
            1);
    }

    // ----- codec: MessagesChanged / PersonsChanged events -----
    void messagesChangedEventDecode() {
        DecodedEventsPage page;
        QVERIFY(NodeApiCodec::decodeEventsPage(
            daemonapp::test::buildEventsPage({neMessagesChanged("demo/acct", "!room:demo")}, 3, 3),
            &page));
        QCOMPARE(page.events.size(), 1);
        QCOMPARE(page.events.at(0).kind, DecodedNodeEvent::Kind::MessagesChanged);
        QCOMPARE(page.events.at(0).transport, QStringLiteral("demo/acct"));
        QCOMPARE(page.events.at(0).conv, QStringLiteral("!room:demo"));
    }

    void personsChangedEventDecode() {
        DecodedEventsPage page;
        QVERIFY(NodeApiCodec::decodeEventsPage(
            daemonapp::test::buildEventsPage({nePersonsChanged()}, 4, 4), &page));
        QCOMPARE(page.events.size(), 1);
        QCOMPARE(page.events.at(0).kind, DecodedNodeEvent::Kind::PersonsChanged);
    }

    // ----- ChatRepository: history + send over the mux -----
    void chatRepositoryHistory() {
        WireMuxServer fake;
        QVERIFY2(fake.start(sock(QStringLiteral("chat_hist.sock"))), "listen");
        fake.setReplyPayload(convHistoryResponse());
        DaemonTransport tx;
        tx.setSocketPath(sock(QStringLiteral("chat_hist.sock")));
        NodeApiClient client(&tx);
        DaemonCacheStore cache(db(QStringLiteral("chat_hist.db")));
        ChatRepository repo(&client, &cache);
        QSignalSpy refreshed(&repo, &ChatRepository::historyRefreshed);
        repo.refreshHistory(QStringLiteral("demo/acct"), QStringLiteral("!room:demo"));
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        const QList<DecodedChatMessage> msgs =
            repo.messages(QStringLiteral("demo/acct"), QStringLiteral("!room:demo"));
        QCOMPARE(msgs.size(), 2);
        QCOMPARE(msgs.at(0).text, QStringLiteral("hi"));
        QCOMPARE(msgs.at(1).text, QStringLiteral("yo"));
    }

    void chatRepositorySend() {
        WireMuxServer fake;
        QVERIFY2(fake.start(sock(QStringLiteral("chat_send.sock"))), "listen");
        fake.setReplyPayload(okResponse());
        DaemonTransport tx;
        tx.setSocketPath(sock(QStringLiteral("chat_send.sock")));
        NodeApiClient client(&tx);
        DaemonCacheStore cache(db(QStringLiteral("chat_send.db")));
        ChatRepository repo(&client, &cache);
        QSignalSpy sent(&repo, &ChatRepository::messageSent);
        repo.send(QStringLiteral("demo/acct"), QStringLiteral("!room:demo"), QStringLiteral("hi"));
        QTRY_COMPARE_WITH_TIMEOUT(sent.count(), 1, 3000);
        // The captured request is a ConvSend carrying the text.
        QVERIFY(!fake.callPayloads().isEmpty());
        auto decoded = std::make_unique<api_request_r>();
        QVERIFY(decodeReq(fake.callPayloads().first(), decoded.get()));
        QCOMPARE(decoded->api_request_choice, api_request_r::api_request_request_conv_send_m_c);
    }

    // ----- PersonsRepository over the mux -----
    void personsRepositoryRefresh() {
        WireMuxServer fake;
        QVERIFY2(fake.start(sock(QStringLiteral("persons.sock"))), "listen");
        fake.setReplyPayload(personsResponse());
        DaemonTransport tx;
        tx.setSocketPath(sock(QStringLiteral("persons.sock")));
        NodeApiClient client(&tx);
        DaemonCacheStore cache(db(QStringLiteral("persons.db")));
        PersonsRepository repo(&client, &cache);
        QSignalSpy refreshed(&repo, &PersonsRepository::personsRefreshed);
        repo.refresh();
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        QCOMPARE(repo.persons().size(), 1);
        QCOMPARE(repo.persons().at(0).id, QStringLiteral("person-1"));
    }

    // ----- TransportRepository: settings read + configure over the mux -----
    void transportSettingsRepository() {
        WireMuxServer fake;
        QVERIFY2(fake.start(sock(QStringLiteral("settings.sock"))), "listen");
        fake.setReplyPayload(transportSettingsResponse());
        DaemonTransport tx;
        tx.setSocketPath(sock(QStringLiteral("settings.sock")));
        NodeApiClient client(&tx);
        DaemonCacheStore cache(db(QStringLiteral("settings.db")));
        TransportRepository repo(&client, &cache);
        QSignalSpy refreshed(&repo, &TransportRepository::settingsRefreshed);
        repo.refreshSettings(QStringLiteral("demo/acct"));
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        QCOMPARE(repo.settings(QStringLiteral("demo/acct")).value(QStringLiteral("homeserver")),
                 QStringLiteral("https://demo"));

        // configure sends a TransportConfigure; on Ok it re-reads the settings (a second Call).
        fake.setReplyPayload(okResponse());
        const int before = fake.requestCount();
        QMap<QString, QString> values;
        values.insert(QStringLiteral("homeserver"), QStringLiteral("https://new"));
        repo.configure(QStringLiteral("demo/acct"), values);
        QTRY_VERIFY_WITH_TIMEOUT(fake.requestCount() > before, 3000);
        auto decoded = std::make_unique<api_request_r>();
        QVERIFY(decodeReq(fake.callPayloads().at(before), decoded.get()));
        QCOMPARE(decoded->api_request_choice,
                 api_request_r::api_request_request_transport_configure_m_c);
    }

    // ----- DaemonChatService projects ChatRepository into IChatService rows -----
    void daemonChatServiceProjects() {
        WireMuxServer fake;
        QVERIFY2(fake.start(sock(QStringLiteral("chatsvc.sock"))), "listen");
        fake.setReplyPayload(convHistoryResponse());
        DaemonTransport tx;
        tx.setSocketPath(sock(QStringLiteral("chatsvc.sock")));
        NodeApiClient client(&tx);
        DaemonCacheStore cache(db(QStringLiteral("chatsvc.db")));
        ChatRepository repo(&client, &cache);
        transports::DaemonChatService svc(&repo);
        QSignalSpy changed(&svc, &transports::IChatService::messagesChanged);
        svc.refreshHistory(QStringLiteral("demo/acct"), QStringLiteral("!room:demo"));
        QTRY_COMPARE_WITH_TIMEOUT(changed.count(), 1, 3000);
        const QVariantList rows =
            svc.messages(QStringLiteral("demo/acct"), QStringLiteral("!room:demo"));
        QCOMPARE(rows.size(), 2);
        QCOMPARE(rows.at(0).toMap().value(QStringLiteral("text")).toString(), QStringLiteral("hi"));
        QCOMPARE(rows.at(0).toMap().value(QStringLiteral("authorName")).toString(),
                 QStringLiteral("Bob"));
    }

    // ----- DaemonPersonsService projects PersonsRepository into IPersonsService rows -----
    void daemonPersonsServiceProjects() {
        WireMuxServer fake;
        QVERIFY2(fake.start(sock(QStringLiteral("personsvc.sock"))), "listen");
        fake.setReplyPayload(personsResponse());
        DaemonTransport tx;
        tx.setSocketPath(sock(QStringLiteral("personsvc.sock")));
        NodeApiClient client(&tx);
        DaemonCacheStore cache(db(QStringLiteral("personsvc.db")));
        PersonsRepository repo(&client, &cache);
        transports::DaemonPersonsService svc(&repo);
        QSignalSpy changed(&svc, &transports::IPersonsService::personsChanged);
        svc.refresh();
        QTRY_COMPARE_WITH_TIMEOUT(changed.count(), 1, 3000);
        const QVariantList rows = svc.persons();
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows.at(0).toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("person-1"));
        const QVariantList eps = rows.at(0).toMap().value(QStringLiteral("endpoints")).toList();
        QCOMPARE(eps.size(), 1);
        QCOMPARE(eps.at(0).toMap().value(QStringLiteral("transport")).toString(),
                 QStringLiteral("demo/acct"));
    }

    // ----- DaemonTransportRegistry: settings surface + accountSchema projection -----
    void daemonRegistrySettings() {
        WireMuxServer fake;
        QVERIFY2(fake.start(sock(QStringLiteral("reg.sock"))), "listen");
        fake.setReplyPayload(transportSettingsResponse());
        DaemonTransport tx;
        tx.setSocketPath(sock(QStringLiteral("reg.sock")));
        NodeApiClient client(&tx);
        DaemonCacheStore cache(db(QStringLiteral("reg.db")));
        TransportRepository repo(&client, &cache);
        transports::DaemonTransportRegistry registry(&repo);
        QSignalSpy changed(&registry, &transports::ITransportRegistry::settingsChanged);
        registry.refreshSettings(QStringLiteral("demo/acct"));
        QTRY_COMPARE_WITH_TIMEOUT(changed.count(), 1, 3000);
        QCOMPARE(registry.settings(QStringLiteral("demo/acct"))
                     .value(QStringLiteral("homeserver"))
                     .toString(),
                 QStringLiteral("https://demo"));

        // availableAdapters() carries the account schema (with enriched field metadata).
        fake.setReplyPayload(adaptersWithAccountSchema());
        QSignalSpy adapters(&registry, &transports::ITransportRegistry::adaptersChanged);
        repo.refreshAdapters();
        QTRY_COMPARE_WITH_TIMEOUT(adapters.count(), 1, 3000);
        const QVariantList schema =
            registry.availableAdapters().at(0).toMap().value(QStringLiteral("schema")).toList();
        QCOMPARE(schema.size(), 1);
        QCOMPARE(schema.at(0).toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("Password"));
    }

    // ----- Mocks: canned rows + emit on refresh/send (dev stand-ins for the seams) -----
    void mockPersonsService() {
        transports::MockPersonsService svc;
        QSignalSpy changed(&svc, &transports::IPersonsService::personsChanged);
        svc.refresh();
        QCOMPARE(changed.count(), 1);
        QVERIFY(!svc.persons().isEmpty());
    }

    void mockChatService() {
        transports::MockChatService svc;
        QSignalSpy changed(&svc, &transports::IChatService::messagesChanged);
        svc.send(QStringLiteral("demo/acct"), QStringLiteral("!room:demo"), QStringLiteral("hi"));
        QCOMPARE(changed.count(), 1);
        const QVariantList rows =
            svc.messages(QStringLiteral("demo/acct"), QStringLiteral("!room:demo"));
        QVERIFY(!rows.isEmpty());
        QCOMPARE(rows.last().toMap().value(QStringLiteral("text")).toString(),
                 QStringLiteral("hi"));
    }

    // ----- SubscriptionManager fan-out: MessagesChanged -> ChatRepository, PersonsChanged ->
    // Persons
    void messagesChangedRoutesToChat() {
        WireMuxServer fake;
        QVERIFY2(fake.start(sock(QStringLiteral("subchat.sock"))), "listen");
        fake.setReplyPayload(convHistoryResponse());
        DaemonTransport tx;
        tx.setSocketPath(sock(QStringLiteral("subchat.sock")));
        NodeApiClient client(&tx);
        DaemonCacheStore cache(db(QStringLiteral("subchat.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        ChatRepository chat(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);
        mgr.setChatRepository(&chat);
        QSignalSpy refreshed(&chat, &ChatRepository::historyRefreshed);
        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        fake.pushItem(
            daemonapp::test::buildEventsPage({neMessagesChanged("demo/acct", "!room:demo")}, 1, 1));
        // The routed applyMessagesChanged() issues a ConvHistory Call whose Journal reply
        // refreshes.
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
    }

    void personsChangedRoutesToPersons() {
        WireMuxServer fake;
        QVERIFY2(fake.start(sock(QStringLiteral("subpers.sock"))), "listen");
        fake.setReplyPayload(personsResponse());
        DaemonTransport tx;
        tx.setSocketPath(sock(QStringLiteral("subpers.sock")));
        NodeApiClient client(&tx);
        DaemonCacheStore cache(db(QStringLiteral("subpers.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        PersonsRepository persons(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);
        mgr.setPersonsRepository(&persons);
        QSignalSpy refreshed(&persons, &PersonsRepository::personsRefreshed);
        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        fake.pushItem(daemonapp::test::buildEventsPage({nePersonsChanged()}, 1, 1));
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
    }

    // ----- IAuthFlowService DTO plumbing: the enriched field metadata reaches the seam maps -----
    // A3's generic auth component binds IAuthFlowService (via AuthFlowController), never the codec
    // — so the v38 kind/default/placeholder/choices must ride the provider params and Form
    // challenge field maps, or a masked/prefilled/choice field is unrenderable.
    void authSeamCarriesEnrichedFieldMetadata() {
        WireMuxServer fake;
        QVERIFY2(fake.start(sock(QStringLiteral("authseam.sock"))), "listen");
        fake.setReplySequence({authProvidersEnriched(), authBegunFormEnriched()});
        DaemonTransport tx;
        tx.setSocketPath(sock(QStringLiteral("authseam.sock")));
        NodeApiClient client(&tx);
        DaemonCacheStore cache(db(QStringLiteral("authseam.db")));
        AuthRepository repo(&client, &cache);
        auth::DaemonAuthFlowService svc(&repo);

        // Provider params: the schema field carries the enriched metadata.
        QSignalSpy providersChanged(&svc, &auth::IAuthFlowService::providersChanged);
        svc.refreshProviders();
        QTRY_COMPARE_WITH_TIMEOUT(providersChanged.count(), 1, 3000);
        const QVariantList providers = svc.providers();
        QCOMPARE(providers.size(), 1);
        QCOMPARE(providers.at(0).toMap().value(QStringLiteral("flowKind")).toString(),
                 QStringLiteral("UserPassword"));
        const QVariantMap param =
            providers.at(0).toMap().value(QStringLiteral("params")).toList().at(0).toMap();
        QCOMPARE(param.value(QStringLiteral("kind")).toString(), QStringLiteral("Password"));
        QCOMPARE(param.value(QStringLiteral("default")).toString(), QStringLiteral("hunter2"));
        QCOMPARE(param.value(QStringLiteral("placeholder")).toString(),
                 QStringLiteral("your password"));
        QCOMPARE(param.value(QStringLiteral("choices")).toStringList(),
                 QStringList({QStringLiteral("a"), QStringLiteral("b")}));

        // Form challenge fields: begun() carries the same metadata per field.
        QSignalSpy begun(&svc, &auth::IAuthFlowService::begun);
        svc.begin(QStringLiteral("demo"), {}, QStringLiteral("http://127.0.0.1:0/cb"));
        QTRY_COMPARE_WITH_TIMEOUT(begun.count(), 1, 3000);
        const QVariantMap challenge = begun.at(0).at(1).toMap();
        QCOMPARE(challenge.value(QStringLiteral("kind")).toString(), QStringLiteral("form"));
        const QVariantMap field = challenge.value(QStringLiteral("fields")).toList().at(0).toMap();
        QCOMPARE(field.value(QStringLiteral("kind")).toString(), QStringLiteral("Password"));
        QCOMPARE(field.value(QStringLiteral("default")).toString(), QStringLiteral("hunter2"));
        QCOMPARE(field.value(QStringLiteral("placeholder")).toString(),
                 QStringLiteral("your password"));
        QCOMPARE(field.value(QStringLiteral("choices")).toStringList().size(), 2);
    }

    // ----- MockAuthFlowService: the full challenge matrix includes a Qr flow -----
    // The plan's A1 section wants the scripted mock to cover every challenge kind so A3 can build
    // the QR surface offline: a "qr" family begins with a Qr challenge (payload + poll cadence) and
    // completes on the first Poll step.
    void mockAuthFlowCoversQrChallenge() {
        auth::MockAuthFlowService svc;
        bool qrFamilyOffered = false;
        for (const QVariant& row : svc.providers()) {
            if (row.toMap().value(QStringLiteral("family")).toString() == QLatin1String("qr")) {
                qrFamilyOffered = true;
            }
        }
        QVERIFY(qrFamilyOffered);

        QSignalSpy begun(&svc, &auth::IAuthFlowService::begun);
        QSignalSpy completed(&svc, &auth::IAuthFlowService::completed);
        svc.begin(QStringLiteral("qr"), {}, QStringLiteral("http://127.0.0.1:0/cb"));
        QTRY_COMPARE_WITH_TIMEOUT(begun.count(), 1, 3000);
        const QVariantMap challenge = begun.at(0).at(1).toMap();
        QCOMPARE(challenge.value(QStringLiteral("kind")).toString(), QStringLiteral("qr"));
        QVERIFY(!challenge.value(QStringLiteral("payload")).toString().isEmpty());
        QVERIFY(challenge.value(QStringLiteral("pollIntervalMs")).toInt() > 0);

        auth::StepInput poll;
        poll.kind = auth::StepInputKind::Poll;
        svc.step(begun.at(0).at(0).toString(), poll);
        QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 3000);
    }

    // ----- Conversation parent/Space survives the cache + the registry projection -----
    // A2's tree consumes ITransportRegistry::conversations() (projected from the offline-first
    // daemon_conversations cache), so `parent` and the "space" kind must round-trip through both —
    // decode-only support would strand the hierarchy at the codec layer.
    void conversationParentSurvivesCacheAndRegistry() {
        WireMuxServer fake;
        QVERIFY2(fake.start(sock(QStringLiteral("convparent.sock"))), "listen");
        fake.setReplyPayload(conversationsWithSpaceAndParent());
        DaemonTransport tx;
        tx.setSocketPath(sock(QStringLiteral("convparent.sock")));
        NodeApiClient client(&tx);
        DaemonCacheStore cache(db(QStringLiteral("convparent.db")));
        TransportRepository repo(&client, &cache);
        transports::DaemonTransportRegistry registry(&repo);
        QSignalSpy refreshed(&registry, &transports::ITransportRegistry::conversationsChanged);
        repo.refreshConversations(QStringLiteral("demo/acct"));
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);

        // The repository's cached rows carry kind="space" and the child's parent id.
        const auto rows = repo.cachedConversations(QStringLiteral("demo/acct"));
        QCOMPARE(rows.size(), 2);
        QString spaceParentOfRoom;
        bool sawSpace = false;
        for (const auto& r : rows) {
            if (r.convId == QStringLiteral("!space:demo")) {
                sawSpace = r.kind == QStringLiteral("space") && r.parent.isEmpty();
            } else if (r.convId == QStringLiteral("!room:demo")) {
                spaceParentOfRoom = r.parent;
            }
        }
        QVERIFY(sawSpace);
        QCOMPARE(spaceParentOfRoom, QStringLiteral("!space:demo"));

        // The seam projection carries both facts to the tree consumer.
        QString projectedParent;
        QString projectedKind;
        for (const QVariant& rv : registry.conversations(QStringLiteral("demo/acct"))) {
            const QVariantMap m = rv.toMap();
            if (m.value(QStringLiteral("id")).toString() == QLatin1String("!room:demo")) {
                projectedParent = m.value(QStringLiteral("parent")).toString();
            } else if (m.value(QStringLiteral("id")).toString() == QLatin1String("!space:demo")) {
                projectedKind = m.value(QStringLiteral("kind")).toString();
            }
        }
        QCOMPARE(projectedParent, QStringLiteral("!space:demo"));
        QCOMPARE(projectedKind, QStringLiteral("space"));
    }
};

#include "tst_integrations_seams.moc"

QTEST_MAIN(TestIntegrationsSeams)
