// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_accounts_service.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_contacts_service.h"
#include "daemon/daemon_presence_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/daemon_transport_registry.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "transports/mock_contacts_service.h"
#include "transports/mock_transport_registry.h"
#include "uimodels/variant_list_model.h"
#include "wire_mux_fixture.h"

#include <memory>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::CachedTransportInstanceRow;
using daemonapp::daemon::ContactsRepository;
using daemonapp::daemon::CredentialRepository;
using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::DecodedAdapterInfo;
using daemonapp::daemon::DecodedContact;
using daemonapp::daemon::DecodedConversation;
using daemonapp::daemon::DecodedCredentialInfo;
using daemonapp::daemon::DecodedEventsPage;
using daemonapp::daemon::DecodedNodeEvent;
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

// [acct-mgmt] {"Adapters":[ AdapterInfo{ matrix + all four per-verb ops + directory } ]} — the
// wire v33 shape. Conversation ops: everything but delete; membership: everything but ban, so the
// decode proves per-verb values (not just presence).
QByteArray adaptersResponseWithOps() {
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
    a.adapter_info_conversation_ops_present = true;
    a.adapter_info_conversation_ops.adapter_info_conversation_ops_choice =
        adapter_info_conversation_ops_r::adapter_info_conversation_ops_conversation_ops_m_c;
    conversation_ops& co =
        a.adapter_info_conversation_ops.adapter_info_conversation_ops_conversation_ops_m;
    co.conversation_ops_create = true;
    co.conversation_ops_join_channel = true;
    co.conversation_ops_leave = true;
    co.conversation_ops_delete = false;
    co.conversation_ops_send = true;
    co.conversation_ops_set_topic = true;
    co.conversation_ops_set_title = false;
    co.conversation_ops_set_description = false;
    a.adapter_info_membership_ops_present = true;
    a.adapter_info_membership_ops.adapter_info_membership_ops_choice =
        adapter_info_membership_ops_r::adapter_info_membership_ops_membership_ops_m_c;
    membership_ops& mo = a.adapter_info_membership_ops.adapter_info_membership_ops_membership_ops_m;
    mo.membership_ops_invite = true;
    mo.membership_ops_remove = true;
    mo.membership_ops_ban = false;
    mo.membership_ops_set_role = true;
    a.adapter_info_contacts_ops_present = true;
    a.adapter_info_contacts_ops.adapter_info_contacts_ops_choice =
        adapter_info_contacts_ops_r::adapter_info_contacts_ops_contacts_ops_m_c;
    contacts_ops& cto = a.adapter_info_contacts_ops.adapter_info_contacts_ops_contacts_ops_m;
    cto.contacts_ops_get_profile = true;
    cto.contacts_ops_action_menu = false;
    cto.contacts_ops_set_alias = true;
    a.adapter_info_roster_ops_present = true;
    a.adapter_info_roster_ops.adapter_info_roster_ops_choice =
        adapter_info_roster_ops_r::adapter_info_roster_ops_roster_ops_m_c;
    roster_ops& ro = a.adapter_info_roster_ops.adapter_info_roster_ops_roster_ops_m;
    ro.roster_ops_list = true; // wire v34: gates the Contacts section
    ro.roster_ops_add = true;
    ro.roster_ops_update = false;
    ro.roster_ops_remove = true;
    a.adapter_info_directory_present = true;
    a.adapter_info_directory.adapter_info_directory = true;
    return encodeResponse(*resp);
}

