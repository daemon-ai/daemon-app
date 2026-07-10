// ONE-TIME skeleton — bodies are human-owned; regeneration NEVER overwrites this file.
// The drift gate checks that every declared mapper (entities_map_gen.h) has a matching
// definition here with the current codec signature — never the body contents.
//
// Each body maps a decoded wire DTO to the canonical entity per the provenance in
// src/core/mirror/entity-map.toml (merging extra reads / scope context as needed).

#include "entities_map_gen.h"

namespace mirror {

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
    ChatMessage out;
    (void)in;
    // TODO(mirror-map): populate ChatMessage from wire per entity-map.toml provenance.
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
    Contact out;
    (void)in;
    // TODO(mirror-map): populate Contact from wire per entity-map.toml provenance.
    return out;
}

Conversation map_conversation(const ::conversation_info& in) {
    Conversation out;
    (void)in;
    // TODO(mirror-map): populate Conversation from wire per entity-map.toml provenance.
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
    Person out;
    (void)in;
    // TODO(mirror-map): populate Person from wire per entity-map.toml provenance.
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
