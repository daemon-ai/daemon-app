// ONE-TIME skeleton — bodies are human-owned; regeneration NEVER overwrites this file.
// The drift gate checks that every declared mapper (entities_map_gen.h) has a matching
// definition here with the current codec signature — never the body contents.
//
// Each body maps a decoded wire DTO to the canonical entity per the provenance in
// src/core/mirror/entity-map.toml (merging extra reads / scope context as needed).

#include "entities_map_gen.h"

#include <QString>

namespace mirror {
namespace {

// zcbor borrows its string bytes (never owns) — copy into a QString once at the map boundary
// (the single wire→canonical point, §3.6 SPEC-DECISION QString-in-entities).
QString qstr(const zcbor_string& s) {
    if (s.value == nullptr || s.len == 0) {
        return {};
    }
    return QString::fromUtf8(reinterpret_cast<const char*>(s.value), static_cast<int>(s.len));
}

// conversation-type enum → canonical string (named ONCE here, per entity-map provenance).
QString conversationKind(const ::conversation_type_r& k) {
    switch (k.conversation_type_choice) {
    case ::conversation_type_r::conversation_type_Dm_tstr_c:
        return QStringLiteral("dm");
    case ::conversation_type_r::conversation_type_GroupDm_tstr_c:
        return QStringLiteral("group_dm");
    case ::conversation_type_r::conversation_type_Channel_tstr_c:
        return QStringLiteral("channel");
    case ::conversation_type_r::conversation_type_Thread_tstr_c:
        return QStringLiteral("thread");
    case ::conversation_type_r::conversation_type_Space_tstr_c:
        return QStringLiteral("space");
    case ::conversation_type_r::conversation_type_Unset_tstr_c:
        return {};
    }
    return {};
}

// contact-permission enum → canonical string.
QString contactPermission(const ::contact_permission_r& p) {
    switch (p.contact_permission_choice) {
    case ::contact_permission_r::contact_permission_Allow_tstr_c:
        return QStringLiteral("allow");
    case ::contact_permission_r::contact_permission_Deny_tstr_c:
        return QStringLiteral("deny");
    case ::contact_permission_r::contact_permission_Unset_tstr_c:
        return {};
    }
    return {};
}

// presence.primitive enum → canonical string.
QString presencePrimitive(const ::presence_primitive_t_r& p) {
    switch (p.presence_primitive_t_choice) {
    case ::presence_primitive_t_r::presence_primitive_t_Offline_tstr_c:
        return QStringLiteral("offline");
    case ::presence_primitive_t_r::presence_primitive_t_Available_tstr_c:
        return QStringLiteral("available");
    case ::presence_primitive_t_r::presence_primitive_t_Idle_tstr_c:
        return QStringLiteral("idle");
    case ::presence_primitive_t_r::presence_primitive_t_Invisible_tstr_c:
        return QStringLiteral("invisible");
    case ::presence_primitive_t_r::presence_primitive_t_Away_tstr_c:
        return QStringLiteral("away");
    case ::presence_primitive_t_r::presence_primitive_t_DoNotDisturb_tstr_c:
        return QStringLiteral("dnd");
    case ::presence_primitive_t_r::presence_primitive_t_Streaming_tstr_c:
        return QStringLiteral("streaming");
    case ::presence_primitive_t_r::presence_primitive_t_OutOfOffice_tstr_c:
        return QStringLiteral("out_of_office");
    }
    return {};
}

// content-hash ([* uint] byte list) → lowercase hex string (the #blob_hash derivation, §3.3).
QString contentHashHex(const ::content_hash& h) {
    QString out;
    out.reserve(static_cast<qsizetype>(h.content_hash_uint_count) * 2);
    for (size_t i = 0; i < h.content_hash_uint_count; ++i) {
        out += QString::asprintf("%02x", static_cast<unsigned>(h.content_hash_uint[i]) & 0xFFu);
    }
    return out;
}

// participant union → a display id (contact id, or agent profile␟member) — the #participant
// derivation named once here (§3.3).
QString participantDisplay(const ::participant_r& p) {
    if (p.participant_choice == ::participant_r::participant_contact_m_c) {
        return qstr(p.participant_contact_m.participant_contact_Contact.contact_info_id);
    }
    if (p.participant_choice == ::participant_r::participant_agent_m_c) {
        return qstr(p.participant_agent_m.Agent_profile);
    }
    return {};
}

} // namespace

AccessUser map_access_user(const ::access_user& in) {
    AccessUser out;
    (void)in;
    // TODO(mirror-map): populate AccessUser from wire per entity-map.toml provenance.
    return out;
}

Adapter map_adapter(const ::adapter_info& in) {
    Adapter out;
    (void)in;
    // TODO(mirror-map): populate Adapter from wire per entity-map.toml provenance.
    return out;
}

AgentEntry map_agent_entry(const ::agent_entry& in) {
    AgentEntry out;
    (void)in;
    // TODO(mirror-map): populate AgentEntry from wire per entity-map.toml provenance.
    return out;
}

Approval map_approval(const ::approval_info& in) {
    Approval out;
    (void)in;
    // TODO(mirror-map): populate Approval from wire per entity-map.toml provenance.
    return out;
}

CapsReport map_caps_report(const ::caps_report& in) {
    CapsReport out;
    (void)in;
    // TODO(mirror-map): populate CapsReport from wire per entity-map.toml provenance.
    return out;
}

ChatMessage map_chat_message(const ::chat_message& in) {
    // A4 arm: MessagesChanged → ConvHistory. Payload-derived fields only; the scope fields
    // (transport, conv, cursor) and rung-3 origin_op ride the ConvHistory request scope + the
    // journal-record envelope (entity-map provenance), set by the caller (§3.6 merging rule).
    ChatMessage out;
    if (in.chat_message_id_present &&
        in.chat_message_id.chat_message_id_choice == ::chat_message_id_r::chat_message_id_tstr_c) {
        out.id = qstr(in.chat_message_id.chat_message_id_tstr);
    }
    if (in.chat_message_author_present &&
        in.chat_message_author.chat_message_author_choice ==
            ::chat_message_author_r::chat_message_author_participant_m_c) {
        out.author = participantDisplay(in.chat_message_author.chat_message_author_participant_m);
    }
    if (in.chat_message_replying_to_present &&
        in.chat_message_replying_to.chat_message_replying_to_choice ==
            ::chat_message_replying_to_r::chat_message_replying_to_tstr_c) {
        out.replying_to = qstr(in.chat_message_replying_to.chat_message_replying_to_tstr);
    }
    out.text = qstr(in.chat_message_text);
    if (in.chat_message_timestamp_present &&
        in.chat_message_timestamp.chat_message_timestamp_choice ==
            ::chat_message_timestamp_r::chat_message_timestamp_uint64_m_c) {
        out.timestamp = in.chat_message_timestamp.chat_message_timestamp_uint64_m;
    }
    if (in.chat_message_edited_at_present &&
        in.chat_message_edited_at.chat_message_edited_at_choice ==
            ::chat_message_edited_at_r::chat_message_edited_at_uint64_m_c) {
        out.edited_at = in.chat_message_edited_at.chat_message_edited_at_uint64_m;
    }
    if (in.chat_message_error_present && in.chat_message_error.chat_message_error_choice ==
                                             ::chat_message_error_r::chat_message_error_tstr_c) {
        out.error = qstr(in.chat_message_error.chat_message_error_tstr);
    }
    if (in.chat_message_attachments_present) {
        out.attachment_count = static_cast<int>(
            in.chat_message_attachments.chat_message_attachments_message_attachment_m_count);
    }
    return out;
}

Checkpoint map_checkpoint(const ::checkpoint_info& in) {
    Checkpoint out;
    (void)in;
    // TODO(mirror-map): populate Checkpoint from wire per entity-map.toml provenance.
    return out;
}

CommandSpec map_command_spec(const ::command_spec& in) {
    CommandSpec out;
    (void)in;
    // TODO(mirror-map): populate CommandSpec from wire per entity-map.toml provenance.
    return out;
}

Contact map_contact(const ::contact_info& in) {
    // A4 arm: ContactsChanged → RosterList. Note `transport` is the RosterList request scope
    // (entity-map: request-roster-list.RosterList#transport) — the caller sets it; the payload
    // carries only the id + optional fields.
    Contact out;
    out.id = qstr(in.contact_info_id);
    if (in.contact_info_display_name_present &&
        in.contact_info_display_name.contact_info_display_name_choice ==
            ::contact_info_display_name_r::contact_info_display_name_tstr_c) {
        out.display_name = qstr(in.contact_info_display_name.contact_info_display_name_tstr);
    }
    if (in.contact_info_permission_present) {
        out.permission = contactPermission(in.contact_info_permission.contact_info_permission);
    }
    if (in.contact_info_presence_present) {
        out.presence_primitive =
            presencePrimitive(in.contact_info_presence.contact_info_presence.presence_primitive);
    }
    return out;
}

Conversation map_conversation(const ::conversation_info& in) {
    // A4 arms: ConversationsChanged/MembershipChanged → ConvGet; connect → ConvList.
    Conversation out;
    out.transport = qstr(in.conversation_info_transport);
    out.id = qstr(in.conversation_info_id);
    out.kind = conversationKind(in.conversation_info_kind);
    if (in.conversation_info_title_present &&
        in.conversation_info_title.conversation_info_title_choice ==
            ::conversation_info_title_r::conversation_info_title_tstr_c) {
        out.title = qstr(in.conversation_info_title.conversation_info_title_tstr);
    }
    if (in.conversation_info_topic_present &&
        in.conversation_info_topic.conversation_info_topic_choice ==
            ::conversation_info_topic_r::conversation_info_topic_tstr_c) {
        out.topic = qstr(in.conversation_info_topic.conversation_info_topic_tstr);
    }
    if (in.conversation_info_description_present &&
        in.conversation_info_description.conversation_info_description_choice ==
            ::conversation_info_description_r::conversation_info_description_tstr_c) {
        out.description = qstr(in.conversation_info_description.conversation_info_description_tstr);
    }
    if (in.conversation_info_parent_present &&
        in.conversation_info_parent.conversation_info_parent_choice ==
            ::conversation_info_parent_r::conversation_info_parent_tstr_c) {
        out.parent = qstr(in.conversation_info_parent.conversation_info_parent_tstr);
    }
    // member_count: derived-at-map #count over the members list (entity-map provenance).
    if (in.conversation_info_members_present) {
        out.member_count = static_cast<int>(
            in.conversation_info_members.conversation_info_members_conversation_member_m_count);
    }
    return out;
}

ConversationMember map_conversation_member(const ::conversation_member& in) {
    ConversationMember out;
    (void)in;
    // TODO(mirror-map): populate ConversationMember from wire per entity-map.toml provenance.
    return out;
}

Credential map_credential(const ::credential_info& in) {
    Credential out;
    (void)in;
    // TODO(mirror-map): populate Credential from wire per entity-map.toml provenance.
    return out;
}

CronJob map_cron_job(const ::cron_job& in) {
    CronJob out;
    (void)in;
    // TODO(mirror-map): populate CronJob from wire per entity-map.toml provenance.
    return out;
}

CustomProvider map_custom_provider(const ::custom_provider& in) {
    CustomProvider out;
    (void)in;
    // TODO(mirror-map): populate CustomProvider from wire per entity-map.toml provenance.
    return out;
}

FleetUnit map_fleet_unit(const ::unit_node& in) {
    FleetUnit out;
    (void)in;
    // TODO(mirror-map): populate FleetUnit from wire per entity-map.toml provenance.
    return out;
}

FsEntry map_fs_entry(const ::fs_entry& in) {
    FsEntry out;
    (void)in;
    // TODO(mirror-map): populate FsEntry from wire per entity-map.toml provenance.
    return out;
}

GatewayStatus map_gateway_status(const ::gateway_status& in) {
    GatewayStatus out;
    (void)in;
    // TODO(mirror-map): populate GatewayStatus from wire per entity-map.toml provenance.
    return out;
}

InstalledModel map_installed_model(const ::installed_model& in) {
    InstalledModel out;
    (void)in;
    // TODO(mirror-map): populate InstalledModel from wire per entity-map.toml provenance.
    return out;
}

ModelDownload map_model_download(const ::download_status& in) {
    ModelDownload out;
    (void)in;
    // TODO(mirror-map): populate ModelDownload from wire per entity-map.toml provenance.
    return out;
}

Notification map_notification(const ::notification_info& in) {
    Notification out;
    (void)in;
    // TODO(mirror-map): populate Notification from wire per entity-map.toml provenance.
    return out;
}

Person map_person(const ::person& in) {
    // A4 arm: PersonsChanged → PersonList.
    Person out;
    out.id = qstr(in.person_id);
    if (in.person_alias_present &&
        in.person_alias.person_alias_choice == ::person_alias_r::person_alias_tstr_c) {
        out.alias = qstr(in.person_alias.person_alias_tstr);
    }
    // avatar#blob_hash: the image blob content-hash (entity-map derivation). The blob_ref hash is
    // the stable identity; hex string when present.
    if (in.person_avatar_present &&
        in.person_avatar.person_avatar_choice == ::person_avatar_r::person_avatar_image_m_c) {
        out.avatar_blob =
            contentHashHex(in.person_avatar.person_avatar_image_m.image_blob.blob_ref_hash);
    }
    if (in.person_endpoints_present) {
        out.endpoint_count =
            static_cast<int>(in.person_endpoints.person_endpoints_person_endpoint_m_count);
    }
    return out;
}

PersonEndpoint map_person_endpoint(const ::person_endpoint& in) {
    PersonEndpoint out;
    (void)in;
    // TODO(mirror-map): populate PersonEndpoint from wire per entity-map.toml provenance.
    return out;
}

Profile map_profile(const ::profile_info& in) {
    Profile out;
    (void)in;
    // TODO(mirror-map): populate Profile from wire per entity-map.toml provenance.
    return out;
}

ProviderDescriptor map_provider_descriptor(const ::provider_descriptor& in) {
    ProviderDescriptor out;
    (void)in;
    // TODO(mirror-map): populate ProviderDescriptor from wire per entity-map.toml provenance.
    return out;
}

QuantizeJob map_quantize_job(const ::quantize_status& in) {
    QuantizeJob out;
    (void)in;
    // TODO(mirror-map): populate QuantizeJob from wire per entity-map.toml provenance.
    return out;
}

RememberedFingerprint map_remembered_fingerprint(const ::remembered_fingerprint& in) {
    RememberedFingerprint out;
    (void)in;
    // TODO(mirror-map): populate RememberedFingerprint from wire per entity-map.toml provenance.
    return out;
}

RoleInfo map_role_info(const ::role_info& in) {
    RoleInfo out;
    (void)in;
    // TODO(mirror-map): populate RoleInfo from wire per entity-map.toml provenance.
    return out;
}

Room map_room(const ::room_info& in) {
    Room out;
    (void)in;
    // TODO(mirror-map): populate Room from wire per entity-map.toml provenance.
    return out;
}

RoutePin map_route_pin(const ::chat_route& in) {
    RoutePin out;
    (void)in;
    // TODO(mirror-map): populate RoutePin from wire per entity-map.toml provenance.
    return out;
}

SavedPresence map_saved_presence(const ::saved_presence& in) {
    SavedPresence out;
    (void)in;
    // TODO(mirror-map): populate SavedPresence from wire per entity-map.toml provenance.
    return out;
}

Session map_session(const ::session_info& in) {
    Session out;
    (void)in;
    // TODO(mirror-map): populate Session from wire per entity-map.toml provenance.
    return out;
}

ToolInfo map_tool_info(const ::tool_info& in) {
    ToolInfo out;
    (void)in;
    // TODO(mirror-map): populate ToolInfo from wire per entity-map.toml provenance.
    return out;
}

TranscriptBlock map_transcript_block(const ::journal_record& in) {
    TranscriptBlock out;
    (void)in;
    // TODO(mirror-map): populate TranscriptBlock from wire per entity-map.toml provenance.
    return out;
}

TransportAccount map_transport_account(const ::transport_instance_info& in) {
    TransportAccount out;
    (void)in;
    // TODO(mirror-map): populate TransportAccount from wire per entity-map.toml provenance.
    return out;
}

} // namespace mirror