// [acct-mgmt] AdapterInfo with all four ops fields PRESENT but carrying the null arm (an adapter
// without those features) — must decode exactly like the absent (v32) shape: has*Ops false.
QByteArray adaptersResponseNullOps() {
    const QByteArray fam = QByteArrayLiteral("http");
    const QByteArray disp = QByteArrayLiteral("HTTP");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_adapters_m_c;
    response_adapters& ra = resp->api_response_response_adapters_m;
    ra.response_adapters_Adapters_adapter_info_m_count = 1;
    adapter_info& a = ra.response_adapters_Adapters_adapter_info_m[0];
    setZ(a.adapter_info_family, fam);
    setZ(a.adapter_info_display_name, disp);
    a.adapter_info_conversation_ops_present = true;
    a.adapter_info_conversation_ops.adapter_info_conversation_ops_choice =
        adapter_info_conversation_ops_r::adapter_info_conversation_ops_null_m_c;
    a.adapter_info_membership_ops_present = true;
    a.adapter_info_membership_ops.adapter_info_membership_ops_choice =
        adapter_info_membership_ops_r::adapter_info_membership_ops_null_m_c;
    a.adapter_info_contacts_ops_present = true;
    a.adapter_info_contacts_ops.adapter_info_contacts_ops_choice =
        adapter_info_contacts_ops_r::adapter_info_contacts_ops_null_m_c;
    a.adapter_info_roster_ops_present = true;
    a.adapter_info_roster_ops.adapter_info_roster_ops_choice =
        adapter_info_roster_ops_r::adapter_info_roster_ops_null_m_c;
    a.adapter_info_directory_present = false;
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

// [acct-mgmt] wire v35: a disabled, labeled instance — `enabled=false` + `label="Work bot"`.
QByteArray instancesResponseDisabledLabeled() {
    const QByteArray t = QByteArrayLiteral("matrix/@bot:hs");
    const QByteArray fam = QByteArrayLiteral("matrix");
    const QByteArray disp = QByteArrayLiteral("@bot:hs");
    const QByteArray label = QByteArrayLiteral("Work bot");
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
        .connection_state_choice = connection_state_r::connection_state_Offline_tstr_c;
    ti.transport_instance_info_presence_present = false;
    ti.transport_instance_info_bound_profile_present = false;
    ti.transport_instance_info_enabled_present = true;
    ti.transport_instance_info_enabled.transport_instance_info_enabled = false;
    ti.transport_instance_info_label_present = true;
    ti.transport_instance_info_label.transport_instance_info_label_choice =
        transport_instance_info_label_r::transport_instance_info_label_tstr_c;
    setZ(ti.transport_instance_info_label.transport_instance_info_label_tstr, label);
    return encodeResponse(*resp);
}

// [acct-mgmt] wire v35: {"Credentials":[ {matrix:@bot:hs, present, "Personal"}, {irc:libera} ]} —
// the first credential carries a label, the second omits it (proves both decode paths).
QByteArray credentialsResponseLabeled() {
    const QByteArray p0 = QByteArrayLiteral("matrix:@bot:hs");
    const QByteArray h0 = QByteArrayLiteral("token …abcd");
    const QByteArray l0 = QByteArrayLiteral("Personal");
    const QByteArray p1 = QByteArrayLiteral("irc:libera");
    const QByteArray h1 = QByteArrayLiteral("pass …9999");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_credentials_m_c;
    response_credentials& rc = resp->api_response_response_credentials_m;
    rc.response_credentials_Credentials_credential_info_m_count = 2;
    credential_info& c0 = rc.response_credentials_Credentials_credential_info_m[0];
    setZ(c0.credential_info_profile, p0);
    c0.credential_info_present = true;
    setZ(c0.credential_info_hint, h0);
    c0.credential_info_label_present = true;
    c0.credential_info_label.credential_info_label_choice =
        credential_info_label_r::credential_info_label_tstr_c;
    setZ(c0.credential_info_label.credential_info_label_tstr, l0);
    credential_info& c1 = rc.response_credentials_Credentials_credential_info_m[1];
    setZ(c1.credential_info_profile, p1);
    c1.credential_info_present = true;
    setZ(c1.credential_info_hint, h1);
    c1.credential_info_label_present = false;
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

// [acct-mgmt] Fill a contact-info: id + optional display name; presence Available, permission
// Allow (so the decode proves every optional field decodes).
void putContact(contact_info& c, const QByteArray& id, const QByteArray& displayName) {
    setZ(c.contact_info_id, id);
    c.contact_info_display_name_present = !displayName.isEmpty();
    if (!displayName.isEmpty()) {
        c.contact_info_display_name.contact_info_display_name_choice =
            contact_info_display_name_r::contact_info_display_name_tstr_c;
        setZ(c.contact_info_display_name.contact_info_display_name_tstr, displayName);
    }
    c.contact_info_presence_present = true;
    auto& pp = c.contact_info_presence.contact_info_presence;
    pp.presence_primitive.presence_primitive_t_choice =
        presence_primitive_t_r::presence_primitive_t_Available_tstr_c;
    pp.presence_message_present = false;
    pp.presence_emoji_present = false;
    pp.presence_mobile_present = false;
    pp.presence_idle_since_present = false;
    c.contact_info_permission_present = true;
    c.contact_info_permission.contact_info_permission.contact_permission_choice =
        contact_permission_r::contact_permission_Allow_tstr_c;
}

// [acct-mgmt] {"ContactPage": contact-page{ items:[alice, bob], next }}. `next` empty = last page.
QByteArray contactPageResponse(const QByteArray& next) {
    static const QByteArray alice = QByteArrayLiteral("@alice:matrix.org");
    static const QByteArray aliceName = QByteArrayLiteral("Alice");
    static const QByteArray bob = QByteArrayLiteral("@bob:matrix.org");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_contact_page_m_c;
    contact_page& page =
        resp->api_response_response_contact_page_m.response_contact_page_ContactPage;
    page.contact_page_items_contact_info_m_count = 2;
    putContact(page.contact_page_items_contact_info_m[0], alice, aliceName);
    putContact(page.contact_page_items_contact_info_m[1], bob, QByteArray()); // no display name
    page.contact_page_next_present = !next.isEmpty();
    if (!next.isEmpty()) {
        page.contact_page_next.contact_page_next_choice =
            contact_page_next_r::contact_page_next_tstr_c;
        setZ(page.contact_page_next.contact_page_next_tstr, next);
    }
    return encodeResponse(*resp);
}

// [acct-mgmt] The second RosterList page (one contact, no next) for the paging test.
QByteArray contactPageResponsePage2() {
    static const QByteArray carol = QByteArrayLiteral("@carol:matrix.org");
    static const QByteArray carolName = QByteArrayLiteral("Carol");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_contact_page_m_c;
    contact_page& page =
        resp->api_response_response_contact_page_m.response_contact_page_ContactPage;
    page.contact_page_items_contact_info_m_count = 1;
    putContact(page.contact_page_items_contact_info_m[0], carol, carolName);
    page.contact_page_next_present = false;
    return encodeResponse(*resp);
}

// [acct-mgmt] {"Contacts":[dave]} — a DirectorySearch result.
QByteArray directoryResponse() {
    static const QByteArray dave = QByteArrayLiteral("@dave:matrix.org");
    static const QByteArray daveName = QByteArrayLiteral("Dave");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_contacts_m_c;
    response_contacts& c = resp->api_response_response_contacts_m;
    c.response_contacts_Contacts_contact_info_m_count = 1;
    putContact(c.response_contacts_Contacts_contact_info_m[0], dave, daveName);
    return encodeResponse(*resp);
}

// [acct-mgmt] {"ContactProfile": "..."} — the node-rendered profile string.
QByteArray contactProfileResponse() {
    static const QByteArray profile = QByteArrayLiteral("Alice\n@alice:matrix.org");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_contact_profile_m_c;
    setZ(resp->api_response_response_contact_profile_m.response_contact_profile_ContactProfile,
         profile);
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
        // [acct-mgmt] the legacy (v32) shape omits every per-verb ops field -> has*Ops false
        // ("no per-verb info"; the UI falls back to the coarse capability).
        QVERIFY(!out.at(0).hasConversationOps);
        QVERIFY(!out.at(0).hasMembershipOps);
        QVERIFY(!out.at(0).hasContactsOps);
        QVERIFY(!out.at(0).hasRosterOps);
        QVERIFY(!out.at(0).hasDirectory);
    }

    // [acct-mgmt] Concrete per-verb ops maps (wire v33) decode with per-verb values; directory
    // rides along.
    void adapterOpsRoundTrip() {
        QList<DecodedAdapterInfo> out;
        QVERIFY(NodeApiCodec::decodeAdapters(adaptersResponseWithOps(), &out));
        QCOMPARE(out.size(), 1);
        const DecodedAdapterInfo& a = out.at(0);
        QVERIFY(a.hasConversationOps);
        QVERIFY(a.conversationOps.value(QStringLiteral("create")).toBool());
        QVERIFY(a.conversationOps.value(QStringLiteral("joinChannel")).toBool());
        QVERIFY(a.conversationOps.value(QStringLiteral("leave")).toBool());
        QVERIFY(!a.conversationOps.value(QStringLiteral("delete")).toBool());
        QVERIFY(a.conversationOps.value(QStringLiteral("send")).toBool());
        QVERIFY(a.conversationOps.value(QStringLiteral("setTopic")).toBool());
        QVERIFY(!a.conversationOps.value(QStringLiteral("setTitle")).toBool());
        QVERIFY(a.hasMembershipOps);
        QVERIFY(a.membershipOps.value(QStringLiteral("invite")).toBool());
        QVERIFY(a.membershipOps.value(QStringLiteral("remove")).toBool());
        QVERIFY(!a.membershipOps.value(QStringLiteral("ban")).toBool());
        QVERIFY(a.membershipOps.value(QStringLiteral("setRole")).toBool());
        QVERIFY(a.hasContactsOps);
        QVERIFY(a.contactsOps.value(QStringLiteral("getProfile")).toBool());
        QVERIFY(!a.contactsOps.value(QStringLiteral("actionMenu")).toBool());
        QVERIFY(a.contactsOps.value(QStringLiteral("setAlias")).toBool());
        QVERIFY(a.hasRosterOps);
        // [acct-mgmt] wire v34: `list` gates the Contacts section, sibling to add/update/remove.
        QVERIFY(a.rosterOps.value(QStringLiteral("list")).toBool());
        QVERIFY(a.rosterOps.value(QStringLiteral("add")).toBool());
        QVERIFY(!a.rosterOps.value(QStringLiteral("update")).toBool());
        QVERIFY(a.rosterOps.value(QStringLiteral("remove")).toBool());
        QVERIFY(a.hasDirectory);
        QVERIFY(a.directory);
    }

    // [acct-mgmt] The null arm (adapter without those features) decodes exactly like absent:
    // has*Ops false — the client cannot tell them apart and must not try.
    void adapterOpsNullDecodesAsAbsent() {
        QList<DecodedAdapterInfo> out;
        QVERIFY(NodeApiCodec::decodeAdapters(adaptersResponseNullOps(), &out));
        QCOMPARE(out.size(), 1);
        QVERIFY(!out.at(0).hasConversationOps);
        QVERIFY(out.at(0).conversationOps.isEmpty());
        QVERIFY(!out.at(0).hasMembershipOps);
        QVERIFY(!out.at(0).hasContactsOps);
        QVERIFY(!out.at(0).hasRosterOps);
        QVERIFY(!out.at(0).hasDirectory);
    }

    // [acct-mgmt] The daemon registry projects a present ops map into the adapter row (the key is
    // present ONLY then), so QML/TUI distinguish authoritative per-verb info from the fallback.
    void registryProjectsPerVerbOps() {
        const QString sock = m_tmp.filePath(QStringLiteral("ops.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(adaptersResponseWithOps());
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("ops.db")));
        TransportRepository repo(&tx.client, &cache);
        DaemonTransportRegistry registry(&repo);

        QSignalSpy adapters(&registry, &transports::ITransportRegistry::adaptersChanged);
        repo.refreshAdapters();
        QTRY_COMPARE_WITH_TIMEOUT(adapters.count(), 1, 3000);
        const QVariantMap row = registry.availableAdapters().at(0).toMap();
        QVERIFY(row.contains(QStringLiteral("conversationOps")));
        QVERIFY(!row.value(QStringLiteral("conversationOps"))
                     .toMap()
                     .value(QStringLiteral("delete"))
                     .toBool());
        QVERIFY(row.value(QStringLiteral("conversationOps"))
                    .toMap()
                    .value(QStringLiteral("joinChannel"))
                    .toBool());
        QVERIFY(row.contains(QStringLiteral("membershipOps")));
        QVERIFY(!row.value(QStringLiteral("membershipOps"))
                     .toMap()
                     .value(QStringLiteral("ban"))
                     .toBool());
        QVERIFY(row.value(QStringLiteral("directory")).toBool());

        // The null-ops shape projects NO ops keys (fallback signal for consumers).
        fake.setReplyPayload(adaptersResponseNullOps());
        repo.refreshAdapters();
        QTRY_COMPARE_WITH_TIMEOUT(adapters.count(), 2, 3000);
        const QVariantMap nullRow = registry.availableAdapters().at(0).toMap();
        QVERIFY(!nullRow.contains(QStringLiteral("conversationOps")));
        QVERIFY(!nullRow.contains(QStringLiteral("membershipOps")));
        QVERIFY(!nullRow.contains(QStringLiteral("contactsOps")));
        QVERIFY(!nullRow.contains(QStringLiteral("rosterOps")));
        QVERIFY(!nullRow.contains(QStringLiteral("directory")));
    }

    // [acct-mgmt] The mock's canned families carry differing per-verb ops so offline dev
    // exercises the gating: matrix allows delete/ban/setRole, the Rooms loopback does not — the
    // exact bits the GUI buttons / TUI keys read.
    void mockAdvertisesPerVerbOps() {
        transports::MockTransportRegistry mock;
        const QVariantList adapters = mock.availableAdapters();
        QCOMPARE(adapters.size(), 2);
        const QVariantMap matrix = adapters.at(0).toMap();
        const QVariantMap rooms = adapters.at(1).toMap();
        QCOMPARE(matrix.value(QStringLiteral("family")).toString(), QStringLiteral("matrix"));
        QCOMPARE(rooms.value(QStringLiteral("family")).toString(), QStringLiteral("room"));
        QVERIFY(matrix.value(QStringLiteral("conversationOps"))
                    .toMap()
                    .value(QStringLiteral("delete"))
                    .toBool());
        QVERIFY(!rooms.value(QStringLiteral("conversationOps"))
                     .toMap()
                     .value(QStringLiteral("delete"))
                     .toBool());
        QVERIFY(matrix.value(QStringLiteral("membershipOps"))
                    .toMap()
                    .value(QStringLiteral("ban"))
                    .toBool());
        QVERIFY(!rooms.value(QStringLiteral("membershipOps"))
                     .toMap()
                     .value(QStringLiteral("ban"))
                     .toBool());
        QVERIFY(!rooms.value(QStringLiteral("membershipOps"))
                     .toMap()
                     .value(QStringLiteral("setRole"))
                     .toBool());
        // Both canned families still advertise join/create/invite (functional offline flows).
        QVERIFY(rooms.value(QStringLiteral("conversationOps"))
                    .toMap()
                    .value(QStringLiteral("joinChannel"))
                    .toBool());
        QVERIFY(rooms.value(QStringLiteral("membershipOps"))
                    .toMap()
                    .value(QStringLiteral("invite"))
                    .toBool());
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
        // [acct-mgmt] wire v35: absent `enabled` defaults true; absent `label` is empty.
        QVERIFY(out.at(0).enabled);
        QVERIFY(out.at(0).label.isEmpty());
    }

    // [acct-mgmt] wire v35: an explicit `enabled=false` + `label` decode onto the instance row.
    void instancesEnabledLabelRoundTrip() {
        QList<DecodedTransportInstance> out;
        QVERIFY(NodeApiCodec::decodeTransportInstances(instancesResponseDisabledLabeled(), &out));
        QCOMPARE(out.size(), 1);
        QVERIFY(!out.at(0).enabled);
        QCOMPARE(out.at(0).label, QStringLiteral("Work bot"));
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
        // AD (1a.2, §10.3 rung 3): the sent frame is the SetRole encode PLUS a freshly-minted
        // retry-safety op_id, so byte-equality holds only up to the random key — assert the
        // fixed prefix and the op_id tail instead.
        const QByteArray withoutOpId = NodeApiCodec::encodeMemberSetRoleRequest(
            t, c, QStringLiteral("@bob:matrix.org"), QStringLiteral("Voice"));
        QVERIFY(calls.at(1).contains("MemberSetRole"));
        QVERIFY(calls.at(1).contains("op_id"));
        QVERIFY(!withoutOpId.contains("op_id"));
        QVERIFY(calls.at(1).size() > withoutOpId.size());
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

    // [acct-mgmt] wire v35: a cached disabled+labeled account overlays displayName with the label
    // and exposes enabled + the raw label (for the rename pre-fill) on the projected row.
    void registryOverlaysLabelEnabled() {
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("ch-lbl.db")));
        QVERIFY(cache.isOpen());
        CachedTransportInstanceRow row;
        row.transport = QStringLiteral("matrix/@bot:hs");
        row.family = QStringLiteral("matrix");
        row.displayName = QStringLiteral("@bot:hs");
        row.enabled = false;
        row.label = QStringLiteral("Work bot");
        row.updatedAtMs = 1;
        QVERIFY(cache.upsertTransportInstance(row));

        DaemonTransport transport; // unconnected
        NodeApiClient client(&transport);
        TransportRepository repo(&client, &cache);
        DaemonTransportRegistry registry(&repo);

        const QVariantMap acc = registry.instances().at(0).toMap();
        // The label overlays the display name; the raw label + enabled ride along.
        QCOMPARE(acc.value(QStringLiteral("displayName")).toString(), QStringLiteral("Work bot"));
        QCOMPARE(acc.value(QStringLiteral("label")).toString(), QStringLiteral("Work bot"));
        QVERIFY(!acc.value(QStringLiteral("enabled")).toBool());

        // With no label the display name is the raw display name.
        CachedTransportInstanceRow bare = row;
        bare.transport = QStringLiteral("matrix/@other:hs");
        bare.label.clear();
        bare.enabled = true;
        QVERIFY(cache.upsertTransportInstance(bare));
        for (const QVariant& v : registry.instances()) {
            const QVariantMap m = v.toMap();
            if (m.value(QStringLiteral("transport")).toString() ==
                QStringLiteral("matrix/@other:hs")) {
                QCOMPARE(m.value(QStringLiteral("displayName")).toString(),
                         QStringLiteral("@bot:hs"));
                QVERIFY(m.value(QStringLiteral("label")).toString().isEmpty());
                QVERIFY(m.value(QStringLiteral("enabled")).toBool());
            }
        }
    }

    // [acct-mgmt] wire v35: connectAccount sends TransportConnect + re-lists on Ok (the seam path).
    void registryConnectOverMux() {
        const QString sock = m_tmp.filePath(QStringLiteral("connect.sock"));
        const QString t = QStringLiteral("matrix/@bot:hs");
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplySequence({okResponse(), instancesResponse()});
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("connect.db")));
        TransportRepository repo(&tx.client, &cache);
        DaemonTransportRegistry registry(&repo);

        QSignalSpy changed(&registry, &transports::ITransportRegistry::instancesChanged);
        registry.connectAccount(t);
        QTRY_COMPARE_WITH_TIMEOUT(changed.count(), 1, 3000);
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 2); // TransportConnect, TransportInstances
        QCOMPARE(calls.at(0), NodeApiCodec::encodeTransportConnectRequest(t));
    }

    // [acct-mgmt] wire v35: setEnabled sends TransportSetEnabled(enabled) + re-lists on Ok.
    void registrySetEnabledOverMux() {
        const QString sock = m_tmp.filePath(QStringLiteral("enable.sock"));
        const QString t = QStringLiteral("matrix/@bot:hs");
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplySequence({okResponse(), instancesResponse()});
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("enable.db")));
        TransportRepository repo(&tx.client, &cache);
        DaemonTransportRegistry registry(&repo);

        QSignalSpy changed(&registry, &transports::ITransportRegistry::instancesChanged);
        registry.setEnabled(t, false);
        QTRY_COMPARE_WITH_TIMEOUT(changed.count(), 1, 3000);
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 2);
        QCOMPARE(calls.at(0), NodeApiCodec::encodeTransportSetEnabledRequest(t, false));
    }

    // [acct-mgmt] wire v35: setLabel("value") sets a tstr; setLabel("") maps to the explicit-null
    // clear (hasLabel=false) — both through the registry seam, re-listing on Ok.
    void registrySetLabelValueAndClearOverMux() {
        const QString sock = m_tmp.filePath(QStringLiteral("label.sock"));
        const QString t = QStringLiteral("matrix/@bot:hs");
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplySequence({okResponse(), instancesResponse()});
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("label.db")));
        TransportRepository repo(&tx.client, &cache);
        DaemonTransportRegistry registry(&repo);

        QSignalSpy changed(&registry, &transports::ITransportRegistry::instancesChanged);
        registry.setLabel(t, QStringLiteral("Work bot"));
        QTRY_COMPARE_WITH_TIMEOUT(changed.count(), 1, 3000);
        QCOMPARE(fake.callPayloads().at(0), NodeApiCodec::encodeTransportSetLabelRequest(
                                                t, /*hasLabel=*/true, QStringLiteral("Work bot")));

        fake.setReplySequence({okResponse(), instancesResponse()});
        registry.setLabel(t, QString()); // empty clears
        QTRY_COMPARE_WITH_TIMEOUT(changed.count(), 2, 3000);
        QCOMPARE(fake.callPayloads().at(2),
                 NodeApiCodec::encodeTransportSetLabelRequest(t, /*hasLabel=*/false, QString()));
    }

    // [acct-mgmt] wire v35: the mock registry mutates its canned account so the enable/rename paths
    // are exercisable offline: setEnabled toggles enabled, setLabel overlays displayName.
    void mockRegistryEnableLabelPaths() {
        transports::MockTransportRegistry mock;
        const QString t =
            mock.instances().at(0).toMap().value(QStringLiteral("transport")).toString();
        QVERIFY(mock.instances().at(0).toMap().value(QStringLiteral("enabled")).toBool());

        QSignalSpy changed(&mock, &transports::ITransportRegistry::instancesChanged);
        mock.setEnabled(t, false);
        QCOMPARE(changed.count(), 1);
        QVERIFY(!mock.instances().at(0).toMap().value(QStringLiteral("enabled")).toBool());

        mock.setLabel(t, QStringLiteral("Work bot"));
        QCOMPARE(changed.count(), 2);
        const QVariantMap row = mock.instances().at(0).toMap();
        QCOMPARE(row.value(QStringLiteral("displayName")).toString(), QStringLiteral("Work bot"));
        QCOMPARE(row.value(QStringLiteral("label")).toString(), QStringLiteral("Work bot"));

        mock.setLabel(t, QString()); // clear → back to the raw display name
        QCOMPARE(mock.instances().at(0).toMap().value(QStringLiteral("displayName")).toString(),
                 QStringLiteral("@you:matrix.org"));
    }

    // [acct-mgmt] wire v35: DaemonAccountsService.rename persists the label over the wire
    // (CredentialSetLabel) and rebuilds the account rows from the refetched credential-info.label —
    // no client-local label mutation (the m_meta rename path is gone).
    void accountsRenameUsesWireLabel() {
        const QString sock = m_tmp.filePath(QStringLiteral("acct.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        // rename → CredentialSetLabel Ok → refresh CredentialList (reports the new label).
        fake.setReplySequence({okResponse(), credentialsResponseLabeled()});
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("acct.db")));
        CredentialRepository creds(&tx.client, &cache);
        accounts::DaemonAccountsService svc(&creds, /*profiles=*/nullptr);

        QSignalSpy changed(&svc, &accounts::IAccountsService::credentialsChanged);
        svc.rename(QStringLiteral("matrix:@bot:hs"), QStringLiteral("Personal"));
        QTRY_COMPARE_WITH_TIMEOUT(changed.count(), 1, 3000);
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 2); // CredentialSetLabel, CredentialList
        QCOMPARE(calls.at(0), NodeApiCodec::encodeCredentialSetLabelRequest(
                                  QStringLiteral("matrix:@bot:hs"),
                                  /*hasLabel=*/true, QStringLiteral("Personal")));
        // The rebuilt row carries the wire label.
        auto* model = qobject_cast<uimodels::VariantListModel*>(svc.accounts());
        QVERIFY(model != nullptr);
        const int row = model->indexOfId(QStringLiteral("matrix:@bot:hs"));
        QVERIFY(row >= 0);
        QCOMPARE(model->at(row).value(QStringLiteral("label")).toString(),
                 QStringLiteral("Personal"));
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

    // [acct-mgmt] wire v35: connect / setEnabled / setLabel encode distinct intents; setEnabled
    // toggles on the bool; setLabel's explicit-null clear differs from setting a value.
    void connectEnabledLabelEncodeDistinctIntents() {
        const QString t = QStringLiteral("matrix/@bot:hs");
        const QByteArray conn = NodeApiCodec::encodeTransportConnectRequest(t);
        const QByteArray enOn = NodeApiCodec::encodeTransportSetEnabledRequest(t, true);
        const QByteArray enOff = NodeApiCodec::encodeTransportSetEnabledRequest(t, false);
        const QByteArray labSet = NodeApiCodec::encodeTransportSetLabelRequest(
            t, /*hasLabel=*/true, QStringLiteral("Work bot"));
        const QByteArray labClear =
            NodeApiCodec::encodeTransportSetLabelRequest(t, /*hasLabel=*/false, QString());
        QVERIFY(!conn.isEmpty());
        QVERIFY(!enOn.isEmpty());
        QVERIFY(!labSet.isEmpty());
        QVERIFY(!labClear.isEmpty());
        QVERIFY(enOn != enOff);      // the persisted bool distinguishes them
        QVERIFY(labSet != labClear); // tstr vs explicit null
        QVERIFY(conn != enOn);
        QVERIFY(conn != labSet);
    }

    // [acct-mgmt] wire v35: CredentialSetLabel encodes distinct set vs explicit-null clear intents.
    void credentialSetLabelEncodeDistinctIntents() {
        const QString p = QStringLiteral("matrix:@bot:hs");
        const QByteArray set = NodeApiCodec::encodeCredentialSetLabelRequest(
            p, /*hasLabel=*/true, QStringLiteral("Personal"));
        const QByteArray clear =
            NodeApiCodec::encodeCredentialSetLabelRequest(p, /*hasLabel=*/false, QString());
        QVERIFY(!set.isEmpty());
        QVERIFY(!clear.isEmpty());
        QVERIFY(set != clear);
    }

    // [acct-mgmt] wire v35: the credential-info `label` decodes (empty when absent, set otherwise).
    void credentialsLabelRoundTrip() {
        QList<DecodedCredentialInfo> out;
        QVERIFY(NodeApiCodec::decodeCredentials(credentialsResponseLabeled(), &out));
        QCOMPARE(out.size(), 2);
        QCOMPARE(out.at(0).profile, QStringLiteral("matrix:@bot:hs"));
        QCOMPARE(out.at(0).label, QStringLiteral("Personal"));
        QVERIFY(out.at(1).label.isEmpty()); // second credential has no label
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

    // --- [acct-mgmt] Transport contacts / roster (wire v34) --------------------------------------

    // ContactPage decodes its items (id + display name / presence / permission) and clears the
    // resume cursor on the last page.
    void contactPageRoundTrip() {
        QList<DecodedContact> out;
        QString next;
        QVERIFY(NodeApiCodec::decodeContactPage(contactPageResponse(QByteArray()), &out, &next));
        QCOMPARE(out.size(), 2);
        QCOMPARE(out.at(0).id, QStringLiteral("@alice:matrix.org"));
        QCOMPARE(out.at(0).displayName, QStringLiteral("Alice"));
        QCOMPARE(out.at(0).presence, QStringLiteral("available"));
        QCOMPARE(out.at(0).permission, QStringLiteral("allow"));
        // The second contact carries no display name (the UI falls back to the id).
        QCOMPARE(out.at(1).id, QStringLiteral("@bob:matrix.org"));
        QVERIFY(out.at(1).displayName.isEmpty());
        QVERIFY(next.isEmpty()); // last page
    }

    // A non-empty ContactPage.next surfaces as the resume cursor (more pages remain).
    void contactPageNextCursor() {
        QList<DecodedContact> out;
        QString next;
        QVERIFY(NodeApiCodec::decodeContactPage(contactPageResponse(QByteArrayLiteral("@bob:hs")),
                                                &out, &next));
        QCOMPARE(next, QStringLiteral("@bob:hs"));
    }

    // DirectorySearch (Contacts) decodes into the unpaged people list.
    void directoryRoundTrip() {
        QList<DecodedContact> out;
        QVERIFY(NodeApiCodec::decodeContacts(directoryResponse(), &out));
        QCOMPARE(out.size(), 1);
        QCOMPARE(out.at(0).id, QStringLiteral("@dave:matrix.org"));
        QCOMPARE(out.at(0).displayName, QStringLiteral("Dave"));
    }

    // ContactProfile decodes the node-rendered string verbatim.
    void contactProfileRoundTrip() {
        QString profile;
        QVERIFY(NodeApiCodec::decodeContactProfile(contactProfileResponse(), &profile));
        QCOMPARE(profile, QStringLiteral("Alice\n@alice:matrix.org"));
    }

    // The ContactsChanged node event decodes off the feed (transport-only payload; distinct from
    // the session-inbox RosterChanged).
    void contactsChangedEventDecodes() {
        const QByteArray page = daemonapp::test::buildEventsPage(
            {daemonapp::test::neContactsChanged("matrix/@bot:hs")}, 7, 7);
        DecodedEventsPage out;
        QVERIFY(NodeApiCodec::decodeEventsPage(page, &out));
        QCOMPARE(out.events.size(), 1);
        QCOMPARE(out.events.at(0).kind, DecodedNodeEvent::Kind::ContactsChanged);
        QCOMPARE(out.events.at(0).transport, QStringLiteral("matrix/@bot:hs"));
    }

    // ContactsRepository loops RosterList through the `next` cursor and swaps contacts() ONCE.
    void rosterPagesThroughNext() {
        const QString sock = m_tmp.filePath(QStringLiteral("roster.sock"));
        const QString t = QStringLiteral("matrix/@bot:hs");
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplySequence({contactPageResponse(QByteArrayLiteral("@bob:matrix.org")),
                               contactPageResponsePage2()});
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("roster.db")));
        ContactsRepository repo(&tx.client, &cache);

        QSignalSpy refreshed(&repo, &ContactsRepository::contactsRefreshed);
        repo.refreshContacts(t);
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        QTest::qWait(100); // a page-loop bug would emit again / issue extra requests
        QCOMPARE(refreshed.count(), 1);
        // Both pages accumulated into one roster (2 + 1 = 3).
        QCOMPARE(repo.contacts(t).size(), 3);
        QCOMPARE(repo.contacts(t).at(2).id, QStringLiteral("@carol:matrix.org"));
        // The continuation carried the cursor: byte-identical to the codec's own encoding.
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 2);
        QCOMPARE(calls.at(0), NodeApiCodec::encodeRosterListRequest(t));
        QCOMPARE(calls.at(1),
                 NodeApiCodec::encodeRosterListRequest(t, QStringLiteral("@bob:matrix.org")));
    }

    // A roster mutation (RosterAdd) replies Ok, then the repo re-issues RosterList (a second
    // contactsRefreshed) — the node's ContactsChanged event drives the same refetch in production.
    void contactMutationRefreshesOnOk() {
        const QString sock = m_tmp.filePath(QStringLiteral("radd.sock"));
        const QString t = QStringLiteral("matrix/@bot:hs");
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplySequence({okResponse(), contactPageResponse(QByteArray())});
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("radd.db")));
        ContactsRepository repo(&tx.client, &cache);

        QSignalSpy refreshed(&repo, &ContactsRepository::contactsRefreshed);
        repo.addContact(t, QStringLiteral("@zoe:matrix.org"));
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        QCOMPARE(repo.contacts(t).size(), 2);
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 2); // RosterAdd, RosterList
        DecodedContact zoe;
        zoe.id = QStringLiteral("@zoe:matrix.org");
        QCOMPARE(calls.at(0), NodeApiCodec::encodeRosterAddRequest(t, zoe));
        QCOMPARE(calls.at(1), NodeApiCodec::encodeRosterListRequest(t));
    }

    // setAlias("") clears the alias (the optional is omitted): byte-identical to the codec's
    // no-alias encoding.
    void setAliasEmptyClearsOptional() {
        const QString sock = m_tmp.filePath(QStringLiteral("alias.sock"));
        const QString t = QStringLiteral("matrix/@bot:hs");
        const QString id = QStringLiteral("@alice:matrix.org");
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplySequence({okResponse(), contactPageResponse(QByteArray())});
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("alias.db")));
        ContactsRepository repo(&tx.client, &cache);

        QSignalSpy refreshed(&repo, &ContactsRepository::contactsRefreshed);
        repo.setAlias(t, id, QString());
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.at(0),
                 NodeApiCodec::encodeContactSetAliasRequest(t, id, /*hasAlias=*/false, QString()));
    }

    // The DaemonContactsService projects the repo: searchDirectory -> directoryResults rows,
    // getProfile -> profileReady string.
    void serviceProjectsDirectoryAndProfile() {
        const QString sock = m_tmp.filePath(QStringLiteral("svc.sock"));
        const QString t = QStringLiteral("matrix/@bot:hs");
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        DaemonTransportFixture tx(sock);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("svc.db")));
        ContactsRepository repo(&tx.client, &cache);
        transports::DaemonContactsService svc(&repo);

        fake.setReplyPayload(directoryResponse());
        QSignalSpy dir(&svc, &transports::IContactsService::directoryResults);
        svc.searchDirectory(t, QStringLiteral("dave"));
        QTRY_COMPARE_WITH_TIMEOUT(dir.count(), 1, 3000);
        const QVariantList people = dir.at(0).at(1).toList();
        QCOMPARE(people.size(), 1);
        QCOMPARE(people.at(0).toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("@dave:matrix.org"));

        fake.setReplyPayload(contactProfileResponse());
        QSignalSpy prof(&svc, &transports::IContactsService::profileReady);
        svc.getProfile(t, QStringLiteral("@alice:matrix.org"));
        QTRY_COMPARE_WITH_TIMEOUT(prof.count(), 1, 3000);
        QCOMPARE(prof.at(0).at(2).toString(), QStringLiteral("Alice\n@alice:matrix.org"));
    }

    // The mock service exposes canned per-transport contacts offline; mutations emit
    // contactsChanged.
    void mockContactsCannedData() {
        transports::MockContactsService mock;
        const QString t = QStringLiteral("matrix/@you:matrix.org");
        QVERIFY(mock.contacts(t).size() >= 1);
        QSignalSpy changed(&mock, &transports::IContactsService::contactsChanged);
        mock.addContact(t, QStringLiteral("@new:matrix.org"));
        QCOMPARE(changed.count(), 1);
        bool found = false;
        for (const QVariant& v : mock.contacts(t)) {
            if (v.toMap().value(QStringLiteral("id")).toString() ==
                QStringLiteral("@new:matrix.org")) {
                found = true;
            }
        }
        QVERIFY(found);
        // getProfile fires profileReady (the canned profile string).
        QSignalSpy prof(&mock, &transports::IContactsService::profileReady);
        mock.getProfile(t, QStringLiteral("@new:matrix.org"));
        QCOMPARE(prof.count(), 1);
    }
};

QTEST_GUILESS_MAIN(TestDaemonTransports)
#include "tst_daemon_transports.moc"
