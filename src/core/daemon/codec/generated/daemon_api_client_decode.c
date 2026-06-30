/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 64
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "daemon_api_client_decode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 64
#error "The type file was generated with a different default_max_qty than this file"
#endif

#define log_result(state, result, func) do { \
	if (!result) { \
		zcbor_trace_file(state); \
		zcbor_log("%s error: %s\r\n", func, zcbor_error_str(zcbor_peek_error(state))); \
	} else { \
		zcbor_log("%s success\r\n", func); \
	} \
} while(0)

static bool decode_content_hash(zcbor_state_t *state, struct content_hash *result);
static bool decode_repeated_blob_ref_name(zcbor_state_t *state, struct blob_ref_name_r *result);
static bool decode_repeated_blob_ref_mime(zcbor_state_t *state, struct blob_ref_mime_r *result);
static bool decode_blob_ref(zcbor_state_t *state, struct blob_ref *result);
static bool decode_repeated_user_msg_attachments(zcbor_state_t *state, struct user_msg_attachments_r *result);
static bool decode_user_msg(zcbor_state_t *state, struct user_msg *result);
static bool decode_agent_command_start_turn(zcbor_state_t *state, struct agent_command_start_turn *result);
static bool decode_agent_command_steer(zcbor_state_t *state, struct agent_command_steer *result);
static bool decode_agent_command_observe(zcbor_state_t *state, struct agent_command_observe *result);
static bool decode_agent_command_interrupt(zcbor_state_t *state, struct agent_command_interrupt *result);
static bool decode_agent_command_snapshot(zcbor_state_t *state, struct agent_command_snapshot *result);
static bool decode_rewind_anchor_user_turn(zcbor_state_t *state, struct rewind_anchor_user_turn *result);
static bool decode_rewind_anchor_reply_after(zcbor_state_t *state, struct rewind_anchor_reply_after *result);
static bool decode_rewind_anchor_cursor(zcbor_state_t *state, struct rewind_anchor_cursor *result);
static bool decode_rewind_anchor(zcbor_state_t *state, struct rewind_anchor_r *result);
static bool decode_agent_command_rewind_to(zcbor_state_t *state, struct agent_command_rewind_to *result);
static bool decode_agent_command(zcbor_state_t *state, struct agent_command_r *result);
static bool decode_origin_scope_dm(zcbor_state_t *state, struct origin_scope_dm *result);
static bool decode_origin_scope_group(zcbor_state_t *state, struct origin_scope_group *result);
static bool decode_origin_scope_api(zcbor_state_t *state, struct origin_scope_api *result);
static bool decode_origin_scope_t(zcbor_state_t *state, struct origin_scope_t_r *result);
static bool decode_origin(zcbor_state_t *state, struct origin *result);
static bool decode_repeated_Submit_origin(zcbor_state_t *state, struct Submit_origin_r *result);
static bool decode_repeated_Submit_profile(zcbor_state_t *state, struct Submit_profile_r *result);
static bool decode_request_submit(zcbor_state_t *state, struct request_submit *result);
static bool decode_request_submit_routed(zcbor_state_t *state, struct request_submit_routed *result);
static bool decode_request_poll(zcbor_state_t *state, struct request_poll *result);
static bool decode_host_response_body_approved(zcbor_state_t *state, struct host_response_body_approved *result);
static bool decode_host_response_body_input(zcbor_state_t *state, struct host_response_body_input *result);
static bool decode_host_response_body_chosen(zcbor_state_t *state, struct host_response_body_chosen *result);
static bool decode_host_response_body_delegated(zcbor_state_t *state, struct host_response_body_delegated *result);
static bool decode_host_response_body_spawned(zcbor_state_t *state, struct host_response_body_spawned *result);
static bool decode_host_response_body_deferred(zcbor_state_t *state, struct host_response_body_deferred *result);
static bool decode_host_response_body_t(zcbor_state_t *state, struct host_response_body_t_r *result);
static bool decode_host_response(zcbor_state_t *state, struct host_response *result);
static bool decode_request_respond(zcbor_state_t *state, struct request_respond *result);
static bool decode_request_assign(zcbor_state_t *state, struct request_assign *result);
static bool decode_request_cancel(zcbor_state_t *state, struct request_cancel *result);
static bool decode_request_unit(zcbor_state_t *state, struct request_unit *result);
static bool decode_request_unit_events(zcbor_state_t *state, struct request_unit_events *result);
static bool decode_request_unit_outbound(zcbor_state_t *state, struct request_unit_outbound *result);
static bool decode_request_session_history(zcbor_state_t *state, struct request_session_history *result);
static bool decode_request_subscribe(zcbor_state_t *state, struct request_subscribe *result);
static bool decode_repeated_EventsSince_wait_ms(zcbor_state_t *state, struct EventsSince_wait_ms_r *result);
static bool decode_request_events_since(zcbor_state_t *state, struct request_events_since *result);
static bool decode_request_delivery_targets(zcbor_state_t *state, struct request_delivery_targets *result);
static bool decode_request_delivery_sessions(zcbor_state_t *state, struct request_delivery_sessions *result);
static bool decode_sink_kind(zcbor_state_t *state, struct sink_kind_r *result);
static bool decode_delivery_target(zcbor_state_t *state, struct delivery_target *result);
static bool decode_request_handover(zcbor_state_t *state, struct request_handover *result);
static bool decode_record_meta_args(zcbor_state_t *state, struct record_meta_args *result);
static bool decode_request_record_meta(zcbor_state_t *state, struct request_record_meta *result);
static bool decode_request_unit_history(zcbor_state_t *state, struct request_unit_history *result);
static bool decode_request_pause(zcbor_state_t *state, struct request_pause *result);
static bool decode_request_resume(zcbor_state_t *state, struct request_resume *result);
static bool decode_request_scale(zcbor_state_t *state, struct request_scale *result);
static bool decode_provider_selector(zcbor_state_t *state, struct provider_selector_r *result);
static bool decode_repeated_SetSessionModel_provider(zcbor_state_t *state, struct SetSessionModel_provider_r *result);
static bool decode_request_set_session_model(zcbor_state_t *state, struct request_set_session_model *result);
static bool decode_approval_mode(zcbor_state_t *state, struct approval_mode_r *result);
static bool decode_request_set_session_mode(zcbor_state_t *state, struct request_set_session_mode *result);
static bool decode_repeated_session_overlay_model(zcbor_state_t *state, struct session_overlay_model_r *result);
static bool decode_repeated_session_overlay_provider(zcbor_state_t *state, struct session_overlay_provider_r *result);
static bool decode_tools_override_allowlist(zcbor_state_t *state, struct tools_override_allowlist *result);
static bool decode_tools_override(zcbor_state_t *state, struct tools_override_r *result);
static bool decode_repeated_session_overlay_tool_allowlist(zcbor_state_t *state, struct session_overlay_tool_allowlist *result);
static bool decode_repeated_session_overlay_approval_mode(zcbor_state_t *state, struct session_overlay_approval_mode_r *result);
static bool decode_workspace_binding_bound(zcbor_state_t *state, struct workspace_binding_bound *result);
static bool decode_workspace_binding(zcbor_state_t *state, struct workspace_binding_r *result);
static bool decode_repeated_session_overlay_workspace(zcbor_state_t *state, struct session_overlay_workspace_r *result);
static bool decode_session_overlay(zcbor_state_t *state, struct session_overlay *result);
static bool decode_request_set_session_overlay(zcbor_state_t *state, struct request_set_session_overlay *result);
static bool decode_repeated_ApprovalsPending_session(zcbor_state_t *state, struct ApprovalsPending_session_r *result);
static bool decode_request_approvals_pending(zcbor_state_t *state, struct request_approvals_pending *result);
static bool decode_request_approval_decide(zcbor_state_t *state, struct request_approval_decide *result);
static bool decode_request_profile_get(zcbor_state_t *state, struct request_profile_get *result);
static bool decode_budget(zcbor_state_t *state, struct budget *result);
static bool decode_engine_tunables(zcbor_state_t *state, struct engine_tunables *result);
static bool decode_context_engine_sel(zcbor_state_t *state, struct context_engine_sel_r *result);
static bool decode_memory_provider_sel(zcbor_state_t *state, struct memory_provider_sel_r *result);
static bool decode_bound_account(zcbor_state_t *state, struct bound_account *result);
static bool decode_profile_spec(zcbor_state_t *state, struct profile_spec *result);
static bool decode_request_profile_create(zcbor_state_t *state, struct request_profile_create *result);
static bool decode_request_profile_update(zcbor_state_t *state, struct request_profile_update *result);
static bool decode_request_profile_delete(zcbor_state_t *state, struct request_profile_delete *result);
static bool decode_request_profile_select(zcbor_state_t *state, struct request_profile_select *result);
static bool decode_request_profile_clone(zcbor_state_t *state, struct request_profile_clone *result);
static bool decode_request_profile_export(zcbor_state_t *state, struct request_profile_export *result);
static bool decode_repeated_files_tstrtstr(zcbor_state_t *state, struct files_tstrtstr *result);
static bool decode_skill_bundle(zcbor_state_t *state, struct skill_bundle *result);
static bool decode_repeated_distribution_head_seq(zcbor_state_t *state, struct distribution_head_seq_r *result);
static bool decode_repeated_distribution_source(zcbor_state_t *state, struct distribution_source_r *result);
static bool decode_distribution(zcbor_state_t *state, struct distribution *result);
static bool decode_repeated_ProfileImport_new_id(zcbor_state_t *state, struct ProfileImport_new_id_r *result);
static bool decode_request_profile_import(zcbor_state_t *state, struct request_profile_import *result);
static bool decode_request_profile_history(zcbor_state_t *state, struct request_profile_history *result);
static bool decode_request_profile_at(zcbor_state_t *state, struct request_profile_at *result);
static bool decode_request_profile_revert(zcbor_state_t *state, struct request_profile_revert *result);
static bool decode_request_skill_history(zcbor_state_t *state, struct request_skill_history *result);
static bool decode_request_skill_at(zcbor_state_t *state, struct request_skill_at *result);
static bool decode_request_skill_revert(zcbor_state_t *state, struct request_skill_revert *result);
static bool decode_repeated_CuratorList_profile(zcbor_state_t *state, struct CuratorList_profile_r *result);
static bool decode_request_curator_list(zcbor_state_t *state, struct request_curator_list *result);
static bool decode_repeated_CuratorPin_profile(zcbor_state_t *state, struct CuratorPin_profile_r *result);
static bool decode_request_curator_pin(zcbor_state_t *state, struct request_curator_pin *result);
static bool decode_repeated_CuratorUnpin_profile(zcbor_state_t *state, struct CuratorUnpin_profile_r *result);
static bool decode_request_curator_unpin(zcbor_state_t *state, struct request_curator_unpin *result);
static bool decode_repeated_CuratorArchive_profile(zcbor_state_t *state, struct CuratorArchive_profile_r *result);
static bool decode_request_curator_archive(zcbor_state_t *state, struct request_curator_archive *result);
static bool decode_repeated_CuratorRestore_profile(zcbor_state_t *state, struct CuratorRestore_profile_r *result);
static bool decode_request_curator_restore(zcbor_state_t *state, struct request_curator_restore *result);
static bool decode_repeated_CuratorRun_profile(zcbor_state_t *state, struct CuratorRun_profile_r *result);
static bool decode_request_curator_run(zcbor_state_t *state, struct request_curator_run *result);
static bool decode_request_credential_set(zcbor_state_t *state, struct request_credential_set *result);
static bool decode_request_credential_remove(zcbor_state_t *state, struct request_credential_remove *result);
static bool decode_model_engine(zcbor_state_t *state, struct model_engine_r *result);
static bool decode_search_sort(zcbor_state_t *state, struct search_sort_r *result);
static bool decode_search_query(zcbor_state_t *state, struct search_query *result);
static bool decode_request_model_search(zcbor_state_t *state, struct request_model_search *result);
static bool decode_request_model_files(zcbor_state_t *state, struct request_model_files *result);
static bool decode_model_source_hf(zcbor_state_t *state, struct model_source_hf *result);
static bool decode_model_source_local(zcbor_state_t *state, struct model_source_local *result);
static bool decode_model_source(zcbor_state_t *state, struct model_source_r *result);
static bool decode_model_ref(zcbor_state_t *state, struct model_ref *result);
static bool decode_request_model_download(zcbor_state_t *state, struct request_model_download *result);
static bool decode_request_model_cancel(zcbor_state_t *state, struct request_model_cancel *result);
static bool decode_request_model_pause(zcbor_state_t *state, struct request_model_pause *result);
static bool decode_request_model_resume(zcbor_state_t *state, struct request_model_resume *result);
static bool decode_request_model_delete(zcbor_state_t *state, struct request_model_delete *result);
static bool decode_request_model_activate(zcbor_state_t *state, struct request_model_activate *result);
static bool decode_model_recommend_args(zcbor_state_t *state, struct model_recommend_args *result);
static bool decode_request_model_recommend(zcbor_state_t *state, struct request_model_recommend *result);
static bool decode_model_quantize_args(zcbor_state_t *state, struct model_quantize_args *result);
static bool decode_request_model_quantize(zcbor_state_t *state, struct request_model_quantize *result);
static bool decode_request_model_inspect(zcbor_state_t *state, struct request_model_inspect *result);
static bool decode_request_model_current(zcbor_state_t *state, struct request_model_current *result);
static bool decode_repeated_params_tstrtstr(zcbor_state_t *state, struct params_tstrtstr *result);
static bool decode_auth_bind_request(zcbor_state_t *state, struct auth_bind_request *result);
static bool decode_repeated_auth_begin_request_bind(zcbor_state_t *state, struct auth_begin_request_bind_r *result);
static bool decode_auth_begin_request(zcbor_state_t *state, struct auth_begin_request *result);
static bool decode_request_auth_begin(zcbor_state_t *state, struct request_auth_begin *result);
static bool decode_auth_complete_request(zcbor_state_t *state, struct auth_complete_request *result);
static bool decode_request_auth_complete(zcbor_state_t *state, struct request_auth_complete *result);
static bool decode_request_auth_cancel(zcbor_state_t *state, struct request_auth_cancel *result);
static bool decode_repeated_CheckpointList_session(zcbor_state_t *state, struct CheckpointList_session_r *result);
static bool decode_request_checkpoint_list(zcbor_state_t *state, struct request_checkpoint_list *result);
static bool decode_request_checkpoint_rewind(zcbor_state_t *state, struct request_checkpoint_rewind *result);
static bool decode_session_scope_by_profile(zcbor_state_t *state, struct session_scope_by_profile *result);
static bool decode_session_scope_by_transport(zcbor_state_t *state, struct session_scope_by_transport *result);
static bool decode_session_scope(zcbor_state_t *state, struct session_scope_r *result);
static bool decode_repeated_session_query_scope(zcbor_state_t *state, struct session_query_scope *result);
static bool decode_repeated_session_query_after(zcbor_state_t *state, struct session_query_after_r *result);
static bool decode_repeated_session_query_limit(zcbor_state_t *state, struct session_query_limit *result);
static bool decode_repeated_session_query_since_rev(zcbor_state_t *state, struct session_query_since_rev_r *result);
static bool decode_session_query(zcbor_state_t *state, struct session_query *result);
static bool decode_request_sessions_query(zcbor_state_t *state, struct request_sessions_query *result);
static bool decode_request_session_get(zcbor_state_t *state, struct request_session_get *result);
static bool decode_request_session_search(zcbor_state_t *state, struct request_session_search *result);
static bool decode_repeated_session_meta_patch_title(zcbor_state_t *state, struct session_meta_patch_title_r *result);
static bool decode_repeated_session_meta_patch_pinned(zcbor_state_t *state, struct session_meta_patch_pinned_r *result);
static bool decode_repeated_session_meta_patch_archived(zcbor_state_t *state, struct session_meta_patch_archived_r *result);
static bool decode_session_meta_patch(zcbor_state_t *state, struct session_meta_patch *result);
static bool decode_request_session_update_meta(zcbor_state_t *state, struct request_session_update_meta *result);
static bool decode_repeated_rewind_point_restore_workspace(zcbor_state_t *state, struct rewind_point_restore_workspace *result);
static bool decode_rewind_point(zcbor_state_t *state, struct rewind_point *result);
static bool decode_request_rewind(zcbor_state_t *state, struct request_rewind *result);
static bool decode_repeated_acp_recipe_program(zcbor_state_t *state, struct acp_recipe_program_r *result);
static bool decode_repeated_acp_recipe_args(zcbor_state_t *state, struct acp_recipe_args_r *result);
static bool decode_kv_pair(zcbor_state_t *state, struct kv_pair *result);
static bool decode_repeated_acp_recipe_env(zcbor_state_t *state, struct acp_recipe_env_r *result);
static bool decode_repeated_acp_recipe_endpoint(zcbor_state_t *state, struct acp_recipe_endpoint_r *result);
static bool decode_acp_recipe(zcbor_state_t *state, struct acp_recipe *result);
static bool decode_acp_source(zcbor_state_t *state, struct acp_source_r *result);
static bool decode_repeated_acp_agent_entry_installed(zcbor_state_t *state, struct acp_agent_entry_installed *result);
static bool decode_repeated_acp_agent_entry_version(zcbor_state_t *state, struct acp_agent_entry_version_r *result);
static bool decode_repeated_acp_agent_entry_capabilities(zcbor_state_t *state, struct acp_agent_entry_capabilities_r *result);
static bool decode_acp_agent_entry(zcbor_state_t *state, struct acp_agent_entry *result);
static bool decode_request_acp_register(zcbor_state_t *state, struct request_acp_register *result);
static bool decode_request_acp_remove(zcbor_state_t *state, struct request_acp_remove *result);
static bool decode_request_skill_get(zcbor_state_t *state, struct request_skill_get *result);
static bool decode_request_skill_put(zcbor_state_t *state, struct request_skill_put *result);
static bool decode_repeated_provider_info_base_url(zcbor_state_t *state, struct provider_info_base_url_r *result);
static bool decode_repeated_provider_info_available(zcbor_state_t *state, struct provider_info_available *result);
static bool decode_provider_info(zcbor_state_t *state, struct provider_info *result);
static bool decode_request_provider_register(zcbor_state_t *state, struct request_provider_register *result);
static bool decode_repeated_tool_info_description(zcbor_state_t *state, struct tool_info_description_r *result);
static bool decode_tool_info(zcbor_state_t *state, struct tool_info *result);
static bool decode_request_tool_register(zcbor_state_t *state, struct request_tool_register *result);
static bool decode_command_invocation(zcbor_state_t *state, struct command_invocation *result);
static bool decode_request_command_invoke(zcbor_state_t *state, struct request_command_invoke *result);
static bool decode_node_config_view(zcbor_state_t *state, struct node_config_view *result);
static bool decode_request_config_set(zcbor_state_t *state, struct request_config_set *result);
static bool decode_repeated_cron_spec_target(zcbor_state_t *state, struct cron_spec_target_r *result);
static bool decode_repeated_cron_spec_payload(zcbor_state_t *state, struct cron_spec_payload *result);
static bool decode_repeated_cron_spec_enabled(zcbor_state_t *state, struct cron_spec_enabled *result);
static bool decode_repeated_cron_spec_timezone(zcbor_state_t *state, struct cron_spec_timezone_r *result);
static bool decode_repeated_cron_spec_repeat(zcbor_state_t *state, struct cron_spec_repeat_r *result);
static bool decode_repeated_cron_spec_jitter_secs(zcbor_state_t *state, struct cron_spec_jitter_secs_r *result);
static bool decode_overlap_policy(zcbor_state_t *state, struct overlap_policy_r *result);
static bool decode_repeated_cron_spec_overlap(zcbor_state_t *state, struct cron_spec_overlap *result);
static bool decode_catch_up_policy(zcbor_state_t *state, struct catch_up_policy_r *result);
static bool decode_repeated_cron_spec_catch_up(zcbor_state_t *state, struct cron_spec_catch_up *result);
static bool decode_repeated_cron_spec_script(zcbor_state_t *state, struct cron_spec_script_r *result);
static bool decode_repeated_cron_spec_no_agent(zcbor_state_t *state, struct cron_spec_no_agent *result);
static bool decode_repeated_cron_spec_context_from(zcbor_state_t *state, struct cron_spec_context_from_r *result);
static bool decode_repeated_cron_spec_deliver(zcbor_state_t *state, struct cron_spec_deliver_r *result);
static bool decode_repeated_cron_spec_enabled_toolsets(zcbor_state_t *state, struct cron_spec_enabled_toolsets_r *result);
static bool decode_repeated_cron_spec_workdir(zcbor_state_t *state, struct cron_spec_workdir_r *result);
static bool decode_repeated_cron_spec_model(zcbor_state_t *state, struct cron_spec_model_r *result);
static bool decode_repeated_cron_spec_provider(zcbor_state_t *state, struct cron_spec_provider_r *result);
static bool decode_repeated_cron_spec_skills(zcbor_state_t *state, struct cron_spec_skills_r *result);
static bool decode_repeated_cron_spec_origin(zcbor_state_t *state, struct cron_spec_origin_r *result);
static bool decode_cron_spec(zcbor_state_t *state, struct cron_spec *result);
static bool decode_request_cron_create(zcbor_state_t *state, struct request_cron_create *result);
static bool decode_request_cron_update(zcbor_state_t *state, struct request_cron_update *result);
static bool decode_request_cron_delete(zcbor_state_t *state, struct request_cron_delete *result);
static bool decode_request_cron_trigger(zcbor_state_t *state, struct request_cron_trigger *result);
static bool decode_request_cron_runs(zcbor_state_t *state, struct request_cron_runs *result);
static bool decode_request_cron_pause(zcbor_state_t *state, struct request_cron_pause *result);
static bool decode_request_cron_accept_suggestion(zcbor_state_t *state, struct request_cron_accept_suggestion *result);
static bool decode_request_cron_dismiss_suggestion(zcbor_state_t *state, struct request_cron_dismiss_suggestion *result);
static bool decode_request_routing_get(zcbor_state_t *state, struct request_routing_get *result);
static bool decode_repeated_chat_route_profile(zcbor_state_t *state, struct chat_route_profile_r *result);
static bool decode_isolation_policy(zcbor_state_t *state, struct isolation_policy_r *result);
static bool decode_repeated_chat_route_isolation(zcbor_state_t *state, struct chat_route_isolation *result);
static bool decode_chat_route(zcbor_state_t *state, struct chat_route *result);
static bool decode_request_routing_set(zcbor_state_t *state, struct request_routing_set *result);
static bool decode_repeated_RoutingBindChat_profile(zcbor_state_t *state, struct RoutingBindChat_profile_r *result);
static bool decode_request_routing_bind_chat(zcbor_state_t *state, struct request_routing_bind_chat *result);
static bool decode_request_routing_unbind_chat(zcbor_state_t *state, struct request_routing_unbind_chat *result);
static bool decode_request_transport_rooms(zcbor_state_t *state, struct request_transport_rooms *result);
static bool decode_request_conv_list(zcbor_state_t *state, struct request_conv_list *result);
static bool decode_request_conv_get(zcbor_state_t *state, struct request_conv_get *result);
static bool decode_request_conv_create_details(zcbor_state_t *state, struct request_conv_create_details *result);
static bool decode_repeated_create_conversation_details_max_participants(zcbor_state_t *state, struct create_conversation_details_max_participants *result);
static bool decode_repeated_contact_info_display_name(zcbor_state_t *state, struct contact_info_display_name_r *result);
static bool decode_presence_primitive_t(zcbor_state_t *state, struct presence_primitive_t_r *result);
static bool decode_repeated_presence_message(zcbor_state_t *state, struct presence_message_r *result);
static bool decode_repeated_presence_emoji(zcbor_state_t *state, struct presence_emoji_r *result);
static bool decode_repeated_presence_mobile(zcbor_state_t *state, struct presence_mobile *result);
static bool decode_repeated_presence_idle_since(zcbor_state_t *state, struct presence_idle_since_r *result);
static bool decode_presence(zcbor_state_t *state, struct presence *result);
static bool decode_repeated_contact_info_presence(zcbor_state_t *state, struct contact_info_presence *result);
static bool decode_contact_permission(zcbor_state_t *state, struct contact_permission_r *result);
static bool decode_repeated_contact_info_permission(zcbor_state_t *state, struct contact_info_permission *result);
static bool decode_contact_info(zcbor_state_t *state, struct contact_info *result);
static bool decode_repeated_create_conversation_details_participants(zcbor_state_t *state, struct create_conversation_details_participants_r *result);
static bool decode_auth_param_field(zcbor_state_t *state, struct auth_param_field *result);
static bool decode_repeated_account_settings_schema_fields(zcbor_state_t *state, struct account_settings_schema_fields_r *result);
static bool decode_account_settings_schema(zcbor_state_t *state, struct account_settings_schema *result);
static bool decode_repeated_create_conversation_details_extras_schema(zcbor_state_t *state, struct create_conversation_details_extras_schema *result);
static bool decode_repeated_values_tstrtstr(zcbor_state_t *state, struct values_tstrtstr *result);
static bool decode_repeated_account_settings_values_values(zcbor_state_t *state, struct account_settings_values_values_r *result);
static bool decode_account_settings_values(zcbor_state_t *state, struct account_settings_values *result);
static bool decode_repeated_create_conversation_details_extras(zcbor_state_t *state, struct create_conversation_details_extras *result);
static bool decode_create_conversation_details(zcbor_state_t *state, struct create_conversation_details *result);
static bool decode_request_conv_create(zcbor_state_t *state, struct request_conv_create *result);
static bool decode_request_conv_join_details(zcbor_state_t *state, struct request_conv_join_details *result);
static bool decode_repeated_channel_join_details_name(zcbor_state_t *state, struct channel_join_details_name_r *result);
static bool decode_repeated_channel_join_details_name_max_length(zcbor_state_t *state, struct channel_join_details_name_max_length *result);
static bool decode_repeated_channel_join_details_nickname(zcbor_state_t *state, struct channel_join_details_nickname_r *result);
static bool decode_repeated_channel_join_details_nickname_supported(zcbor_state_t *state, struct channel_join_details_nickname_supported *result);
static bool decode_repeated_channel_join_details_nickname_max_length(zcbor_state_t *state, struct channel_join_details_nickname_max_length *result);
static bool decode_repeated_channel_join_details_password(zcbor_state_t *state, struct channel_join_details_password_r *result);
static bool decode_repeated_channel_join_details_password_supported(zcbor_state_t *state, struct channel_join_details_password_supported *result);
static bool decode_repeated_channel_join_details_password_max_length(zcbor_state_t *state, struct channel_join_details_password_max_length *result);
static bool decode_repeated_channel_join_details_extras_schema(zcbor_state_t *state, struct channel_join_details_extras_schema *result);
static bool decode_repeated_channel_join_details_extras(zcbor_state_t *state, struct channel_join_details_extras *result);
static bool decode_channel_join_details(zcbor_state_t *state, struct channel_join_details *result);
static bool decode_request_conv_join(zcbor_state_t *state, struct request_conv_join *result);
static bool decode_request_conv_leave(zcbor_state_t *state, struct request_conv_leave *result);
static bool decode_participant_contact(zcbor_state_t *state, struct participant_contact *result);
static bool decode_participant_agent(zcbor_state_t *state, struct participant_agent *result);
static bool decode_participant(zcbor_state_t *state, struct participant_r *result);
static bool decode_repeated_conv_send_args_from(zcbor_state_t *state, struct conv_send_args_from_r *result);
static bool decode_conv_send_args(zcbor_state_t *state, struct conv_send_args *result);
static bool decode_request_conv_send(zcbor_state_t *state, struct request_conv_send *result);
static bool decode_repeated_ConvSetTopic_topic(zcbor_state_t *state, struct ConvSetTopic_topic_r *result);
static bool decode_request_conv_set_topic(zcbor_state_t *state, struct request_conv_set_topic *result);
static bool decode_repeated_ConvSetTitle_title(zcbor_state_t *state, struct ConvSetTitle_title_r *result);
static bool decode_request_conv_set_title(zcbor_state_t *state, struct request_conv_set_title *result);
static bool decode_repeated_ConvSetDescription_description(zcbor_state_t *state, struct ConvSetDescription_description_r *result);
static bool decode_request_conv_set_description(zcbor_state_t *state, struct request_conv_set_description *result);
static bool decode_request_conv_delete(zcbor_state_t *state, struct request_conv_delete *result);
static bool decode_repeated_conv_history_args_after_cursor(zcbor_state_t *state, struct conv_history_args_after_cursor *result);
static bool decode_repeated_conv_history_args_max(zcbor_state_t *state, struct conv_history_args_max *result);
static bool decode_conv_history_args(zcbor_state_t *state, struct conv_history_args *result);
static bool decode_request_conv_history(zcbor_state_t *state, struct request_conv_history *result);
static bool decode_repeated_member_invite_args_message(zcbor_state_t *state, struct member_invite_args_message_r *result);
static bool decode_member_invite_args(zcbor_state_t *state, struct member_invite_args *result);
static bool decode_request_member_invite(zcbor_state_t *state, struct request_member_invite *result);
static bool decode_repeated_member_remove_args_reason(zcbor_state_t *state, struct member_remove_args_reason_r *result);
static bool decode_member_remove_args(zcbor_state_t *state, struct member_remove_args *result);
static bool decode_request_member_remove(zcbor_state_t *state, struct request_member_remove *result);
static bool decode_repeated_member_ban_args_reason(zcbor_state_t *state, struct member_ban_args_reason_r *result);
static bool decode_member_ban_args(zcbor_state_t *state, struct member_ban_args *result);
static bool decode_request_member_ban(zcbor_state_t *state, struct request_member_ban *result);
static bool decode_member_role(zcbor_state_t *state, struct member_role_r *result);
static bool decode_member_set_role_args(zcbor_state_t *state, struct member_set_role_args *result);
static bool decode_request_member_set_role(zcbor_state_t *state, struct request_member_set_role *result);
static bool decode_request_contact_get_profile(zcbor_state_t *state, struct request_contact_get_profile *result);
static bool decode_repeated_ContactSetAlias_alias(zcbor_state_t *state, struct ContactSetAlias_alias_r *result);
static bool decode_request_contact_set_alias(zcbor_state_t *state, struct request_contact_set_alias *result);
static bool decode_request_contact_action_menu(zcbor_state_t *state, struct request_contact_action_menu *result);
static bool decode_repeated_DirectorySearch_query(zcbor_state_t *state, struct DirectorySearch_query_r *result);
static bool decode_request_directory_search(zcbor_state_t *state, struct request_directory_search *result);
static bool decode_fs_root_id_host(zcbor_state_t *state, struct fs_root_id_host *result);
static bool decode_fs_root_id_session(zcbor_state_t *state, struct fs_root_id_session *result);
static bool decode_fs_root_id_t(zcbor_state_t *state, struct fs_root_id_t_r *result);
static bool decode_repeated_FsList_show_ignored(zcbor_state_t *state, struct FsList_show_ignored *result);
static bool decode_request_fs_list(zcbor_state_t *state, struct request_fs_list *result);
static bool decode_request_fs_stat(zcbor_state_t *state, struct request_fs_stat *result);
static bool decode_repeated_FsRead_max_bytes(zcbor_state_t *state, struct FsRead_max_bytes *result);
static bool decode_request_fs_read(zcbor_state_t *state, struct request_fs_read *result);
static bool decode_fs_revision(zcbor_state_t *state, struct fs_revision *result);
static bool decode_repeated_fs_write_args_base_revision(zcbor_state_t *state, struct fs_write_args_base_revision_r *result);
static bool decode_repeated_fs_write_args_force(zcbor_state_t *state, struct fs_write_args_force *result);
static bool decode_fs_write_args(zcbor_state_t *state, struct fs_write_args *result);
static bool decode_request_fs_write(zcbor_state_t *state, struct request_fs_write *result);
static bool decode_repeated_fs_search_query_regex(zcbor_state_t *state, struct fs_search_query_regex *result);
static bool decode_repeated_fs_search_query_case_sensitive(zcbor_state_t *state, struct fs_search_query_case_sensitive *result);
static bool decode_repeated_fs_search_query_max_results(zcbor_state_t *state, struct fs_search_query_max_results *result);
static bool decode_repeated_fs_search_query_page(zcbor_state_t *state, struct fs_search_query_page *result);
static bool decode_fs_search_query(zcbor_state_t *state, struct fs_search_query *result);
static bool decode_request_fs_search(zcbor_state_t *state, struct request_fs_search *result);
static bool decode_fs_watch_after_args(zcbor_state_t *state, struct fs_watch_after_args *result);
static bool decode_request_fs_watch_poll(zcbor_state_t *state, struct request_fs_watch_poll *result);
static bool decode_request_blob_put(zcbor_state_t *state, struct request_blob_put *result);
static bool decode_byte_range(zcbor_state_t *state, struct byte_range *result);
static bool decode_repeated_BlobGet_range(zcbor_state_t *state, struct BlobGet_range_r *result);
static bool decode_request_blob_get(zcbor_state_t *state, struct request_blob_get *result);
static bool decode_request_blob_stat(zcbor_state_t *state, struct request_blob_stat *result);
static bool decode_repeated_fs_write_from_blob_args_base_revision(zcbor_state_t *state, struct fs_write_from_blob_args_base_revision_r *result);
static bool decode_repeated_fs_write_from_blob_args_force(zcbor_state_t *state, struct fs_write_from_blob_args_force *result);
static bool decode_fs_write_from_blob_args(zcbor_state_t *state, struct fs_write_from_blob_args *result);
static bool decode_request_fs_write_from_blob(zcbor_state_t *state, struct request_fs_write_from_blob *result);
static bool decode_request_user_create(zcbor_state_t *state, struct request_user_create *result);
static bool decode_request_user_disable(zcbor_state_t *state, struct request_user_disable *result);
static bool decode_request_user_set_roles(zcbor_state_t *state, struct request_user_set_roles *result);
static bool decode_request_user_set_password(zcbor_state_t *state, struct request_user_set_password *result);
static bool decode_request_session_revoke(zcbor_state_t *state, struct request_session_revoke *result);
static bool decode_request_resource_grant_create(zcbor_state_t *state, struct request_resource_grant_create *result);
static bool decode_repeated_ResourceGrantList_user_id(zcbor_state_t *state, struct ResourceGrantList_user_id_r *result);
static bool decode_request_resource_grant_list(zcbor_state_t *state, struct request_resource_grant_list *result);
static bool decode_request_resource_grant_revoke(zcbor_state_t *state, struct request_resource_grant_revoke *result);
static bool decode_response_routed(zcbor_state_t *state, struct response_routed *result);
static bool decode_completion_source_process(zcbor_state_t *state, struct completion_source_process *result);
static bool decode_completion_source_delegation(zcbor_state_t *state, struct completion_source_delegation *result);
static bool decode_completion_source(zcbor_state_t *state, struct completion_source_r *result);
static bool decode_turn_trigger_background(zcbor_state_t *state, struct turn_trigger_background *result);
static bool decode_turn_trigger_scheduled(zcbor_state_t *state, struct turn_trigger_scheduled *result);
static bool decode_turn_trigger(zcbor_state_t *state, struct turn_trigger_r *result);
static bool decode_agent_event_turn_started(zcbor_state_t *state, struct agent_event_turn_started *result);
static bool decode_agent_event_text_delta(zcbor_state_t *state, struct agent_event_text_delta *result);
static bool decode_agent_event_reasoning_delta(zcbor_state_t *state, struct agent_event_reasoning_delta *result);
static bool decode_agent_event_content_delta(zcbor_state_t *state, struct agent_event_content_delta *result);
static bool decode_tool_detail(zcbor_state_t *state, struct tool_detail *result);
static bool decode_repeated_tool_call_view_detail(zcbor_state_t *state, struct tool_call_view_detail_r *result);
static bool decode_tool_call_view(zcbor_state_t *state, struct tool_call_view *result);
static bool decode_agent_event_tool_started(zcbor_state_t *state, struct agent_event_tool_started *result);
static bool decode_repeated_tool_result_view_detail(zcbor_state_t *state, struct tool_result_view_detail_r *result);
static bool decode_tool_result_view(zcbor_state_t *state, struct tool_result_view *result);
static bool decode_agent_event_tool_finished(zcbor_state_t *state, struct agent_event_tool_finished *result);
static bool decode_usage_delta(zcbor_state_t *state, struct usage_delta *result);
static bool decode_agent_event_usage(zcbor_state_t *state, struct agent_event_usage *result);
static bool decode_context_status(zcbor_state_t *state, struct context_status *result);
static bool decode_agent_event_context(zcbor_state_t *state, struct agent_event_context *result);
static bool decode_rate_limit_snapshot(zcbor_state_t *state, struct rate_limit_snapshot *result);
static bool decode_agent_event_rate_limit(zcbor_state_t *state, struct agent_event_rate_limit *result);
static bool decode_end_reason(zcbor_state_t *state, struct end_reason_r *result);
static bool decode_turn_summary(zcbor_state_t *state, struct turn_summary *result);
static bool decode_agent_event_turn_finished(zcbor_state_t *state, struct agent_event_turn_finished *result);
static bool decode_agent_event_error(zcbor_state_t *state, struct agent_event_error *result);
static bool decode_agent_event_steered(zcbor_state_t *state, struct agent_event_steered *result);
static bool decode_conv_turn_view(zcbor_state_t *state, struct conv_turn_view *result);
static bool decode_conv_view(zcbor_state_t *state, struct conv_view *result);
static bool decode_agent_event_snapshot(zcbor_state_t *state, struct agent_event_snapshot *result);
static bool decode_agent_event_rewound(zcbor_state_t *state, struct agent_event_rewound *result);
static bool decode_agent_event(zcbor_state_t *state, struct agent_event_r *result);
static bool decode_outbound_event(zcbor_state_t *state, struct outbound_event *result);
static bool decode_host_request_kind_approval(zcbor_state_t *state, struct host_request_kind_approval *result);
static bool decode_host_request_kind_input(zcbor_state_t *state, struct host_request_kind_input *result);
static bool decode_host_request_kind_choice(zcbor_state_t *state, struct host_request_kind_choice *result);
static bool decode_host_request_kind_delegate(zcbor_state_t *state, struct host_request_kind_delegate *result);
static bool decode_spawn_spec(zcbor_state_t *state, struct spawn_spec *result);
static bool decode_host_request_kind_spawn(zcbor_state_t *state, struct host_request_kind_spawn *result);
static bool decode_host_request_kind_t(zcbor_state_t *state, struct host_request_kind_t_r *result);
static bool decode_host_request(zcbor_state_t *state, struct host_request *result);
static bool decode_outbound_request(zcbor_state_t *state, struct outbound_request *result);
static bool decode_outbound(zcbor_state_t *state, struct outbound_r *result);
static bool decode_response_drained(zcbor_state_t *state, struct response_drained *result);
static bool decode_service_health(zcbor_state_t *state, struct service_health *result);
static bool decode_health_report(zcbor_state_t *state, struct health_report *result);
static bool decode_response_health(zcbor_state_t *state, struct response_health *result);
static bool decode_stats_report(zcbor_state_t *state, struct stats_report *result);
static bool decode_response_stats(zcbor_state_t *state, struct response_stats *result);
static bool decode_telemetry_dump(zcbor_state_t *state, struct telemetry_dump *result);
static bool decode_response_telemetry(zcbor_state_t *state, struct response_telemetry *result);
static bool decode_session_state_suspended(zcbor_state_t *state, struct session_state_suspended *result);
static bool decode_session_state(zcbor_state_t *state, struct session_state_r *result);
static bool decode_repeated_session_info_rewindable(zcbor_state_t *state, struct session_info_rewindable *result);
static bool decode_repeated_session_info_bound_profile(zcbor_state_t *state, struct session_info_bound_profile_r *result);
static bool decode_repeated_session_info_title(zcbor_state_t *state, struct session_info_title_r *result);
static bool decode_repeated_session_info_last_activity_ms(zcbor_state_t *state, struct session_info_last_activity_ms_r *result);
static bool decode_lifecycle(zcbor_state_t *state, struct lifecycle_r *result);
static bool decode_repeated_session_info_lifecycle(zcbor_state_t *state, struct session_info_lifecycle *result);
static bool decode_session_role(zcbor_state_t *state, struct session_role_r *result);
static bool decode_repeated_session_info_role(zcbor_state_t *state, struct session_info_role *result);
static bool decode_repeated_session_info_parent(zcbor_state_t *state, struct session_info_parent_r *result);
static bool decode_repeated_session_info_pinned(zcbor_state_t *state, struct session_info_pinned *result);
static bool decode_repeated_session_info_archived(zcbor_state_t *state, struct session_info_archived *result);
static bool decode_session_info(zcbor_state_t *state, struct session_info *result);
static bool decode_response_sessions(zcbor_state_t *state, struct response_sessions *result);
static bool decode_repeated_approval_info_path(zcbor_state_t *state, struct approval_info_path_r *result);
static bool decode_approval_info(zcbor_state_t *state, struct approval_info *result);
static bool decode_response_approvals(zcbor_state_t *state, struct response_approvals *result);
static bool decode_fleet_report(zcbor_state_t *state, struct fleet_report *result);
static bool decode_response_fleet(zcbor_state_t *state, struct response_fleet *result);
static bool decode_unit_kind(zcbor_state_t *state, struct unit_kind_r *result);
static bool decode_unit_state_finished(zcbor_state_t *state, struct unit_state_finished *result);
static bool decode_unit_state(zcbor_state_t *state, struct unit_state_r *result);
static bool decode_repeated_unit_node_profile(zcbor_state_t *state, struct unit_node_profile_r *result);
static bool decode_repeated_unit_node_session(zcbor_state_t *state, struct unit_node_session_r *result);
static bool decode_repeated_unit_node_title(zcbor_state_t *state, struct unit_node_title_r *result);
static bool decode_repeated_unit_node_role(zcbor_state_t *state, struct unit_node_role_r *result);
static bool decode_unit_node(zcbor_state_t *state, struct unit_node *result);
static bool decode_tree_report(zcbor_state_t *state, struct tree_report *result);
static bool decode_response_tree(zcbor_state_t *state, struct response_tree *result);
static bool decode_response_unit(zcbor_state_t *state, struct response_unit *result);
static bool decode_manage_event_view_started(zcbor_state_t *state, struct manage_event_view_started *result);
static bool decode_manage_event_view_progress(zcbor_state_t *state, struct manage_event_view_progress *result);
static bool decode_manage_event_view_usage(zcbor_state_t *state, struct manage_event_view_usage *result);
static bool decode_manage_event_view_finished(zcbor_state_t *state, struct manage_event_view_finished *result);
static bool decode_manage_event_view_error(zcbor_state_t *state, struct manage_event_view_error *result);
static bool decode_subagent_phase(zcbor_state_t *state, struct subagent_phase_r *result);
static bool decode_manage_event_view_subagent(zcbor_state_t *state, struct manage_event_view_subagent *result);
static bool decode_manage_event_view(zcbor_state_t *state, struct manage_event_view_r *result);
static bool decode_response_unit_events(zcbor_state_t *state, struct response_unit_events *result);
static bool decode_journal_record_payload_management(zcbor_state_t *state, struct journal_record_payload_management *result);
static bool decode_transcript_role(zcbor_state_t *state, struct transcript_role_r *result);
static bool decode_transcript_block_message(zcbor_state_t *state, struct transcript_block_message *result);
static bool decode_repeated_ToolCall_detail(zcbor_state_t *state, struct ToolCall_detail_r *result);
static bool decode_transcript_block_tool_call(zcbor_state_t *state, struct transcript_block_tool_call *result);
static bool decode_repeated_ToolResult_detail(zcbor_state_t *state, struct ToolResult_detail_r *result);
static bool decode_transcript_block_tool_result(zcbor_state_t *state, struct transcript_block_tool_result *result);
static bool decode_transcript_block_request(zcbor_state_t *state, struct transcript_block_request *result);
static bool decode_transcript_block_content(zcbor_state_t *state, struct transcript_block_content *result);
static bool decode_transcript_block(zcbor_state_t *state, struct transcript_block_r *result);
static bool decode_journal_record_payload_block(zcbor_state_t *state, struct journal_record_payload_block *result);
static bool decode_journal_record_payload_t(zcbor_state_t *state, struct journal_record_payload_t_r *result);
static bool decode_journal_record(zcbor_state_t *state, struct journal_record *result);
static bool decode_journal_page_view(zcbor_state_t *state, struct journal_page_view *result);
static bool decode_response_journal(zcbor_state_t *state, struct response_journal *result);
static bool decode_direction(zcbor_state_t *state, struct direction_r *result);
static bool decode_disposition(zcbor_state_t *state, struct disposition_r *result);
static bool decode_session_payload_event(zcbor_state_t *state, struct session_payload_event *result);
static bool decode_session_payload_request(zcbor_state_t *state, struct session_payload_request *result);
static bool decode_session_payload_command(zcbor_state_t *state, struct session_payload_command *result);
static bool decode_session_payload_response(zcbor_state_t *state, struct session_payload_response *result);
static bool decode_session_payload_meta(zcbor_state_t *state, struct session_payload_meta *result);
static bool decode_session_payload(zcbor_state_t *state, struct session_payload_r *result);
static bool decode_session_log_entry(zcbor_state_t *state, struct session_log_entry *result);
static bool decode_log_page_view(zcbor_state_t *state, struct log_page_view *result);
static bool decode_response_log_page(zcbor_state_t *state, struct response_log_page *result);
static bool decode_node_event_session_advanced(zcbor_state_t *state, struct node_event_session_advanced *result);
static bool decode_node_event_session_meta_changed(zcbor_state_t *state, struct node_event_session_meta_changed *result);
static bool decode_node_event_roster_changed(zcbor_state_t *state, struct node_event_roster_changed *result);
static bool decode_node_event_fleet_changed(zcbor_state_t *state, struct node_event_fleet_changed *result);
static bool decode_node_event_approval_pending(zcbor_state_t *state, struct node_event_approval_pending *result);
static bool decode_node_event_download_progress(zcbor_state_t *state, struct node_event_download_progress *result);
static bool decode_node_event_resync_needed(zcbor_state_t *state, struct node_event_resync_needed *result);
static bool decode_node_event(zcbor_state_t *state, struct node_event_r *result);
static bool decode_events_page(zcbor_state_t *state, struct events_page *result);
static bool decode_response_events_page(zcbor_state_t *state, struct response_events_page *result);
static bool decode_response_delivery_targets(zcbor_state_t *state, struct response_delivery_targets *result);
static bool decode_response_delivery_sessions(zcbor_state_t *state, struct response_delivery_sessions *result);
static bool decode_response_verifying_key(zcbor_state_t *state, struct response_verifying_key *result);
static bool decode_search_hit(zcbor_state_t *state, struct search_hit *result);
static bool decode_search_page(zcbor_state_t *state, struct search_page *result);
static bool decode_response_model_search(zcbor_state_t *state, struct response_model_search *result);
static bool decode_model_file(zcbor_state_t *state, struct model_file *result);
static bool decode_response_model_files(zcbor_state_t *state, struct response_model_files *result);
static bool decode_response_model_download_started(zcbor_state_t *state, struct response_model_download_started *result);
static bool decode_download_state(zcbor_state_t *state, struct download_state_r *result);
static bool decode_download_status(zcbor_state_t *state, struct download_status *result);
static bool decode_response_model_downloads(zcbor_state_t *state, struct response_model_downloads *result);
static bool decode_repeated_installed_model_arch(zcbor_state_t *state, struct installed_model_arch_r *result);
static bool decode_repeated_installed_model_context_length(zcbor_state_t *state, struct installed_model_context_length_r *result);
static bool decode_repeated_installed_model_file_type(zcbor_state_t *state, struct installed_model_file_type_r *result);
static bool decode_installed_model(zcbor_state_t *state, struct installed_model *result);
static bool decode_response_model_catalog(zcbor_state_t *state, struct response_model_catalog *result);
static bool decode_quant_candidate(zcbor_state_t *state, struct quant_candidate *result);
static bool decode_quant_recommendation(zcbor_state_t *state, struct quant_recommendation *result);
static bool decode_response_model_recommend(zcbor_state_t *state, struct response_model_recommend *result);
static bool decode_response_model_quantize_started(zcbor_state_t *state, struct response_model_quantize_started *result);
static bool decode_quantize_state(zcbor_state_t *state, struct quantize_state_r *result);
static bool decode_quantize_status(zcbor_state_t *state, struct quantize_status *result);
static bool decode_response_model_quantizes(zcbor_state_t *state, struct response_model_quantizes *result);
static bool decode_gguf_info(zcbor_state_t *state, struct gguf_info *result);
static bool decode_response_model_inspect(zcbor_state_t *state, struct response_model_inspect *result);
static bool decode_profile_info(zcbor_state_t *state, struct profile_info *result);
static bool decode_response_profiles(zcbor_state_t *state, struct response_profiles *result);
static bool decode_response_profile(zcbor_state_t *state, struct response_profile *result);
static bool decode_credential_info(zcbor_state_t *state, struct credential_info *result);
static bool decode_response_credentials(zcbor_state_t *state, struct response_credentials *result);
static bool decode_model_descriptor(zcbor_state_t *state, struct model_descriptor *result);
static bool decode_response_models(zcbor_state_t *state, struct response_models *result);
static bool decode_response_model_current(zcbor_state_t *state, struct response_model_current *result);
static bool decode_response_distribution(zcbor_state_t *state, struct response_distribution *result);
static bool decode_response_profile_id(zcbor_state_t *state, struct response_profile_id *result);
static bool decode_author_agent(zcbor_state_t *state, struct author_agent *result);
static bool decode_author(zcbor_state_t *state, struct author_r *result);
static bool decode_revision(zcbor_state_t *state, struct revision *result);
static bool decode_response_revisions(zcbor_state_t *state, struct response_revisions *result);
static bool decode_response_skill_bundle(zcbor_state_t *state, struct response_skill_bundle *result);
static bool decode_skill_creator(zcbor_state_t *state, struct skill_creator_r *result);
static bool decode_skill_state(zcbor_state_t *state, struct skill_state_r *result);
static bool decode_skill_usage(zcbor_state_t *state, struct skill_usage *result);
static bool decode_curator_entry(zcbor_state_t *state, struct curator_entry *result);
static bool decode_response_curator_skills(zcbor_state_t *state, struct response_curator_skills *result);
static bool decode_curator_change(zcbor_state_t *state, struct curator_change *result);
static bool decode_response_curator_run(zcbor_state_t *state, struct response_curator_run *result);
static bool decode_auth_flow_kind(zcbor_state_t *state, struct auth_flow_kind_r *result);
static bool decode_auth_begin_response(zcbor_state_t *state, struct auth_begin_response *result);
static bool decode_response_auth_begun(zcbor_state_t *state, struct response_auth_begun *result);
static bool decode_auth_complete_response(zcbor_state_t *state, struct auth_complete_response *result);
static bool decode_response_auth_completed(zcbor_state_t *state, struct response_auth_completed *result);
static bool decode_auth_provider_info(zcbor_state_t *state, struct auth_provider_info *result);
static bool decode_response_auth_providers(zcbor_state_t *state, struct response_auth_providers *result);
static bool decode_repeated_checkpoint_info_turn_ordinal(zcbor_state_t *state, struct checkpoint_info_turn_ordinal_r *result);
static bool decode_repeated_checkpoint_info_cursor(zcbor_state_t *state, struct checkpoint_info_cursor_r *result);
static bool decode_checkpoint_info(zcbor_state_t *state, struct checkpoint_info *result);
static bool decode_response_checkpoints(zcbor_state_t *state, struct response_checkpoints *result);
static bool decode_repeated_session_page_next_cursor(zcbor_state_t *state, struct session_page_next_cursor_r *result);
static bool decode_repeated_session_page_removed(zcbor_state_t *state, struct session_page_removed_r *result);
static bool decode_session_page(zcbor_state_t *state, struct session_page *result);
static bool decode_response_session_page(zcbor_state_t *state, struct response_session_page *result);
static bool decode_repeated_session_detail_overlay(zcbor_state_t *state, struct session_detail_overlay_r *result);
static bool decode_repeated_session_detail_model(zcbor_state_t *state, struct session_detail_model_r *result);
static bool decode_repeated_session_detail_delivery_targets(zcbor_state_t *state, struct session_detail_delivery_targets_r *result);
static bool decode_repeated_session_detail_children(zcbor_state_t *state, struct session_detail_children_r *result);
static bool decode_repeated_session_detail_checkpoints(zcbor_state_t *state, struct session_detail_checkpoints *result);
static bool decode_session_detail(zcbor_state_t *state, struct session_detail *result);
static bool decode_response_session_detail(zcbor_state_t *state, struct response_session_detail *result);
static bool decode_repeated_SessionsByProfile_profile_l(zcbor_state_t *state, struct SessionsByProfile_profile_l *result);
static bool decode_response_sessions_by_profile(zcbor_state_t *state, struct response_sessions_by_profile *result);
static bool decode_session_search_hit(zcbor_state_t *state, struct session_search_hit *result);
static bool decode_response_session_search(zcbor_state_t *state, struct response_session_search *result);
static bool decode_response_acp_catalog(zcbor_state_t *state, struct response_acp_catalog *result);
static bool decode_response_providers(zcbor_state_t *state, struct response_providers *result);
static bool decode_response_tools(zcbor_state_t *state, struct response_tools *result);
static bool decode_command_scope(zcbor_state_t *state, struct command_scope_r *result);
static bool decode_command_surface(zcbor_state_t *state, struct command_surface_r *result);
static bool decode_command_access(zcbor_state_t *state, struct command_access_r *result);
static bool decode_command_spec(zcbor_state_t *state, struct command_spec *result);
static bool decode_response_commands(zcbor_state_t *state, struct response_commands *result);
static bool decode_command_output(zcbor_state_t *state, struct command_output *result);
static bool decode_response_command_output(zcbor_state_t *state, struct response_command_output *result);
static bool decode_response_config(zcbor_state_t *state, struct response_config *result);
static bool decode_repeated_cron_job_next_fire_unix(zcbor_state_t *state, struct cron_job_next_fire_unix_r *result);
static bool decode_repeated_cron_job_paused(zcbor_state_t *state, struct cron_job_paused *result);
static bool decode_repeated_cron_job_last_run_unix(zcbor_state_t *state, struct cron_job_last_run_unix_r *result);
static bool decode_repeated_cron_job_last_ok(zcbor_state_t *state, struct cron_job_last_ok_r *result);
static bool decode_repeated_cron_job_last_detail(zcbor_state_t *state, struct cron_job_last_detail_r *result);
static bool decode_repeated_cron_job_fire_count(zcbor_state_t *state, struct cron_job_fire_count *result);
static bool decode_cron_job(zcbor_state_t *state, struct cron_job *result);
static bool decode_response_cron_jobs(zcbor_state_t *state, struct response_cron_jobs *result);
static bool decode_response_cron_id(zcbor_state_t *state, struct response_cron_id *result);
static bool decode_repeated_cron_run_detail(zcbor_state_t *state, struct cron_run_detail_r *result);
static bool decode_repeated_cron_run_finished_unix(zcbor_state_t *state, struct cron_run_finished_unix_r *result);
static bool decode_repeated_cron_run_session(zcbor_state_t *state, struct cron_run_session_r *result);
static bool decode_run_trigger(zcbor_state_t *state, struct run_trigger_r *result);
static bool decode_repeated_cron_run_trigger(zcbor_state_t *state, struct cron_run_trigger *result);
static bool decode_cron_run(zcbor_state_t *state, struct cron_run *result);
static bool decode_response_cron_runs(zcbor_state_t *state, struct response_cron_runs *result);
static bool decode_repeated_cron_suggestion_description(zcbor_state_t *state, struct cron_suggestion_description *result);
static bool decode_repeated_cron_suggestion_source(zcbor_state_t *state, struct cron_suggestion_source *result);
static bool decode_suggestion_status(zcbor_state_t *state, struct suggestion_status_r *result);
static bool decode_repeated_cron_suggestion_status(zcbor_state_t *state, struct cron_suggestion_status *result);
static bool decode_cron_suggestion(zcbor_state_t *state, struct cron_suggestion *result);
static bool decode_response_cron_suggestions(zcbor_state_t *state, struct response_cron_suggestions *result);
static bool decode_response_chat_routes(zcbor_state_t *state, struct response_chat_routes *result);
static bool decode_response_chat_route(zcbor_state_t *state, struct response_chat_route *result);
static bool decode_repeated_room_info_name(zcbor_state_t *state, struct room_info_name_r *result);
static bool decode_repeated_room_info_session(zcbor_state_t *state, struct room_info_session_r *result);
static bool decode_room_info(zcbor_state_t *state, struct room_info *result);
static bool decode_response_rooms(zcbor_state_t *state, struct response_rooms *result);
static bool decode_adapter_capabilities(zcbor_state_t *state, struct adapter_capabilities *result);
static bool decode_repeated_adapter_info_account_schema(zcbor_state_t *state, struct adapter_info_account_schema *result);
static bool decode_adapter_info(zcbor_state_t *state, struct adapter_info *result);
static bool decode_response_adapters(zcbor_state_t *state, struct response_adapters *result);
static bool decode_connection_state(zcbor_state_t *state, struct connection_state_r *result);
static bool decode_repeated_transport_instance_info_connection(zcbor_state_t *state, struct transport_instance_info_connection *result);
static bool decode_presence_state(zcbor_state_t *state, struct presence_state_r *result);
static bool decode_repeated_transport_instance_info_presence(zcbor_state_t *state, struct transport_instance_info_presence *result);
static bool decode_repeated_transport_instance_info_bound_profile(zcbor_state_t *state, struct transport_instance_info_bound_profile_r *result);
static bool decode_transport_instance_info(zcbor_state_t *state, struct transport_instance_info *result);
static bool decode_response_transport_instances(zcbor_state_t *state, struct response_transport_instances *result);
static bool decode_conversation_type(zcbor_state_t *state, struct conversation_type_r *result);
static bool decode_repeated_conversation_info_title(zcbor_state_t *state, struct conversation_info_title_r *result);
static bool decode_repeated_conversation_info_topic(zcbor_state_t *state, struct conversation_info_topic_r *result);
static bool decode_repeated_conversation_info_description(zcbor_state_t *state, struct conversation_info_description_r *result);
static bool decode_repeated_conversation_member_alias(zcbor_state_t *state, struct conversation_member_alias_r *result);
static bool decode_repeated_conversation_member_nickname(zcbor_state_t *state, struct conversation_member_nickname_r *result);
static bool decode_typing_state(zcbor_state_t *state, struct typing_state_r *result);
static bool decode_repeated_conversation_member_typing(zcbor_state_t *state, struct conversation_member_typing *result);
static bool decode_repeated_conversation_member_role(zcbor_state_t *state, struct conversation_member_role *result);
static bool decode_repeated_conversation_member_session(zcbor_state_t *state, struct conversation_member_session_r *result);
static bool decode_conversation_member(zcbor_state_t *state, struct conversation_member *result);
static bool decode_repeated_conversation_info_members(zcbor_state_t *state, struct conversation_info_members_r *result);
static bool decode_conversation_info(zcbor_state_t *state, struct conversation_info *result);
static bool decode_response_conversations(zcbor_state_t *state, struct response_conversations *result);
static bool decode_response_conversation(zcbor_state_t *state, struct response_conversation *result);
static bool decode_response_conv_create_details(zcbor_state_t *state, struct response_conv_create_details *result);
static bool decode_response_conv_join_details(zcbor_state_t *state, struct response_conv_join_details *result);
static bool decode_response_contact_profile(zcbor_state_t *state, struct response_contact_profile *result);
static bool decode_response_contacts(zcbor_state_t *state, struct response_contacts *result);
static bool decode_repeated_action_menu_items(zcbor_state_t *state, struct action_menu_items_r *result);
static bool decode_action_menu(zcbor_state_t *state, struct action_menu *result);
static bool decode_response_action_menu(zcbor_state_t *state, struct response_action_menu *result);
static bool decode_api_error_unknown_session(zcbor_state_t *state, struct api_error_unknown_session *result);
static bool decode_api_error_unsupported(zcbor_state_t *state, struct api_error_unsupported *result);
static bool decode_api_error_conflict(zcbor_state_t *state, struct api_error_conflict *result);
static bool decode_api_error_unauthenticated(zcbor_state_t *state, struct api_error_unauthenticated *result);
static bool decode_api_error_forbidden(zcbor_state_t *state, struct api_error_forbidden *result);
static bool decode_api_error_other(zcbor_state_t *state, struct api_error_other *result);
static bool decode_api_error(zcbor_state_t *state, struct api_error_r *result);
static bool decode_response_error(zcbor_state_t *state, struct response_error *result);
static bool decode_fs_root_kind_t(zcbor_state_t *state, struct fs_root_kind_t_r *result);
static bool decode_repeated_fs_root_session(zcbor_state_t *state, struct fs_root_session_r *result);
static bool decode_fs_root(zcbor_state_t *state, struct fs_root *result);
static bool decode_response_fs_roots(zcbor_state_t *state, struct response_fs_roots *result);
static bool decode_fs_entry_kind_t(zcbor_state_t *state, struct fs_entry_kind_t_r *result);
static bool decode_repeated_fs_entry_ignored(zcbor_state_t *state, struct fs_entry_ignored *result);
static bool decode_fs_entry(zcbor_state_t *state, struct fs_entry *result);
static bool decode_response_fs_list(zcbor_state_t *state, struct response_fs_list *result);
static bool decode_response_fs_stat(zcbor_state_t *state, struct response_fs_stat *result);
static bool decode_repeated_fs_content_truncated(zcbor_state_t *state, struct fs_content_truncated *result);
static bool decode_repeated_fs_content_blob_ref(zcbor_state_t *state, struct fs_content_blob_ref_r *result);
static bool decode_fs_content(zcbor_state_t *state, struct fs_content *result);
static bool decode_response_fs_read(zcbor_state_t *state, struct response_fs_read *result);
static bool decode_response_fs_write(zcbor_state_t *state, struct response_fs_write *result);
static bool decode_fs_search_hit(zcbor_state_t *state, struct fs_search_hit *result);
static bool decode_repeated_fs_search_page_has_more(zcbor_state_t *state, struct fs_search_page_has_more *result);
static bool decode_fs_search_page(zcbor_state_t *state, struct fs_search_page *result);
static bool decode_response_fs_search(zcbor_state_t *state, struct response_fs_search *result);
static bool decode_fs_change_kind_t(zcbor_state_t *state, struct fs_change_kind_t_r *result);
static bool decode_fs_change(zcbor_state_t *state, struct fs_change *result);
static bool decode_fs_watch_page_view(zcbor_state_t *state, struct fs_watch_page_view *result);
static bool decode_response_fs_watch(zcbor_state_t *state, struct response_fs_watch *result);
static bool decode_response_blob_put(zcbor_state_t *state, struct response_blob_put *result);
static bool decode_response_blob_get(zcbor_state_t *state, struct response_blob_get *result);
static bool decode_blob_stat(zcbor_state_t *state, struct blob_stat *result);
static bool decode_response_blob_stat(zcbor_state_t *state, struct response_blob_stat *result);
static bool decode_access_user(zcbor_state_t *state, struct access_user *result);
static bool decode_response_access_user(zcbor_state_t *state, struct response_access_user *result);
static bool decode_response_access_users(zcbor_state_t *state, struct response_access_users *result);
static bool decode_role_info(zcbor_state_t *state, struct role_info *result);
static bool decode_response_access_roles(zcbor_state_t *state, struct response_access_roles *result);
static bool decode_principal_view(zcbor_state_t *state, struct principal_view *result);
static bool decode_response_who_am_i(zcbor_state_t *state, struct response_who_am_i *result);
static bool decode_api_response(zcbor_state_t *state, struct api_response_r *result);
static bool decode_api_request(zcbor_state_t *state, struct api_request_r *result);


static bool decode_content_hash(
		zcbor_state_t *state, struct content_hash *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).content_hash_uint_count, (zcbor_decoder_t *)zcbor_uint32_decode, state, (*&(*result).content_hash_uint), sizeof(uint32_t))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_uint32_decode(state, (*&(*result).content_hash_uint));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_blob_ref_name(
		zcbor_state_t *state, struct blob_ref_name_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).blob_ref_name_tstr)))) && (((*result).blob_ref_name_choice = blob_ref_name_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).blob_ref_name_choice = blob_ref_name_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_blob_ref_mime(
		zcbor_state_t *state, struct blob_ref_mime_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"mime", tmp_str.len = sizeof("mime") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).blob_ref_mime_tstr)))) && (((*result).blob_ref_mime_choice = blob_ref_mime_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).blob_ref_mime_choice = blob_ref_mime_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_blob_ref(
		zcbor_state_t *state, struct blob_ref *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"hash", tmp_str.len = sizeof("hash") - 1, &tmp_str)))))
	&& (decode_content_hash(state, (&(*result).blob_ref_hash))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"size", tmp_str.len = sizeof("size") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).blob_ref_size))))
	&& zcbor_present_decode(&((*result).blob_ref_name_present), (zcbor_decoder_t *)decode_repeated_blob_ref_name, state, (&(*result).blob_ref_name))
	&& zcbor_present_decode(&((*result).blob_ref_mime_present), (zcbor_decoder_t *)decode_repeated_blob_ref_mime, state, (&(*result).blob_ref_mime))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_blob_ref_name(state, (&(*result).blob_ref_name));
		decode_repeated_blob_ref_mime(state, (&(*result).blob_ref_mime));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_user_msg_attachments(
		zcbor_state_t *state, struct user_msg_attachments_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"attachments", tmp_str.len = sizeof("attachments") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).user_msg_attachments_blob_ref_m_count, (zcbor_decoder_t *)decode_blob_ref, state, (*&(*result).user_msg_attachments_blob_ref_m), sizeof(struct blob_ref))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_blob_ref(state, (*&(*result).user_msg_attachments_blob_ref_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_user_msg(
		zcbor_state_t *state, struct user_msg *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).user_msg_text))))
	&& zcbor_present_decode(&((*result).user_msg_attachments_present), (zcbor_decoder_t *)decode_repeated_user_msg_attachments, state, (&(*result).user_msg_attachments))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_user_msg_attachments(state, (&(*result).user_msg_attachments));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_command_start_turn(
		zcbor_state_t *state, struct agent_command_start_turn *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"StartTurn", tmp_str.len = sizeof("StartTurn") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"input", tmp_str.len = sizeof("input") - 1, &tmp_str)))))
	&& (decode_user_msg(state, (&(*result).StartTurn_input))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).StartTurn_request_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_command_steer(
		zcbor_state_t *state, struct agent_command_steer *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Steer", tmp_str.len = sizeof("Steer") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Steer_text))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Steer_request_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_command_observe(
		zcbor_state_t *state, struct agent_command_observe *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Observe", tmp_str.len = sizeof("Observe") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"input", tmp_str.len = sizeof("input") - 1, &tmp_str)))))
	&& (decode_user_msg(state, (&(*result).Observe_input))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Observe_request_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_command_interrupt(
		zcbor_state_t *state, struct agent_command_interrupt *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Interrupt", tmp_str.len = sizeof("Interrupt") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"reason", tmp_str.len = sizeof("reason") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).Interrupt_reason_tstr)))) && (((*result).Interrupt_reason_choice = Interrupt_reason_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).Interrupt_reason_choice = Interrupt_reason_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_command_snapshot(
		zcbor_state_t *state, struct agent_command_snapshot *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Snapshot", tmp_str.len = sizeof("Snapshot") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Snapshot_request_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_rewind_anchor_user_turn(
		zcbor_state_t *state, struct rewind_anchor_user_turn *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"UserTurn", tmp_str.len = sizeof("UserTurn") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ordinal", tmp_str.len = sizeof("ordinal") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).UserTurn_ordinal))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_rewind_anchor_reply_after(
		zcbor_state_t *state, struct rewind_anchor_reply_after *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ReplyAfter", tmp_str.len = sizeof("ReplyAfter") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ordinal", tmp_str.len = sizeof("ordinal") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).ReplyAfter_ordinal))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_rewind_anchor_cursor(
		zcbor_state_t *state, struct rewind_anchor_cursor *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Cursor", tmp_str.len = sizeof("Cursor") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Cursor_seq))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_rewind_anchor(
		zcbor_state_t *state, struct rewind_anchor_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_rewind_anchor_user_turn(state, (&(*result).rewind_anchor_user_turn_m)))) && (((*result).rewind_anchor_choice = rewind_anchor_user_turn_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_rewind_anchor_reply_after(state, (&(*result).rewind_anchor_reply_after_m)))) && (((*result).rewind_anchor_choice = rewind_anchor_reply_after_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_rewind_anchor_cursor(state, (&(*result).rewind_anchor_cursor_m)))) && (((*result).rewind_anchor_choice = rewind_anchor_cursor_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_command_rewind_to(
		zcbor_state_t *state, struct agent_command_rewind_to *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"RewindTo", tmp_str.len = sizeof("RewindTo") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"anchor", tmp_str.len = sizeof("anchor") - 1, &tmp_str)))))
	&& (decode_rewind_anchor(state, (&(*result).RewindTo_anchor))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).RewindTo_request_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_command(
		zcbor_state_t *state, struct agent_command_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_agent_command_start_turn(state, (&(*result).agent_command_start_turn_m)))) && (((*result).agent_command_choice = agent_command_start_turn_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_agent_command_steer(state, (&(*result).agent_command_steer_m)))) && (((*result).agent_command_choice = agent_command_steer_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_command_observe(state, (&(*result).agent_command_observe_m)))) && (((*result).agent_command_choice = agent_command_observe_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_command_interrupt(state, (&(*result).agent_command_interrupt_m)))) && (((*result).agent_command_choice = agent_command_interrupt_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_command_snapshot(state, (&(*result).agent_command_snapshot_m)))) && (((*result).agent_command_choice = agent_command_snapshot_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_command_rewind_to(state, (&(*result).agent_command_rewind_to_m)))) && (((*result).agent_command_choice = agent_command_rewind_to_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Shutdown", tmp_str.len = sizeof("Shutdown") - 1, &tmp_str))))) && (((*result).agent_command_choice = agent_command_Shutdown_tstr_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_origin_scope_dm(
		zcbor_state_t *state, struct origin_scope_dm *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Dm", tmp_str.len = sizeof("Dm") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"user", tmp_str.len = sizeof("user") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Dm_user))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_origin_scope_group(
		zcbor_state_t *state, struct origin_scope_group *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Group", tmp_str.len = sizeof("Group") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"chat", tmp_str.len = sizeof("chat") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Group_chat))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"thread", tmp_str.len = sizeof("thread") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).Group_thread_tstr)))) && (((*result).Group_thread_choice = Group_thread_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).Group_thread_choice = Group_thread_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_origin_scope_api(
		zcbor_state_t *state, struct origin_scope_api *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Api", tmp_str.len = sizeof("Api") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"key", tmp_str.len = sizeof("key") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Api_key))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_origin_scope_t(
		zcbor_state_t *state, struct origin_scope_t_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_origin_scope_dm(state, (&(*result).origin_scope_t_origin_scope_dm_m)))) && (((*result).origin_scope_t_choice = origin_scope_t_origin_scope_dm_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_origin_scope_group(state, (&(*result).origin_scope_t_origin_scope_group_m)))) && (((*result).origin_scope_t_choice = origin_scope_t_origin_scope_group_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_origin_scope_api(state, (&(*result).origin_scope_t_origin_scope_api_m)))) && (((*result).origin_scope_t_choice = origin_scope_t_origin_scope_api_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Internal", tmp_str.len = sizeof("Internal") - 1, &tmp_str))))) && (((*result).origin_scope_t_choice = origin_scope_t_Internal_tstr_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_origin(
		zcbor_state_t *state, struct origin *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).origin_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"scope", tmp_str.len = sizeof("scope") - 1, &tmp_str)))))
	&& (decode_origin_scope_t(state, (&(*result).origin_scope))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_Submit_origin(
		zcbor_state_t *state, struct Submit_origin_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_origin(state, (&(*result).Submit_origin_origin_m)))) && (((*result).Submit_origin_choice = Submit_origin_origin_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).Submit_origin_choice = Submit_origin_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_Submit_profile(
		zcbor_state_t *state, struct Submit_profile_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).Submit_profile_profile_ref_m)))) && (((*result).Submit_profile_choice = Submit_profile_profile_ref_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).Submit_profile_choice = Submit_profile_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_submit(
		zcbor_state_t *state, struct request_submit *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Submit", tmp_str.len = sizeof("Submit") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Submit_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"command", tmp_str.len = sizeof("command") - 1, &tmp_str)))))
	&& (decode_agent_command(state, (&(*result).Submit_command))))
	&& zcbor_present_decode(&((*result).Submit_origin_present), (zcbor_decoder_t *)decode_repeated_Submit_origin, state, (&(*result).Submit_origin))
	&& zcbor_present_decode(&((*result).Submit_profile_present), (zcbor_decoder_t *)decode_repeated_Submit_profile, state, (&(*result).Submit_profile))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_Submit_origin(state, (&(*result).Submit_origin));
		decode_repeated_Submit_profile(state, (&(*result).Submit_profile));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_submit_routed(
		zcbor_state_t *state, struct request_submit_routed *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SubmitRouted", tmp_str.len = sizeof("SubmitRouted") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (decode_origin(state, (&(*result).SubmitRouted_origin))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"command", tmp_str.len = sizeof("command") - 1, &tmp_str)))))
	&& (decode_agent_command(state, (&(*result).SubmitRouted_command))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_poll(
		zcbor_state_t *state, struct request_poll *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Poll", tmp_str.len = sizeof("Poll") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Poll_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).Poll_max))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_response_body_approved(
		zcbor_state_t *state, struct host_response_body_approved *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Approved", tmp_str.len = sizeof("Approved") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).host_response_body_approved_Approved))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_response_body_input(
		zcbor_state_t *state, struct host_response_body_input *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Input", tmp_str.len = sizeof("Input") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).host_response_body_input_Input))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_response_body_chosen(
		zcbor_state_t *state, struct host_response_body_chosen *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Chosen", tmp_str.len = sizeof("Chosen") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).host_response_body_chosen_Chosen))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_response_body_delegated(
		zcbor_state_t *state, struct host_response_body_delegated *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Delegated", tmp_str.len = sizeof("Delegated") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).host_response_body_delegated_Delegated))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_response_body_spawned(
		zcbor_state_t *state, struct host_response_body_spawned *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Spawned", tmp_str.len = sizeof("Spawned") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).host_response_body_spawned_Spawned))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_response_body_deferred(
		zcbor_state_t *state, struct host_response_body_deferred *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Deferred", tmp_str.len = sizeof("Deferred") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).host_response_body_deferred_Deferred))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_response_body_t(
		zcbor_state_t *state, struct host_response_body_t_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_host_response_body_approved(state, (&(*result).host_response_body_t_host_response_body_approved_m)))) && (((*result).host_response_body_t_choice = host_response_body_t_host_response_body_approved_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_host_response_body_input(state, (&(*result).host_response_body_t_host_response_body_input_m)))) && (((*result).host_response_body_t_choice = host_response_body_t_host_response_body_input_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_host_response_body_chosen(state, (&(*result).host_response_body_t_host_response_body_chosen_m)))) && (((*result).host_response_body_t_choice = host_response_body_t_host_response_body_chosen_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_host_response_body_delegated(state, (&(*result).host_response_body_t_host_response_body_delegated_m)))) && (((*result).host_response_body_t_choice = host_response_body_t_host_response_body_delegated_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_host_response_body_spawned(state, (&(*result).host_response_body_t_host_response_body_spawned_m)))) && (((*result).host_response_body_t_choice = host_response_body_t_host_response_body_spawned_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_host_response_body_deferred(state, (&(*result).host_response_body_t_host_response_body_deferred_m)))) && (((*result).host_response_body_t_choice = host_response_body_t_host_response_body_deferred_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_response(
		zcbor_state_t *state, struct host_response *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).host_response_request_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (decode_host_response_body_t(state, (&(*result).host_response_body))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_respond(
		zcbor_state_t *state, struct request_respond *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Respond", tmp_str.len = sizeof("Respond") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Respond_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"response", tmp_str.len = sizeof("response") - 1, &tmp_str)))))
	&& (decode_host_response(state, (&(*result).Respond_response))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_assign(
		zcbor_state_t *state, struct request_assign *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Assign", tmp_str.len = sizeof("Assign") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Assign_session))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_cancel(
		zcbor_state_t *state, struct request_cancel *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Cancel", tmp_str.len = sizeof("Cancel") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Cancel_session))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_unit(
		zcbor_state_t *state, struct request_unit *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Unit", tmp_str.len = sizeof("Unit") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Unit_unit))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_unit_events(
		zcbor_state_t *state, struct request_unit_events *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"UnitEvents", tmp_str.len = sizeof("UnitEvents") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).UnitEvents_unit))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).UnitEvents_max))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_unit_outbound(
		zcbor_state_t *state, struct request_unit_outbound *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"UnitOutbound", tmp_str.len = sizeof("UnitOutbound") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).UnitOutbound_unit))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).UnitOutbound_max))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_session_history(
		zcbor_state_t *state, struct request_session_history *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionHistory", tmp_str.len = sizeof("SessionHistory") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SessionHistory_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"after_cursor", tmp_str.len = sizeof("after_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).SessionHistory_after_cursor))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).SessionHistory_max))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_subscribe(
		zcbor_state_t *state, struct request_subscribe *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Subscribe", tmp_str.len = sizeof("Subscribe") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Subscribe_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"after_seq", tmp_str.len = sizeof("after_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Subscribe_after_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).Subscribe_max))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_EventsSince_wait_ms(
		zcbor_state_t *state, struct EventsSince_wait_ms_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"wait_ms", tmp_str.len = sizeof("wait_ms") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint32_decode(state, (&(*result).EventsSince_wait_ms_uint)))) && (((*result).EventsSince_wait_ms_choice = EventsSince_wait_ms_uint_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).EventsSince_wait_ms_choice = EventsSince_wait_ms_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_events_since(
		zcbor_state_t *state, struct request_events_since *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"EventsSince", tmp_str.len = sizeof("EventsSince") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"cursor", tmp_str.len = sizeof("cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).EventsSince_cursor))))
	&& zcbor_present_decode(&((*result).EventsSince_wait_ms_present), (zcbor_decoder_t *)decode_repeated_EventsSince_wait_ms, state, (&(*result).EventsSince_wait_ms))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_EventsSince_wait_ms(state, (&(*result).EventsSince_wait_ms));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_delivery_targets(
		zcbor_state_t *state, struct request_delivery_targets *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"DeliveryTargets", tmp_str.len = sizeof("DeliveryTargets") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).DeliveryTargets_session))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_delivery_sessions(
		zcbor_state_t *state, struct request_delivery_sessions *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"DeliverySessions", tmp_str.len = sizeof("DeliverySessions") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).DeliverySessions_transport))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_sink_kind(
		zcbor_state_t *state, struct sink_kind_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Primary", tmp_str.len = sizeof("Primary") - 1, &tmp_str))))) && (((*result).sink_kind_choice = sink_kind_Primary_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Spectator", tmp_str.len = sizeof("Spectator") - 1, &tmp_str))))) && (((*result).sink_kind_choice = sink_kind_Spectator_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_delivery_target(
		zcbor_state_t *state, struct delivery_target *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).delivery_target_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"route", tmp_str.len = sizeof("route") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).delivery_target_route))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (decode_sink_kind(state, (&(*result).delivery_target_kind))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_handover(
		zcbor_state_t *state, struct request_handover *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Handover", tmp_str.len = sizeof("Handover") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Handover_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"target", tmp_str.len = sizeof("target") - 1, &tmp_str)))))
	&& (decode_delivery_target(state, (&(*result).Handover_target))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_record_meta_args(
		zcbor_state_t *state, struct record_meta_args *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).record_meta_args_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (decode_origin(state, (&(*result).record_meta_args_origin))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).record_meta_args_kind))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (zcbor_bstr_decode(state, (&(*result).record_meta_args_body))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_record_meta(
		zcbor_state_t *state, struct request_record_meta *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"RecordMeta", tmp_str.len = sizeof("RecordMeta") - 1, &tmp_str)))))
	&& (decode_record_meta_args(state, (&(*result).request_record_meta_RecordMeta))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_unit_history(
		zcbor_state_t *state, struct request_unit_history *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"UnitHistory", tmp_str.len = sizeof("UnitHistory") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).UnitHistory_unit))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"after_cursor", tmp_str.len = sizeof("after_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).UnitHistory_after_cursor))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).UnitHistory_max))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_pause(
		zcbor_state_t *state, struct request_pause *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Pause", tmp_str.len = sizeof("Pause") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Pause_unit))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_resume(
		zcbor_state_t *state, struct request_resume *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Resume", tmp_str.len = sizeof("Resume") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Resume_unit))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_scale(
		zcbor_state_t *state, struct request_scale *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Scale", tmp_str.len = sizeof("Scale") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Scale_unit))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"n", tmp_str.len = sizeof("n") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).Scale_n))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_provider_selector(
		zcbor_state_t *state, struct provider_selector_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"mock", tmp_str.len = sizeof("mock") - 1, &tmp_str))))) && (((*result).provider_selector_choice = provider_selector_mock_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"genai", tmp_str.len = sizeof("genai") - 1, &tmp_str))))) && (((*result).provider_selector_choice = provider_selector_genai_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"llama_cpp", tmp_str.len = sizeof("llama_cpp") - 1, &tmp_str))))) && (((*result).provider_selector_choice = provider_selector_llama_cpp_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"mistral_rs", tmp_str.len = sizeof("mistral_rs") - 1, &tmp_str))))) && (((*result).provider_selector_choice = provider_selector_mistral_rs_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_SetSessionModel_provider(
		zcbor_state_t *state, struct SetSessionModel_provider_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_provider_selector(state, (&(*result).SetSessionModel_provider_provider_selector_m)))) && (((*result).SetSessionModel_provider_choice = SetSessionModel_provider_provider_selector_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).SetSessionModel_provider_choice = SetSessionModel_provider_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_set_session_model(
		zcbor_state_t *state, struct request_set_session_model *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SetSessionModel", tmp_str.len = sizeof("SetSessionModel") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SetSessionModel_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SetSessionModel_model))))
	&& zcbor_present_decode(&((*result).SetSessionModel_provider_present), (zcbor_decoder_t *)decode_repeated_SetSessionModel_provider, state, (&(*result).SetSessionModel_provider))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_SetSessionModel_provider(state, (&(*result).SetSessionModel_provider));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_approval_mode(
		zcbor_state_t *state, struct approval_mode_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ask", tmp_str.len = sizeof("ask") - 1, &tmp_str))))) && (((*result).approval_mode_choice = approval_mode_ask_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"accept_edits", tmp_str.len = sizeof("accept_edits") - 1, &tmp_str))))) && (((*result).approval_mode_choice = approval_mode_accept_edits_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"auto_allow", tmp_str.len = sizeof("auto_allow") - 1, &tmp_str))))) && (((*result).approval_mode_choice = approval_mode_auto_allow_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"deny", tmp_str.len = sizeof("deny") - 1, &tmp_str))))) && (((*result).approval_mode_choice = approval_mode_deny_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_set_session_mode(
		zcbor_state_t *state, struct request_set_session_mode *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SetSessionMode", tmp_str.len = sizeof("SetSessionMode") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SetSessionMode_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"mode", tmp_str.len = sizeof("mode") - 1, &tmp_str)))))
	&& (decode_approval_mode(state, (&(*result).SetSessionMode_mode))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_overlay_model(
		zcbor_state_t *state, struct session_overlay_model_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).session_overlay_model_tstr)))) && (((*result).session_overlay_model_choice = session_overlay_model_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).session_overlay_model_choice = session_overlay_model_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_overlay_provider(
		zcbor_state_t *state, struct session_overlay_provider_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_provider_selector(state, (&(*result).session_overlay_provider_provider_selector_m)))) && (((*result).session_overlay_provider_choice = session_overlay_provider_provider_selector_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).session_overlay_provider_choice = session_overlay_provider_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_tools_override_allowlist(
		zcbor_state_t *state, struct tools_override_allowlist *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"allowlist", tmp_str.len = sizeof("allowlist") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).tools_override_allowlist_allowlist_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).tools_override_allowlist_allowlist_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).tools_override_allowlist_allowlist_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_tools_override(
		zcbor_state_t *state, struct tools_override_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"inherit", tmp_str.len = sizeof("inherit") - 1, &tmp_str))))) && (((*result).tools_override_choice = tools_override_inherit_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"full_toolset", tmp_str.len = sizeof("full_toolset") - 1, &tmp_str))))) && (((*result).tools_override_choice = tools_override_full_toolset_tstr_c), true))
	|| (((decode_tools_override_allowlist(state, (&(*result).tools_override_allowlist_m)))) && (((*result).tools_override_choice = tools_override_allowlist_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_overlay_tool_allowlist(
		zcbor_state_t *state, struct session_overlay_tool_allowlist *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"tool_allowlist", tmp_str.len = sizeof("tool_allowlist") - 1, &tmp_str)))))
	&& (decode_tools_override(state, (&(*result).session_overlay_tool_allowlist)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_overlay_approval_mode(
		zcbor_state_t *state, struct session_overlay_approval_mode_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"approval_mode", tmp_str.len = sizeof("approval_mode") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_approval_mode(state, (&(*result).session_overlay_approval_mode_approval_mode_m)))) && (((*result).session_overlay_approval_mode_choice = session_overlay_approval_mode_approval_mode_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).session_overlay_approval_mode_choice = session_overlay_approval_mode_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_workspace_binding_bound(
		zcbor_state_t *state, struct workspace_binding_bound *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"bound", tmp_str.len = sizeof("bound") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).workspace_binding_bound_bound))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_workspace_binding(
		zcbor_state_t *state, struct workspace_binding_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"isolated", tmp_str.len = sizeof("isolated") - 1, &tmp_str))))) && (((*result).workspace_binding_choice = workspace_binding_isolated_tstr_c), true))
	|| (((decode_workspace_binding_bound(state, (&(*result).workspace_binding_bound_m)))) && (((*result).workspace_binding_choice = workspace_binding_bound_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_overlay_workspace(
		zcbor_state_t *state, struct session_overlay_workspace_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"workspace", tmp_str.len = sizeof("workspace") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_workspace_binding(state, (&(*result).session_overlay_workspace_workspace_binding_m)))) && (((*result).session_overlay_workspace_choice = session_overlay_workspace_workspace_binding_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).session_overlay_workspace_choice = session_overlay_workspace_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_overlay(
		zcbor_state_t *state, struct session_overlay *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).session_overlay_model_present), (zcbor_decoder_t *)decode_repeated_session_overlay_model, state, (&(*result).session_overlay_model))
	&& zcbor_present_decode(&((*result).session_overlay_provider_present), (zcbor_decoder_t *)decode_repeated_session_overlay_provider, state, (&(*result).session_overlay_provider))
	&& zcbor_present_decode(&((*result).session_overlay_tool_allowlist_present), (zcbor_decoder_t *)decode_repeated_session_overlay_tool_allowlist, state, (&(*result).session_overlay_tool_allowlist))
	&& zcbor_present_decode(&((*result).session_overlay_approval_mode_present), (zcbor_decoder_t *)decode_repeated_session_overlay_approval_mode, state, (&(*result).session_overlay_approval_mode))
	&& zcbor_present_decode(&((*result).session_overlay_workspace_present), (zcbor_decoder_t *)decode_repeated_session_overlay_workspace, state, (&(*result).session_overlay_workspace))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_session_overlay_model(state, (&(*result).session_overlay_model));
		decode_repeated_session_overlay_provider(state, (&(*result).session_overlay_provider));
		decode_repeated_session_overlay_tool_allowlist(state, (&(*result).session_overlay_tool_allowlist));
		decode_repeated_session_overlay_approval_mode(state, (&(*result).session_overlay_approval_mode));
		decode_repeated_session_overlay_workspace(state, (&(*result).session_overlay_workspace));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_set_session_overlay(
		zcbor_state_t *state, struct request_set_session_overlay *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SetSessionOverlay", tmp_str.len = sizeof("SetSessionOverlay") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SetSessionOverlay_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"overlay", tmp_str.len = sizeof("overlay") - 1, &tmp_str)))))
	&& (decode_session_overlay(state, (&(*result).SetSessionOverlay_overlay))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_ApprovalsPending_session(
		zcbor_state_t *state, struct ApprovalsPending_session_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).ApprovalsPending_session_session_id_m)))) && (((*result).ApprovalsPending_session_choice = ApprovalsPending_session_session_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).ApprovalsPending_session_choice = ApprovalsPending_session_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_approvals_pending(
		zcbor_state_t *state, struct request_approvals_pending *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ApprovalsPending", tmp_str.len = sizeof("ApprovalsPending") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).ApprovalsPending_session_present), (zcbor_decoder_t *)decode_repeated_ApprovalsPending_session, state, (&(*result).ApprovalsPending_session))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_ApprovalsPending_session(state, (&(*result).ApprovalsPending_session));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_approval_decide(
		zcbor_state_t *state, struct request_approval_decide *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ApprovalDecide", tmp_str.len = sizeof("ApprovalDecide") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ApprovalDecide_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ApprovalDecide_request_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"allow", tmp_str.len = sizeof("allow") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).ApprovalDecide_allow))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_profile_get(
		zcbor_state_t *state, struct request_profile_get *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileGet", tmp_str.len = sizeof("ProfileGet") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ProfileGet_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_budget(
		zcbor_state_t *state, struct budget *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"tokens", tmp_str.len = sizeof("tokens") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).budget_tokens_uint64_m)))) && (((*result).budget_tokens_choice = budget_tokens_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).budget_tokens_choice = budget_tokens_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"wall_ms", tmp_str.len = sizeof("wall_ms") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).budget_wall_ms_uint64_m)))) && (((*result).budget_wall_ms_choice = budget_wall_ms_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).budget_wall_ms_choice = budget_wall_ms_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_engine_tunables(
		zcbor_state_t *state, struct engine_tunables *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"model_retry_attempts", tmp_str.len = sizeof("model_retry_attempts") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint32_decode(state, (&(*result).engine_tunables_model_retry_attempts_uint)))) && (((*result).engine_tunables_model_retry_attempts_choice = engine_tunables_model_retry_attempts_uint_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).engine_tunables_model_retry_attempts_choice = engine_tunables_model_retry_attempts_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"context_budget_tokens", tmp_str.len = sizeof("context_budget_tokens") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint32_decode(state, (&(*result).engine_tunables_context_budget_tokens_uint)))) && (((*result).engine_tunables_context_budget_tokens_choice = engine_tunables_context_budget_tokens_uint_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).engine_tunables_context_budget_tokens_choice = engine_tunables_context_budget_tokens_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max_iterations", tmp_str.len = sizeof("max_iterations") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint32_decode(state, (&(*result).engine_tunables_max_iterations_uint)))) && (((*result).engine_tunables_max_iterations_choice = engine_tunables_max_iterations_uint_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).engine_tunables_max_iterations_choice = engine_tunables_max_iterations_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"tool_result_budget", tmp_str.len = sizeof("tool_result_budget") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint32_decode(state, (&(*result).engine_tunables_tool_result_budget_uint)))) && (((*result).engine_tunables_tool_result_budget_choice = engine_tunables_tool_result_budget_uint_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).engine_tunables_tool_result_budget_choice = engine_tunables_tool_result_budget_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_context_engine_sel(
		zcbor_state_t *state, struct context_engine_sel_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"lcm", tmp_str.len = sizeof("lcm") - 1, &tmp_str))))) && (((*result).context_engine_sel_choice = context_engine_sel_lcm_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"budgeted", tmp_str.len = sizeof("budgeted") - 1, &tmp_str))))) && (((*result).context_engine_sel_choice = context_engine_sel_budgeted_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_memory_provider_sel(
		zcbor_state_t *state, struct memory_provider_sel_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"mnemosyne", tmp_str.len = sizeof("mnemosyne") - 1, &tmp_str))))) && (((*result).memory_provider_sel_choice = memory_provider_sel_mnemosyne_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"file", tmp_str.len = sizeof("file") - 1, &tmp_str))))) && (((*result).memory_provider_sel_choice = memory_provider_sel_file_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"none", tmp_str.len = sizeof("none") - 1, &tmp_str))))) && (((*result).memory_provider_sel_choice = memory_provider_sel_none_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_bound_account(
		zcbor_state_t *state, struct bound_account *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport_instance", tmp_str.len = sizeof("transport_instance") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).bound_account_transport_instance))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"credential_ref", tmp_str.len = sizeof("credential_ref") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).bound_account_credential_ref))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_profile_spec(
		zcbor_state_t *state, struct profile_spec *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).profile_spec_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (decode_provider_selector(state, (&(*result).profile_spec_provider))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).profile_spec_model))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"base_url", tmp_str.len = sizeof("base_url") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).profile_spec_base_url_tstr)))) && (((*result).profile_spec_base_url_choice = profile_spec_base_url_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).profile_spec_base_url_choice = profile_spec_base_url_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"system_prompt", tmp_str.len = sizeof("system_prompt") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).profile_spec_system_prompt))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"tool_allowlist", tmp_str.len = sizeof("tool_allowlist") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).tool_allowlist_tstr_l_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).tool_allowlist_tstr_l_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))) && (((*result).profile_spec_tool_allowlist_choice = tool_allowlist_tstr_l_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).profile_spec_tool_allowlist_choice = profile_spec_tool_allowlist_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"budget", tmp_str.len = sizeof("budget") - 1, &tmp_str)))))
	&& (decode_budget(state, (&(*result).profile_spec_budget))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"tunables", tmp_str.len = sizeof("tunables") - 1, &tmp_str)))))
	&& (decode_engine_tunables(state, (&(*result).profile_spec_tunables))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"context_engine", tmp_str.len = sizeof("context_engine") - 1, &tmp_str)))))
	&& (decode_context_engine_sel(state, (&(*result).profile_spec_context_engine))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"memory_provider", tmp_str.len = sizeof("memory_provider") - 1, &tmp_str)))))
	&& (decode_memory_provider_sel(state, (&(*result).profile_spec_memory_provider))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"credential_ref", tmp_str.len = sizeof("credential_ref") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).profile_spec_credential_ref_tstr)))) && (((*result).profile_spec_credential_ref_choice = profile_spec_credential_ref_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).profile_spec_credential_ref_choice = profile_spec_credential_ref_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"fallback_credential_ref", tmp_str.len = sizeof("fallback_credential_ref") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).profile_spec_fallback_credential_ref_tstr)))) && (((*result).profile_spec_fallback_credential_ref_choice = profile_spec_fallback_credential_ref_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).profile_spec_fallback_credential_ref_choice = profile_spec_fallback_credential_ref_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"bound_accounts", tmp_str.len = sizeof("bound_accounts") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).profile_spec_bound_accounts_bound_account_m_count, (zcbor_decoder_t *)decode_bound_account, state, (*&(*result).profile_spec_bound_accounts_bound_account_m), sizeof(struct bound_account))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).tool_allowlist_tstr_l_tstr));
		decode_bound_account(state, (*&(*result).profile_spec_bound_accounts_bound_account_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_profile_create(
		zcbor_state_t *state, struct request_profile_create *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileCreate", tmp_str.len = sizeof("ProfileCreate") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (decode_profile_spec(state, (&(*result).ProfileCreate_spec))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_profile_update(
		zcbor_state_t *state, struct request_profile_update *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileUpdate", tmp_str.len = sizeof("ProfileUpdate") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (decode_profile_spec(state, (&(*result).ProfileUpdate_spec))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_profile_delete(
		zcbor_state_t *state, struct request_profile_delete *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileDelete", tmp_str.len = sizeof("ProfileDelete") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ProfileDelete_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_profile_select(
		zcbor_state_t *state, struct request_profile_select *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileSelect", tmp_str.len = sizeof("ProfileSelect") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ProfileSelect_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_profile_clone(
		zcbor_state_t *state, struct request_profile_clone *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileClone", tmp_str.len = sizeof("ProfileClone") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ProfileClone_source))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"new_id", tmp_str.len = sizeof("new_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ProfileClone_new_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_profile_export(
		zcbor_state_t *state, struct request_profile_export *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileExport", tmp_str.len = sizeof("ProfileExport") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ProfileExport_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_files_tstrtstr(
		zcbor_state_t *state, struct files_tstrtstr *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_tstr_decode(state, (&(*result).skill_bundle_files_tstrtstr_key))))
	&& (zcbor_tstr_decode(state, (&(*result).files_tstrtstr)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_skill_bundle(
		zcbor_state_t *state, struct skill_bundle *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).skill_bundle_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"category", tmp_str.len = sizeof("category") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).skill_bundle_category_tstr)))) && (((*result).skill_bundle_category_choice = skill_bundle_category_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).skill_bundle_category_choice = skill_bundle_category_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"files", tmp_str.len = sizeof("files") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).files_tstrtstr_count, (zcbor_decoder_t *)decode_repeated_files_tstrtstr, state, (*&(*result).files_tstrtstr), sizeof(struct files_tstrtstr))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_files_tstrtstr(state, (*&(*result).files_tstrtstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_distribution_head_seq(
		zcbor_state_t *state, struct distribution_head_seq_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"head_seq", tmp_str.len = sizeof("head_seq") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).distribution_head_seq_uint64_m)))) && (((*result).distribution_head_seq_choice = distribution_head_seq_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).distribution_head_seq_choice = distribution_head_seq_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_distribution_source(
		zcbor_state_t *state, struct distribution_source_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).distribution_source_tstr)))) && (((*result).distribution_source_choice = distribution_source_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).distribution_source_choice = distribution_source_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_distribution(
		zcbor_state_t *state, struct distribution *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"wire_version", tmp_str.len = sizeof("wire_version") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).distribution_wire_version))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (decode_profile_spec(state, (&(*result).distribution_profile))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"skills", tmp_str.len = sizeof("skills") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).distribution_skills_skill_bundle_m_count, (zcbor_decoder_t *)decode_skill_bundle, state, (*&(*result).distribution_skills_skill_bundle_m), sizeof(struct skill_bundle))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& zcbor_present_decode(&((*result).distribution_head_seq_present), (zcbor_decoder_t *)decode_repeated_distribution_head_seq, state, (&(*result).distribution_head_seq))
	&& zcbor_present_decode(&((*result).distribution_source_present), (zcbor_decoder_t *)decode_repeated_distribution_source, state, (&(*result).distribution_source))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_skill_bundle(state, (*&(*result).distribution_skills_skill_bundle_m));
		decode_repeated_distribution_head_seq(state, (&(*result).distribution_head_seq));
		decode_repeated_distribution_source(state, (&(*result).distribution_source));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_ProfileImport_new_id(
		zcbor_state_t *state, struct ProfileImport_new_id_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"new_id", tmp_str.len = sizeof("new_id") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).ProfileImport_new_id_tstr)))) && (((*result).ProfileImport_new_id_choice = ProfileImport_new_id_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).ProfileImport_new_id_choice = ProfileImport_new_id_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_profile_import(
		zcbor_state_t *state, struct request_profile_import *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileImport", tmp_str.len = sizeof("ProfileImport") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"dist", tmp_str.len = sizeof("dist") - 1, &tmp_str)))))
	&& (decode_distribution(state, (&(*result).ProfileImport_dist))))
	&& zcbor_present_decode(&((*result).ProfileImport_new_id_present), (zcbor_decoder_t *)decode_repeated_ProfileImport_new_id, state, (&(*result).ProfileImport_new_id))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_ProfileImport_new_id(state, (&(*result).ProfileImport_new_id));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_profile_history(
		zcbor_state_t *state, struct request_profile_history *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileHistory", tmp_str.len = sizeof("ProfileHistory") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ProfileHistory_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_profile_at(
		zcbor_state_t *state, struct request_profile_at *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileAt", tmp_str.len = sizeof("ProfileAt") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ProfileAt_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).ProfileAt_seq))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_profile_revert(
		zcbor_state_t *state, struct request_profile_revert *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileRevert", tmp_str.len = sizeof("ProfileRevert") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ProfileRevert_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).ProfileRevert_seq))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_skill_history(
		zcbor_state_t *state, struct request_skill_history *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SkillHistory", tmp_str.len = sizeof("SkillHistory") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SkillHistory_name))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_skill_at(
		zcbor_state_t *state, struct request_skill_at *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SkillAt", tmp_str.len = sizeof("SkillAt") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SkillAt_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).SkillAt_seq))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_skill_revert(
		zcbor_state_t *state, struct request_skill_revert *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SkillRevert", tmp_str.len = sizeof("SkillRevert") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SkillRevert_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).SkillRevert_seq))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_CuratorList_profile(
		zcbor_state_t *state, struct CuratorList_profile_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).CuratorList_profile_tstr)))) && (((*result).CuratorList_profile_choice = CuratorList_profile_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).CuratorList_profile_choice = CuratorList_profile_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_curator_list(
		zcbor_state_t *state, struct request_curator_list *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CuratorList", tmp_str.len = sizeof("CuratorList") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).CuratorList_profile_present), (zcbor_decoder_t *)decode_repeated_CuratorList_profile, state, (&(*result).CuratorList_profile))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_CuratorList_profile(state, (&(*result).CuratorList_profile));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_CuratorPin_profile(
		zcbor_state_t *state, struct CuratorPin_profile_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).CuratorPin_profile_tstr)))) && (((*result).CuratorPin_profile_choice = CuratorPin_profile_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).CuratorPin_profile_choice = CuratorPin_profile_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_curator_pin(
		zcbor_state_t *state, struct request_curator_pin *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CuratorPin", tmp_str.len = sizeof("CuratorPin") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).CuratorPin_profile_present), (zcbor_decoder_t *)decode_repeated_CuratorPin_profile, state, (&(*result).CuratorPin_profile))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CuratorPin_name))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_CuratorPin_profile(state, (&(*result).CuratorPin_profile));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_CuratorUnpin_profile(
		zcbor_state_t *state, struct CuratorUnpin_profile_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).CuratorUnpin_profile_tstr)))) && (((*result).CuratorUnpin_profile_choice = CuratorUnpin_profile_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).CuratorUnpin_profile_choice = CuratorUnpin_profile_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_curator_unpin(
		zcbor_state_t *state, struct request_curator_unpin *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CuratorUnpin", tmp_str.len = sizeof("CuratorUnpin") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).CuratorUnpin_profile_present), (zcbor_decoder_t *)decode_repeated_CuratorUnpin_profile, state, (&(*result).CuratorUnpin_profile))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CuratorUnpin_name))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_CuratorUnpin_profile(state, (&(*result).CuratorUnpin_profile));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_CuratorArchive_profile(
		zcbor_state_t *state, struct CuratorArchive_profile_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).CuratorArchive_profile_tstr)))) && (((*result).CuratorArchive_profile_choice = CuratorArchive_profile_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).CuratorArchive_profile_choice = CuratorArchive_profile_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_curator_archive(
		zcbor_state_t *state, struct request_curator_archive *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CuratorArchive", tmp_str.len = sizeof("CuratorArchive") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).CuratorArchive_profile_present), (zcbor_decoder_t *)decode_repeated_CuratorArchive_profile, state, (&(*result).CuratorArchive_profile))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CuratorArchive_name))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_CuratorArchive_profile(state, (&(*result).CuratorArchive_profile));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_CuratorRestore_profile(
		zcbor_state_t *state, struct CuratorRestore_profile_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).CuratorRestore_profile_tstr)))) && (((*result).CuratorRestore_profile_choice = CuratorRestore_profile_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).CuratorRestore_profile_choice = CuratorRestore_profile_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_curator_restore(
		zcbor_state_t *state, struct request_curator_restore *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CuratorRestore", tmp_str.len = sizeof("CuratorRestore") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).CuratorRestore_profile_present), (zcbor_decoder_t *)decode_repeated_CuratorRestore_profile, state, (&(*result).CuratorRestore_profile))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CuratorRestore_name))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_CuratorRestore_profile(state, (&(*result).CuratorRestore_profile));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_CuratorRun_profile(
		zcbor_state_t *state, struct CuratorRun_profile_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).CuratorRun_profile_tstr)))) && (((*result).CuratorRun_profile_choice = CuratorRun_profile_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).CuratorRun_profile_choice = CuratorRun_profile_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_curator_run(
		zcbor_state_t *state, struct request_curator_run *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CuratorRun", tmp_str.len = sizeof("CuratorRun") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).CuratorRun_profile_present), (zcbor_decoder_t *)decode_repeated_CuratorRun_profile, state, (&(*result).CuratorRun_profile))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_CuratorRun_profile(state, (&(*result).CuratorRun_profile));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_credential_set(
		zcbor_state_t *state, struct request_credential_set *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CredentialSet", tmp_str.len = sizeof("CredentialSet") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CredentialSet_profile))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"secret", tmp_str.len = sizeof("secret") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CredentialSet_secret))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_credential_remove(
		zcbor_state_t *state, struct request_credential_remove *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CredentialRemove", tmp_str.len = sizeof("CredentialRemove") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CredentialRemove_profile))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_model_engine(
		zcbor_state_t *state, struct model_engine_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Llama", tmp_str.len = sizeof("Llama") - 1, &tmp_str))))) && (((*result).model_engine_choice = model_engine_Llama_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"MistralRs", tmp_str.len = sizeof("MistralRs") - 1, &tmp_str))))) && (((*result).model_engine_choice = model_engine_MistralRs_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_search_sort(
		zcbor_state_t *state, struct search_sort_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Trending", tmp_str.len = sizeof("Trending") - 1, &tmp_str))))) && (((*result).search_sort_choice = search_sort_Trending_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Downloads", tmp_str.len = sizeof("Downloads") - 1, &tmp_str))))) && (((*result).search_sort_choice = search_sort_Downloads_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Likes", tmp_str.len = sizeof("Likes") - 1, &tmp_str))))) && (((*result).search_sort_choice = search_sort_Likes_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Modified", tmp_str.len = sizeof("Modified") - 1, &tmp_str))))) && (((*result).search_sort_choice = search_sort_Modified_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Created", tmp_str.len = sizeof("Created") - 1, &tmp_str))))) && (((*result).search_sort_choice = search_sort_Created_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_search_query(
		zcbor_state_t *state, struct search_query *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).search_query_text))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"engine", tmp_str.len = sizeof("engine") - 1, &tmp_str)))))
	&& (decode_model_engine(state, (&(*result).search_query_engine))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"sort", tmp_str.len = sizeof("sort") - 1, &tmp_str)))))
	&& (decode_search_sort(state, (&(*result).search_query_sort))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"page", tmp_str.len = sizeof("page") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).search_query_page))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"limit", tmp_str.len = sizeof("limit") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).search_query_limit))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_model_search(
		zcbor_state_t *state, struct request_model_search *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelSearch", tmp_str.len = sizeof("ModelSearch") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"query", tmp_str.len = sizeof("query") - 1, &tmp_str)))))
	&& (decode_search_query(state, (&(*result).ModelSearch_query))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_model_files(
		zcbor_state_t *state, struct request_model_files *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelFiles", tmp_str.len = sizeof("ModelFiles") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ModelFiles_repo))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"revision", tmp_str.len = sizeof("revision") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).ModelFiles_revision_tstr)))) && (((*result).ModelFiles_revision_choice = ModelFiles_revision_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).ModelFiles_revision_choice = ModelFiles_revision_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"engine", tmp_str.len = sizeof("engine") - 1, &tmp_str)))))
	&& (decode_model_engine(state, (&(*result).ModelFiles_engine))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_model_source_hf(
		zcbor_state_t *state, struct model_source_hf *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Hf", tmp_str.len = sizeof("Hf") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Hf_repo))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"file", tmp_str.len = sizeof("file") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).Hf_file_tstr)))) && (((*result).Hf_file_choice = Hf_file_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).Hf_file_choice = Hf_file_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"revision", tmp_str.len = sizeof("revision") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Hf_revision))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_model_source_local(
		zcbor_state_t *state, struct model_source_local *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Local", tmp_str.len = sizeof("Local") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Local_path))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_model_source(
		zcbor_state_t *state, struct model_source_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_model_source_hf(state, (&(*result).model_source_hf_m)))) && (((*result).model_source_choice = model_source_hf_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_model_source_local(state, (&(*result).model_source_local_m)))) && (((*result).model_source_choice = model_source_local_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_model_ref(
		zcbor_state_t *state, struct model_ref *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"engine", tmp_str.len = sizeof("engine") - 1, &tmp_str)))))
	&& (decode_model_engine(state, (&(*result).model_ref_engine))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (decode_model_source(state, (&(*result).model_ref_source))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_model_download(
		zcbor_state_t *state, struct request_model_download *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelDownload", tmp_str.len = sizeof("ModelDownload") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (decode_model_ref(state, (&(*result).ModelDownload_model))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_model_cancel(
		zcbor_state_t *state, struct request_model_cancel *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelCancel", tmp_str.len = sizeof("ModelCancel") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).ModelCancel_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_model_pause(
		zcbor_state_t *state, struct request_model_pause *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelPause", tmp_str.len = sizeof("ModelPause") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).ModelPause_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_model_resume(
		zcbor_state_t *state, struct request_model_resume *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelResume", tmp_str.len = sizeof("ModelResume") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).ModelResume_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_model_delete(
		zcbor_state_t *state, struct request_model_delete *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelDelete", tmp_str.len = sizeof("ModelDelete") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ModelDelete_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_model_activate(
		zcbor_state_t *state, struct request_model_activate *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelActivate", tmp_str.len = sizeof("ModelActivate") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ModelActivate_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).ModelActivate_profile_tstr)))) && (((*result).ModelActivate_profile_choice = ModelActivate_profile_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).ModelActivate_profile_choice = ModelActivate_profile_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_model_recommend_args(
		zcbor_state_t *state, struct model_recommend_args *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).model_recommend_args_repo))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"revision", tmp_str.len = sizeof("revision") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).model_recommend_args_revision_tstr)))) && (((*result).model_recommend_args_revision_choice = model_recommend_args_revision_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).model_recommend_args_revision_choice = model_recommend_args_revision_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"engine", tmp_str.len = sizeof("engine") - 1, &tmp_str)))))
	&& (decode_model_engine(state, (&(*result).model_recommend_args_engine))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"budget_bytes", tmp_str.len = sizeof("budget_bytes") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).model_recommend_args_budget_bytes_uint64_m)))) && (((*result).model_recommend_args_budget_bytes_choice = model_recommend_args_budget_bytes_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).model_recommend_args_budget_bytes_choice = model_recommend_args_budget_bytes_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_model_recommend(
		zcbor_state_t *state, struct request_model_recommend *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelRecommend", tmp_str.len = sizeof("ModelRecommend") - 1, &tmp_str)))))
	&& (decode_model_recommend_args(state, (&(*result).request_model_recommend_ModelRecommend))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_model_quantize_args(
		zcbor_state_t *state, struct model_quantize_args *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).model_quantize_args_repo))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"revision", tmp_str.len = sizeof("revision") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).model_quantize_args_revision_tstr)))) && (((*result).model_quantize_args_revision_choice = model_quantize_args_revision_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).model_quantize_args_revision_choice = model_quantize_args_revision_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"target_quant", tmp_str.len = sizeof("target_quant") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).model_quantize_args_target_quant))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"source_file", tmp_str.len = sizeof("source_file") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).model_quantize_args_source_file_tstr)))) && (((*result).model_quantize_args_source_file_choice = model_quantize_args_source_file_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).model_quantize_args_source_file_choice = model_quantize_args_source_file_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_model_quantize(
		zcbor_state_t *state, struct request_model_quantize *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelQuantize", tmp_str.len = sizeof("ModelQuantize") - 1, &tmp_str)))))
	&& (decode_model_quantize_args(state, (&(*result).request_model_quantize_ModelQuantize))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_model_inspect(
		zcbor_state_t *state, struct request_model_inspect *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelInspect", tmp_str.len = sizeof("ModelInspect") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ModelInspect_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_model_current(
		zcbor_state_t *state, struct request_model_current *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelCurrent", tmp_str.len = sizeof("ModelCurrent") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).ModelCurrent_profile_tstr)))) && (((*result).ModelCurrent_profile_choice = ModelCurrent_profile_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).ModelCurrent_profile_choice = ModelCurrent_profile_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_params_tstrtstr(
		zcbor_state_t *state, struct params_tstrtstr *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_tstr_decode(state, (&(*result).auth_begin_request_params_tstrtstr_key))))
	&& (zcbor_tstr_decode(state, (&(*result).params_tstrtstr)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_auth_bind_request(
		zcbor_state_t *state, struct auth_bind_request *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_bind_request_profile))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport_instance", tmp_str.len = sizeof("transport_instance") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).auth_bind_request_transport_instance_transport_id_m)))) && (((*result).auth_bind_request_transport_instance_choice = auth_bind_request_transport_instance_transport_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).auth_bind_request_transport_instance_choice = auth_bind_request_transport_instance_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"credential_ref", tmp_str.len = sizeof("credential_ref") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).auth_bind_request_credential_ref_tstr)))) && (((*result).auth_bind_request_credential_ref_choice = auth_bind_request_credential_ref_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).auth_bind_request_credential_ref_choice = auth_bind_request_credential_ref_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_auth_begin_request_bind(
		zcbor_state_t *state, struct auth_begin_request_bind_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"bind", tmp_str.len = sizeof("bind") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_auth_bind_request(state, (&(*result).auth_begin_request_bind_auth_bind_request_m)))) && (((*result).auth_begin_request_bind_choice = auth_begin_request_bind_auth_bind_request_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).auth_begin_request_bind_choice = auth_begin_request_bind_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_auth_begin_request(
		zcbor_state_t *state, struct auth_begin_request *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"family", tmp_str.len = sizeof("family") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_begin_request_family))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"params", tmp_str.len = sizeof("params") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).params_tstrtstr_count, (zcbor_decoder_t *)decode_repeated_params_tstrtstr, state, (*&(*result).params_tstrtstr), sizeof(struct params_tstrtstr))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"redirect_uri", tmp_str.len = sizeof("redirect_uri") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_begin_request_redirect_uri))))
	&& zcbor_present_decode(&((*result).auth_begin_request_bind_present), (zcbor_decoder_t *)decode_repeated_auth_begin_request_bind, state, (&(*result).auth_begin_request_bind))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_params_tstrtstr(state, (*&(*result).params_tstrtstr));
		decode_repeated_auth_begin_request_bind(state, (&(*result).auth_begin_request_bind));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_auth_begin(
		zcbor_state_t *state, struct request_auth_begin *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AuthBegin", tmp_str.len = sizeof("AuthBegin") - 1, &tmp_str)))))
	&& (decode_auth_begin_request(state, (&(*result).request_auth_begin_AuthBegin))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_auth_complete_request(
		zcbor_state_t *state, struct auth_complete_request *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"flow_id", tmp_str.len = sizeof("flow_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_complete_request_flow_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"callback", tmp_str.len = sizeof("callback") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_complete_request_callback))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_auth_complete(
		zcbor_state_t *state, struct request_auth_complete *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AuthComplete", tmp_str.len = sizeof("AuthComplete") - 1, &tmp_str)))))
	&& (decode_auth_complete_request(state, (&(*result).request_auth_complete_AuthComplete))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_auth_cancel(
		zcbor_state_t *state, struct request_auth_cancel *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AuthCancel", tmp_str.len = sizeof("AuthCancel") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"flow_id", tmp_str.len = sizeof("flow_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).AuthCancel_flow_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_CheckpointList_session(
		zcbor_state_t *state, struct CheckpointList_session_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).CheckpointList_session_session_id_m)))) && (((*result).CheckpointList_session_choice = CheckpointList_session_session_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).CheckpointList_session_choice = CheckpointList_session_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_checkpoint_list(
		zcbor_state_t *state, struct request_checkpoint_list *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CheckpointList", tmp_str.len = sizeof("CheckpointList") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).CheckpointList_session_present), (zcbor_decoder_t *)decode_repeated_CheckpointList_session, state, (&(*result).CheckpointList_session))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_CheckpointList_session(state, (&(*result).CheckpointList_session));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_checkpoint_rewind(
		zcbor_state_t *state, struct request_checkpoint_rewind *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CheckpointRewind", tmp_str.len = sizeof("CheckpointRewind") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CheckpointRewind_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"checkpoint_id", tmp_str.len = sizeof("checkpoint_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CheckpointRewind_checkpoint_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_scope_by_profile(
		zcbor_state_t *state, struct session_scope_by_profile *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ByProfile", tmp_str.len = sizeof("ByProfile") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).session_scope_by_profile_ByProfile))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_scope_by_transport(
		zcbor_state_t *state, struct session_scope_by_transport *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ByTransport", tmp_str.len = sizeof("ByTransport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).session_scope_by_transport_ByTransport))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_scope(
		zcbor_state_t *state, struct session_scope_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"TopLevel", tmp_str.len = sizeof("TopLevel") - 1, &tmp_str))))) && (((*result).session_scope_choice = session_scope_TopLevel_tstr_c), true))
	|| (((decode_session_scope_by_profile(state, (&(*result).session_scope_by_profile_m)))) && (((*result).session_scope_choice = session_scope_by_profile_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_session_scope_by_transport(state, (&(*result).session_scope_by_transport_m)))) && (((*result).session_scope_choice = session_scope_by_transport_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Archived", tmp_str.len = sizeof("Archived") - 1, &tmp_str))))) && (((*result).session_scope_choice = session_scope_Archived_tstr_c), true)))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"All", tmp_str.len = sizeof("All") - 1, &tmp_str))))) && (((*result).session_scope_choice = session_scope_All_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_query_scope(
		zcbor_state_t *state, struct session_query_scope *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"scope", tmp_str.len = sizeof("scope") - 1, &tmp_str)))))
	&& (decode_session_scope(state, (&(*result).session_query_scope)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_query_after(
		zcbor_state_t *state, struct session_query_after_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"after", tmp_str.len = sizeof("after") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).session_query_after_session_id_m)))) && (((*result).session_query_after_choice = session_query_after_session_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).session_query_after_choice = session_query_after_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_query_limit(
		zcbor_state_t *state, struct session_query_limit *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"limit", tmp_str.len = sizeof("limit") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).session_query_limit)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_query_since_rev(
		zcbor_state_t *state, struct session_query_since_rev_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"since_rev", tmp_str.len = sizeof("since_rev") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).session_query_since_rev_uint64_m)))) && (((*result).session_query_since_rev_choice = session_query_since_rev_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).session_query_since_rev_choice = session_query_since_rev_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_query(
		zcbor_state_t *state, struct session_query *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).session_query_scope_present), (zcbor_decoder_t *)decode_repeated_session_query_scope, state, (&(*result).session_query_scope))
	&& zcbor_present_decode(&((*result).session_query_after_present), (zcbor_decoder_t *)decode_repeated_session_query_after, state, (&(*result).session_query_after))
	&& zcbor_present_decode(&((*result).session_query_limit_present), (zcbor_decoder_t *)decode_repeated_session_query_limit, state, (&(*result).session_query_limit))
	&& zcbor_present_decode(&((*result).session_query_since_rev_present), (zcbor_decoder_t *)decode_repeated_session_query_since_rev, state, (&(*result).session_query_since_rev))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_session_query_scope(state, (&(*result).session_query_scope));
		decode_repeated_session_query_after(state, (&(*result).session_query_after));
		decode_repeated_session_query_limit(state, (&(*result).session_query_limit));
		decode_repeated_session_query_since_rev(state, (&(*result).session_query_since_rev));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_sessions_query(
		zcbor_state_t *state, struct request_sessions_query *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionsQuery", tmp_str.len = sizeof("SessionsQuery") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"query", tmp_str.len = sizeof("query") - 1, &tmp_str)))))
	&& (decode_session_query(state, (&(*result).SessionsQuery_query))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_session_get(
		zcbor_state_t *state, struct request_session_get *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionGet", tmp_str.len = sizeof("SessionGet") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SessionGet_session))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_session_search(
		zcbor_state_t *state, struct request_session_search *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionSearch", tmp_str.len = sizeof("SessionSearch") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"query", tmp_str.len = sizeof("query") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SessionSearch_query))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"limit", tmp_str.len = sizeof("limit") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).SessionSearch_limit))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_meta_patch_title(
		zcbor_state_t *state, struct session_meta_patch_title_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).session_meta_patch_title_tstr)))) && (((*result).session_meta_patch_title_choice = session_meta_patch_title_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).session_meta_patch_title_choice = session_meta_patch_title_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_meta_patch_pinned(
		zcbor_state_t *state, struct session_meta_patch_pinned_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"pinned", tmp_str.len = sizeof("pinned") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_bool_decode(state, (&(*result).session_meta_patch_pinned_bool)))) && (((*result).session_meta_patch_pinned_choice = session_meta_patch_pinned_bool_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).session_meta_patch_pinned_choice = session_meta_patch_pinned_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_meta_patch_archived(
		zcbor_state_t *state, struct session_meta_patch_archived_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"archived", tmp_str.len = sizeof("archived") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_bool_decode(state, (&(*result).session_meta_patch_archived_bool)))) && (((*result).session_meta_patch_archived_choice = session_meta_patch_archived_bool_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).session_meta_patch_archived_choice = session_meta_patch_archived_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_meta_patch(
		zcbor_state_t *state, struct session_meta_patch *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).session_meta_patch_title_present), (zcbor_decoder_t *)decode_repeated_session_meta_patch_title, state, (&(*result).session_meta_patch_title))
	&& zcbor_present_decode(&((*result).session_meta_patch_pinned_present), (zcbor_decoder_t *)decode_repeated_session_meta_patch_pinned, state, (&(*result).session_meta_patch_pinned))
	&& zcbor_present_decode(&((*result).session_meta_patch_archived_present), (zcbor_decoder_t *)decode_repeated_session_meta_patch_archived, state, (&(*result).session_meta_patch_archived))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_session_meta_patch_title(state, (&(*result).session_meta_patch_title));
		decode_repeated_session_meta_patch_pinned(state, (&(*result).session_meta_patch_pinned));
		decode_repeated_session_meta_patch_archived(state, (&(*result).session_meta_patch_archived));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_session_update_meta(
		zcbor_state_t *state, struct request_session_update_meta *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionUpdateMeta", tmp_str.len = sizeof("SessionUpdateMeta") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SessionUpdateMeta_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"patch", tmp_str.len = sizeof("patch") - 1, &tmp_str)))))
	&& (decode_session_meta_patch(state, (&(*result).SessionUpdateMeta_patch))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_rewind_point_restore_workspace(
		zcbor_state_t *state, struct rewind_point_restore_workspace *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"restore_workspace", tmp_str.len = sizeof("restore_workspace") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).rewind_point_restore_workspace)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_rewind_point(
		zcbor_state_t *state, struct rewind_point *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"anchor", tmp_str.len = sizeof("anchor") - 1, &tmp_str)))))
	&& (decode_rewind_anchor(state, (&(*result).rewind_point_anchor))))
	&& zcbor_present_decode(&((*result).rewind_point_restore_workspace_present), (zcbor_decoder_t *)decode_repeated_rewind_point_restore_workspace, state, (&(*result).rewind_point_restore_workspace))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_rewind_point_restore_workspace(state, (&(*result).rewind_point_restore_workspace));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_rewind(
		zcbor_state_t *state, struct request_rewind *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Rewind", tmp_str.len = sizeof("Rewind") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Rewind_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"point", tmp_str.len = sizeof("point") - 1, &tmp_str)))))
	&& (decode_rewind_point(state, (&(*result).Rewind_point))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_acp_recipe_program(
		zcbor_state_t *state, struct acp_recipe_program_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"program", tmp_str.len = sizeof("program") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).acp_recipe_program_tstr)))) && (((*result).acp_recipe_program_choice = acp_recipe_program_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).acp_recipe_program_choice = acp_recipe_program_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_acp_recipe_args(
		zcbor_state_t *state, struct acp_recipe_args_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"args", tmp_str.len = sizeof("args") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).acp_recipe_args_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).acp_recipe_args_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).acp_recipe_args_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_kv_pair(
		zcbor_state_t *state, struct kv_pair *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_decode(state) && ((((zcbor_tstr_decode(state, (&(*result).kv_pair_k))))
	&& ((zcbor_tstr_decode(state, (&(*result).kv_pair_v))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_acp_recipe_env(
		zcbor_state_t *state, struct acp_recipe_env_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"env", tmp_str.len = sizeof("env") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).acp_recipe_env_kv_pair_m_count, (zcbor_decoder_t *)decode_kv_pair, state, (*&(*result).acp_recipe_env_kv_pair_m), sizeof(struct kv_pair))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_kv_pair(state, (*&(*result).acp_recipe_env_kv_pair_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_acp_recipe_endpoint(
		zcbor_state_t *state, struct acp_recipe_endpoint_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"endpoint", tmp_str.len = sizeof("endpoint") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).acp_recipe_endpoint_tstr)))) && (((*result).acp_recipe_endpoint_choice = acp_recipe_endpoint_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).acp_recipe_endpoint_choice = acp_recipe_endpoint_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_acp_recipe(
		zcbor_state_t *state, struct acp_recipe *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).acp_recipe_program_present), (zcbor_decoder_t *)decode_repeated_acp_recipe_program, state, (&(*result).acp_recipe_program))
	&& zcbor_present_decode(&((*result).acp_recipe_args_present), (zcbor_decoder_t *)decode_repeated_acp_recipe_args, state, (&(*result).acp_recipe_args))
	&& zcbor_present_decode(&((*result).acp_recipe_env_present), (zcbor_decoder_t *)decode_repeated_acp_recipe_env, state, (&(*result).acp_recipe_env))
	&& zcbor_present_decode(&((*result).acp_recipe_endpoint_present), (zcbor_decoder_t *)decode_repeated_acp_recipe_endpoint, state, (&(*result).acp_recipe_endpoint))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_acp_recipe_program(state, (&(*result).acp_recipe_program));
		decode_repeated_acp_recipe_args(state, (&(*result).acp_recipe_args));
		decode_repeated_acp_recipe_env(state, (&(*result).acp_recipe_env));
		decode_repeated_acp_recipe_endpoint(state, (&(*result).acp_recipe_endpoint));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_acp_source(
		zcbor_state_t *state, struct acp_source_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Builtin", tmp_str.len = sizeof("Builtin") - 1, &tmp_str))))) && (((*result).acp_source_choice = acp_source_Builtin_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Manual", tmp_str.len = sizeof("Manual") - 1, &tmp_str))))) && (((*result).acp_source_choice = acp_source_Manual_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Endpoint", tmp_str.len = sizeof("Endpoint") - 1, &tmp_str))))) && (((*result).acp_source_choice = acp_source_Endpoint_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_acp_agent_entry_installed(
		zcbor_state_t *state, struct acp_agent_entry_installed *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"installed", tmp_str.len = sizeof("installed") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).acp_agent_entry_installed)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_acp_agent_entry_version(
		zcbor_state_t *state, struct acp_agent_entry_version_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"version", tmp_str.len = sizeof("version") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).acp_agent_entry_version_tstr)))) && (((*result).acp_agent_entry_version_choice = acp_agent_entry_version_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).acp_agent_entry_version_choice = acp_agent_entry_version_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_acp_agent_entry_capabilities(
		zcbor_state_t *state, struct acp_agent_entry_capabilities_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"capabilities", tmp_str.len = sizeof("capabilities") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).acp_agent_entry_capabilities_kv_pair_m_count, (zcbor_decoder_t *)decode_kv_pair, state, (*&(*result).acp_agent_entry_capabilities_kv_pair_m), sizeof(struct kv_pair))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_kv_pair(state, (*&(*result).acp_agent_entry_capabilities_kv_pair_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_acp_agent_entry(
		zcbor_state_t *state, struct acp_agent_entry *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).acp_agent_entry_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"recipe", tmp_str.len = sizeof("recipe") - 1, &tmp_str)))))
	&& (decode_acp_recipe(state, (&(*result).acp_agent_entry_recipe))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (decode_acp_source(state, (&(*result).acp_agent_entry_source))))
	&& zcbor_present_decode(&((*result).acp_agent_entry_installed_present), (zcbor_decoder_t *)decode_repeated_acp_agent_entry_installed, state, (&(*result).acp_agent_entry_installed))
	&& zcbor_present_decode(&((*result).acp_agent_entry_version_present), (zcbor_decoder_t *)decode_repeated_acp_agent_entry_version, state, (&(*result).acp_agent_entry_version))
	&& zcbor_present_decode(&((*result).acp_agent_entry_capabilities_present), (zcbor_decoder_t *)decode_repeated_acp_agent_entry_capabilities, state, (&(*result).acp_agent_entry_capabilities))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_acp_agent_entry_installed(state, (&(*result).acp_agent_entry_installed));
		decode_repeated_acp_agent_entry_version(state, (&(*result).acp_agent_entry_version));
		decode_repeated_acp_agent_entry_capabilities(state, (&(*result).acp_agent_entry_capabilities));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_acp_register(
		zcbor_state_t *state, struct request_acp_register *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AcpRegister", tmp_str.len = sizeof("AcpRegister") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"entry", tmp_str.len = sizeof("entry") - 1, &tmp_str)))))
	&& (decode_acp_agent_entry(state, (&(*result).AcpRegister_entry))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_acp_remove(
		zcbor_state_t *state, struct request_acp_remove *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AcpRemove", tmp_str.len = sizeof("AcpRemove") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).AcpRemove_name))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_skill_get(
		zcbor_state_t *state, struct request_skill_get *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SkillGet", tmp_str.len = sizeof("SkillGet") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SkillGet_name))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_skill_put(
		zcbor_state_t *state, struct request_skill_put *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SkillPut", tmp_str.len = sizeof("SkillPut") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"bundle", tmp_str.len = sizeof("bundle") - 1, &tmp_str)))))
	&& (decode_skill_bundle(state, (&(*result).SkillPut_bundle))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_provider_info_base_url(
		zcbor_state_t *state, struct provider_info_base_url_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"base_url", tmp_str.len = sizeof("base_url") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).provider_info_base_url_tstr)))) && (((*result).provider_info_base_url_choice = provider_info_base_url_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).provider_info_base_url_choice = provider_info_base_url_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_provider_info_available(
		zcbor_state_t *state, struct provider_info_available *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"available", tmp_str.len = sizeof("available") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).provider_info_available)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_provider_info(
		zcbor_state_t *state, struct provider_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).provider_info_name))))
	&& zcbor_present_decode(&((*result).provider_info_base_url_present), (zcbor_decoder_t *)decode_repeated_provider_info_base_url, state, (&(*result).provider_info_base_url))
	&& zcbor_present_decode(&((*result).provider_info_available_present), (zcbor_decoder_t *)decode_repeated_provider_info_available, state, (&(*result).provider_info_available))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_provider_info_base_url(state, (&(*result).provider_info_base_url));
		decode_repeated_provider_info_available(state, (&(*result).provider_info_available));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_provider_register(
		zcbor_state_t *state, struct request_provider_register *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProviderRegister", tmp_str.len = sizeof("ProviderRegister") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (decode_provider_info(state, (&(*result).ProviderRegister_provider))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_tool_info_description(
		zcbor_state_t *state, struct tool_info_description_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"description", tmp_str.len = sizeof("description") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).tool_info_description_tstr)))) && (((*result).tool_info_description_choice = tool_info_description_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).tool_info_description_choice = tool_info_description_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_tool_info(
		zcbor_state_t *state, struct tool_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).tool_info_name))))
	&& zcbor_present_decode(&((*result).tool_info_description_present), (zcbor_decoder_t *)decode_repeated_tool_info_description, state, (&(*result).tool_info_description))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_tool_info_description(state, (&(*result).tool_info_description));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_tool_register(
		zcbor_state_t *state, struct request_tool_register *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ToolRegister", tmp_str.len = sizeof("ToolRegister") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"tool", tmp_str.len = sizeof("tool") - 1, &tmp_str)))))
	&& (decode_tool_info(state, (&(*result).ToolRegister_tool))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_command_invocation(
		zcbor_state_t *state, struct command_invocation *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).command_invocation_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"args", tmp_str.len = sizeof("args") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).command_invocation_args))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).command_invocation_session_session_id_m)))) && (((*result).command_invocation_session_choice = command_invocation_session_session_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).command_invocation_session_choice = command_invocation_session_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_origin(state, (&(*result).command_invocation_origin_origin_m)))) && (((*result).command_invocation_origin_choice = command_invocation_origin_origin_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).command_invocation_origin_choice = command_invocation_origin_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_command_invoke(
		zcbor_state_t *state, struct request_command_invoke *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CommandInvoke", tmp_str.len = sizeof("CommandInvoke") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"invocation", tmp_str.len = sizeof("invocation") - 1, &tmp_str)))))
	&& (decode_command_invocation(state, (&(*result).CommandInvoke_invocation))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_node_config_view(
		zcbor_state_t *state, struct node_config_view *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"format", tmp_str.len = sizeof("format") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).node_config_view_format))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).node_config_view_body))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_config_set(
		zcbor_state_t *state, struct request_config_set *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConfigSet", tmp_str.len = sizeof("ConfigSet") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"config", tmp_str.len = sizeof("config") - 1, &tmp_str)))))
	&& (decode_node_config_view(state, (&(*result).ConfigSet_config))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_target(
		zcbor_state_t *state, struct cron_spec_target_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"target", tmp_str.len = sizeof("target") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).cron_spec_target_tstr)))) && (((*result).cron_spec_target_choice = cron_spec_target_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_spec_target_choice = cron_spec_target_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_payload(
		zcbor_state_t *state, struct cron_spec_payload *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"payload", tmp_str.len = sizeof("payload") - 1, &tmp_str)))))
	&& (zcbor_bstr_decode(state, (&(*result).cron_spec_payload)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_enabled(
		zcbor_state_t *state, struct cron_spec_enabled *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"enabled", tmp_str.len = sizeof("enabled") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).cron_spec_enabled)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_timezone(
		zcbor_state_t *state, struct cron_spec_timezone_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"timezone", tmp_str.len = sizeof("timezone") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).cron_spec_timezone_tstr)))) && (((*result).cron_spec_timezone_choice = cron_spec_timezone_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_spec_timezone_choice = cron_spec_timezone_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_repeat(
		zcbor_state_t *state, struct cron_spec_repeat_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"repeat", tmp_str.len = sizeof("repeat") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint32_decode(state, (&(*result).cron_spec_repeat_uint)))) && (((*result).cron_spec_repeat_choice = cron_spec_repeat_uint_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_spec_repeat_choice = cron_spec_repeat_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_jitter_secs(
		zcbor_state_t *state, struct cron_spec_jitter_secs_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"jitter_secs", tmp_str.len = sizeof("jitter_secs") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint32_decode(state, (&(*result).cron_spec_jitter_secs_uint)))) && (((*result).cron_spec_jitter_secs_choice = cron_spec_jitter_secs_uint_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_spec_jitter_secs_choice = cron_spec_jitter_secs_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_overlap_policy(
		zcbor_state_t *state, struct overlap_policy_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Skip", tmp_str.len = sizeof("Skip") - 1, &tmp_str))))) && (((*result).overlap_policy_choice = overlap_policy_Skip_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Allow", tmp_str.len = sizeof("Allow") - 1, &tmp_str))))) && (((*result).overlap_policy_choice = overlap_policy_Allow_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Queue", tmp_str.len = sizeof("Queue") - 1, &tmp_str))))) && (((*result).overlap_policy_choice = overlap_policy_Queue_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_overlap(
		zcbor_state_t *state, struct cron_spec_overlap *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"overlap", tmp_str.len = sizeof("overlap") - 1, &tmp_str)))))
	&& (decode_overlap_policy(state, (&(*result).cron_spec_overlap)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_catch_up_policy(
		zcbor_state_t *state, struct catch_up_policy_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Grace", tmp_str.len = sizeof("Grace") - 1, &tmp_str))))) && (((*result).catch_up_policy_choice = catch_up_policy_Grace_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Skip", tmp_str.len = sizeof("Skip") - 1, &tmp_str))))) && (((*result).catch_up_policy_choice = catch_up_policy_Skip_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Always", tmp_str.len = sizeof("Always") - 1, &tmp_str))))) && (((*result).catch_up_policy_choice = catch_up_policy_Always_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_catch_up(
		zcbor_state_t *state, struct cron_spec_catch_up *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"catch_up", tmp_str.len = sizeof("catch_up") - 1, &tmp_str)))))
	&& (decode_catch_up_policy(state, (&(*result).cron_spec_catch_up)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_script(
		zcbor_state_t *state, struct cron_spec_script_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"script", tmp_str.len = sizeof("script") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).cron_spec_script_tstr)))) && (((*result).cron_spec_script_choice = cron_spec_script_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_spec_script_choice = cron_spec_script_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_no_agent(
		zcbor_state_t *state, struct cron_spec_no_agent *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"no_agent", tmp_str.len = sizeof("no_agent") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).cron_spec_no_agent)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_context_from(
		zcbor_state_t *state, struct cron_spec_context_from_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"context_from", tmp_str.len = sizeof("context_from") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).cron_spec_context_from_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).cron_spec_context_from_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).cron_spec_context_from_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_deliver(
		zcbor_state_t *state, struct cron_spec_deliver_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"deliver", tmp_str.len = sizeof("deliver") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).cron_spec_deliver_tstr)))) && (((*result).cron_spec_deliver_choice = cron_spec_deliver_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_spec_deliver_choice = cron_spec_deliver_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_enabled_toolsets(
		zcbor_state_t *state, struct cron_spec_enabled_toolsets_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"enabled_toolsets", tmp_str.len = sizeof("enabled_toolsets") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).enabled_toolsets_tstr_l_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).enabled_toolsets_tstr_l_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))) && (((*result).cron_spec_enabled_toolsets_choice = enabled_toolsets_tstr_l_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).cron_spec_enabled_toolsets_choice = cron_spec_enabled_toolsets_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).enabled_toolsets_tstr_l_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_workdir(
		zcbor_state_t *state, struct cron_spec_workdir_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"workdir", tmp_str.len = sizeof("workdir") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).cron_spec_workdir_tstr)))) && (((*result).cron_spec_workdir_choice = cron_spec_workdir_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_spec_workdir_choice = cron_spec_workdir_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_model(
		zcbor_state_t *state, struct cron_spec_model_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).cron_spec_model_tstr)))) && (((*result).cron_spec_model_choice = cron_spec_model_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_spec_model_choice = cron_spec_model_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_provider(
		zcbor_state_t *state, struct cron_spec_provider_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).cron_spec_provider_tstr)))) && (((*result).cron_spec_provider_choice = cron_spec_provider_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_spec_provider_choice = cron_spec_provider_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_skills(
		zcbor_state_t *state, struct cron_spec_skills_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"skills", tmp_str.len = sizeof("skills") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).cron_spec_skills_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).cron_spec_skills_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).cron_spec_skills_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_spec_origin(
		zcbor_state_t *state, struct cron_spec_origin_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_origin(state, (&(*result).cron_spec_origin_origin_m)))) && (((*result).cron_spec_origin_choice = cron_spec_origin_origin_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).cron_spec_origin_choice = cron_spec_origin_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_cron_spec(
		zcbor_state_t *state, struct cron_spec *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).cron_spec_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"schedule", tmp_str.len = sizeof("schedule") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).cron_spec_schedule))))
	&& zcbor_present_decode(&((*result).cron_spec_target_present), (zcbor_decoder_t *)decode_repeated_cron_spec_target, state, (&(*result).cron_spec_target))
	&& zcbor_present_decode(&((*result).cron_spec_payload_present), (zcbor_decoder_t *)decode_repeated_cron_spec_payload, state, (&(*result).cron_spec_payload))
	&& zcbor_present_decode(&((*result).cron_spec_enabled_present), (zcbor_decoder_t *)decode_repeated_cron_spec_enabled, state, (&(*result).cron_spec_enabled))
	&& zcbor_present_decode(&((*result).cron_spec_timezone_present), (zcbor_decoder_t *)decode_repeated_cron_spec_timezone, state, (&(*result).cron_spec_timezone))
	&& zcbor_present_decode(&((*result).cron_spec_repeat_present), (zcbor_decoder_t *)decode_repeated_cron_spec_repeat, state, (&(*result).cron_spec_repeat))
	&& zcbor_present_decode(&((*result).cron_spec_jitter_secs_present), (zcbor_decoder_t *)decode_repeated_cron_spec_jitter_secs, state, (&(*result).cron_spec_jitter_secs))
	&& zcbor_present_decode(&((*result).cron_spec_overlap_present), (zcbor_decoder_t *)decode_repeated_cron_spec_overlap, state, (&(*result).cron_spec_overlap))
	&& zcbor_present_decode(&((*result).cron_spec_catch_up_present), (zcbor_decoder_t *)decode_repeated_cron_spec_catch_up, state, (&(*result).cron_spec_catch_up))
	&& zcbor_present_decode(&((*result).cron_spec_script_present), (zcbor_decoder_t *)decode_repeated_cron_spec_script, state, (&(*result).cron_spec_script))
	&& zcbor_present_decode(&((*result).cron_spec_no_agent_present), (zcbor_decoder_t *)decode_repeated_cron_spec_no_agent, state, (&(*result).cron_spec_no_agent))
	&& zcbor_present_decode(&((*result).cron_spec_context_from_present), (zcbor_decoder_t *)decode_repeated_cron_spec_context_from, state, (&(*result).cron_spec_context_from))
	&& zcbor_present_decode(&((*result).cron_spec_deliver_present), (zcbor_decoder_t *)decode_repeated_cron_spec_deliver, state, (&(*result).cron_spec_deliver))
	&& zcbor_present_decode(&((*result).cron_spec_enabled_toolsets_present), (zcbor_decoder_t *)decode_repeated_cron_spec_enabled_toolsets, state, (&(*result).cron_spec_enabled_toolsets))
	&& zcbor_present_decode(&((*result).cron_spec_workdir_present), (zcbor_decoder_t *)decode_repeated_cron_spec_workdir, state, (&(*result).cron_spec_workdir))
	&& zcbor_present_decode(&((*result).cron_spec_model_present), (zcbor_decoder_t *)decode_repeated_cron_spec_model, state, (&(*result).cron_spec_model))
	&& zcbor_present_decode(&((*result).cron_spec_provider_present), (zcbor_decoder_t *)decode_repeated_cron_spec_provider, state, (&(*result).cron_spec_provider))
	&& zcbor_present_decode(&((*result).cron_spec_skills_present), (zcbor_decoder_t *)decode_repeated_cron_spec_skills, state, (&(*result).cron_spec_skills))
	&& zcbor_present_decode(&((*result).cron_spec_origin_present), (zcbor_decoder_t *)decode_repeated_cron_spec_origin, state, (&(*result).cron_spec_origin))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_cron_spec_target(state, (&(*result).cron_spec_target));
		decode_repeated_cron_spec_payload(state, (&(*result).cron_spec_payload));
		decode_repeated_cron_spec_enabled(state, (&(*result).cron_spec_enabled));
		decode_repeated_cron_spec_timezone(state, (&(*result).cron_spec_timezone));
		decode_repeated_cron_spec_repeat(state, (&(*result).cron_spec_repeat));
		decode_repeated_cron_spec_jitter_secs(state, (&(*result).cron_spec_jitter_secs));
		decode_repeated_cron_spec_overlap(state, (&(*result).cron_spec_overlap));
		decode_repeated_cron_spec_catch_up(state, (&(*result).cron_spec_catch_up));
		decode_repeated_cron_spec_script(state, (&(*result).cron_spec_script));
		decode_repeated_cron_spec_no_agent(state, (&(*result).cron_spec_no_agent));
		decode_repeated_cron_spec_context_from(state, (&(*result).cron_spec_context_from));
		decode_repeated_cron_spec_deliver(state, (&(*result).cron_spec_deliver));
		decode_repeated_cron_spec_enabled_toolsets(state, (&(*result).cron_spec_enabled_toolsets));
		decode_repeated_cron_spec_workdir(state, (&(*result).cron_spec_workdir));
		decode_repeated_cron_spec_model(state, (&(*result).cron_spec_model));
		decode_repeated_cron_spec_provider(state, (&(*result).cron_spec_provider));
		decode_repeated_cron_spec_skills(state, (&(*result).cron_spec_skills));
		decode_repeated_cron_spec_origin(state, (&(*result).cron_spec_origin));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_cron_create(
		zcbor_state_t *state, struct request_cron_create *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronCreate", tmp_str.len = sizeof("CronCreate") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (decode_cron_spec(state, (&(*result).CronCreate_spec))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_cron_update(
		zcbor_state_t *state, struct request_cron_update *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronUpdate", tmp_str.len = sizeof("CronUpdate") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CronUpdate_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (decode_cron_spec(state, (&(*result).CronUpdate_spec))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_cron_delete(
		zcbor_state_t *state, struct request_cron_delete *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronDelete", tmp_str.len = sizeof("CronDelete") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CronDelete_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_cron_trigger(
		zcbor_state_t *state, struct request_cron_trigger *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronTrigger", tmp_str.len = sizeof("CronTrigger") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CronTrigger_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_cron_runs(
		zcbor_state_t *state, struct request_cron_runs *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronRuns", tmp_str.len = sizeof("CronRuns") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CronRuns_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_cron_pause(
		zcbor_state_t *state, struct request_cron_pause *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronPause", tmp_str.len = sizeof("CronPause") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CronPause_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"paused", tmp_str.len = sizeof("paused") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).CronPause_paused))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_cron_accept_suggestion(
		zcbor_state_t *state, struct request_cron_accept_suggestion *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronAcceptSuggestion", tmp_str.len = sizeof("CronAcceptSuggestion") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CronAcceptSuggestion_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_cron_dismiss_suggestion(
		zcbor_state_t *state, struct request_cron_dismiss_suggestion *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronDismissSuggestion", tmp_str.len = sizeof("CronDismissSuggestion") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).CronDismissSuggestion_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_routing_get(
		zcbor_state_t *state, struct request_routing_get *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"RoutingGet", tmp_str.len = sizeof("RoutingGet") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (decode_origin(state, (&(*result).RoutingGet_origin))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_chat_route_profile(
		zcbor_state_t *state, struct chat_route_profile_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).chat_route_profile_profile_ref_m)))) && (((*result).chat_route_profile_choice = chat_route_profile_profile_ref_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).chat_route_profile_choice = chat_route_profile_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_isolation_policy(
		zcbor_state_t *state, struct isolation_policy_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"PerUser", tmp_str.len = sizeof("PerUser") - 1, &tmp_str))))) && (((*result).isolation_policy_choice = isolation_policy_PerUser_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"PerChat", tmp_str.len = sizeof("PerChat") - 1, &tmp_str))))) && (((*result).isolation_policy_choice = isolation_policy_PerChat_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"PerThread", tmp_str.len = sizeof("PerThread") - 1, &tmp_str))))) && (((*result).isolation_policy_choice = isolation_policy_PerThread_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Shared", tmp_str.len = sizeof("Shared") - 1, &tmp_str))))) && (((*result).isolation_policy_choice = isolation_policy_Shared_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_chat_route_isolation(
		zcbor_state_t *state, struct chat_route_isolation *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"isolation", tmp_str.len = sizeof("isolation") - 1, &tmp_str)))))
	&& (decode_isolation_policy(state, (&(*result).chat_route_isolation)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_chat_route(
		zcbor_state_t *state, struct chat_route *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (decode_origin(state, (&(*result).chat_route_origin))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).chat_route_session))))
	&& zcbor_present_decode(&((*result).chat_route_profile_present), (zcbor_decoder_t *)decode_repeated_chat_route_profile, state, (&(*result).chat_route_profile))
	&& zcbor_present_decode(&((*result).chat_route_isolation_present), (zcbor_decoder_t *)decode_repeated_chat_route_isolation, state, (&(*result).chat_route_isolation))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_chat_route_profile(state, (&(*result).chat_route_profile));
		decode_repeated_chat_route_isolation(state, (&(*result).chat_route_isolation));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_routing_set(
		zcbor_state_t *state, struct request_routing_set *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"RoutingSet", tmp_str.len = sizeof("RoutingSet") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"route", tmp_str.len = sizeof("route") - 1, &tmp_str)))))
	&& (decode_chat_route(state, (&(*result).RoutingSet_route))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_RoutingBindChat_profile(
		zcbor_state_t *state, struct RoutingBindChat_profile_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).RoutingBindChat_profile_profile_ref_m)))) && (((*result).RoutingBindChat_profile_choice = RoutingBindChat_profile_profile_ref_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).RoutingBindChat_profile_choice = RoutingBindChat_profile_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_routing_bind_chat(
		zcbor_state_t *state, struct request_routing_bind_chat *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"RoutingBindChat", tmp_str.len = sizeof("RoutingBindChat") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (decode_origin(state, (&(*result).RoutingBindChat_origin))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).RoutingBindChat_session))))
	&& zcbor_present_decode(&((*result).RoutingBindChat_profile_present), (zcbor_decoder_t *)decode_repeated_RoutingBindChat_profile, state, (&(*result).RoutingBindChat_profile))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_RoutingBindChat_profile(state, (&(*result).RoutingBindChat_profile));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_routing_unbind_chat(
		zcbor_state_t *state, struct request_routing_unbind_chat *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"RoutingUnbindChat", tmp_str.len = sizeof("RoutingUnbindChat") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (decode_origin(state, (&(*result).RoutingUnbindChat_origin))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_transport_rooms(
		zcbor_state_t *state, struct request_transport_rooms *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"TransportRooms", tmp_str.len = sizeof("TransportRooms") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).TransportRooms_transport))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_list(
		zcbor_state_t *state, struct request_conv_list *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvList", tmp_str.len = sizeof("ConvList") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvList_transport))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_get(
		zcbor_state_t *state, struct request_conv_get *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvGet", tmp_str.len = sizeof("ConvGet") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvGet_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvGet_conv))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_create_details(
		zcbor_state_t *state, struct request_conv_create_details *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvCreateDetails", tmp_str.len = sizeof("ConvCreateDetails") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvCreateDetails_transport))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_create_conversation_details_max_participants(
		zcbor_state_t *state, struct create_conversation_details_max_participants *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max_participants", tmp_str.len = sizeof("max_participants") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).create_conversation_details_max_participants)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_contact_info_display_name(
		zcbor_state_t *state, struct contact_info_display_name_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"display_name", tmp_str.len = sizeof("display_name") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).contact_info_display_name_tstr)))) && (((*result).contact_info_display_name_choice = contact_info_display_name_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).contact_info_display_name_choice = contact_info_display_name_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_presence_primitive_t(
		zcbor_state_t *state, struct presence_primitive_t_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Offline", tmp_str.len = sizeof("Offline") - 1, &tmp_str))))) && (((*result).presence_primitive_t_choice = presence_primitive_t_Offline_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Available", tmp_str.len = sizeof("Available") - 1, &tmp_str))))) && (((*result).presence_primitive_t_choice = presence_primitive_t_Available_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Idle", tmp_str.len = sizeof("Idle") - 1, &tmp_str))))) && (((*result).presence_primitive_t_choice = presence_primitive_t_Idle_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Invisible", tmp_str.len = sizeof("Invisible") - 1, &tmp_str))))) && (((*result).presence_primitive_t_choice = presence_primitive_t_Invisible_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Away", tmp_str.len = sizeof("Away") - 1, &tmp_str))))) && (((*result).presence_primitive_t_choice = presence_primitive_t_Away_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"DoNotDisturb", tmp_str.len = sizeof("DoNotDisturb") - 1, &tmp_str))))) && (((*result).presence_primitive_t_choice = presence_primitive_t_DoNotDisturb_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Streaming", tmp_str.len = sizeof("Streaming") - 1, &tmp_str))))) && (((*result).presence_primitive_t_choice = presence_primitive_t_Streaming_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"OutOfOffice", tmp_str.len = sizeof("OutOfOffice") - 1, &tmp_str))))) && (((*result).presence_primitive_t_choice = presence_primitive_t_OutOfOffice_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_presence_message(
		zcbor_state_t *state, struct presence_message_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"message", tmp_str.len = sizeof("message") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).presence_message_tstr)))) && (((*result).presence_message_choice = presence_message_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).presence_message_choice = presence_message_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_presence_emoji(
		zcbor_state_t *state, struct presence_emoji_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"emoji", tmp_str.len = sizeof("emoji") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).presence_emoji_tstr)))) && (((*result).presence_emoji_choice = presence_emoji_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).presence_emoji_choice = presence_emoji_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_presence_mobile(
		zcbor_state_t *state, struct presence_mobile *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"mobile", tmp_str.len = sizeof("mobile") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).presence_mobile)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_presence_idle_since(
		zcbor_state_t *state, struct presence_idle_since_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"idle_since", tmp_str.len = sizeof("idle_since") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).presence_idle_since_uint64_m)))) && (((*result).presence_idle_since_choice = presence_idle_since_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).presence_idle_since_choice = presence_idle_since_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_presence(
		zcbor_state_t *state, struct presence *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"primitive", tmp_str.len = sizeof("primitive") - 1, &tmp_str)))))
	&& (decode_presence_primitive_t(state, (&(*result).presence_primitive))))
	&& zcbor_present_decode(&((*result).presence_message_present), (zcbor_decoder_t *)decode_repeated_presence_message, state, (&(*result).presence_message))
	&& zcbor_present_decode(&((*result).presence_emoji_present), (zcbor_decoder_t *)decode_repeated_presence_emoji, state, (&(*result).presence_emoji))
	&& zcbor_present_decode(&((*result).presence_mobile_present), (zcbor_decoder_t *)decode_repeated_presence_mobile, state, (&(*result).presence_mobile))
	&& zcbor_present_decode(&((*result).presence_idle_since_present), (zcbor_decoder_t *)decode_repeated_presence_idle_since, state, (&(*result).presence_idle_since))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_presence_message(state, (&(*result).presence_message));
		decode_repeated_presence_emoji(state, (&(*result).presence_emoji));
		decode_repeated_presence_mobile(state, (&(*result).presence_mobile));
		decode_repeated_presence_idle_since(state, (&(*result).presence_idle_since));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_contact_info_presence(
		zcbor_state_t *state, struct contact_info_presence *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"presence", tmp_str.len = sizeof("presence") - 1, &tmp_str)))))
	&& (decode_presence(state, (&(*result).contact_info_presence)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_contact_permission(
		zcbor_state_t *state, struct contact_permission_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Unset", tmp_str.len = sizeof("Unset") - 1, &tmp_str))))) && (((*result).contact_permission_choice = contact_permission_Unset_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Allow", tmp_str.len = sizeof("Allow") - 1, &tmp_str))))) && (((*result).contact_permission_choice = contact_permission_Allow_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Deny", tmp_str.len = sizeof("Deny") - 1, &tmp_str))))) && (((*result).contact_permission_choice = contact_permission_Deny_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_contact_info_permission(
		zcbor_state_t *state, struct contact_info_permission *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"permission", tmp_str.len = sizeof("permission") - 1, &tmp_str)))))
	&& (decode_contact_permission(state, (&(*result).contact_info_permission)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_contact_info(
		zcbor_state_t *state, struct contact_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).contact_info_id))))
	&& zcbor_present_decode(&((*result).contact_info_display_name_present), (zcbor_decoder_t *)decode_repeated_contact_info_display_name, state, (&(*result).contact_info_display_name))
	&& zcbor_present_decode(&((*result).contact_info_presence_present), (zcbor_decoder_t *)decode_repeated_contact_info_presence, state, (&(*result).contact_info_presence))
	&& zcbor_present_decode(&((*result).contact_info_permission_present), (zcbor_decoder_t *)decode_repeated_contact_info_permission, state, (&(*result).contact_info_permission))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_contact_info_display_name(state, (&(*result).contact_info_display_name));
		decode_repeated_contact_info_presence(state, (&(*result).contact_info_presence));
		decode_repeated_contact_info_permission(state, (&(*result).contact_info_permission));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_create_conversation_details_participants(
		zcbor_state_t *state, struct create_conversation_details_participants_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"participants", tmp_str.len = sizeof("participants") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).create_conversation_details_participants_contact_info_m_count, (zcbor_decoder_t *)decode_contact_info, state, (*&(*result).create_conversation_details_participants_contact_info_m), sizeof(struct contact_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_contact_info(state, (*&(*result).create_conversation_details_participants_contact_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_auth_param_field(
		zcbor_state_t *state, struct auth_param_field *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"key", tmp_str.len = sizeof("key") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_param_field_key))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"label", tmp_str.len = sizeof("label") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_param_field_label))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"required", tmp_str.len = sizeof("required") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).auth_param_field_required))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_account_settings_schema_fields(
		zcbor_state_t *state, struct account_settings_schema_fields_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"fields", tmp_str.len = sizeof("fields") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).account_settings_schema_fields_auth_param_field_m_count, (zcbor_decoder_t *)decode_auth_param_field, state, (*&(*result).account_settings_schema_fields_auth_param_field_m), sizeof(struct auth_param_field))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_auth_param_field(state, (*&(*result).account_settings_schema_fields_auth_param_field_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_account_settings_schema(
		zcbor_state_t *state, struct account_settings_schema *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).account_settings_schema_fields_present), (zcbor_decoder_t *)decode_repeated_account_settings_schema_fields, state, (&(*result).account_settings_schema_fields))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_account_settings_schema_fields(state, (&(*result).account_settings_schema_fields));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_create_conversation_details_extras_schema(
		zcbor_state_t *state, struct create_conversation_details_extras_schema *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"extras_schema", tmp_str.len = sizeof("extras_schema") - 1, &tmp_str)))))
	&& (decode_account_settings_schema(state, (&(*result).create_conversation_details_extras_schema)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_values_tstrtstr(
		zcbor_state_t *state, struct values_tstrtstr *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_tstr_decode(state, (&(*result).account_settings_values_values_tstrtstr_key))))
	&& (zcbor_tstr_decode(state, (&(*result).values_tstrtstr)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_account_settings_values_values(
		zcbor_state_t *state, struct account_settings_values_values_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"values", tmp_str.len = sizeof("values") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).values_tstrtstr_count, (zcbor_decoder_t *)decode_repeated_values_tstrtstr, state, (*&(*result).values_tstrtstr), sizeof(struct values_tstrtstr))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_values_tstrtstr(state, (*&(*result).values_tstrtstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_account_settings_values(
		zcbor_state_t *state, struct account_settings_values *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).account_settings_values_values_present), (zcbor_decoder_t *)decode_repeated_account_settings_values_values, state, (&(*result).account_settings_values_values))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_account_settings_values_values(state, (&(*result).account_settings_values_values));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_create_conversation_details_extras(
		zcbor_state_t *state, struct create_conversation_details_extras *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"extras", tmp_str.len = sizeof("extras") - 1, &tmp_str)))))
	&& (decode_account_settings_values(state, (&(*result).create_conversation_details_extras)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_create_conversation_details(
		zcbor_state_t *state, struct create_conversation_details *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).create_conversation_details_max_participants_present), (zcbor_decoder_t *)decode_repeated_create_conversation_details_max_participants, state, (&(*result).create_conversation_details_max_participants))
	&& zcbor_present_decode(&((*result).create_conversation_details_participants_present), (zcbor_decoder_t *)decode_repeated_create_conversation_details_participants, state, (&(*result).create_conversation_details_participants))
	&& zcbor_present_decode(&((*result).create_conversation_details_extras_schema_present), (zcbor_decoder_t *)decode_repeated_create_conversation_details_extras_schema, state, (&(*result).create_conversation_details_extras_schema))
	&& zcbor_present_decode(&((*result).create_conversation_details_extras_present), (zcbor_decoder_t *)decode_repeated_create_conversation_details_extras, state, (&(*result).create_conversation_details_extras))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_create_conversation_details_max_participants(state, (&(*result).create_conversation_details_max_participants));
		decode_repeated_create_conversation_details_participants(state, (&(*result).create_conversation_details_participants));
		decode_repeated_create_conversation_details_extras_schema(state, (&(*result).create_conversation_details_extras_schema));
		decode_repeated_create_conversation_details_extras(state, (&(*result).create_conversation_details_extras));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_create(
		zcbor_state_t *state, struct request_conv_create *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvCreate", tmp_str.len = sizeof("ConvCreate") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvCreate_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"details", tmp_str.len = sizeof("details") - 1, &tmp_str)))))
	&& (decode_create_conversation_details(state, (&(*result).ConvCreate_details))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_join_details(
		zcbor_state_t *state, struct request_conv_join_details *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvJoinDetails", tmp_str.len = sizeof("ConvJoinDetails") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvJoinDetails_transport))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_channel_join_details_name(
		zcbor_state_t *state, struct channel_join_details_name_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).channel_join_details_name_tstr)))) && (((*result).channel_join_details_name_choice = channel_join_details_name_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).channel_join_details_name_choice = channel_join_details_name_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_channel_join_details_name_max_length(
		zcbor_state_t *state, struct channel_join_details_name_max_length *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name_max_length", tmp_str.len = sizeof("name_max_length") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).channel_join_details_name_max_length)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_channel_join_details_nickname(
		zcbor_state_t *state, struct channel_join_details_nickname_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"nickname", tmp_str.len = sizeof("nickname") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).channel_join_details_nickname_tstr)))) && (((*result).channel_join_details_nickname_choice = channel_join_details_nickname_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).channel_join_details_nickname_choice = channel_join_details_nickname_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_channel_join_details_nickname_supported(
		zcbor_state_t *state, struct channel_join_details_nickname_supported *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"nickname_supported", tmp_str.len = sizeof("nickname_supported") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).channel_join_details_nickname_supported)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_channel_join_details_nickname_max_length(
		zcbor_state_t *state, struct channel_join_details_nickname_max_length *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"nickname_max_length", tmp_str.len = sizeof("nickname_max_length") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).channel_join_details_nickname_max_length)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_channel_join_details_password(
		zcbor_state_t *state, struct channel_join_details_password_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"password", tmp_str.len = sizeof("password") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).channel_join_details_password_tstr)))) && (((*result).channel_join_details_password_choice = channel_join_details_password_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).channel_join_details_password_choice = channel_join_details_password_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_channel_join_details_password_supported(
		zcbor_state_t *state, struct channel_join_details_password_supported *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"password_supported", tmp_str.len = sizeof("password_supported") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).channel_join_details_password_supported)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_channel_join_details_password_max_length(
		zcbor_state_t *state, struct channel_join_details_password_max_length *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"password_max_length", tmp_str.len = sizeof("password_max_length") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).channel_join_details_password_max_length)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_channel_join_details_extras_schema(
		zcbor_state_t *state, struct channel_join_details_extras_schema *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"extras_schema", tmp_str.len = sizeof("extras_schema") - 1, &tmp_str)))))
	&& (decode_account_settings_schema(state, (&(*result).channel_join_details_extras_schema)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_channel_join_details_extras(
		zcbor_state_t *state, struct channel_join_details_extras *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"extras", tmp_str.len = sizeof("extras") - 1, &tmp_str)))))
	&& (decode_account_settings_values(state, (&(*result).channel_join_details_extras)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_channel_join_details(
		zcbor_state_t *state, struct channel_join_details *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).channel_join_details_name_present), (zcbor_decoder_t *)decode_repeated_channel_join_details_name, state, (&(*result).channel_join_details_name))
	&& zcbor_present_decode(&((*result).channel_join_details_name_max_length_present), (zcbor_decoder_t *)decode_repeated_channel_join_details_name_max_length, state, (&(*result).channel_join_details_name_max_length))
	&& zcbor_present_decode(&((*result).channel_join_details_nickname_present), (zcbor_decoder_t *)decode_repeated_channel_join_details_nickname, state, (&(*result).channel_join_details_nickname))
	&& zcbor_present_decode(&((*result).channel_join_details_nickname_supported_present), (zcbor_decoder_t *)decode_repeated_channel_join_details_nickname_supported, state, (&(*result).channel_join_details_nickname_supported))
	&& zcbor_present_decode(&((*result).channel_join_details_nickname_max_length_present), (zcbor_decoder_t *)decode_repeated_channel_join_details_nickname_max_length, state, (&(*result).channel_join_details_nickname_max_length))
	&& zcbor_present_decode(&((*result).channel_join_details_password_present), (zcbor_decoder_t *)decode_repeated_channel_join_details_password, state, (&(*result).channel_join_details_password))
	&& zcbor_present_decode(&((*result).channel_join_details_password_supported_present), (zcbor_decoder_t *)decode_repeated_channel_join_details_password_supported, state, (&(*result).channel_join_details_password_supported))
	&& zcbor_present_decode(&((*result).channel_join_details_password_max_length_present), (zcbor_decoder_t *)decode_repeated_channel_join_details_password_max_length, state, (&(*result).channel_join_details_password_max_length))
	&& zcbor_present_decode(&((*result).channel_join_details_extras_schema_present), (zcbor_decoder_t *)decode_repeated_channel_join_details_extras_schema, state, (&(*result).channel_join_details_extras_schema))
	&& zcbor_present_decode(&((*result).channel_join_details_extras_present), (zcbor_decoder_t *)decode_repeated_channel_join_details_extras, state, (&(*result).channel_join_details_extras))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_channel_join_details_name(state, (&(*result).channel_join_details_name));
		decode_repeated_channel_join_details_name_max_length(state, (&(*result).channel_join_details_name_max_length));
		decode_repeated_channel_join_details_nickname(state, (&(*result).channel_join_details_nickname));
		decode_repeated_channel_join_details_nickname_supported(state, (&(*result).channel_join_details_nickname_supported));
		decode_repeated_channel_join_details_nickname_max_length(state, (&(*result).channel_join_details_nickname_max_length));
		decode_repeated_channel_join_details_password(state, (&(*result).channel_join_details_password));
		decode_repeated_channel_join_details_password_supported(state, (&(*result).channel_join_details_password_supported));
		decode_repeated_channel_join_details_password_max_length(state, (&(*result).channel_join_details_password_max_length));
		decode_repeated_channel_join_details_extras_schema(state, (&(*result).channel_join_details_extras_schema));
		decode_repeated_channel_join_details_extras(state, (&(*result).channel_join_details_extras));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_join(
		zcbor_state_t *state, struct request_conv_join *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvJoin", tmp_str.len = sizeof("ConvJoin") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvJoin_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"details", tmp_str.len = sizeof("details") - 1, &tmp_str)))))
	&& (decode_channel_join_details(state, (&(*result).ConvJoin_details))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_leave(
		zcbor_state_t *state, struct request_conv_leave *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvLeave", tmp_str.len = sizeof("ConvLeave") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvLeave_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvLeave_conv))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_participant_contact(
		zcbor_state_t *state, struct participant_contact *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Contact", tmp_str.len = sizeof("Contact") - 1, &tmp_str)))))
	&& (decode_contact_info(state, (&(*result).participant_contact_Contact))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_participant_agent(
		zcbor_state_t *state, struct participant_agent *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Agent", tmp_str.len = sizeof("Agent") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Agent_profile))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"member", tmp_str.len = sizeof("member") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Agent_member))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_participant(
		zcbor_state_t *state, struct participant_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_participant_contact(state, (&(*result).participant_contact_m)))) && (((*result).participant_choice = participant_contact_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_participant_agent(state, (&(*result).participant_agent_m)))) && (((*result).participant_choice = participant_agent_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_conv_send_args_from(
		zcbor_state_t *state, struct conv_send_args_from_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"from", tmp_str.len = sizeof("from") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_participant(state, (&(*result).conv_send_args_from_participant_m)))) && (((*result).conv_send_args_from_choice = conv_send_args_from_participant_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).conv_send_args_from_choice = conv_send_args_from_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_conv_send_args(
		zcbor_state_t *state, struct conv_send_args *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).conv_send_args_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).conv_send_args_conv))))
	&& zcbor_present_decode(&((*result).conv_send_args_from_present), (zcbor_decoder_t *)decode_repeated_conv_send_args_from, state, (&(*result).conv_send_args_from))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"message", tmp_str.len = sizeof("message") - 1, &tmp_str)))))
	&& (decode_user_msg(state, (&(*result).conv_send_args_message))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_conv_send_args_from(state, (&(*result).conv_send_args_from));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_send(
		zcbor_state_t *state, struct request_conv_send *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvSend", tmp_str.len = sizeof("ConvSend") - 1, &tmp_str)))))
	&& (decode_conv_send_args(state, (&(*result).request_conv_send_ConvSend))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_ConvSetTopic_topic(
		zcbor_state_t *state, struct ConvSetTopic_topic_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"topic", tmp_str.len = sizeof("topic") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).ConvSetTopic_topic_tstr)))) && (((*result).ConvSetTopic_topic_choice = ConvSetTopic_topic_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).ConvSetTopic_topic_choice = ConvSetTopic_topic_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_set_topic(
		zcbor_state_t *state, struct request_conv_set_topic *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvSetTopic", tmp_str.len = sizeof("ConvSetTopic") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvSetTopic_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvSetTopic_conv))))
	&& zcbor_present_decode(&((*result).ConvSetTopic_topic_present), (zcbor_decoder_t *)decode_repeated_ConvSetTopic_topic, state, (&(*result).ConvSetTopic_topic))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_ConvSetTopic_topic(state, (&(*result).ConvSetTopic_topic));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_ConvSetTitle_title(
		zcbor_state_t *state, struct ConvSetTitle_title_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).ConvSetTitle_title_tstr)))) && (((*result).ConvSetTitle_title_choice = ConvSetTitle_title_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).ConvSetTitle_title_choice = ConvSetTitle_title_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_set_title(
		zcbor_state_t *state, struct request_conv_set_title *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvSetTitle", tmp_str.len = sizeof("ConvSetTitle") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvSetTitle_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvSetTitle_conv))))
	&& zcbor_present_decode(&((*result).ConvSetTitle_title_present), (zcbor_decoder_t *)decode_repeated_ConvSetTitle_title, state, (&(*result).ConvSetTitle_title))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_ConvSetTitle_title(state, (&(*result).ConvSetTitle_title));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_ConvSetDescription_description(
		zcbor_state_t *state, struct ConvSetDescription_description_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"description", tmp_str.len = sizeof("description") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).ConvSetDescription_description_tstr)))) && (((*result).ConvSetDescription_description_choice = ConvSetDescription_description_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).ConvSetDescription_description_choice = ConvSetDescription_description_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_set_description(
		zcbor_state_t *state, struct request_conv_set_description *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvSetDescription", tmp_str.len = sizeof("ConvSetDescription") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvSetDescription_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvSetDescription_conv))))
	&& zcbor_present_decode(&((*result).ConvSetDescription_description_present), (zcbor_decoder_t *)decode_repeated_ConvSetDescription_description, state, (&(*result).ConvSetDescription_description))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_ConvSetDescription_description(state, (&(*result).ConvSetDescription_description));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_delete(
		zcbor_state_t *state, struct request_conv_delete *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvDelete", tmp_str.len = sizeof("ConvDelete") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvDelete_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ConvDelete_conv))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_conv_history_args_after_cursor(
		zcbor_state_t *state, struct conv_history_args_after_cursor *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"after_cursor", tmp_str.len = sizeof("after_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).conv_history_args_after_cursor)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_conv_history_args_max(
		zcbor_state_t *state, struct conv_history_args_max *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).conv_history_args_max)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_conv_history_args(
		zcbor_state_t *state, struct conv_history_args *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).conv_history_args_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).conv_history_args_conv))))
	&& zcbor_present_decode(&((*result).conv_history_args_after_cursor_present), (zcbor_decoder_t *)decode_repeated_conv_history_args_after_cursor, state, (&(*result).conv_history_args_after_cursor))
	&& zcbor_present_decode(&((*result).conv_history_args_max_present), (zcbor_decoder_t *)decode_repeated_conv_history_args_max, state, (&(*result).conv_history_args_max))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_conv_history_args_after_cursor(state, (&(*result).conv_history_args_after_cursor));
		decode_repeated_conv_history_args_max(state, (&(*result).conv_history_args_max));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_conv_history(
		zcbor_state_t *state, struct request_conv_history *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvHistory", tmp_str.len = sizeof("ConvHistory") - 1, &tmp_str)))))
	&& (decode_conv_history_args(state, (&(*result).request_conv_history_ConvHistory))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_member_invite_args_message(
		zcbor_state_t *state, struct member_invite_args_message_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"message", tmp_str.len = sizeof("message") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).member_invite_args_message_tstr)))) && (((*result).member_invite_args_message_choice = member_invite_args_message_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).member_invite_args_message_choice = member_invite_args_message_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_member_invite_args(
		zcbor_state_t *state, struct member_invite_args *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).member_invite_args_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).member_invite_args_conv))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"who", tmp_str.len = sizeof("who") - 1, &tmp_str)))))
	&& (decode_participant(state, (&(*result).member_invite_args_who))))
	&& zcbor_present_decode(&((*result).member_invite_args_message_present), (zcbor_decoder_t *)decode_repeated_member_invite_args_message, state, (&(*result).member_invite_args_message))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_member_invite_args_message(state, (&(*result).member_invite_args_message));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_member_invite(
		zcbor_state_t *state, struct request_member_invite *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"MemberInvite", tmp_str.len = sizeof("MemberInvite") - 1, &tmp_str)))))
	&& (decode_member_invite_args(state, (&(*result).request_member_invite_MemberInvite))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_member_remove_args_reason(
		zcbor_state_t *state, struct member_remove_args_reason_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"reason", tmp_str.len = sizeof("reason") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).member_remove_args_reason_tstr)))) && (((*result).member_remove_args_reason_choice = member_remove_args_reason_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).member_remove_args_reason_choice = member_remove_args_reason_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_member_remove_args(
		zcbor_state_t *state, struct member_remove_args *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).member_remove_args_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).member_remove_args_conv))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"who", tmp_str.len = sizeof("who") - 1, &tmp_str)))))
	&& (decode_participant(state, (&(*result).member_remove_args_who))))
	&& zcbor_present_decode(&((*result).member_remove_args_reason_present), (zcbor_decoder_t *)decode_repeated_member_remove_args_reason, state, (&(*result).member_remove_args_reason))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_member_remove_args_reason(state, (&(*result).member_remove_args_reason));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_member_remove(
		zcbor_state_t *state, struct request_member_remove *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"MemberRemove", tmp_str.len = sizeof("MemberRemove") - 1, &tmp_str)))))
	&& (decode_member_remove_args(state, (&(*result).request_member_remove_MemberRemove))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_member_ban_args_reason(
		zcbor_state_t *state, struct member_ban_args_reason_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"reason", tmp_str.len = sizeof("reason") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).member_ban_args_reason_tstr)))) && (((*result).member_ban_args_reason_choice = member_ban_args_reason_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).member_ban_args_reason_choice = member_ban_args_reason_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_member_ban_args(
		zcbor_state_t *state, struct member_ban_args *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).member_ban_args_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).member_ban_args_conv))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"who", tmp_str.len = sizeof("who") - 1, &tmp_str)))))
	&& (decode_participant(state, (&(*result).member_ban_args_who))))
	&& zcbor_present_decode(&((*result).member_ban_args_reason_present), (zcbor_decoder_t *)decode_repeated_member_ban_args_reason, state, (&(*result).member_ban_args_reason))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_member_ban_args_reason(state, (&(*result).member_ban_args_reason));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_member_ban(
		zcbor_state_t *state, struct request_member_ban *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"MemberBan", tmp_str.len = sizeof("MemberBan") - 1, &tmp_str)))))
	&& (decode_member_ban_args(state, (&(*result).request_member_ban_MemberBan))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_member_role(
		zcbor_state_t *state, struct member_role_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"None", tmp_str.len = sizeof("None") - 1, &tmp_str))))) && (((*result).member_role_choice = member_role_None_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Voice", tmp_str.len = sizeof("Voice") - 1, &tmp_str))))) && (((*result).member_role_choice = member_role_Voice_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"HalfOp", tmp_str.len = sizeof("HalfOp") - 1, &tmp_str))))) && (((*result).member_role_choice = member_role_HalfOp_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Op", tmp_str.len = sizeof("Op") - 1, &tmp_str))))) && (((*result).member_role_choice = member_role_Op_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Founder", tmp_str.len = sizeof("Founder") - 1, &tmp_str))))) && (((*result).member_role_choice = member_role_Founder_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_member_set_role_args(
		zcbor_state_t *state, struct member_set_role_args *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).member_set_role_args_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).member_set_role_args_conv))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"who", tmp_str.len = sizeof("who") - 1, &tmp_str)))))
	&& (decode_participant(state, (&(*result).member_set_role_args_who))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (decode_member_role(state, (&(*result).member_set_role_args_role))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_member_set_role(
		zcbor_state_t *state, struct request_member_set_role *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"MemberSetRole", tmp_str.len = sizeof("MemberSetRole") - 1, &tmp_str)))))
	&& (decode_member_set_role_args(state, (&(*result).request_member_set_role_MemberSetRole))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_contact_get_profile(
		zcbor_state_t *state, struct request_contact_get_profile *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ContactGetProfile", tmp_str.len = sizeof("ContactGetProfile") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ContactGetProfile_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"contact", tmp_str.len = sizeof("contact") - 1, &tmp_str)))))
	&& (decode_contact_info(state, (&(*result).ContactGetProfile_contact))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_ContactSetAlias_alias(
		zcbor_state_t *state, struct ContactSetAlias_alias_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"alias", tmp_str.len = sizeof("alias") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).ContactSetAlias_alias_tstr)))) && (((*result).ContactSetAlias_alias_choice = ContactSetAlias_alias_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).ContactSetAlias_alias_choice = ContactSetAlias_alias_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_contact_set_alias(
		zcbor_state_t *state, struct request_contact_set_alias *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ContactSetAlias", tmp_str.len = sizeof("ContactSetAlias") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ContactSetAlias_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"contact", tmp_str.len = sizeof("contact") - 1, &tmp_str)))))
	&& (decode_contact_info(state, (&(*result).ContactSetAlias_contact))))
	&& zcbor_present_decode(&((*result).ContactSetAlias_alias_present), (zcbor_decoder_t *)decode_repeated_ContactSetAlias_alias, state, (&(*result).ContactSetAlias_alias))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_ContactSetAlias_alias(state, (&(*result).ContactSetAlias_alias));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_contact_action_menu(
		zcbor_state_t *state, struct request_contact_action_menu *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ContactActionMenu", tmp_str.len = sizeof("ContactActionMenu") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ContactActionMenu_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"contact", tmp_str.len = sizeof("contact") - 1, &tmp_str)))))
	&& (decode_contact_info(state, (&(*result).ContactActionMenu_contact))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_DirectorySearch_query(
		zcbor_state_t *state, struct DirectorySearch_query_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"query", tmp_str.len = sizeof("query") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).DirectorySearch_query_tstr)))) && (((*result).DirectorySearch_query_choice = DirectorySearch_query_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).DirectorySearch_query_choice = DirectorySearch_query_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_directory_search(
		zcbor_state_t *state, struct request_directory_search *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"DirectorySearch", tmp_str.len = sizeof("DirectorySearch") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).DirectorySearch_transport))))
	&& zcbor_present_decode(&((*result).DirectorySearch_query_present), (zcbor_decoder_t *)decode_repeated_DirectorySearch_query, state, (&(*result).DirectorySearch_query))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_DirectorySearch_query(state, (&(*result).DirectorySearch_query));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_root_id_host(
		zcbor_state_t *state, struct fs_root_id_host *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"host", tmp_str.len = sizeof("host") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_root_id_host_host))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_root_id_session(
		zcbor_state_t *state, struct fs_root_id_session *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_root_id_session_session))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_root_id_t(
		zcbor_state_t *state, struct fs_root_id_t_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_fs_root_id_host(state, (&(*result).fs_root_id_t_fs_root_id_host_m)))) && (((*result).fs_root_id_t_choice = fs_root_id_t_fs_root_id_host_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"workspace", tmp_str.len = sizeof("workspace") - 1, &tmp_str))))) && (((*result).fs_root_id_t_choice = fs_root_id_t_workspace_tstr_c), true)))
	|| (((decode_fs_root_id_session(state, (&(*result).fs_root_id_t_fs_root_id_session_m)))) && (((*result).fs_root_id_t_choice = fs_root_id_t_fs_root_id_session_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_FsList_show_ignored(
		zcbor_state_t *state, struct FsList_show_ignored *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"show_ignored", tmp_str.len = sizeof("show_ignored") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).FsList_show_ignored)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_fs_list(
		zcbor_state_t *state, struct request_fs_list *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsList", tmp_str.len = sizeof("FsList") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (decode_fs_root_id_t(state, (&(*result).FsList_root))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"dir", tmp_str.len = sizeof("dir") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).FsList_dir))))
	&& zcbor_present_decode(&((*result).FsList_show_ignored_present), (zcbor_decoder_t *)decode_repeated_FsList_show_ignored, state, (&(*result).FsList_show_ignored))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_FsList_show_ignored(state, (&(*result).FsList_show_ignored));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_fs_stat(
		zcbor_state_t *state, struct request_fs_stat *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsStat", tmp_str.len = sizeof("FsStat") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (decode_fs_root_id_t(state, (&(*result).FsStat_root))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).FsStat_path))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_FsRead_max_bytes(
		zcbor_state_t *state, struct FsRead_max_bytes *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max_bytes", tmp_str.len = sizeof("max_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).FsRead_max_bytes)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_fs_read(
		zcbor_state_t *state, struct request_fs_read *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsRead", tmp_str.len = sizeof("FsRead") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (decode_fs_root_id_t(state, (&(*result).FsRead_root))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).FsRead_path))))
	&& zcbor_present_decode(&((*result).FsRead_max_bytes_present), (zcbor_decoder_t *)decode_repeated_FsRead_max_bytes, state, (&(*result).FsRead_max_bytes))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_FsRead_max_bytes(state, (&(*result).FsRead_max_bytes));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_revision(
		zcbor_state_t *state, struct fs_revision *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"mtime_ms", tmp_str.len = sizeof("mtime_ms") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).fs_revision_mtime_ms))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"size", tmp_str.len = sizeof("size") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).fs_revision_size))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_write_args_base_revision(
		zcbor_state_t *state, struct fs_write_args_base_revision_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"base_revision", tmp_str.len = sizeof("base_revision") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_fs_revision(state, (&(*result).fs_write_args_base_revision_fs_revision_m)))) && (((*result).fs_write_args_base_revision_choice = fs_write_args_base_revision_fs_revision_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).fs_write_args_base_revision_choice = fs_write_args_base_revision_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_write_args_force(
		zcbor_state_t *state, struct fs_write_args_force *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"force", tmp_str.len = sizeof("force") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).fs_write_args_force)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_write_args(
		zcbor_state_t *state, struct fs_write_args *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (decode_fs_root_id_t(state, (&(*result).fs_write_args_root))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_write_args_path))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"bytes", tmp_str.len = sizeof("bytes") - 1, &tmp_str)))))
	&& (zcbor_bstr_decode(state, (&(*result).fs_write_args_bytes))))
	&& zcbor_present_decode(&((*result).fs_write_args_base_revision_present), (zcbor_decoder_t *)decode_repeated_fs_write_args_base_revision, state, (&(*result).fs_write_args_base_revision))
	&& zcbor_present_decode(&((*result).fs_write_args_force_present), (zcbor_decoder_t *)decode_repeated_fs_write_args_force, state, (&(*result).fs_write_args_force))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_fs_write_args_base_revision(state, (&(*result).fs_write_args_base_revision));
		decode_repeated_fs_write_args_force(state, (&(*result).fs_write_args_force));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_fs_write(
		zcbor_state_t *state, struct request_fs_write *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsWrite", tmp_str.len = sizeof("FsWrite") - 1, &tmp_str)))))
	&& (decode_fs_write_args(state, (&(*result).request_fs_write_FsWrite))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_search_query_regex(
		zcbor_state_t *state, struct fs_search_query_regex *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"regex", tmp_str.len = sizeof("regex") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).fs_search_query_regex)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_search_query_case_sensitive(
		zcbor_state_t *state, struct fs_search_query_case_sensitive *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"case_sensitive", tmp_str.len = sizeof("case_sensitive") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).fs_search_query_case_sensitive)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_search_query_max_results(
		zcbor_state_t *state, struct fs_search_query_max_results *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max_results", tmp_str.len = sizeof("max_results") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).fs_search_query_max_results)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_search_query_page(
		zcbor_state_t *state, struct fs_search_query_page *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"page", tmp_str.len = sizeof("page") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).fs_search_query_page)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_search_query(
		zcbor_state_t *state, struct fs_search_query *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"query", tmp_str.len = sizeof("query") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_search_query_query))))
	&& zcbor_present_decode(&((*result).fs_search_query_regex_present), (zcbor_decoder_t *)decode_repeated_fs_search_query_regex, state, (&(*result).fs_search_query_regex))
	&& zcbor_present_decode(&((*result).fs_search_query_case_sensitive_present), (zcbor_decoder_t *)decode_repeated_fs_search_query_case_sensitive, state, (&(*result).fs_search_query_case_sensitive))
	&& zcbor_present_decode(&((*result).fs_search_query_max_results_present), (zcbor_decoder_t *)decode_repeated_fs_search_query_max_results, state, (&(*result).fs_search_query_max_results))
	&& zcbor_present_decode(&((*result).fs_search_query_page_present), (zcbor_decoder_t *)decode_repeated_fs_search_query_page, state, (&(*result).fs_search_query_page))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_fs_search_query_regex(state, (&(*result).fs_search_query_regex));
		decode_repeated_fs_search_query_case_sensitive(state, (&(*result).fs_search_query_case_sensitive));
		decode_repeated_fs_search_query_max_results(state, (&(*result).fs_search_query_max_results));
		decode_repeated_fs_search_query_page(state, (&(*result).fs_search_query_page));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_fs_search(
		zcbor_state_t *state, struct request_fs_search *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsSearch", tmp_str.len = sizeof("FsSearch") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (decode_fs_root_id_t(state, (&(*result).FsSearch_root))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"query", tmp_str.len = sizeof("query") - 1, &tmp_str)))))
	&& (decode_fs_search_query(state, (&(*result).FsSearch_query))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_watch_after_args(
		zcbor_state_t *state, struct fs_watch_after_args *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (decode_fs_root_id_t(state, (&(*result).fs_watch_after_args_root))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"dir", tmp_str.len = sizeof("dir") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_watch_after_args_dir))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"after_seq", tmp_str.len = sizeof("after_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).fs_watch_after_args_after_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).fs_watch_after_args_max))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_fs_watch_poll(
		zcbor_state_t *state, struct request_fs_watch_poll *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsWatchPoll", tmp_str.len = sizeof("FsWatchPoll") - 1, &tmp_str)))))
	&& (decode_fs_watch_after_args(state, (&(*result).request_fs_watch_poll_FsWatchPoll))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_blob_put(
		zcbor_state_t *state, struct request_blob_put *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"BlobPut", tmp_str.len = sizeof("BlobPut") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"bytes", tmp_str.len = sizeof("bytes") - 1, &tmp_str)))))
	&& (zcbor_bstr_decode(state, (&(*result).BlobPut_bytes))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_byte_range(
		zcbor_state_t *state, struct byte_range *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"offset", tmp_str.len = sizeof("offset") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).byte_range_offset))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"len", tmp_str.len = sizeof("len") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).byte_range_len))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_BlobGet_range(
		zcbor_state_t *state, struct BlobGet_range_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"range", tmp_str.len = sizeof("range") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_byte_range(state, (&(*result).BlobGet_range_byte_range_m)))) && (((*result).BlobGet_range_choice = BlobGet_range_byte_range_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).BlobGet_range_choice = BlobGet_range_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_blob_get(
		zcbor_state_t *state, struct request_blob_get *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"BlobGet", tmp_str.len = sizeof("BlobGet") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"hash", tmp_str.len = sizeof("hash") - 1, &tmp_str)))))
	&& (decode_content_hash(state, (&(*result).BlobGet_hash))))
	&& zcbor_present_decode(&((*result).BlobGet_range_present), (zcbor_decoder_t *)decode_repeated_BlobGet_range, state, (&(*result).BlobGet_range))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_BlobGet_range(state, (&(*result).BlobGet_range));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_blob_stat(
		zcbor_state_t *state, struct request_blob_stat *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"BlobStat", tmp_str.len = sizeof("BlobStat") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"hash", tmp_str.len = sizeof("hash") - 1, &tmp_str)))))
	&& (decode_content_hash(state, (&(*result).BlobStat_hash))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_write_from_blob_args_base_revision(
		zcbor_state_t *state, struct fs_write_from_blob_args_base_revision_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"base_revision", tmp_str.len = sizeof("base_revision") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_fs_revision(state, (&(*result).fs_write_from_blob_args_base_revision_fs_revision_m)))) && (((*result).fs_write_from_blob_args_base_revision_choice = fs_write_from_blob_args_base_revision_fs_revision_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).fs_write_from_blob_args_base_revision_choice = fs_write_from_blob_args_base_revision_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_write_from_blob_args_force(
		zcbor_state_t *state, struct fs_write_from_blob_args_force *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"force", tmp_str.len = sizeof("force") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).fs_write_from_blob_args_force)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_write_from_blob_args(
		zcbor_state_t *state, struct fs_write_from_blob_args *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (decode_fs_root_id_t(state, (&(*result).fs_write_from_blob_args_root))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_write_from_blob_args_path))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"hash", tmp_str.len = sizeof("hash") - 1, &tmp_str)))))
	&& (decode_content_hash(state, (&(*result).fs_write_from_blob_args_hash))))
	&& zcbor_present_decode(&((*result).fs_write_from_blob_args_base_revision_present), (zcbor_decoder_t *)decode_repeated_fs_write_from_blob_args_base_revision, state, (&(*result).fs_write_from_blob_args_base_revision))
	&& zcbor_present_decode(&((*result).fs_write_from_blob_args_force_present), (zcbor_decoder_t *)decode_repeated_fs_write_from_blob_args_force, state, (&(*result).fs_write_from_blob_args_force))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_fs_write_from_blob_args_base_revision(state, (&(*result).fs_write_from_blob_args_base_revision));
		decode_repeated_fs_write_from_blob_args_force(state, (&(*result).fs_write_from_blob_args_force));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_fs_write_from_blob(
		zcbor_state_t *state, struct request_fs_write_from_blob *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsWriteFromBlob", tmp_str.len = sizeof("FsWriteFromBlob") - 1, &tmp_str)))))
	&& (decode_fs_write_from_blob_args(state, (&(*result).request_fs_write_from_blob_FsWriteFromBlob))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_user_create(
		zcbor_state_t *state, struct request_user_create *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"UserCreate", tmp_str.len = sizeof("UserCreate") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"username", tmp_str.len = sizeof("username") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).UserCreate_username))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"password", tmp_str.len = sizeof("password") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).UserCreate_password))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"roles", tmp_str.len = sizeof("roles") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).UserCreate_roles_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).UserCreate_roles_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).UserCreate_roles_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_user_disable(
		zcbor_state_t *state, struct request_user_disable *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"UserDisable", tmp_str.len = sizeof("UserDisable") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).UserDisable_user_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"disabled", tmp_str.len = sizeof("disabled") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).UserDisable_disabled))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_user_set_roles(
		zcbor_state_t *state, struct request_user_set_roles *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"UserSetRoles", tmp_str.len = sizeof("UserSetRoles") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).UserSetRoles_user_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"roles", tmp_str.len = sizeof("roles") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).UserSetRoles_roles_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).UserSetRoles_roles_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).UserSetRoles_roles_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_user_set_password(
		zcbor_state_t *state, struct request_user_set_password *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"UserSetPassword", tmp_str.len = sizeof("UserSetPassword") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).UserSetPassword_user_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"password", tmp_str.len = sizeof("password") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).UserSetPassword_password))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_session_revoke(
		zcbor_state_t *state, struct request_session_revoke *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionRevoke", tmp_str.len = sizeof("SessionRevoke") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SessionRevoke_user_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_resource_grant_create(
		zcbor_state_t *state, struct request_resource_grant_create *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ResourceGrantCreate", tmp_str.len = sizeof("ResourceGrantCreate") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ResourceGrantCreate_user_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"resource_kind", tmp_str.len = sizeof("resource_kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ResourceGrantCreate_resource_kind))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"resource_id", tmp_str.len = sizeof("resource_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ResourceGrantCreate_resource_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"capability", tmp_str.len = sizeof("capability") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ResourceGrantCreate_capability))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_ResourceGrantList_user_id(
		zcbor_state_t *state, struct ResourceGrantList_user_id_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).ResourceGrantList_user_id_tstr)))) && (((*result).ResourceGrantList_user_id_choice = ResourceGrantList_user_id_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).ResourceGrantList_user_id_choice = ResourceGrantList_user_id_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_resource_grant_list(
		zcbor_state_t *state, struct request_resource_grant_list *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ResourceGrantList", tmp_str.len = sizeof("ResourceGrantList") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).ResourceGrantList_user_id_present), (zcbor_decoder_t *)decode_repeated_ResourceGrantList_user_id, state, (&(*result).ResourceGrantList_user_id))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_ResourceGrantList_user_id(state, (&(*result).ResourceGrantList_user_id));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_request_resource_grant_revoke(
		zcbor_state_t *state, struct request_resource_grant_revoke *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ResourceGrantRevoke", tmp_str.len = sizeof("ResourceGrantRevoke") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ResourceGrantRevoke_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_routed(
		zcbor_state_t *state, struct response_routed *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Routed", tmp_str.len = sizeof("Routed") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Routed_session))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_completion_source_process(
		zcbor_state_t *state, struct completion_source_process *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Process", tmp_str.len = sizeof("Process") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).completion_source_process_Process))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_completion_source_delegation(
		zcbor_state_t *state, struct completion_source_delegation *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Delegation", tmp_str.len = sizeof("Delegation") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).completion_source_delegation_Delegation))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_completion_source(
		zcbor_state_t *state, struct completion_source_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_completion_source_process(state, (&(*result).completion_source_process_m)))) && (((*result).completion_source_choice = completion_source_process_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_completion_source_delegation(state, (&(*result).completion_source_delegation_m)))) && (((*result).completion_source_choice = completion_source_delegation_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_turn_trigger_background(
		zcbor_state_t *state, struct turn_trigger_background *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"BackgroundCompletion", tmp_str.len = sizeof("BackgroundCompletion") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (decode_completion_source(state, (&(*result).BackgroundCompletion_source))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_turn_trigger_scheduled(
		zcbor_state_t *state, struct turn_trigger_scheduled *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Scheduled", tmp_str.len = sizeof("Scheduled") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"job", tmp_str.len = sizeof("job") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Scheduled_job))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_turn_trigger(
		zcbor_state_t *state, struct turn_trigger_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"User", tmp_str.len = sizeof("User") - 1, &tmp_str))))) && (((*result).turn_trigger_choice = turn_trigger_User_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Steer", tmp_str.len = sizeof("Steer") - 1, &tmp_str))))) && (((*result).turn_trigger_choice = turn_trigger_Steer_tstr_c), true))
	|| (((decode_turn_trigger_background(state, (&(*result).turn_trigger_background_m)))) && (((*result).turn_trigger_choice = turn_trigger_background_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_turn_trigger_scheduled(state, (&(*result).turn_trigger_scheduled_m)))) && (((*result).turn_trigger_choice = turn_trigger_scheduled_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_turn_started(
		zcbor_state_t *state, struct agent_event_turn_started *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"TurnStarted", tmp_str.len = sizeof("TurnStarted") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).TurnStarted_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"trigger", tmp_str.len = sizeof("trigger") - 1, &tmp_str)))))
	&& (decode_turn_trigger(state, (&(*result).TurnStarted_trigger))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_text_delta(
		zcbor_state_t *state, struct agent_event_text_delta *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"TextDelta", tmp_str.len = sizeof("TextDelta") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).TextDelta_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).TextDelta_text))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_reasoning_delta(
		zcbor_state_t *state, struct agent_event_reasoning_delta *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ReasoningDelta", tmp_str.len = sizeof("ReasoningDelta") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).ReasoningDelta_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ReasoningDelta_text))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_content_delta(
		zcbor_state_t *state, struct agent_event_content_delta *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ContentDelta", tmp_str.len = sizeof("ContentDelta") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).ContentDelta_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ContentDelta_kind))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (zcbor_bstr_decode(state, (&(*result).ContentDelta_body))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_tool_detail(
		zcbor_state_t *state, struct tool_detail *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).tool_detail_kind))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (zcbor_bstr_decode(state, (&(*result).tool_detail_body))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_tool_call_view_detail(
		zcbor_state_t *state, struct tool_call_view_detail_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_tool_detail(state, (&(*result).tool_call_view_detail_tool_detail_m)))) && (((*result).tool_call_view_detail_choice = tool_call_view_detail_tool_detail_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).tool_call_view_detail_choice = tool_call_view_detail_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_tool_call_view(
		zcbor_state_t *state, struct tool_call_view *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"call_id", tmp_str.len = sizeof("call_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).tool_call_view_call_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).tool_call_view_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"args_summary", tmp_str.len = sizeof("args_summary") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).tool_call_view_args_summary))))
	&& zcbor_present_decode(&((*result).tool_call_view_detail_present), (zcbor_decoder_t *)decode_repeated_tool_call_view_detail, state, (&(*result).tool_call_view_detail))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_tool_call_view_detail(state, (&(*result).tool_call_view_detail));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_tool_started(
		zcbor_state_t *state, struct agent_event_tool_started *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ToolStarted", tmp_str.len = sizeof("ToolStarted") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).ToolStarted_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"call", tmp_str.len = sizeof("call") - 1, &tmp_str)))))
	&& (decode_tool_call_view(state, (&(*result).ToolStarted_call))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_tool_result_view_detail(
		zcbor_state_t *state, struct tool_result_view_detail_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_tool_detail(state, (&(*result).tool_result_view_detail_tool_detail_m)))) && (((*result).tool_result_view_detail_choice = tool_result_view_detail_tool_detail_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).tool_result_view_detail_choice = tool_result_view_detail_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_tool_result_view(
		zcbor_state_t *state, struct tool_result_view *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"call_id", tmp_str.len = sizeof("call_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).tool_result_view_call_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ok", tmp_str.len = sizeof("ok") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).tool_result_view_ok))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"summary", tmp_str.len = sizeof("summary") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).tool_result_view_summary))))
	&& zcbor_present_decode(&((*result).tool_result_view_detail_present), (zcbor_decoder_t *)decode_repeated_tool_result_view_detail, state, (&(*result).tool_result_view_detail))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_tool_result_view_detail(state, (&(*result).tool_result_view_detail));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_tool_finished(
		zcbor_state_t *state, struct agent_event_tool_finished *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ToolFinished", tmp_str.len = sizeof("ToolFinished") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).ToolFinished_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"result", tmp_str.len = sizeof("result") - 1, &tmp_str)))))
	&& (decode_tool_result_view(state, (&(*result).ToolFinished_result))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_usage_delta(
		zcbor_state_t *state, struct usage_delta *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"input_tokens", tmp_str.len = sizeof("input_tokens") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).usage_delta_input_tokens))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"output_tokens", tmp_str.len = sizeof("output_tokens") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).usage_delta_output_tokens))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"api_calls", tmp_str.len = sizeof("api_calls") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).usage_delta_api_calls))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"cache_read_tokens", tmp_str.len = sizeof("cache_read_tokens") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).usage_delta_cache_read_tokens))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"cache_write_tokens", tmp_str.len = sizeof("cache_write_tokens") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).usage_delta_cache_write_tokens))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"reasoning_tokens", tmp_str.len = sizeof("reasoning_tokens") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).usage_delta_reasoning_tokens))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"cost_micros", tmp_str.len = sizeof("cost_micros") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).usage_delta_cost_micros))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_usage(
		zcbor_state_t *state, struct agent_event_usage *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Usage", tmp_str.len = sizeof("Usage") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Usage_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"delta", tmp_str.len = sizeof("delta") - 1, &tmp_str)))))
	&& (decode_usage_delta(state, (&(*result).Usage_delta))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_context_status(
		zcbor_state_t *state, struct context_status *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"used_tokens", tmp_str.len = sizeof("used_tokens") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).context_status_used_tokens))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max_tokens", tmp_str.len = sizeof("max_tokens") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).context_status_max_tokens_uint64_m)))) && (((*result).context_status_max_tokens_choice = context_status_max_tokens_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).context_status_max_tokens_choice = context_status_max_tokens_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"budget_tokens", tmp_str.len = sizeof("budget_tokens") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).context_status_budget_tokens_uint64_m)))) && (((*result).context_status_budget_tokens_choice = context_status_budget_tokens_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).context_status_budget_tokens_choice = context_status_budget_tokens_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"compacted", tmp_str.len = sizeof("compacted") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).context_status_compacted))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"dropped_turns", tmp_str.len = sizeof("dropped_turns") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).context_status_dropped_turns))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_context(
		zcbor_state_t *state, struct agent_event_context *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Context", tmp_str.len = sizeof("Context") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Context_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"status", tmp_str.len = sizeof("status") - 1, &tmp_str)))))
	&& (decode_context_status(state, (&(*result).Context_status))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_rate_limit_snapshot(
		zcbor_state_t *state, struct rate_limit_snapshot *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"remaining", tmp_str.len = sizeof("remaining") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).rate_limit_snapshot_remaining_uint64_m)))) && (((*result).rate_limit_snapshot_remaining_choice = rate_limit_snapshot_remaining_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).rate_limit_snapshot_remaining_choice = rate_limit_snapshot_remaining_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"limit", tmp_str.len = sizeof("limit") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).rate_limit_snapshot_limit_uint64_m)))) && (((*result).rate_limit_snapshot_limit_choice = rate_limit_snapshot_limit_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).rate_limit_snapshot_limit_choice = rate_limit_snapshot_limit_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"reset_ms", tmp_str.len = sizeof("reset_ms") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).rate_limit_snapshot_reset_ms_uint64_m)))) && (((*result).rate_limit_snapshot_reset_ms_choice = rate_limit_snapshot_reset_ms_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).rate_limit_snapshot_reset_ms_choice = rate_limit_snapshot_reset_ms_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_rate_limit(
		zcbor_state_t *state, struct agent_event_rate_limit *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"RateLimit", tmp_str.len = sizeof("RateLimit") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).RateLimit_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"snapshot", tmp_str.len = sizeof("snapshot") - 1, &tmp_str)))))
	&& (decode_rate_limit_snapshot(state, (&(*result).RateLimit_snapshot))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_end_reason(
		zcbor_state_t *state, struct end_reason_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Completed", tmp_str.len = sizeof("Completed") - 1, &tmp_str))))) && (((*result).end_reason_choice = end_reason_Completed_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Suspended", tmp_str.len = sizeof("Suspended") - 1, &tmp_str))))) && (((*result).end_reason_choice = end_reason_Suspended_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Interrupted", tmp_str.len = sizeof("Interrupted") - 1, &tmp_str))))) && (((*result).end_reason_choice = end_reason_Interrupted_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"BudgetExhausted", tmp_str.len = sizeof("BudgetExhausted") - 1, &tmp_str))))) && (((*result).end_reason_choice = end_reason_BudgetExhausted_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"NoProgress", tmp_str.len = sizeof("NoProgress") - 1, &tmp_str))))) && (((*result).end_reason_choice = end_reason_NoProgress_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Failed", tmp_str.len = sizeof("Failed") - 1, &tmp_str))))) && (((*result).end_reason_choice = end_reason_Failed_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_turn_summary(
		zcbor_state_t *state, struct turn_summary *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"end_reason", tmp_str.len = sizeof("end_reason") - 1, &tmp_str)))))
	&& (decode_end_reason(state, (&(*result).turn_summary_end_reason))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"final_text", tmp_str.len = sizeof("final_text") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).turn_summary_final_text_tstr)))) && (((*result).turn_summary_final_text_choice = turn_summary_final_text_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).turn_summary_final_text_choice = turn_summary_final_text_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"usage", tmp_str.len = sizeof("usage") - 1, &tmp_str)))))
	&& (decode_usage_delta(state, (&(*result).turn_summary_usage))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_turn_finished(
		zcbor_state_t *state, struct agent_event_turn_finished *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"TurnFinished", tmp_str.len = sizeof("TurnFinished") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).TurnFinished_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"summary", tmp_str.len = sizeof("summary") - 1, &tmp_str)))))
	&& (decode_turn_summary(state, (&(*result).TurnFinished_summary))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_error(
		zcbor_state_t *state, struct agent_event_error *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Error", tmp_str.len = sizeof("Error") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Error_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"failure", tmp_str.len = sizeof("failure") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Error_failure))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_steered(
		zcbor_state_t *state, struct agent_event_steered *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Steered", tmp_str.len = sizeof("Steered") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Steered_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Steered_request_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"accepted", tmp_str.len = sizeof("accepted") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).Steered_accepted))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_conv_turn_view(
		zcbor_state_t *state, struct conv_turn_view *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).conv_turn_view_role))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).conv_turn_view_text))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"tools", tmp_str.len = sizeof("tools") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).conv_turn_view_tools_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).conv_turn_view_tools_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).conv_turn_view_tools_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_conv_view(
		zcbor_state_t *state, struct conv_view *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"epoch", tmp_str.len = sizeof("epoch") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).conv_view_epoch))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"turns", tmp_str.len = sizeof("turns") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).conv_view_turns_conv_turn_view_m_count, (zcbor_decoder_t *)decode_conv_turn_view, state, (*&(*result).conv_view_turns_conv_turn_view_m), sizeof(struct conv_turn_view))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"waiting_for", tmp_str.len = sizeof("waiting_for") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).conv_view_waiting_for_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).conv_view_waiting_for_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_conv_turn_view(state, (*&(*result).conv_view_turns_conv_turn_view_m));
		zcbor_tstr_decode(state, (*&(*result).conv_view_waiting_for_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_snapshot(
		zcbor_state_t *state, struct agent_event_snapshot *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Snapshot", tmp_str.len = sizeof("Snapshot") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Snapshot_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Snapshot_request_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"view", tmp_str.len = sizeof("view") - 1, &tmp_str)))))
	&& (decode_conv_view(state, (&(*result).Snapshot_view))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event_rewound(
		zcbor_state_t *state, struct agent_event_rewound *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Rewound", tmp_str.len = sizeof("Rewound") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Rewound_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Rewound_request_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"to_cursor", tmp_str.len = sizeof("to_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Rewound_to_cursor))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"epoch", tmp_str.len = sizeof("epoch") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Rewound_epoch))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_agent_event(
		zcbor_state_t *state, struct agent_event_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_agent_event_turn_started(state, (&(*result).agent_event_turn_started_m)))) && (((*result).agent_event_choice = agent_event_turn_started_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_text_delta(state, (&(*result).agent_event_text_delta_m)))) && (((*result).agent_event_choice = agent_event_text_delta_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_reasoning_delta(state, (&(*result).agent_event_reasoning_delta_m)))) && (((*result).agent_event_choice = agent_event_reasoning_delta_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_content_delta(state, (&(*result).agent_event_content_delta_m)))) && (((*result).agent_event_choice = agent_event_content_delta_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_tool_started(state, (&(*result).agent_event_tool_started_m)))) && (((*result).agent_event_choice = agent_event_tool_started_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_tool_finished(state, (&(*result).agent_event_tool_finished_m)))) && (((*result).agent_event_choice = agent_event_tool_finished_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_usage(state, (&(*result).agent_event_usage_m)))) && (((*result).agent_event_choice = agent_event_usage_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_context(state, (&(*result).agent_event_context_m)))) && (((*result).agent_event_choice = agent_event_context_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_rate_limit(state, (&(*result).agent_event_rate_limit_m)))) && (((*result).agent_event_choice = agent_event_rate_limit_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_turn_finished(state, (&(*result).agent_event_turn_finished_m)))) && (((*result).agent_event_choice = agent_event_turn_finished_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_error(state, (&(*result).agent_event_error_m)))) && (((*result).agent_event_choice = agent_event_error_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_steered(state, (&(*result).agent_event_steered_m)))) && (((*result).agent_event_choice = agent_event_steered_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_snapshot(state, (&(*result).agent_event_snapshot_m)))) && (((*result).agent_event_choice = agent_event_snapshot_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_agent_event_rewound(state, (&(*result).agent_event_rewound_m)))) && (((*result).agent_event_choice = agent_event_rewound_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_outbound_event(
		zcbor_state_t *state, struct outbound_event *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Event", tmp_str.len = sizeof("Event") - 1, &tmp_str)))))
	&& (decode_agent_event(state, (&(*result).outbound_event_Event))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_request_kind_approval(
		zcbor_state_t *state, struct host_request_kind_approval *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Approval", tmp_str.len = sizeof("Approval") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"prompt", tmp_str.len = sizeof("prompt") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Approval_prompt))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_request_kind_input(
		zcbor_state_t *state, struct host_request_kind_input *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Input", tmp_str.len = sizeof("Input") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"prompt", tmp_str.len = sizeof("prompt") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Input_prompt))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_request_kind_choice(
		zcbor_state_t *state, struct host_request_kind_choice *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Choice", tmp_str.len = sizeof("Choice") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"prompt", tmp_str.len = sizeof("prompt") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Choice_prompt))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"options", tmp_str.len = sizeof("options") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).Choice_options_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).Choice_options_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).Choice_options_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_request_kind_delegate(
		zcbor_state_t *state, struct host_request_kind_delegate *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Delegate", tmp_str.len = sizeof("Delegate") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"label", tmp_str.len = sizeof("label") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Delegate_label))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"budget", tmp_str.len = sizeof("budget") - 1, &tmp_str)))))
	&& (decode_budget(state, (&(*result).Delegate_budget))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_spawn_spec(
		zcbor_state_t *state, struct spawn_spec *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).spawn_spec_kind))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seed", tmp_str.len = sizeof("seed") - 1, &tmp_str)))))
	&& (zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FromConversation", tmp_str.len = sizeof("FromConversation") - 1, &tmp_str)))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_request_kind_spawn(
		zcbor_state_t *state, struct host_request_kind_spawn *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Spawn", tmp_str.len = sizeof("Spawn") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (decode_spawn_spec(state, (&(*result).Spawn_spec))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_request_kind_t(
		zcbor_state_t *state, struct host_request_kind_t_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_host_request_kind_approval(state, (&(*result).host_request_kind_t_host_request_kind_approval_m)))) && (((*result).host_request_kind_t_choice = host_request_kind_t_host_request_kind_approval_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_host_request_kind_input(state, (&(*result).host_request_kind_t_host_request_kind_input_m)))) && (((*result).host_request_kind_t_choice = host_request_kind_t_host_request_kind_input_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_host_request_kind_choice(state, (&(*result).host_request_kind_t_host_request_kind_choice_m)))) && (((*result).host_request_kind_t_choice = host_request_kind_t_host_request_kind_choice_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_host_request_kind_delegate(state, (&(*result).host_request_kind_t_host_request_kind_delegate_m)))) && (((*result).host_request_kind_t_choice = host_request_kind_t_host_request_kind_delegate_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_host_request_kind_spawn(state, (&(*result).host_request_kind_t_host_request_kind_spawn_m)))) && (((*result).host_request_kind_t_choice = host_request_kind_t_host_request_kind_spawn_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_host_request(
		zcbor_state_t *state, struct host_request *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).host_request_request_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (decode_host_request_kind_t(state, (&(*result).host_request_kind))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_outbound_request(
		zcbor_state_t *state, struct outbound_request *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Request", tmp_str.len = sizeof("Request") - 1, &tmp_str)))))
	&& (decode_host_request(state, (&(*result).outbound_request_Request))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_outbound(
		zcbor_state_t *state, struct outbound_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_outbound_event(state, (&(*result).outbound_event_m)))) && (((*result).outbound_choice = outbound_event_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_outbound_request(state, (&(*result).outbound_request_m)))) && (((*result).outbound_choice = outbound_request_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_drained(
		zcbor_state_t *state, struct response_drained *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Drained", tmp_str.len = sizeof("Drained") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_drained_Drained_outbound_m_count, (zcbor_decoder_t *)decode_outbound, state, (*&(*result).response_drained_Drained_outbound_m), sizeof(struct outbound_r))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_outbound(state, (*&(*result).response_drained_Drained_outbound_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_service_health(
		zcbor_state_t *state, struct service_health *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).service_health_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ok", tmp_str.len = sizeof("ok") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).service_health_ok))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"restarts", tmp_str.len = sizeof("restarts") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).service_health_restarts))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).service_health_detail_tstr)))) && (((*result).service_health_detail_choice = service_health_detail_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).service_health_detail_choice = service_health_detail_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_health_report(
		zcbor_state_t *state, struct health_report *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"all_ok", tmp_str.len = sizeof("all_ok") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).health_report_all_ok))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"services", tmp_str.len = sizeof("services") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).health_report_services_service_health_m_count, (zcbor_decoder_t *)decode_service_health, state, (*&(*result).health_report_services_service_health_m), sizeof(struct service_health))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_service_health(state, (*&(*result).health_report_services_service_health_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_health(
		zcbor_state_t *state, struct response_health *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Health", tmp_str.len = sizeof("Health") - 1, &tmp_str)))))
	&& (decode_health_report(state, (&(*result).response_health_Health))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_stats_report(
		zcbor_state_t *state, struct stats_report *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"pending_jobs", tmp_str.len = sizeof("pending_jobs") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).stats_report_pending_jobs))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"pending_wakes", tmp_str.len = sizeof("pending_wakes") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).stats_report_pending_wakes))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"sessions", tmp_str.len = sizeof("sessions") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).stats_report_sessions))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"active", tmp_str.len = sizeof("active") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).stats_report_active))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"usage", tmp_str.len = sizeof("usage") - 1, &tmp_str)))))
	&& (decode_usage_delta(state, (&(*result).stats_report_usage))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_stats(
		zcbor_state_t *state, struct response_stats *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Stats", tmp_str.len = sizeof("Stats") - 1, &tmp_str)))))
	&& (decode_stats_report(state, (&(*result).response_stats_Stats))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_telemetry_dump(
		zcbor_state_t *state, struct telemetry_dump *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"usage", tmp_str.len = sizeof("usage") - 1, &tmp_str)))))
	&& (decode_usage_delta(state, (&(*result).telemetry_dump_usage))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"events", tmp_str.len = sizeof("events") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).telemetry_dump_events))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"healthy", tmp_str.len = sizeof("healthy") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).telemetry_dump_healthy))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"pending_jobs", tmp_str.len = sizeof("pending_jobs") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).telemetry_dump_pending_jobs))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"pending_wakes", tmp_str.len = sizeof("pending_wakes") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).telemetry_dump_pending_wakes))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"sessions", tmp_str.len = sizeof("sessions") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).telemetry_dump_sessions))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"active", tmp_str.len = sizeof("active") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).telemetry_dump_active))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_telemetry(
		zcbor_state_t *state, struct response_telemetry *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Telemetry", tmp_str.len = sizeof("Telemetry") - 1, &tmp_str)))))
	&& (decode_telemetry_dump(state, (&(*result).response_telemetry_Telemetry))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_state_suspended(
		zcbor_state_t *state, struct session_state_suspended *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Suspended", tmp_str.len = sizeof("Suspended") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"job_id", tmp_str.len = sizeof("job_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Suspended_job_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_state(
		zcbor_state_t *state, struct session_state_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Active", tmp_str.len = sizeof("Active") - 1, &tmp_str))))) && (((*result).session_state_choice = session_state_Active_tstr_c), true))
	|| (((decode_session_state_suspended(state, (&(*result).session_state_suspended_m)))) && (((*result).session_state_choice = session_state_suspended_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Ready", tmp_str.len = sizeof("Ready") - 1, &tmp_str))))) && (((*result).session_state_choice = session_state_Ready_tstr_c), true)))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Completed", tmp_str.len = sizeof("Completed") - 1, &tmp_str))))) && (((*result).session_state_choice = session_state_Completed_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Unknown", tmp_str.len = sizeof("Unknown") - 1, &tmp_str))))) && (((*result).session_state_choice = session_state_Unknown_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_info_rewindable(
		zcbor_state_t *state, struct session_info_rewindable *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"rewindable", tmp_str.len = sizeof("rewindable") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).session_info_rewindable)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_info_bound_profile(
		zcbor_state_t *state, struct session_info_bound_profile_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"bound_profile", tmp_str.len = sizeof("bound_profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).session_info_bound_profile_profile_ref_m)))) && (((*result).session_info_bound_profile_choice = session_info_bound_profile_profile_ref_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).session_info_bound_profile_choice = session_info_bound_profile_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_info_title(
		zcbor_state_t *state, struct session_info_title_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).session_info_title_tstr)))) && (((*result).session_info_title_choice = session_info_title_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).session_info_title_choice = session_info_title_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_info_last_activity_ms(
		zcbor_state_t *state, struct session_info_last_activity_ms_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"last_activity_ms", tmp_str.len = sizeof("last_activity_ms") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).session_info_last_activity_ms_uint64_m)))) && (((*result).session_info_last_activity_ms_choice = session_info_last_activity_ms_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).session_info_last_activity_ms_choice = session_info_last_activity_ms_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_lifecycle(
		zcbor_state_t *state, struct lifecycle_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Durable", tmp_str.len = sizeof("Durable") - 1, &tmp_str))))) && (((*result).lifecycle_choice = lifecycle_Durable_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Live", tmp_str.len = sizeof("Live") - 1, &tmp_str))))) && (((*result).lifecycle_choice = lifecycle_Live_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_info_lifecycle(
		zcbor_state_t *state, struct session_info_lifecycle *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"lifecycle", tmp_str.len = sizeof("lifecycle") - 1, &tmp_str)))))
	&& (decode_lifecycle(state, (&(*result).session_info_lifecycle)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_role(
		zcbor_state_t *state, struct session_role_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Primary", tmp_str.len = sizeof("Primary") - 1, &tmp_str))))) && (((*result).session_role_choice = session_role_Primary_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ManagedChild", tmp_str.len = sizeof("ManagedChild") - 1, &tmp_str))))) && (((*result).session_role_choice = session_role_ManagedChild_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"EphemeralSubagent", tmp_str.len = sizeof("EphemeralSubagent") - 1, &tmp_str))))) && (((*result).session_role_choice = session_role_EphemeralSubagent_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_info_role(
		zcbor_state_t *state, struct session_info_role *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (decode_session_role(state, (&(*result).session_info_role)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_info_parent(
		zcbor_state_t *state, struct session_info_parent_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"parent", tmp_str.len = sizeof("parent") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).session_info_parent_session_id_m)))) && (((*result).session_info_parent_choice = session_info_parent_session_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).session_info_parent_choice = session_info_parent_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_info_pinned(
		zcbor_state_t *state, struct session_info_pinned *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"pinned", tmp_str.len = sizeof("pinned") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).session_info_pinned)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_info_archived(
		zcbor_state_t *state, struct session_info_archived *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"archived", tmp_str.len = sizeof("archived") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).session_info_archived)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_info(
		zcbor_state_t *state, struct session_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).session_info_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (decode_session_state(state, (&(*result).session_info_state))))
	&& zcbor_present_decode(&((*result).session_info_rewindable_present), (zcbor_decoder_t *)decode_repeated_session_info_rewindable, state, (&(*result).session_info_rewindable))
	&& zcbor_present_decode(&((*result).session_info_bound_profile_present), (zcbor_decoder_t *)decode_repeated_session_info_bound_profile, state, (&(*result).session_info_bound_profile))
	&& zcbor_present_decode(&((*result).session_info_title_present), (zcbor_decoder_t *)decode_repeated_session_info_title, state, (&(*result).session_info_title))
	&& zcbor_present_decode(&((*result).session_info_last_activity_ms_present), (zcbor_decoder_t *)decode_repeated_session_info_last_activity_ms, state, (&(*result).session_info_last_activity_ms))
	&& zcbor_present_decode(&((*result).session_info_lifecycle_present), (zcbor_decoder_t *)decode_repeated_session_info_lifecycle, state, (&(*result).session_info_lifecycle))
	&& zcbor_present_decode(&((*result).session_info_role_present), (zcbor_decoder_t *)decode_repeated_session_info_role, state, (&(*result).session_info_role))
	&& zcbor_present_decode(&((*result).session_info_parent_present), (zcbor_decoder_t *)decode_repeated_session_info_parent, state, (&(*result).session_info_parent))
	&& zcbor_present_decode(&((*result).session_info_pinned_present), (zcbor_decoder_t *)decode_repeated_session_info_pinned, state, (&(*result).session_info_pinned))
	&& zcbor_present_decode(&((*result).session_info_archived_present), (zcbor_decoder_t *)decode_repeated_session_info_archived, state, (&(*result).session_info_archived))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_session_info_rewindable(state, (&(*result).session_info_rewindable));
		decode_repeated_session_info_bound_profile(state, (&(*result).session_info_bound_profile));
		decode_repeated_session_info_title(state, (&(*result).session_info_title));
		decode_repeated_session_info_last_activity_ms(state, (&(*result).session_info_last_activity_ms));
		decode_repeated_session_info_lifecycle(state, (&(*result).session_info_lifecycle));
		decode_repeated_session_info_role(state, (&(*result).session_info_role));
		decode_repeated_session_info_parent(state, (&(*result).session_info_parent));
		decode_repeated_session_info_pinned(state, (&(*result).session_info_pinned));
		decode_repeated_session_info_archived(state, (&(*result).session_info_archived));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_sessions(
		zcbor_state_t *state, struct response_sessions *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Sessions", tmp_str.len = sizeof("Sessions") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_sessions_Sessions_session_info_m_count, (zcbor_decoder_t *)decode_session_info, state, (*&(*result).response_sessions_Sessions_session_info_m), sizeof(struct session_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_session_info(state, (*&(*result).response_sessions_Sessions_session_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_approval_info_path(
		zcbor_state_t *state, struct approval_info_path_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).approval_info_path_tstr)))) && (((*result).approval_info_path_choice = approval_info_path_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).approval_info_path_choice = approval_info_path_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_approval_info(
		zcbor_state_t *state, struct approval_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).approval_info_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).approval_info_request_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"prompt", tmp_str.len = sizeof("prompt") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).approval_info_prompt))))
	&& zcbor_present_decode(&((*result).approval_info_path_present), (zcbor_decoder_t *)decode_repeated_approval_info_path, state, (&(*result).approval_info_path))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_approval_info_path(state, (&(*result).approval_info_path));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_approvals(
		zcbor_state_t *state, struct response_approvals *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Approvals", tmp_str.len = sizeof("Approvals") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_approvals_Approvals_approval_info_m_count, (zcbor_decoder_t *)decode_approval_info, state, (*&(*result).response_approvals_Approvals_approval_info_m), sizeof(struct approval_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_approval_info(state, (*&(*result).response_approvals_Approvals_approval_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_fleet_report(
		zcbor_state_t *state, struct fleet_report *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"children", tmp_str.len = sizeof("children") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).fleet_report_children_unit_id_m_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).fleet_report_children_unit_id_m), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"usage", tmp_str.len = sizeof("usage") - 1, &tmp_str)))))
	&& (decode_usage_delta(state, (&(*result).fleet_report_usage))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).fleet_report_children_unit_id_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_fleet(
		zcbor_state_t *state, struct response_fleet *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Fleet", tmp_str.len = sizeof("Fleet") - 1, &tmp_str)))))
	&& (decode_fleet_report(state, (&(*result).response_fleet_Fleet))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_unit_kind(
		zcbor_state_t *state, struct unit_kind_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Engine", tmp_str.len = sizeof("Engine") - 1, &tmp_str))))) && (((*result).unit_kind_choice = unit_kind_Engine_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Host", tmp_str.len = sizeof("Host") - 1, &tmp_str))))) && (((*result).unit_kind_choice = unit_kind_Host_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Orchestrator", tmp_str.len = sizeof("Orchestrator") - 1, &tmp_str))))) && (((*result).unit_kind_choice = unit_kind_Orchestrator_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_unit_state_finished(
		zcbor_state_t *state, struct unit_state_finished *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Finished", tmp_str.len = sizeof("Finished") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"end_reason", tmp_str.len = sizeof("end_reason") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Finished_end_reason))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_unit_state(
		zcbor_state_t *state, struct unit_state_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Running", tmp_str.len = sizeof("Running") - 1, &tmp_str))))) && (((*result).unit_state_choice = unit_state_Running_tstr_c), true))
	|| (((decode_unit_state_finished(state, (&(*result).unit_state_finished_m)))) && (((*result).unit_state_choice = unit_state_finished_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Unknown", tmp_str.len = sizeof("Unknown") - 1, &tmp_str))))) && (((*result).unit_state_choice = unit_state_Unknown_tstr_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_unit_node_profile(
		zcbor_state_t *state, struct unit_node_profile_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).unit_node_profile_profile_ref_m)))) && (((*result).unit_node_profile_choice = unit_node_profile_profile_ref_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).unit_node_profile_choice = unit_node_profile_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_unit_node_session(
		zcbor_state_t *state, struct unit_node_session_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).unit_node_session_session_id_m)))) && (((*result).unit_node_session_choice = unit_node_session_session_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).unit_node_session_choice = unit_node_session_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_unit_node_title(
		zcbor_state_t *state, struct unit_node_title_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).unit_node_title_tstr)))) && (((*result).unit_node_title_choice = unit_node_title_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).unit_node_title_choice = unit_node_title_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_unit_node_role(
		zcbor_state_t *state, struct unit_node_role_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_session_role(state, (&(*result).unit_node_role_session_role_m)))) && (((*result).unit_node_role_choice = unit_node_role_session_role_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).unit_node_role_choice = unit_node_role_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_unit_node(
		zcbor_state_t *state, struct unit_node *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).unit_node_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (decode_unit_kind(state, (&(*result).unit_node_kind))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (decode_unit_state(state, (&(*result).unit_node_state))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"work", tmp_str.len = sizeof("work") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).unit_node_work_tstr)))) && (((*result).unit_node_work_choice = unit_node_work_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).unit_node_work_choice = unit_node_work_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"usage", tmp_str.len = sizeof("usage") - 1, &tmp_str)))))
	&& (decode_usage_delta(state, (&(*result).unit_node_usage))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"children", tmp_str.len = sizeof("children") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).unit_node_children_unit_id_m_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).unit_node_children_unit_id_m), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& zcbor_present_decode(&((*result).unit_node_profile_present), (zcbor_decoder_t *)decode_repeated_unit_node_profile, state, (&(*result).unit_node_profile))
	&& zcbor_present_decode(&((*result).unit_node_session_present), (zcbor_decoder_t *)decode_repeated_unit_node_session, state, (&(*result).unit_node_session))
	&& zcbor_present_decode(&((*result).unit_node_title_present), (zcbor_decoder_t *)decode_repeated_unit_node_title, state, (&(*result).unit_node_title))
	&& zcbor_present_decode(&((*result).unit_node_role_present), (zcbor_decoder_t *)decode_repeated_unit_node_role, state, (&(*result).unit_node_role))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).unit_node_children_unit_id_m));
		decode_repeated_unit_node_profile(state, (&(*result).unit_node_profile));
		decode_repeated_unit_node_session(state, (&(*result).unit_node_session));
		decode_repeated_unit_node_title(state, (&(*result).unit_node_title));
		decode_repeated_unit_node_role(state, (&(*result).unit_node_role));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_tree_report(
		zcbor_state_t *state, struct tree_report *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).tree_report_root_unit_id_m)))) && (((*result).tree_report_root_choice = tree_report_root_unit_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).tree_report_root_choice = tree_report_root_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"nodes", tmp_str.len = sizeof("nodes") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).tree_report_nodes_unit_node_m_count, (zcbor_decoder_t *)decode_unit_node, state, (*&(*result).tree_report_nodes_unit_node_m), sizeof(struct unit_node))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_unit_node(state, (*&(*result).tree_report_nodes_unit_node_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_tree(
		zcbor_state_t *state, struct response_tree *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Tree", tmp_str.len = sizeof("Tree") - 1, &tmp_str)))))
	&& (decode_tree_report(state, (&(*result).response_tree_Tree))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_unit(
		zcbor_state_t *state, struct response_unit *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Unit", tmp_str.len = sizeof("Unit") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_unit_node(state, (&(*result).response_unit_Unit_unit_node_m)))) && (((*result).response_unit_Unit_choice = response_unit_Unit_unit_node_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).response_unit_Unit_choice = response_unit_Unit_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_manage_event_view_started(
		zcbor_state_t *state, struct manage_event_view_started *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Started", tmp_str.len = sizeof("Started") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Started_seq))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_manage_event_view_progress(
		zcbor_state_t *state, struct manage_event_view_progress *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Progress", tmp_str.len = sizeof("Progress") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Progress_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).Progress_text_tstr)))) && (((*result).Progress_text_choice = Progress_text_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).Progress_text_choice = Progress_text_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_manage_event_view_usage(
		zcbor_state_t *state, struct manage_event_view_usage *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Usage", tmp_str.len = sizeof("Usage") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Usage_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"delta", tmp_str.len = sizeof("delta") - 1, &tmp_str)))))
	&& (decode_usage_delta(state, (&(*result).Usage_delta))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_manage_event_view_finished(
		zcbor_state_t *state, struct manage_event_view_finished *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Finished", tmp_str.len = sizeof("Finished") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Finished_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"end_reason", tmp_str.len = sizeof("end_reason") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Finished_end_reason))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"summary", tmp_str.len = sizeof("summary") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).Finished_summary_tstr)))) && (((*result).Finished_summary_choice = Finished_summary_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).Finished_summary_choice = Finished_summary_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_manage_event_view_error(
		zcbor_state_t *state, struct manage_event_view_error *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Error", tmp_str.len = sizeof("Error") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Error_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"message", tmp_str.len = sizeof("message") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Error_message))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_subagent_phase(
		zcbor_state_t *state, struct subagent_phase_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Spawned", tmp_str.len = sizeof("Spawned") - 1, &tmp_str))))) && (((*result).subagent_phase_choice = subagent_phase_Spawned_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Finished", tmp_str.len = sizeof("Finished") - 1, &tmp_str))))) && (((*result).subagent_phase_choice = subagent_phase_Finished_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_manage_event_view_subagent(
		zcbor_state_t *state, struct manage_event_view_subagent *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Subagent", tmp_str.len = sizeof("Subagent") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Subagent_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"child", tmp_str.len = sizeof("child") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Subagent_child))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (decode_session_role(state, (&(*result).Subagent_role))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"phase", tmp_str.len = sizeof("phase") - 1, &tmp_str)))))
	&& (decode_subagent_phase(state, (&(*result).Subagent_phase))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"active_children", tmp_str.len = sizeof("active_children") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).Subagent_active_children))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_manage_event_view(
		zcbor_state_t *state, struct manage_event_view_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_manage_event_view_started(state, (&(*result).manage_event_view_started_m)))) && (((*result).manage_event_view_choice = manage_event_view_started_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_manage_event_view_progress(state, (&(*result).manage_event_view_progress_m)))) && (((*result).manage_event_view_choice = manage_event_view_progress_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_manage_event_view_usage(state, (&(*result).manage_event_view_usage_m)))) && (((*result).manage_event_view_choice = manage_event_view_usage_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_manage_event_view_finished(state, (&(*result).manage_event_view_finished_m)))) && (((*result).manage_event_view_choice = manage_event_view_finished_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_manage_event_view_error(state, (&(*result).manage_event_view_error_m)))) && (((*result).manage_event_view_choice = manage_event_view_error_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_manage_event_view_subagent(state, (&(*result).manage_event_view_subagent_m)))) && (((*result).manage_event_view_choice = manage_event_view_subagent_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_unit_events(
		zcbor_state_t *state, struct response_unit_events *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"UnitEvents", tmp_str.len = sizeof("UnitEvents") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_unit_events_UnitEvents_manage_event_view_m_count, (zcbor_decoder_t *)decode_manage_event_view, state, (*&(*result).response_unit_events_UnitEvents_manage_event_view_m), sizeof(struct manage_event_view_r))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_manage_event_view(state, (*&(*result).response_unit_events_UnitEvents_manage_event_view_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_journal_record_payload_management(
		zcbor_state_t *state, struct journal_record_payload_management *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Management", tmp_str.len = sizeof("Management") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Management_detail))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_transcript_role(
		zcbor_state_t *state, struct transcript_role_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Assistant", tmp_str.len = sizeof("Assistant") - 1, &tmp_str))))) && (((*result).transcript_role_choice = transcript_role_Assistant_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"User", tmp_str.len = sizeof("User") - 1, &tmp_str))))) && (((*result).transcript_role_choice = transcript_role_User_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"System", tmp_str.len = sizeof("System") - 1, &tmp_str))))) && (((*result).transcript_role_choice = transcript_role_System_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_transcript_block_message(
		zcbor_state_t *state, struct transcript_block_message *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Message", tmp_str.len = sizeof("Message") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (decode_transcript_role(state, (&(*result).Message_role))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Message_text))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_ToolCall_detail(
		zcbor_state_t *state, struct ToolCall_detail_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_tool_detail(state, (&(*result).ToolCall_detail_tool_detail_m)))) && (((*result).ToolCall_detail_choice = ToolCall_detail_tool_detail_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).ToolCall_detail_choice = ToolCall_detail_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_transcript_block_tool_call(
		zcbor_state_t *state, struct transcript_block_tool_call *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ToolCall", tmp_str.len = sizeof("ToolCall") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"call_id", tmp_str.len = sizeof("call_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ToolCall_call_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ToolCall_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"args_summary", tmp_str.len = sizeof("args_summary") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ToolCall_args_summary))))
	&& zcbor_present_decode(&((*result).ToolCall_detail_present), (zcbor_decoder_t *)decode_repeated_ToolCall_detail, state, (&(*result).ToolCall_detail))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_ToolCall_detail(state, (&(*result).ToolCall_detail));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_ToolResult_detail(
		zcbor_state_t *state, struct ToolResult_detail_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_tool_detail(state, (&(*result).ToolResult_detail_tool_detail_m)))) && (((*result).ToolResult_detail_choice = ToolResult_detail_tool_detail_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).ToolResult_detail_choice = ToolResult_detail_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_transcript_block_tool_result(
		zcbor_state_t *state, struct transcript_block_tool_result *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ToolResult", tmp_str.len = sizeof("ToolResult") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"call_id", tmp_str.len = sizeof("call_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ToolResult_call_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ok", tmp_str.len = sizeof("ok") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).ToolResult_ok))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"summary", tmp_str.len = sizeof("summary") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ToolResult_summary))))
	&& zcbor_present_decode(&((*result).ToolResult_detail_present), (zcbor_decoder_t *)decode_repeated_ToolResult_detail, state, (&(*result).ToolResult_detail))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_ToolResult_detail(state, (&(*result).ToolResult_detail));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_transcript_block_request(
		zcbor_state_t *state, struct transcript_block_request *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Request", tmp_str.len = sizeof("Request") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).Request_request_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (decode_host_request_kind_t(state, (&(*result).Request_kind))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_transcript_block_content(
		zcbor_state_t *state, struct transcript_block_content *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Content", tmp_str.len = sizeof("Content") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Content_kind))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (zcbor_bstr_decode(state, (&(*result).Content_body))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_transcript_block(
		zcbor_state_t *state, struct transcript_block_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_transcript_block_message(state, (&(*result).transcript_block_message_m)))) && (((*result).transcript_block_choice = transcript_block_message_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_transcript_block_tool_call(state, (&(*result).transcript_block_tool_call_m)))) && (((*result).transcript_block_choice = transcript_block_tool_call_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_transcript_block_tool_result(state, (&(*result).transcript_block_tool_result_m)))) && (((*result).transcript_block_choice = transcript_block_tool_result_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_transcript_block_request(state, (&(*result).transcript_block_request_m)))) && (((*result).transcript_block_choice = transcript_block_request_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_transcript_block_content(state, (&(*result).transcript_block_content_m)))) && (((*result).transcript_block_choice = transcript_block_content_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_journal_record_payload_block(
		zcbor_state_t *state, struct journal_record_payload_block *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Block", tmp_str.len = sizeof("Block") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"block", tmp_str.len = sizeof("block") - 1, &tmp_str)))))
	&& (decode_transcript_block(state, (&(*result).Block_block))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_journal_record_payload_t(
		zcbor_state_t *state, struct journal_record_payload_t_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_journal_record_payload_management(state, (&(*result).journal_record_payload_t_journal_record_payload_management_m)))) && (((*result).journal_record_payload_t_choice = journal_record_payload_t_journal_record_payload_management_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_journal_record_payload_block(state, (&(*result).journal_record_payload_t_journal_record_payload_block_m)))) && (((*result).journal_record_payload_t_choice = journal_record_payload_t_journal_record_payload_block_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_journal_record(
		zcbor_state_t *state, struct journal_record *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"cursor", tmp_str.len = sizeof("cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).journal_record_cursor))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"segment", tmp_str.len = sizeof("segment") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).journal_record_segment))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).journal_record_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"epoch", tmp_str.len = sizeof("epoch") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).journal_record_epoch))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"trace", tmp_str.len = sizeof("trace") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).journal_record_trace))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).journal_record_kind))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"timestamp_ms", tmp_str.len = sizeof("timestamp_ms") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).journal_record_timestamp_ms))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"verified", tmp_str.len = sizeof("verified") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).journal_record_verified))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"payload", tmp_str.len = sizeof("payload") - 1, &tmp_str)))))
	&& (decode_journal_record_payload_t(state, (&(*result).journal_record_payload))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_journal_page_view(
		zcbor_state_t *state, struct journal_page_view *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"entries", tmp_str.len = sizeof("entries") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).journal_page_view_entries_journal_record_m_count, (zcbor_decoder_t *)decode_journal_record, state, (*&(*result).journal_page_view_entries_journal_record_m), sizeof(struct journal_record))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"next_cursor", tmp_str.len = sizeof("next_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).journal_page_view_next_cursor))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"head_cursor", tmp_str.len = sizeof("head_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).journal_page_view_head_cursor))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"sealed_after", tmp_str.len = sizeof("sealed_after") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).journal_page_view_sealed_after_uint64_m)))) && (((*result).journal_page_view_sealed_after_choice = journal_page_view_sealed_after_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).journal_page_view_sealed_after_choice = journal_page_view_sealed_after_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_journal_record(state, (*&(*result).journal_page_view_entries_journal_record_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_journal(
		zcbor_state_t *state, struct response_journal *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Journal", tmp_str.len = sizeof("Journal") - 1, &tmp_str)))))
	&& (decode_journal_page_view(state, (&(*result).response_journal_Journal))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_direction(
		zcbor_state_t *state, struct direction_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Inbound", tmp_str.len = sizeof("Inbound") - 1, &tmp_str))))) && (((*result).direction_choice = direction_Inbound_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Outbound", tmp_str.len = sizeof("Outbound") - 1, &tmp_str))))) && (((*result).direction_choice = direction_Outbound_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_disposition(
		zcbor_state_t *state, struct disposition_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Context", tmp_str.len = sizeof("Context") - 1, &tmp_str))))) && (((*result).disposition_choice = disposition_Context_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Transport", tmp_str.len = sizeof("Transport") - 1, &tmp_str))))) && (((*result).disposition_choice = disposition_Transport_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_payload_event(
		zcbor_state_t *state, struct session_payload_event *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Event", tmp_str.len = sizeof("Event") - 1, &tmp_str)))))
	&& (decode_agent_event(state, (&(*result).session_payload_event_Event))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_payload_request(
		zcbor_state_t *state, struct session_payload_request *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Request", tmp_str.len = sizeof("Request") - 1, &tmp_str)))))
	&& (decode_host_request(state, (&(*result).session_payload_request_Request))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_payload_command(
		zcbor_state_t *state, struct session_payload_command *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Command", tmp_str.len = sizeof("Command") - 1, &tmp_str)))))
	&& (decode_agent_command(state, (&(*result).session_payload_command_Command))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_payload_response(
		zcbor_state_t *state, struct session_payload_response *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Response", tmp_str.len = sizeof("Response") - 1, &tmp_str)))))
	&& (decode_host_response(state, (&(*result).session_payload_response_Response))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_payload_meta(
		zcbor_state_t *state, struct session_payload_meta *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Meta", tmp_str.len = sizeof("Meta") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Meta_kind))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (zcbor_bstr_decode(state, (&(*result).Meta_body))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_payload(
		zcbor_state_t *state, struct session_payload_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_session_payload_event(state, (&(*result).session_payload_event_m)))) && (((*result).session_payload_choice = session_payload_event_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_session_payload_request(state, (&(*result).session_payload_request_m)))) && (((*result).session_payload_choice = session_payload_request_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_session_payload_command(state, (&(*result).session_payload_command_m)))) && (((*result).session_payload_choice = session_payload_command_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_session_payload_response(state, (&(*result).session_payload_response_m)))) && (((*result).session_payload_choice = session_payload_response_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_session_payload_meta(state, (&(*result).session_payload_meta_m)))) && (((*result).session_payload_choice = session_payload_meta_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_log_entry(
		zcbor_state_t *state, struct session_log_entry *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).session_log_entry_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"direction", tmp_str.len = sizeof("direction") - 1, &tmp_str)))))
	&& (decode_direction(state, (&(*result).session_log_entry_direction))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (decode_origin(state, (&(*result).session_log_entry_origin))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"disposition", tmp_str.len = sizeof("disposition") - 1, &tmp_str)))))
	&& (decode_disposition(state, (&(*result).session_log_entry_disposition))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"payload", tmp_str.len = sizeof("payload") - 1, &tmp_str)))))
	&& (decode_session_payload(state, (&(*result).session_log_entry_payload))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_log_page_view(
		zcbor_state_t *state, struct log_page_view *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"entries", tmp_str.len = sizeof("entries") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).log_page_view_entries_session_log_entry_m_count, (zcbor_decoder_t *)decode_session_log_entry, state, (*&(*result).log_page_view_entries_session_log_entry_m), sizeof(struct session_log_entry))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"next_seq", tmp_str.len = sizeof("next_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).log_page_view_next_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"head_seq", tmp_str.len = sizeof("head_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).log_page_view_head_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"epoch", tmp_str.len = sizeof("epoch") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).log_page_view_epoch))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_session_log_entry(state, (*&(*result).log_page_view_entries_session_log_entry_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_log_page(
		zcbor_state_t *state, struct response_log_page *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"LogPage", tmp_str.len = sizeof("LogPage") - 1, &tmp_str)))))
	&& (decode_log_page_view(state, (&(*result).response_log_page_LogPage))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_node_event_session_advanced(
		zcbor_state_t *state, struct node_event_session_advanced *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionAdvanced", tmp_str.len = sizeof("SessionAdvanced") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SessionAdvanced_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"epoch", tmp_str.len = sizeof("epoch") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).SessionAdvanced_epoch))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"head_seq", tmp_str.len = sizeof("head_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).SessionAdvanced_head_seq))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_node_event_session_meta_changed(
		zcbor_state_t *state, struct node_event_session_meta_changed *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionMetaChanged", tmp_str.len = sizeof("SessionMetaChanged") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).SessionMetaChanged_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"rev", tmp_str.len = sizeof("rev") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).SessionMetaChanged_rev))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_node_event_roster_changed(
		zcbor_state_t *state, struct node_event_roster_changed *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"RosterChanged", tmp_str.len = sizeof("RosterChanged") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"rev", tmp_str.len = sizeof("rev") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).RosterChanged_rev))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_node_event_fleet_changed(
		zcbor_state_t *state, struct node_event_fleet_changed *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FleetChanged", tmp_str.len = sizeof("FleetChanged") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"rev", tmp_str.len = sizeof("rev") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).FleetChanged_rev))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_node_event_approval_pending(
		zcbor_state_t *state, struct node_event_approval_pending *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ApprovalPending", tmp_str.len = sizeof("ApprovalPending") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ApprovalPending_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ApprovalPending_request_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_node_event_download_progress(
		zcbor_state_t *state, struct node_event_download_progress *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"DownloadProgress", tmp_str.len = sizeof("DownloadProgress") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).DownloadProgress_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"pct", tmp_str.len = sizeof("pct") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).DownloadProgress_pct))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).DownloadProgress_state))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_node_event_resync_needed(
		zcbor_state_t *state, struct node_event_resync_needed *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ResyncNeeded", tmp_str.len = sizeof("ResyncNeeded") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"scope", tmp_str.len = sizeof("scope") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).ResyncNeeded_scope))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_node_event(
		zcbor_state_t *state, struct node_event_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_node_event_session_advanced(state, (&(*result).node_event_session_advanced_m)))) && (((*result).node_event_choice = node_event_session_advanced_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_node_event_session_meta_changed(state, (&(*result).node_event_session_meta_changed_m)))) && (((*result).node_event_choice = node_event_session_meta_changed_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_node_event_roster_changed(state, (&(*result).node_event_roster_changed_m)))) && (((*result).node_event_choice = node_event_roster_changed_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_node_event_fleet_changed(state, (&(*result).node_event_fleet_changed_m)))) && (((*result).node_event_choice = node_event_fleet_changed_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_node_event_approval_pending(state, (&(*result).node_event_approval_pending_m)))) && (((*result).node_event_choice = node_event_approval_pending_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_node_event_download_progress(state, (&(*result).node_event_download_progress_m)))) && (((*result).node_event_choice = node_event_download_progress_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_node_event_resync_needed(state, (&(*result).node_event_resync_needed_m)))) && (((*result).node_event_choice = node_event_resync_needed_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_events_page(
		zcbor_state_t *state, struct events_page *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"events", tmp_str.len = sizeof("events") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).events_page_events_node_event_m_count, (zcbor_decoder_t *)decode_node_event, state, (*&(*result).events_page_events_node_event_m), sizeof(struct node_event_r))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"next_cursor", tmp_str.len = sizeof("next_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).events_page_next_cursor))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"head_cursor", tmp_str.len = sizeof("head_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).events_page_head_cursor))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_node_event(state, (*&(*result).events_page_events_node_event_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_events_page(
		zcbor_state_t *state, struct response_events_page *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"EventsPage", tmp_str.len = sizeof("EventsPage") - 1, &tmp_str)))))
	&& (decode_events_page(state, (&(*result).response_events_page_EventsPage))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_delivery_targets(
		zcbor_state_t *state, struct response_delivery_targets *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"DeliveryTargets", tmp_str.len = sizeof("DeliveryTargets") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_delivery_targets_DeliveryTargets_delivery_target_m_count, (zcbor_decoder_t *)decode_delivery_target, state, (*&(*result).response_delivery_targets_DeliveryTargets_delivery_target_m), sizeof(struct delivery_target))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_delivery_target(state, (*&(*result).response_delivery_targets_DeliveryTargets_delivery_target_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_delivery_sessions(
		zcbor_state_t *state, struct response_delivery_sessions *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"DeliverySessions", tmp_str.len = sizeof("DeliverySessions") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_delivery_sessions_DeliverySessions_session_id_m_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).response_delivery_sessions_DeliverySessions_session_id_m), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).response_delivery_sessions_DeliverySessions_session_id_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_verifying_key(
		zcbor_state_t *state, struct response_verifying_key *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"VerifyingKey", tmp_str.len = sizeof("VerifyingKey") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).response_verifying_key_VerifyingKey_tstr)))) && (((*result).response_verifying_key_VerifyingKey_choice = response_verifying_key_VerifyingKey_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).response_verifying_key_VerifyingKey_choice = response_verifying_key_VerifyingKey_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_search_hit(
		zcbor_state_t *state, struct search_hit *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).search_hit_repo))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"author", tmp_str.len = sizeof("author") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).search_hit_author_tstr)))) && (((*result).search_hit_author_choice = search_hit_author_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).search_hit_author_choice = search_hit_author_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"downloads", tmp_str.len = sizeof("downloads") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).search_hit_downloads))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"likes", tmp_str.len = sizeof("likes") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).search_hit_likes))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"num_parameters", tmp_str.len = sizeof("num_parameters") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).search_hit_num_parameters_uint64_m)))) && (((*result).search_hit_num_parameters_choice = search_hit_num_parameters_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).search_hit_num_parameters_choice = search_hit_num_parameters_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"pipeline_tag", tmp_str.len = sizeof("pipeline_tag") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).search_hit_pipeline_tag_tstr)))) && (((*result).search_hit_pipeline_tag_choice = search_hit_pipeline_tag_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).search_hit_pipeline_tag_choice = search_hit_pipeline_tag_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"last_modified", tmp_str.len = sizeof("last_modified") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).search_hit_last_modified_tstr)))) && (((*result).search_hit_last_modified_choice = search_hit_last_modified_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).search_hit_last_modified_choice = search_hit_last_modified_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"gated", tmp_str.len = sizeof("gated") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).search_hit_gated))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"private", tmp_str.len = sizeof("private") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).search_hit_private))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_search_page(
		zcbor_state_t *state, struct search_page *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"page", tmp_str.len = sizeof("page") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).search_page_page))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"results", tmp_str.len = sizeof("results") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).search_page_results_search_hit_m_count, (zcbor_decoder_t *)decode_search_hit, state, (*&(*result).search_page_results_search_hit_m), sizeof(struct search_hit))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"has_more", tmp_str.len = sizeof("has_more") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).search_page_has_more))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_search_hit(state, (*&(*result).search_page_results_search_hit_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_model_search(
		zcbor_state_t *state, struct response_model_search *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelSearch", tmp_str.len = sizeof("ModelSearch") - 1, &tmp_str)))))
	&& (decode_search_page(state, (&(*result).response_model_search_ModelSearch))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_model_file(
		zcbor_state_t *state, struct model_file *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).model_file_path))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"size_bytes", tmp_str.len = sizeof("size_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).model_file_size_bytes))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"quant", tmp_str.len = sizeof("quant") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).model_file_quant_tstr)))) && (((*result).model_file_quant_choice = model_file_quant_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).model_file_quant_choice = model_file_quant_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"is_split", tmp_str.len = sizeof("is_split") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).model_file_is_split))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"is_first_shard", tmp_str.len = sizeof("is_first_shard") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).model_file_is_first_shard))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_model_files(
		zcbor_state_t *state, struct response_model_files *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelFiles", tmp_str.len = sizeof("ModelFiles") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_model_files_ModelFiles_model_file_m_count, (zcbor_decoder_t *)decode_model_file, state, (*&(*result).response_model_files_ModelFiles_model_file_m), sizeof(struct model_file))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_model_file(state, (*&(*result).response_model_files_ModelFiles_model_file_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_model_download_started(
		zcbor_state_t *state, struct response_model_download_started *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelDownloadStarted", tmp_str.len = sizeof("ModelDownloadStarted") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).response_model_download_started_ModelDownloadStarted))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_download_state(
		zcbor_state_t *state, struct download_state_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Queued", tmp_str.len = sizeof("Queued") - 1, &tmp_str))))) && (((*result).download_state_choice = download_state_Queued_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Downloading", tmp_str.len = sizeof("Downloading") - 1, &tmp_str))))) && (((*result).download_state_choice = download_state_Downloading_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Completed", tmp_str.len = sizeof("Completed") - 1, &tmp_str))))) && (((*result).download_state_choice = download_state_Completed_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Paused", tmp_str.len = sizeof("Paused") - 1, &tmp_str))))) && (((*result).download_state_choice = download_state_Paused_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Cancelled", tmp_str.len = sizeof("Cancelled") - 1, &tmp_str))))) && (((*result).download_state_choice = download_state_Cancelled_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Failed", tmp_str.len = sizeof("Failed") - 1, &tmp_str))))) && (((*result).download_state_choice = download_state_Failed_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_download_status(
		zcbor_state_t *state, struct download_status *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).download_status_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (decode_model_ref(state, (&(*result).download_status_model))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (decode_download_state(state, (&(*result).download_status_state))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"downloaded_bytes", tmp_str.len = sizeof("downloaded_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).download_status_downloaded_bytes))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"total_bytes", tmp_str.len = sizeof("total_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).download_status_total_bytes))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"files_done", tmp_str.len = sizeof("files_done") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).download_status_files_done))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"files_total", tmp_str.len = sizeof("files_total") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).download_status_files_total))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"error", tmp_str.len = sizeof("error") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).download_status_error_tstr)))) && (((*result).download_status_error_choice = download_status_error_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).download_status_error_choice = download_status_error_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_model_downloads(
		zcbor_state_t *state, struct response_model_downloads *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelDownloads", tmp_str.len = sizeof("ModelDownloads") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_model_downloads_ModelDownloads_download_status_m_count, (zcbor_decoder_t *)decode_download_status, state, (*&(*result).response_model_downloads_ModelDownloads_download_status_m), sizeof(struct download_status))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_download_status(state, (*&(*result).response_model_downloads_ModelDownloads_download_status_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_installed_model_arch(
		zcbor_state_t *state, struct installed_model_arch_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"arch", tmp_str.len = sizeof("arch") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).installed_model_arch_tstr)))) && (((*result).installed_model_arch_choice = installed_model_arch_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).installed_model_arch_choice = installed_model_arch_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_installed_model_context_length(
		zcbor_state_t *state, struct installed_model_context_length_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"context_length", tmp_str.len = sizeof("context_length") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint32_decode(state, (&(*result).installed_model_context_length_uint)))) && (((*result).installed_model_context_length_choice = installed_model_context_length_uint_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).installed_model_context_length_choice = installed_model_context_length_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_installed_model_file_type(
		zcbor_state_t *state, struct installed_model_file_type_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"file_type", tmp_str.len = sizeof("file_type") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).installed_model_file_type_tstr)))) && (((*result).installed_model_file_type_choice = installed_model_file_type_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).installed_model_file_type_choice = installed_model_file_type_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_installed_model(
		zcbor_state_t *state, struct installed_model *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).installed_model_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (decode_model_ref(state, (&(*result).installed_model_model))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"display_name", tmp_str.len = sizeof("display_name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).installed_model_display_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"local_path", tmp_str.len = sizeof("local_path") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).installed_model_local_path))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"size_bytes", tmp_str.len = sizeof("size_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).installed_model_size_bytes))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"quant", tmp_str.len = sizeof("quant") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).installed_model_quant_tstr)))) && (((*result).installed_model_quant_choice = installed_model_quant_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).installed_model_quant_choice = installed_model_quant_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"installed_at_ms", tmp_str.len = sizeof("installed_at_ms") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).installed_model_installed_at_ms))))
	&& zcbor_present_decode(&((*result).installed_model_arch_present), (zcbor_decoder_t *)decode_repeated_installed_model_arch, state, (&(*result).installed_model_arch))
	&& zcbor_present_decode(&((*result).installed_model_context_length_present), (zcbor_decoder_t *)decode_repeated_installed_model_context_length, state, (&(*result).installed_model_context_length))
	&& zcbor_present_decode(&((*result).installed_model_file_type_present), (zcbor_decoder_t *)decode_repeated_installed_model_file_type, state, (&(*result).installed_model_file_type))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_installed_model_arch(state, (&(*result).installed_model_arch));
		decode_repeated_installed_model_context_length(state, (&(*result).installed_model_context_length));
		decode_repeated_installed_model_file_type(state, (&(*result).installed_model_file_type));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_model_catalog(
		zcbor_state_t *state, struct response_model_catalog *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelCatalog", tmp_str.len = sizeof("ModelCatalog") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_model_catalog_ModelCatalog_installed_model_m_count, (zcbor_decoder_t *)decode_installed_model, state, (*&(*result).response_model_catalog_ModelCatalog_installed_model_m), sizeof(struct installed_model))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_installed_model(state, (*&(*result).response_model_catalog_ModelCatalog_installed_model_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_quant_candidate(
		zcbor_state_t *state, struct quant_candidate *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"quant", tmp_str.len = sizeof("quant") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).quant_candidate_quant))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"file", tmp_str.len = sizeof("file") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).quant_candidate_file_tstr)))) && (((*result).quant_candidate_file_choice = quant_candidate_file_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).quant_candidate_file_choice = quant_candidate_file_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"size_bytes", tmp_str.len = sizeof("size_bytes") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).quant_candidate_size_bytes_uint64_m)))) && (((*result).quant_candidate_size_bytes_choice = quant_candidate_size_bytes_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).quant_candidate_size_bytes_choice = quant_candidate_size_bytes_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"fits", tmp_str.len = sizeof("fits") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).quant_candidate_fits))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_quant_recommendation(
		zcbor_state_t *state, struct quant_recommendation *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"engine", tmp_str.len = sizeof("engine") - 1, &tmp_str)))))
	&& (decode_model_engine(state, (&(*result).quant_recommendation_engine))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).quant_recommendation_repo))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"file", tmp_str.len = sizeof("file") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).quant_recommendation_file_tstr)))) && (((*result).quant_recommendation_file_choice = quant_recommendation_file_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).quant_recommendation_file_choice = quant_recommendation_file_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"quant", tmp_str.len = sizeof("quant") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).quant_recommendation_quant))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"size_bytes", tmp_str.len = sizeof("size_bytes") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).quant_recommendation_size_bytes_uint64_m)))) && (((*result).quant_recommendation_size_bytes_choice = quant_recommendation_size_bytes_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).quant_recommendation_size_bytes_choice = quant_recommendation_size_bytes_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"budget_bytes", tmp_str.len = sizeof("budget_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).quant_recommendation_budget_bytes))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"fits", tmp_str.len = sizeof("fits") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).quant_recommendation_fits))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"reason", tmp_str.len = sizeof("reason") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).quant_recommendation_reason))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"candidates", tmp_str.len = sizeof("candidates") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).quant_recommendation_candidates_quant_candidate_m_count, (zcbor_decoder_t *)decode_quant_candidate, state, (*&(*result).quant_recommendation_candidates_quant_candidate_m), sizeof(struct quant_candidate))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_quant_candidate(state, (*&(*result).quant_recommendation_candidates_quant_candidate_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_model_recommend(
		zcbor_state_t *state, struct response_model_recommend *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelRecommend", tmp_str.len = sizeof("ModelRecommend") - 1, &tmp_str)))))
	&& (decode_quant_recommendation(state, (&(*result).response_model_recommend_ModelRecommend))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_model_quantize_started(
		zcbor_state_t *state, struct response_model_quantize_started *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelQuantizeStarted", tmp_str.len = sizeof("ModelQuantizeStarted") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).response_model_quantize_started_ModelQuantizeStarted))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_quantize_state(
		zcbor_state_t *state, struct quantize_state_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Queued", tmp_str.len = sizeof("Queued") - 1, &tmp_str))))) && (((*result).quantize_state_choice = quantize_state_Queued_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Preparing", tmp_str.len = sizeof("Preparing") - 1, &tmp_str))))) && (((*result).quantize_state_choice = quantize_state_Preparing_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Quantizing", tmp_str.len = sizeof("Quantizing") - 1, &tmp_str))))) && (((*result).quantize_state_choice = quantize_state_Quantizing_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Completed", tmp_str.len = sizeof("Completed") - 1, &tmp_str))))) && (((*result).quantize_state_choice = quantize_state_Completed_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Failed", tmp_str.len = sizeof("Failed") - 1, &tmp_str))))) && (((*result).quantize_state_choice = quantize_state_Failed_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_quantize_status(
		zcbor_state_t *state, struct quantize_status *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).quantize_status_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).quantize_status_repo))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"source_file", tmp_str.len = sizeof("source_file") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).quantize_status_source_file))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"target_quant", tmp_str.len = sizeof("target_quant") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).quantize_status_target_quant))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (decode_quantize_state(state, (&(*result).quantize_status_state))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"output_path", tmp_str.len = sizeof("output_path") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).quantize_status_output_path_tstr)))) && (((*result).quantize_status_output_path_choice = quantize_status_output_path_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).quantize_status_output_path_choice = quantize_status_output_path_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"model_id", tmp_str.len = sizeof("model_id") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).quantize_status_model_id_model_id_m)))) && (((*result).quantize_status_model_id_choice = quantize_status_model_id_model_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).quantize_status_model_id_choice = quantize_status_model_id_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"error", tmp_str.len = sizeof("error") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).quantize_status_error_tstr)))) && (((*result).quantize_status_error_choice = quantize_status_error_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).quantize_status_error_choice = quantize_status_error_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_model_quantizes(
		zcbor_state_t *state, struct response_model_quantizes *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelQuantizes", tmp_str.len = sizeof("ModelQuantizes") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_model_quantizes_ModelQuantizes_quantize_status_m_count, (zcbor_decoder_t *)decode_quantize_status, state, (*&(*result).response_model_quantizes_ModelQuantizes_quantize_status_m), sizeof(struct quantize_status))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_quantize_status(state, (*&(*result).response_model_quantizes_ModelQuantizes_quantize_status_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_gguf_info(
		zcbor_state_t *state, struct gguf_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"architecture", tmp_str.len = sizeof("architecture") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).gguf_info_architecture_tstr)))) && (((*result).gguf_info_architecture_choice = gguf_info_architecture_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).gguf_info_architecture_choice = gguf_info_architecture_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).gguf_info_name_tstr)))) && (((*result).gguf_info_name_choice = gguf_info_name_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).gguf_info_name_choice = gguf_info_name_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"file_type", tmp_str.len = sizeof("file_type") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).gguf_info_file_type_tstr)))) && (((*result).gguf_info_file_type_choice = gguf_info_file_type_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).gguf_info_file_type_choice = gguf_info_file_type_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"context_length", tmp_str.len = sizeof("context_length") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint32_decode(state, (&(*result).gguf_info_context_length_uint)))) && (((*result).gguf_info_context_length_choice = gguf_info_context_length_uint_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).gguf_info_context_length_choice = gguf_info_context_length_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"block_count", tmp_str.len = sizeof("block_count") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint32_decode(state, (&(*result).gguf_info_block_count_uint)))) && (((*result).gguf_info_block_count_choice = gguf_info_block_count_uint_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).gguf_info_block_count_choice = gguf_info_block_count_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"quantization_version", tmp_str.len = sizeof("quantization_version") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint32_decode(state, (&(*result).gguf_info_quantization_version_uint)))) && (((*result).gguf_info_quantization_version_choice = gguf_info_quantization_version_uint_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).gguf_info_quantization_version_choice = gguf_info_quantization_version_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"parameter_count", tmp_str.len = sizeof("parameter_count") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).gguf_info_parameter_count_uint64_m)))) && (((*result).gguf_info_parameter_count_choice = gguf_info_parameter_count_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).gguf_info_parameter_count_choice = gguf_info_parameter_count_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"size_bytes", tmp_str.len = sizeof("size_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).gguf_info_size_bytes))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_model_inspect(
		zcbor_state_t *state, struct response_model_inspect *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelInspect", tmp_str.len = sizeof("ModelInspect") - 1, &tmp_str)))))
	&& (decode_gguf_info(state, (&(*result).response_model_inspect_ModelInspect))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_profile_info(
		zcbor_state_t *state, struct profile_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).profile_info_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (decode_provider_selector(state, (&(*result).profile_info_provider))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).profile_info_model))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"is_active", tmp_str.len = sizeof("is_active") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).profile_info_is_active))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"bound_accounts", tmp_str.len = sizeof("bound_accounts") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).profile_info_bound_accounts_bound_account_m_count, (zcbor_decoder_t *)decode_bound_account, state, (*&(*result).profile_info_bound_accounts_bound_account_m), sizeof(struct bound_account))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_bound_account(state, (*&(*result).profile_info_bound_accounts_bound_account_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_profiles(
		zcbor_state_t *state, struct response_profiles *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Profiles", tmp_str.len = sizeof("Profiles") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_profiles_Profiles_profile_info_m_count, (zcbor_decoder_t *)decode_profile_info, state, (*&(*result).response_profiles_Profiles_profile_info_m), sizeof(struct profile_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_profile_info(state, (*&(*result).response_profiles_Profiles_profile_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_profile(
		zcbor_state_t *state, struct response_profile *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Profile", tmp_str.len = sizeof("Profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_profile_spec(state, (&(*result).response_profile_Profile_profile_spec_m)))) && (((*result).response_profile_Profile_choice = response_profile_Profile_profile_spec_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).response_profile_Profile_choice = response_profile_Profile_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_credential_info(
		zcbor_state_t *state, struct credential_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).credential_info_profile))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"present", tmp_str.len = sizeof("present") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).credential_info_present))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"hint", tmp_str.len = sizeof("hint") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).credential_info_hint))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_credentials(
		zcbor_state_t *state, struct response_credentials *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Credentials", tmp_str.len = sizeof("Credentials") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_credentials_Credentials_credential_info_m_count, (zcbor_decoder_t *)decode_credential_info, state, (*&(*result).response_credentials_Credentials_credential_info_m), sizeof(struct credential_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_credential_info(state, (*&(*result).response_credentials_Credentials_credential_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_model_descriptor(
		zcbor_state_t *state, struct model_descriptor *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).model_descriptor_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (decode_provider_selector(state, (&(*result).model_descriptor_provider))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"context_length", tmp_str.len = sizeof("context_length") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint32_decode(state, (&(*result).model_descriptor_context_length_uint)))) && (((*result).model_descriptor_context_length_choice = model_descriptor_context_length_uint_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).model_descriptor_context_length_choice = model_descriptor_context_length_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"input_price_micros_per_mtok", tmp_str.len = sizeof("input_price_micros_per_mtok") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).model_descriptor_input_price_micros_per_mtok_uint64_m)))) && (((*result).model_descriptor_input_price_micros_per_mtok_choice = model_descriptor_input_price_micros_per_mtok_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).model_descriptor_input_price_micros_per_mtok_choice = model_descriptor_input_price_micros_per_mtok_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"output_price_micros_per_mtok", tmp_str.len = sizeof("output_price_micros_per_mtok") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).model_descriptor_output_price_micros_per_mtok_uint64_m)))) && (((*result).model_descriptor_output_price_micros_per_mtok_choice = model_descriptor_output_price_micros_per_mtok_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).model_descriptor_output_price_micros_per_mtok_choice = model_descriptor_output_price_micros_per_mtok_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"local", tmp_str.len = sizeof("local") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).model_descriptor_local))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_models(
		zcbor_state_t *state, struct response_models *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Models", tmp_str.len = sizeof("Models") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_models_Models_model_descriptor_m_count, (zcbor_decoder_t *)decode_model_descriptor, state, (*&(*result).response_models_Models_model_descriptor_m), sizeof(struct model_descriptor))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_model_descriptor(state, (*&(*result).response_models_Models_model_descriptor_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_model_current(
		zcbor_state_t *state, struct response_model_current *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelCurrent", tmp_str.len = sizeof("ModelCurrent") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_model_descriptor(state, (&(*result).response_model_current_ModelCurrent_model_descriptor_m)))) && (((*result).response_model_current_ModelCurrent_choice = response_model_current_ModelCurrent_model_descriptor_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).response_model_current_ModelCurrent_choice = response_model_current_ModelCurrent_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_distribution(
		zcbor_state_t *state, struct response_distribution *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Distribution", tmp_str.len = sizeof("Distribution") - 1, &tmp_str)))))
	&& (decode_distribution(state, (&(*result).response_distribution_Distribution))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_profile_id(
		zcbor_state_t *state, struct response_profile_id *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileId", tmp_str.len = sizeof("ProfileId") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).response_profile_id_ProfileId))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_author_agent(
		zcbor_state_t *state, struct author_agent *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"agent", tmp_str.len = sizeof("agent") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).author_agent_agent))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_author(
		zcbor_state_t *state, struct author_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"operator", tmp_str.len = sizeof("operator") - 1, &tmp_str))))) && (((*result).author_choice = author_operator_tstr_c), true))
	|| (((decode_author_agent(state, (&(*result).author_agent_m)))) && (((*result).author_choice = author_agent_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_revision(
		zcbor_state_t *state, struct revision *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).revision_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"parent", tmp_str.len = sizeof("parent") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).revision_parent_uint64_m)))) && (((*result).revision_parent_choice = revision_parent_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).revision_parent_choice = revision_parent_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"content_hash", tmp_str.len = sizeof("content_hash") - 1, &tmp_str)))))
	&& (decode_content_hash(state, (&(*result).revision_content_hash))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"author", tmp_str.len = sizeof("author") - 1, &tmp_str)))))
	&& (decode_author(state, (&(*result).revision_author))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"reason", tmp_str.len = sizeof("reason") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).revision_reason))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ts_ms", tmp_str.len = sizeof("ts_ms") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).revision_ts_ms))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_revisions(
		zcbor_state_t *state, struct response_revisions *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Revisions", tmp_str.len = sizeof("Revisions") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_revisions_Revisions_revision_m_count, (zcbor_decoder_t *)decode_revision, state, (*&(*result).response_revisions_Revisions_revision_m), sizeof(struct revision))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_revision(state, (*&(*result).response_revisions_Revisions_revision_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_skill_bundle(
		zcbor_state_t *state, struct response_skill_bundle *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SkillBundle", tmp_str.len = sizeof("SkillBundle") - 1, &tmp_str)))))
	&& (decode_skill_bundle(state, (&(*result).response_skill_bundle_SkillBundle))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_skill_creator(
		zcbor_state_t *state, struct skill_creator_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"agent", tmp_str.len = sizeof("agent") - 1, &tmp_str))))) && (((*result).skill_creator_choice = skill_creator_agent_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"user", tmp_str.len = sizeof("user") - 1, &tmp_str))))) && (((*result).skill_creator_choice = skill_creator_user_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"bundled", tmp_str.len = sizeof("bundled") - 1, &tmp_str))))) && (((*result).skill_creator_choice = skill_creator_bundled_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_skill_state(
		zcbor_state_t *state, struct skill_state_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"active", tmp_str.len = sizeof("active") - 1, &tmp_str))))) && (((*result).skill_state_choice = skill_state_active_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"stale", tmp_str.len = sizeof("stale") - 1, &tmp_str))))) && (((*result).skill_state_choice = skill_state_stale_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"archived", tmp_str.len = sizeof("archived") - 1, &tmp_str))))) && (((*result).skill_state_choice = skill_state_archived_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_skill_usage(
		zcbor_state_t *state, struct skill_usage *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"created_by", tmp_str.len = sizeof("created_by") - 1, &tmp_str)))))
	&& (decode_skill_creator(state, (&(*result).skill_usage_created_by))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (decode_skill_state(state, (&(*result).skill_usage_state))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"pinned", tmp_str.len = sizeof("pinned") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).skill_usage_pinned))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"use_count", tmp_str.len = sizeof("use_count") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).skill_usage_use_count))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"view_count", tmp_str.len = sizeof("view_count") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).skill_usage_view_count))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"patch_count", tmp_str.len = sizeof("patch_count") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).skill_usage_patch_count))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"created_at_ms", tmp_str.len = sizeof("created_at_ms") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).skill_usage_created_at_ms))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"last_used_ms", tmp_str.len = sizeof("last_used_ms") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).skill_usage_last_used_ms_uint64_m)))) && (((*result).skill_usage_last_used_ms_choice = skill_usage_last_used_ms_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).skill_usage_last_used_ms_choice = skill_usage_last_used_ms_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"last_viewed_ms", tmp_str.len = sizeof("last_viewed_ms") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).skill_usage_last_viewed_ms_uint64_m)))) && (((*result).skill_usage_last_viewed_ms_choice = skill_usage_last_viewed_ms_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).skill_usage_last_viewed_ms_choice = skill_usage_last_viewed_ms_null_m_c), true)))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"last_patched_ms", tmp_str.len = sizeof("last_patched_ms") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).skill_usage_last_patched_ms_uint64_m)))) && (((*result).skill_usage_last_patched_ms_choice = skill_usage_last_patched_ms_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).skill_usage_last_patched_ms_choice = skill_usage_last_patched_ms_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_curator_entry(
		zcbor_state_t *state, struct curator_entry *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).curator_entry_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"category", tmp_str.len = sizeof("category") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).curator_entry_category_tstr)))) && (((*result).curator_entry_category_choice = curator_entry_category_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).curator_entry_category_choice = curator_entry_category_null_m_c), true))), zcbor_union_end_code(state), int_res)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"is_bundled", tmp_str.len = sizeof("is_bundled") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).curator_entry_is_bundled))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"usage", tmp_str.len = sizeof("usage") - 1, &tmp_str)))))
	&& (decode_skill_usage(state, (&(*result).curator_entry_usage))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_curator_skills(
		zcbor_state_t *state, struct response_curator_skills *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CuratorSkills", tmp_str.len = sizeof("CuratorSkills") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_curator_skills_CuratorSkills_curator_entry_m_count, (zcbor_decoder_t *)decode_curator_entry, state, (*&(*result).response_curator_skills_CuratorSkills_curator_entry_m), sizeof(struct curator_entry))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_curator_entry(state, (*&(*result).response_curator_skills_CuratorSkills_curator_entry_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_curator_change(
		zcbor_state_t *state, struct curator_change *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).curator_change_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"from", tmp_str.len = sizeof("from") - 1, &tmp_str)))))
	&& (decode_skill_state(state, (&(*result).curator_change_from))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"to", tmp_str.len = sizeof("to") - 1, &tmp_str)))))
	&& (decode_skill_state(state, (&(*result).curator_change_to))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_curator_run(
		zcbor_state_t *state, struct response_curator_run *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CuratorRun", tmp_str.len = sizeof("CuratorRun") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_curator_run_CuratorRun_curator_change_m_count, (zcbor_decoder_t *)decode_curator_change, state, (*&(*result).response_curator_run_CuratorRun_curator_change_m), sizeof(struct curator_change))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_curator_change(state, (*&(*result).response_curator_run_CuratorRun_curator_change_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_auth_flow_kind(
		zcbor_state_t *state, struct auth_flow_kind_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"MatrixSso", tmp_str.len = sizeof("MatrixSso") - 1, &tmp_str))))) && (((*result).auth_flow_kind_choice = auth_flow_kind_MatrixSso_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"OAuth2Pkce", tmp_str.len = sizeof("OAuth2Pkce") - 1, &tmp_str))))) && (((*result).auth_flow_kind_choice = auth_flow_kind_OAuth2Pkce_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_auth_begin_response(
		zcbor_state_t *state, struct auth_begin_response *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"flow_id", tmp_str.len = sizeof("flow_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_begin_response_flow_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"authorization_url", tmp_str.len = sizeof("authorization_url") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_begin_response_authorization_url))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"redirect_uri", tmp_str.len = sizeof("redirect_uri") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_begin_response_redirect_uri))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"expires_at", tmp_str.len = sizeof("expires_at") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).auth_begin_response_expires_at))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"flow_kind", tmp_str.len = sizeof("flow_kind") - 1, &tmp_str)))))
	&& (decode_auth_flow_kind(state, (&(*result).auth_begin_response_flow_kind))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_auth_begun(
		zcbor_state_t *state, struct response_auth_begun *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AuthBegun", tmp_str.len = sizeof("AuthBegun") - 1, &tmp_str)))))
	&& (decode_auth_begin_response(state, (&(*result).response_auth_begun_AuthBegun))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_auth_complete_response(
		zcbor_state_t *state, struct auth_complete_response *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"credential_ref", tmp_str.len = sizeof("credential_ref") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_complete_response_credential_ref))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"account_label", tmp_str.len = sizeof("account_label") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_complete_response_account_label))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport_instance", tmp_str.len = sizeof("transport_instance") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_complete_response_transport_instance))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"bound_profile", tmp_str.len = sizeof("bound_profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).auth_complete_response_bound_profile_tstr)))) && (((*result).auth_complete_response_bound_profile_choice = auth_complete_response_bound_profile_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).auth_complete_response_bound_profile_choice = auth_complete_response_bound_profile_null_m_c), true))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_auth_completed(
		zcbor_state_t *state, struct response_auth_completed *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AuthCompleted", tmp_str.len = sizeof("AuthCompleted") - 1, &tmp_str)))))
	&& (decode_auth_complete_response(state, (&(*result).response_auth_completed_AuthCompleted))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_auth_provider_info(
		zcbor_state_t *state, struct auth_provider_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"family", tmp_str.len = sizeof("family") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_provider_info_family))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"flow_kind", tmp_str.len = sizeof("flow_kind") - 1, &tmp_str)))))
	&& (decode_auth_flow_kind(state, (&(*result).auth_provider_info_flow_kind))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"display_name", tmp_str.len = sizeof("display_name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).auth_provider_info_display_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"params_schema", tmp_str.len = sizeof("params_schema") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).auth_provider_info_params_schema_auth_param_field_m_count, (zcbor_decoder_t *)decode_auth_param_field, state, (*&(*result).auth_provider_info_params_schema_auth_param_field_m), sizeof(struct auth_param_field))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_auth_param_field(state, (*&(*result).auth_provider_info_params_schema_auth_param_field_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_auth_providers(
		zcbor_state_t *state, struct response_auth_providers *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AuthProviders", tmp_str.len = sizeof("AuthProviders") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_auth_providers_AuthProviders_auth_provider_info_m_count, (zcbor_decoder_t *)decode_auth_provider_info, state, (*&(*result).response_auth_providers_AuthProviders_auth_provider_info_m), sizeof(struct auth_provider_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_auth_provider_info(state, (*&(*result).response_auth_providers_AuthProviders_auth_provider_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_checkpoint_info_turn_ordinal(
		zcbor_state_t *state, struct checkpoint_info_turn_ordinal_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"turn_ordinal", tmp_str.len = sizeof("turn_ordinal") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).checkpoint_info_turn_ordinal_uint64_m)))) && (((*result).checkpoint_info_turn_ordinal_choice = checkpoint_info_turn_ordinal_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).checkpoint_info_turn_ordinal_choice = checkpoint_info_turn_ordinal_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_checkpoint_info_cursor(
		zcbor_state_t *state, struct checkpoint_info_cursor_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"cursor", tmp_str.len = sizeof("cursor") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).checkpoint_info_cursor_uint64_m)))) && (((*result).checkpoint_info_cursor_choice = checkpoint_info_cursor_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).checkpoint_info_cursor_choice = checkpoint_info_cursor_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_checkpoint_info(
		zcbor_state_t *state, struct checkpoint_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).checkpoint_info_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).checkpoint_info_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"tool", tmp_str.len = sizeof("tool") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).checkpoint_info_tool))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"created_unix", tmp_str.len = sizeof("created_unix") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).checkpoint_info_created_unix))))
	&& zcbor_present_decode(&((*result).checkpoint_info_turn_ordinal_present), (zcbor_decoder_t *)decode_repeated_checkpoint_info_turn_ordinal, state, (&(*result).checkpoint_info_turn_ordinal))
	&& zcbor_present_decode(&((*result).checkpoint_info_cursor_present), (zcbor_decoder_t *)decode_repeated_checkpoint_info_cursor, state, (&(*result).checkpoint_info_cursor))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_checkpoint_info_turn_ordinal(state, (&(*result).checkpoint_info_turn_ordinal));
		decode_repeated_checkpoint_info_cursor(state, (&(*result).checkpoint_info_cursor));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_checkpoints(
		zcbor_state_t *state, struct response_checkpoints *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Checkpoints", tmp_str.len = sizeof("Checkpoints") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_checkpoints_Checkpoints_checkpoint_info_m_count, (zcbor_decoder_t *)decode_checkpoint_info, state, (*&(*result).response_checkpoints_Checkpoints_checkpoint_info_m), sizeof(struct checkpoint_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_checkpoint_info(state, (*&(*result).response_checkpoints_Checkpoints_checkpoint_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_page_next_cursor(
		zcbor_state_t *state, struct session_page_next_cursor_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"next_cursor", tmp_str.len = sizeof("next_cursor") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).session_page_next_cursor_session_id_m)))) && (((*result).session_page_next_cursor_choice = session_page_next_cursor_session_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).session_page_next_cursor_choice = session_page_next_cursor_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_page_removed(
		zcbor_state_t *state, struct session_page_removed_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"removed", tmp_str.len = sizeof("removed") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).session_page_removed_session_id_m_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).session_page_removed_session_id_m), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).session_page_removed_session_id_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_page(
		zcbor_state_t *state, struct session_page *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"sessions", tmp_str.len = sizeof("sessions") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).session_page_sessions_session_info_m_count, (zcbor_decoder_t *)decode_session_info, state, (*&(*result).session_page_sessions_session_info_m), sizeof(struct session_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& zcbor_present_decode(&((*result).session_page_next_cursor_present), (zcbor_decoder_t *)decode_repeated_session_page_next_cursor, state, (&(*result).session_page_next_cursor))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"rev", tmp_str.len = sizeof("rev") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).session_page_rev))))
	&& zcbor_present_decode(&((*result).session_page_removed_present), (zcbor_decoder_t *)decode_repeated_session_page_removed, state, (&(*result).session_page_removed))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_session_info(state, (*&(*result).session_page_sessions_session_info_m));
		decode_repeated_session_page_next_cursor(state, (&(*result).session_page_next_cursor));
		decode_repeated_session_page_removed(state, (&(*result).session_page_removed));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_session_page(
		zcbor_state_t *state, struct response_session_page *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionPage", tmp_str.len = sizeof("SessionPage") - 1, &tmp_str)))))
	&& (decode_session_page(state, (&(*result).response_session_page_SessionPage))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_detail_overlay(
		zcbor_state_t *state, struct session_detail_overlay_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"overlay", tmp_str.len = sizeof("overlay") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_session_overlay(state, (&(*result).session_detail_overlay_session_overlay_m)))) && (((*result).session_detail_overlay_choice = session_detail_overlay_session_overlay_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).session_detail_overlay_choice = session_detail_overlay_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_detail_model(
		zcbor_state_t *state, struct session_detail_model_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).session_detail_model_tstr)))) && (((*result).session_detail_model_choice = session_detail_model_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).session_detail_model_choice = session_detail_model_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_detail_delivery_targets(
		zcbor_state_t *state, struct session_detail_delivery_targets_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"delivery_targets", tmp_str.len = sizeof("delivery_targets") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).session_detail_delivery_targets_delivery_target_m_count, (zcbor_decoder_t *)decode_delivery_target, state, (*&(*result).session_detail_delivery_targets_delivery_target_m), sizeof(struct delivery_target))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_delivery_target(state, (*&(*result).session_detail_delivery_targets_delivery_target_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_detail_children(
		zcbor_state_t *state, struct session_detail_children_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"children", tmp_str.len = sizeof("children") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).session_detail_children_session_id_m_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).session_detail_children_session_id_m), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).session_detail_children_session_id_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_session_detail_checkpoints(
		zcbor_state_t *state, struct session_detail_checkpoints *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"checkpoints", tmp_str.len = sizeof("checkpoints") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).session_detail_checkpoints)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_detail(
		zcbor_state_t *state, struct session_detail *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"info", tmp_str.len = sizeof("info") - 1, &tmp_str)))))
	&& (decode_session_info(state, (&(*result).session_detail_info))))
	&& zcbor_present_decode(&((*result).session_detail_overlay_present), (zcbor_decoder_t *)decode_repeated_session_detail_overlay, state, (&(*result).session_detail_overlay))
	&& zcbor_present_decode(&((*result).session_detail_model_present), (zcbor_decoder_t *)decode_repeated_session_detail_model, state, (&(*result).session_detail_model))
	&& zcbor_present_decode(&((*result).session_detail_delivery_targets_present), (zcbor_decoder_t *)decode_repeated_session_detail_delivery_targets, state, (&(*result).session_detail_delivery_targets))
	&& zcbor_present_decode(&((*result).session_detail_children_present), (zcbor_decoder_t *)decode_repeated_session_detail_children, state, (&(*result).session_detail_children))
	&& zcbor_present_decode(&((*result).session_detail_checkpoints_present), (zcbor_decoder_t *)decode_repeated_session_detail_checkpoints, state, (&(*result).session_detail_checkpoints))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_session_detail_overlay(state, (&(*result).session_detail_overlay));
		decode_repeated_session_detail_model(state, (&(*result).session_detail_model));
		decode_repeated_session_detail_delivery_targets(state, (&(*result).session_detail_delivery_targets));
		decode_repeated_session_detail_children(state, (&(*result).session_detail_children));
		decode_repeated_session_detail_checkpoints(state, (&(*result).session_detail_checkpoints));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_session_detail(
		zcbor_state_t *state, struct response_session_detail *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionDetail", tmp_str.len = sizeof("SessionDetail") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_session_detail(state, (&(*result).response_session_detail_SessionDetail_session_detail_m)))) && (((*result).response_session_detail_SessionDetail_choice = response_session_detail_SessionDetail_session_detail_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).response_session_detail_SessionDetail_choice = response_session_detail_SessionDetail_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_SessionsByProfile_profile_l(
		zcbor_state_t *state, struct SessionsByProfile_profile_l *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_decode(state) && ((((zcbor_tstr_decode(state, (&(*result).SessionsByProfile_profile_l_profile))))
	&& ((zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).profile_l_sessions_session_info_m_count, (zcbor_decoder_t *)decode_session_info, state, (*&(*result).profile_l_sessions_session_info_m), sizeof(struct session_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_session_info(state, (*&(*result).profile_l_sessions_session_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_sessions_by_profile(
		zcbor_state_t *state, struct response_sessions_by_profile *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionsByProfile", tmp_str.len = sizeof("SessionsByProfile") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).SessionsByProfile_profile_l_count, (zcbor_decoder_t *)decode_repeated_SessionsByProfile_profile_l, state, (*&(*result).SessionsByProfile_profile_l), sizeof(struct SessionsByProfile_profile_l))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_SessionsByProfile_profile_l(state, (*&(*result).SessionsByProfile_profile_l));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_search_hit(
		zcbor_state_t *state, struct session_search_hit *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).session_search_hit_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).session_search_hit_title))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"snippet", tmp_str.len = sizeof("snippet") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).session_search_hit_snippet))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_session_search(
		zcbor_state_t *state, struct response_session_search *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionSearch", tmp_str.len = sizeof("SessionSearch") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_session_search_SessionSearch_session_search_hit_m_count, (zcbor_decoder_t *)decode_session_search_hit, state, (*&(*result).response_session_search_SessionSearch_session_search_hit_m), sizeof(struct session_search_hit))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_session_search_hit(state, (*&(*result).response_session_search_SessionSearch_session_search_hit_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_acp_catalog(
		zcbor_state_t *state, struct response_acp_catalog *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AcpCatalog", tmp_str.len = sizeof("AcpCatalog") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_acp_catalog_AcpCatalog_acp_agent_entry_m_count, (zcbor_decoder_t *)decode_acp_agent_entry, state, (*&(*result).response_acp_catalog_AcpCatalog_acp_agent_entry_m), sizeof(struct acp_agent_entry))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_acp_agent_entry(state, (*&(*result).response_acp_catalog_AcpCatalog_acp_agent_entry_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_providers(
		zcbor_state_t *state, struct response_providers *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Providers", tmp_str.len = sizeof("Providers") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_providers_Providers_provider_info_m_count, (zcbor_decoder_t *)decode_provider_info, state, (*&(*result).response_providers_Providers_provider_info_m), sizeof(struct provider_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_provider_info(state, (*&(*result).response_providers_Providers_provider_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_tools(
		zcbor_state_t *state, struct response_tools *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Tools", tmp_str.len = sizeof("Tools") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_tools_Tools_tool_info_m_count, (zcbor_decoder_t *)decode_tool_info, state, (*&(*result).response_tools_Tools_tool_info_m), sizeof(struct tool_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_tool_info(state, (*&(*result).response_tools_Tools_tool_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_command_scope(
		zcbor_state_t *state, struct command_scope_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Session", tmp_str.len = sizeof("Session") - 1, &tmp_str))))) && (((*result).command_scope_choice = command_scope_Session_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Node", tmp_str.len = sizeof("Node") - 1, &tmp_str))))) && (((*result).command_scope_choice = command_scope_Node_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_command_surface(
		zcbor_state_t *state, struct command_surface_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Cli", tmp_str.len = sizeof("Cli") - 1, &tmp_str))))) && (((*result).command_surface_choice = command_surface_Cli_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Gui", tmp_str.len = sizeof("Gui") - 1, &tmp_str))))) && (((*result).command_surface_choice = command_surface_Gui_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Chat", tmp_str.len = sizeof("Chat") - 1, &tmp_str))))) && (((*result).command_surface_choice = command_surface_Chat_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_command_access(
		zcbor_state_t *state, struct command_access_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"User", tmp_str.len = sizeof("User") - 1, &tmp_str))))) && (((*result).command_access_choice = command_access_User_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Admin", tmp_str.len = sizeof("Admin") - 1, &tmp_str))))) && (((*result).command_access_choice = command_access_Admin_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_command_spec(
		zcbor_state_t *state, struct command_spec *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).command_spec_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"aliases", tmp_str.len = sizeof("aliases") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).command_spec_aliases_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).command_spec_aliases_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"summary", tmp_str.len = sizeof("summary") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).command_spec_summary))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"category", tmp_str.len = sizeof("category") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).command_spec_category))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"args_hint", tmp_str.len = sizeof("args_hint") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).command_spec_args_hint))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"subcommands", tmp_str.len = sizeof("subcommands") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).command_spec_subcommands_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).command_spec_subcommands_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"scope", tmp_str.len = sizeof("scope") - 1, &tmp_str)))))
	&& (decode_command_scope(state, (&(*result).command_spec_scope))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"surfaces", tmp_str.len = sizeof("surfaces") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).command_spec_surfaces_command_surface_m_count, (zcbor_decoder_t *)decode_command_surface, state, (*&(*result).command_spec_surfaces_command_surface_m), sizeof(struct command_surface_r))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"side_effecting", tmp_str.len = sizeof("side_effecting") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).command_spec_side_effecting))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"confirm", tmp_str.len = sizeof("confirm") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).command_spec_confirm))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"min_access", tmp_str.len = sizeof("min_access") - 1, &tmp_str)))))
	&& (decode_command_access(state, (&(*result).command_spec_min_access))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).command_spec_source))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).command_spec_aliases_tstr));
		zcbor_tstr_decode(state, (*&(*result).command_spec_subcommands_tstr));
		decode_command_surface(state, (*&(*result).command_spec_surfaces_command_surface_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_commands(
		zcbor_state_t *state, struct response_commands *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Commands", tmp_str.len = sizeof("Commands") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_commands_Commands_command_spec_m_count, (zcbor_decoder_t *)decode_command_spec, state, (*&(*result).response_commands_Commands_command_spec_m), sizeof(struct command_spec))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_command_spec(state, (*&(*result).response_commands_Commands_command_spec_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_command_output(
		zcbor_state_t *state, struct command_output *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).command_output_text))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ephemeral", tmp_str.len = sizeof("ephemeral") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).command_output_ephemeral))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_command_output(
		zcbor_state_t *state, struct response_command_output *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CommandOutput", tmp_str.len = sizeof("CommandOutput") - 1, &tmp_str)))))
	&& (decode_command_output(state, (&(*result).response_command_output_CommandOutput))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_config(
		zcbor_state_t *state, struct response_config *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Config", tmp_str.len = sizeof("Config") - 1, &tmp_str)))))
	&& (decode_node_config_view(state, (&(*result).response_config_Config))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_job_next_fire_unix(
		zcbor_state_t *state, struct cron_job_next_fire_unix_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"next_fire_unix", tmp_str.len = sizeof("next_fire_unix") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).cron_job_next_fire_unix_uint64_m)))) && (((*result).cron_job_next_fire_unix_choice = cron_job_next_fire_unix_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).cron_job_next_fire_unix_choice = cron_job_next_fire_unix_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_job_paused(
		zcbor_state_t *state, struct cron_job_paused *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"paused", tmp_str.len = sizeof("paused") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).cron_job_paused)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_job_last_run_unix(
		zcbor_state_t *state, struct cron_job_last_run_unix_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"last_run_unix", tmp_str.len = sizeof("last_run_unix") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).cron_job_last_run_unix_uint64_m)))) && (((*result).cron_job_last_run_unix_choice = cron_job_last_run_unix_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).cron_job_last_run_unix_choice = cron_job_last_run_unix_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_job_last_ok(
		zcbor_state_t *state, struct cron_job_last_ok_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"last_ok", tmp_str.len = sizeof("last_ok") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_bool_decode(state, (&(*result).cron_job_last_ok_bool)))) && (((*result).cron_job_last_ok_choice = cron_job_last_ok_bool_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_job_last_ok_choice = cron_job_last_ok_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_job_last_detail(
		zcbor_state_t *state, struct cron_job_last_detail_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"last_detail", tmp_str.len = sizeof("last_detail") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).cron_job_last_detail_tstr)))) && (((*result).cron_job_last_detail_choice = cron_job_last_detail_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_job_last_detail_choice = cron_job_last_detail_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_job_fire_count(
		zcbor_state_t *state, struct cron_job_fire_count *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"fire_count", tmp_str.len = sizeof("fire_count") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).cron_job_fire_count)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_cron_job(
		zcbor_state_t *state, struct cron_job *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).cron_job_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (decode_cron_spec(state, (&(*result).cron_job_spec))))
	&& zcbor_present_decode(&((*result).cron_job_next_fire_unix_present), (zcbor_decoder_t *)decode_repeated_cron_job_next_fire_unix, state, (&(*result).cron_job_next_fire_unix))
	&& zcbor_present_decode(&((*result).cron_job_paused_present), (zcbor_decoder_t *)decode_repeated_cron_job_paused, state, (&(*result).cron_job_paused))
	&& zcbor_present_decode(&((*result).cron_job_last_run_unix_present), (zcbor_decoder_t *)decode_repeated_cron_job_last_run_unix, state, (&(*result).cron_job_last_run_unix))
	&& zcbor_present_decode(&((*result).cron_job_last_ok_present), (zcbor_decoder_t *)decode_repeated_cron_job_last_ok, state, (&(*result).cron_job_last_ok))
	&& zcbor_present_decode(&((*result).cron_job_last_detail_present), (zcbor_decoder_t *)decode_repeated_cron_job_last_detail, state, (&(*result).cron_job_last_detail))
	&& zcbor_present_decode(&((*result).cron_job_fire_count_present), (zcbor_decoder_t *)decode_repeated_cron_job_fire_count, state, (&(*result).cron_job_fire_count))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_cron_job_next_fire_unix(state, (&(*result).cron_job_next_fire_unix));
		decode_repeated_cron_job_paused(state, (&(*result).cron_job_paused));
		decode_repeated_cron_job_last_run_unix(state, (&(*result).cron_job_last_run_unix));
		decode_repeated_cron_job_last_ok(state, (&(*result).cron_job_last_ok));
		decode_repeated_cron_job_last_detail(state, (&(*result).cron_job_last_detail));
		decode_repeated_cron_job_fire_count(state, (&(*result).cron_job_fire_count));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_cron_jobs(
		zcbor_state_t *state, struct response_cron_jobs *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronJobs", tmp_str.len = sizeof("CronJobs") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_cron_jobs_CronJobs_cron_job_m_count, (zcbor_decoder_t *)decode_cron_job, state, (*&(*result).response_cron_jobs_CronJobs_cron_job_m), sizeof(struct cron_job))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_cron_job(state, (*&(*result).response_cron_jobs_CronJobs_cron_job_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_cron_id(
		zcbor_state_t *state, struct response_cron_id *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronId", tmp_str.len = sizeof("CronId") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).response_cron_id_CronId))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_run_detail(
		zcbor_state_t *state, struct cron_run_detail_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).cron_run_detail_tstr)))) && (((*result).cron_run_detail_choice = cron_run_detail_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_run_detail_choice = cron_run_detail_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_run_finished_unix(
		zcbor_state_t *state, struct cron_run_finished_unix_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"finished_unix", tmp_str.len = sizeof("finished_unix") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_uint64_decode(state, (&(*result).cron_run_finished_unix_uint64_m)))) && (((*result).cron_run_finished_unix_choice = cron_run_finished_unix_uint64_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).cron_run_finished_unix_choice = cron_run_finished_unix_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_run_session(
		zcbor_state_t *state, struct cron_run_session_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).cron_run_session_session_id_m)))) && (((*result).cron_run_session_choice = cron_run_session_session_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).cron_run_session_choice = cron_run_session_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_run_trigger(
		zcbor_state_t *state, struct run_trigger_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Scheduled", tmp_str.len = sizeof("Scheduled") - 1, &tmp_str))))) && (((*result).run_trigger_choice = run_trigger_Scheduled_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Manual", tmp_str.len = sizeof("Manual") - 1, &tmp_str))))) && (((*result).run_trigger_choice = run_trigger_Manual_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_run_trigger(
		zcbor_state_t *state, struct cron_run_trigger *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"trigger", tmp_str.len = sizeof("trigger") - 1, &tmp_str)))))
	&& (decode_run_trigger(state, (&(*result).cron_run_trigger)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_cron_run(
		zcbor_state_t *state, struct cron_run *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"started_unix", tmp_str.len = sizeof("started_unix") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).cron_run_started_unix))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ok", tmp_str.len = sizeof("ok") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).cron_run_ok))))
	&& zcbor_present_decode(&((*result).cron_run_detail_present), (zcbor_decoder_t *)decode_repeated_cron_run_detail, state, (&(*result).cron_run_detail))
	&& zcbor_present_decode(&((*result).cron_run_finished_unix_present), (zcbor_decoder_t *)decode_repeated_cron_run_finished_unix, state, (&(*result).cron_run_finished_unix))
	&& zcbor_present_decode(&((*result).cron_run_session_present), (zcbor_decoder_t *)decode_repeated_cron_run_session, state, (&(*result).cron_run_session))
	&& zcbor_present_decode(&((*result).cron_run_trigger_present), (zcbor_decoder_t *)decode_repeated_cron_run_trigger, state, (&(*result).cron_run_trigger))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_cron_run_detail(state, (&(*result).cron_run_detail));
		decode_repeated_cron_run_finished_unix(state, (&(*result).cron_run_finished_unix));
		decode_repeated_cron_run_session(state, (&(*result).cron_run_session));
		decode_repeated_cron_run_trigger(state, (&(*result).cron_run_trigger));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_cron_runs(
		zcbor_state_t *state, struct response_cron_runs *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronRuns", tmp_str.len = sizeof("CronRuns") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_cron_runs_CronRuns_cron_run_m_count, (zcbor_decoder_t *)decode_cron_run, state, (*&(*result).response_cron_runs_CronRuns_cron_run_m), sizeof(struct cron_run))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_cron_run(state, (*&(*result).response_cron_runs_CronRuns_cron_run_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_suggestion_description(
		zcbor_state_t *state, struct cron_suggestion_description *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"description", tmp_str.len = sizeof("description") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).cron_suggestion_description)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_suggestion_source(
		zcbor_state_t *state, struct cron_suggestion_source *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).cron_suggestion_source)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_suggestion_status(
		zcbor_state_t *state, struct suggestion_status_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Pending", tmp_str.len = sizeof("Pending") - 1, &tmp_str))))) && (((*result).suggestion_status_choice = suggestion_status_Pending_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Accepted", tmp_str.len = sizeof("Accepted") - 1, &tmp_str))))) && (((*result).suggestion_status_choice = suggestion_status_Accepted_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Dismissed", tmp_str.len = sizeof("Dismissed") - 1, &tmp_str))))) && (((*result).suggestion_status_choice = suggestion_status_Dismissed_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_cron_suggestion_status(
		zcbor_state_t *state, struct cron_suggestion_status *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"status", tmp_str.len = sizeof("status") - 1, &tmp_str)))))
	&& (decode_suggestion_status(state, (&(*result).cron_suggestion_status)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_cron_suggestion(
		zcbor_state_t *state, struct cron_suggestion *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).cron_suggestion_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).cron_suggestion_title))))
	&& zcbor_present_decode(&((*result).cron_suggestion_description_present), (zcbor_decoder_t *)decode_repeated_cron_suggestion_description, state, (&(*result).cron_suggestion_description))
	&& zcbor_present_decode(&((*result).cron_suggestion_source_present), (zcbor_decoder_t *)decode_repeated_cron_suggestion_source, state, (&(*result).cron_suggestion_source))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (decode_cron_spec(state, (&(*result).cron_suggestion_spec))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"dedup_key", tmp_str.len = sizeof("dedup_key") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).cron_suggestion_dedup_key))))
	&& zcbor_present_decode(&((*result).cron_suggestion_status_present), (zcbor_decoder_t *)decode_repeated_cron_suggestion_status, state, (&(*result).cron_suggestion_status))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_cron_suggestion_description(state, (&(*result).cron_suggestion_description));
		decode_repeated_cron_suggestion_source(state, (&(*result).cron_suggestion_source));
		decode_repeated_cron_suggestion_status(state, (&(*result).cron_suggestion_status));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_cron_suggestions(
		zcbor_state_t *state, struct response_cron_suggestions *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronSuggestions", tmp_str.len = sizeof("CronSuggestions") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_cron_suggestions_CronSuggestions_cron_suggestion_m_count, (zcbor_decoder_t *)decode_cron_suggestion, state, (*&(*result).response_cron_suggestions_CronSuggestions_cron_suggestion_m), sizeof(struct cron_suggestion))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_cron_suggestion(state, (*&(*result).response_cron_suggestions_CronSuggestions_cron_suggestion_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_chat_routes(
		zcbor_state_t *state, struct response_chat_routes *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ChatRoutes", tmp_str.len = sizeof("ChatRoutes") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_chat_routes_ChatRoutes_chat_route_m_count, (zcbor_decoder_t *)decode_chat_route, state, (*&(*result).response_chat_routes_ChatRoutes_chat_route_m), sizeof(struct chat_route))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_chat_route(state, (*&(*result).response_chat_routes_ChatRoutes_chat_route_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_chat_route(
		zcbor_state_t *state, struct response_chat_route *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ChatRoute", tmp_str.len = sizeof("ChatRoute") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_chat_route(state, (&(*result).response_chat_route_ChatRoute_chat_route_m)))) && (((*result).response_chat_route_ChatRoute_choice = response_chat_route_ChatRoute_chat_route_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).response_chat_route_ChatRoute_choice = response_chat_route_ChatRoute_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_room_info_name(
		zcbor_state_t *state, struct room_info_name_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).room_info_name_tstr)))) && (((*result).room_info_name_choice = room_info_name_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).room_info_name_choice = room_info_name_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_room_info_session(
		zcbor_state_t *state, struct room_info_session_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).room_info_session_session_id_m)))) && (((*result).room_info_session_choice = room_info_session_session_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).room_info_session_choice = room_info_session_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_room_info(
		zcbor_state_t *state, struct room_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).room_info_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"room", tmp_str.len = sizeof("room") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).room_info_room))))
	&& zcbor_present_decode(&((*result).room_info_name_present), (zcbor_decoder_t *)decode_repeated_room_info_name, state, (&(*result).room_info_name))
	&& zcbor_present_decode(&((*result).room_info_session_present), (zcbor_decoder_t *)decode_repeated_room_info_session, state, (&(*result).room_info_session))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_room_info_name(state, (&(*result).room_info_name));
		decode_repeated_room_info_session(state, (&(*result).room_info_session));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_rooms(
		zcbor_state_t *state, struct response_rooms *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Rooms", tmp_str.len = sizeof("Rooms") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_rooms_Rooms_room_info_m_count, (zcbor_decoder_t *)decode_room_info, state, (*&(*result).response_rooms_Rooms_room_info_m), sizeof(struct room_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_room_info(state, (*&(*result).response_rooms_Rooms_room_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_adapter_capabilities(
		zcbor_state_t *state, struct adapter_capabilities *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"rooms", tmp_str.len = sizeof("rooms") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).adapter_capabilities_rooms))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"direct_messages", tmp_str.len = sizeof("direct_messages") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).adapter_capabilities_direct_messages))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"presence", tmp_str.len = sizeof("presence") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).adapter_capabilities_presence))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"room_enumeration", tmp_str.len = sizeof("room_enumeration") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).adapter_capabilities_room_enumeration))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"file_transfer", tmp_str.len = sizeof("file_transfer") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).adapter_capabilities_file_transfer))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"interactive_auth", tmp_str.len = sizeof("interactive_auth") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).adapter_capabilities_interactive_auth))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_adapter_info_account_schema(
		zcbor_state_t *state, struct adapter_info_account_schema *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"account_schema", tmp_str.len = sizeof("account_schema") - 1, &tmp_str)))))
	&& (decode_account_settings_schema(state, (&(*result).adapter_info_account_schema)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_adapter_info(
		zcbor_state_t *state, struct adapter_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"family", tmp_str.len = sizeof("family") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).adapter_info_family))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"display_name", tmp_str.len = sizeof("display_name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).adapter_info_display_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"capabilities", tmp_str.len = sizeof("capabilities") - 1, &tmp_str)))))
	&& (decode_adapter_capabilities(state, (&(*result).adapter_info_capabilities))))
	&& zcbor_present_decode(&((*result).adapter_info_account_schema_present), (zcbor_decoder_t *)decode_repeated_adapter_info_account_schema, state, (&(*result).adapter_info_account_schema))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_adapter_info_account_schema(state, (&(*result).adapter_info_account_schema));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_adapters(
		zcbor_state_t *state, struct response_adapters *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Adapters", tmp_str.len = sizeof("Adapters") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_adapters_Adapters_adapter_info_m_count, (zcbor_decoder_t *)decode_adapter_info, state, (*&(*result).response_adapters_Adapters_adapter_info_m), sizeof(struct adapter_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_adapter_info(state, (*&(*result).response_adapters_Adapters_adapter_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_connection_state(
		zcbor_state_t *state, struct connection_state_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Offline", tmp_str.len = sizeof("Offline") - 1, &tmp_str))))) && (((*result).connection_state_choice = connection_state_Offline_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Connecting", tmp_str.len = sizeof("Connecting") - 1, &tmp_str))))) && (((*result).connection_state_choice = connection_state_Connecting_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Connected", tmp_str.len = sizeof("Connected") - 1, &tmp_str))))) && (((*result).connection_state_choice = connection_state_Connected_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Error", tmp_str.len = sizeof("Error") - 1, &tmp_str))))) && (((*result).connection_state_choice = connection_state_Error_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_transport_instance_info_connection(
		zcbor_state_t *state, struct transport_instance_info_connection *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"connection", tmp_str.len = sizeof("connection") - 1, &tmp_str)))))
	&& (decode_connection_state(state, (&(*result).transport_instance_info_connection)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_presence_state(
		zcbor_state_t *state, struct presence_state_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Unknown", tmp_str.len = sizeof("Unknown") - 1, &tmp_str))))) && (((*result).presence_state_choice = presence_state_Unknown_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Offline", tmp_str.len = sizeof("Offline") - 1, &tmp_str))))) && (((*result).presence_state_choice = presence_state_Offline_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Available", tmp_str.len = sizeof("Available") - 1, &tmp_str))))) && (((*result).presence_state_choice = presence_state_Available_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Idle", tmp_str.len = sizeof("Idle") - 1, &tmp_str))))) && (((*result).presence_state_choice = presence_state_Idle_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Away", tmp_str.len = sizeof("Away") - 1, &tmp_str))))) && (((*result).presence_state_choice = presence_state_Away_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Busy", tmp_str.len = sizeof("Busy") - 1, &tmp_str))))) && (((*result).presence_state_choice = presence_state_Busy_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_transport_instance_info_presence(
		zcbor_state_t *state, struct transport_instance_info_presence *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"presence", tmp_str.len = sizeof("presence") - 1, &tmp_str)))))
	&& (decode_presence_state(state, (&(*result).transport_instance_info_presence)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_transport_instance_info_bound_profile(
		zcbor_state_t *state, struct transport_instance_info_bound_profile_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"bound_profile", tmp_str.len = sizeof("bound_profile") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).transport_instance_info_bound_profile_profile_ref_m)))) && (((*result).transport_instance_info_bound_profile_choice = transport_instance_info_bound_profile_profile_ref_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).transport_instance_info_bound_profile_choice = transport_instance_info_bound_profile_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_transport_instance_info(
		zcbor_state_t *state, struct transport_instance_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).transport_instance_info_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"family", tmp_str.len = sizeof("family") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).transport_instance_info_family))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"display_name", tmp_str.len = sizeof("display_name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).transport_instance_info_display_name))))
	&& zcbor_present_decode(&((*result).transport_instance_info_connection_present), (zcbor_decoder_t *)decode_repeated_transport_instance_info_connection, state, (&(*result).transport_instance_info_connection))
	&& zcbor_present_decode(&((*result).transport_instance_info_presence_present), (zcbor_decoder_t *)decode_repeated_transport_instance_info_presence, state, (&(*result).transport_instance_info_presence))
	&& zcbor_present_decode(&((*result).transport_instance_info_bound_profile_present), (zcbor_decoder_t *)decode_repeated_transport_instance_info_bound_profile, state, (&(*result).transport_instance_info_bound_profile))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_transport_instance_info_connection(state, (&(*result).transport_instance_info_connection));
		decode_repeated_transport_instance_info_presence(state, (&(*result).transport_instance_info_presence));
		decode_repeated_transport_instance_info_bound_profile(state, (&(*result).transport_instance_info_bound_profile));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_transport_instances(
		zcbor_state_t *state, struct response_transport_instances *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"TransportInstances", tmp_str.len = sizeof("TransportInstances") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_transport_instances_TransportInstances_transport_instance_info_m_count, (zcbor_decoder_t *)decode_transport_instance_info, state, (*&(*result).response_transport_instances_TransportInstances_transport_instance_info_m), sizeof(struct transport_instance_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_transport_instance_info(state, (*&(*result).response_transport_instances_TransportInstances_transport_instance_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_conversation_type(
		zcbor_state_t *state, struct conversation_type_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Unset", tmp_str.len = sizeof("Unset") - 1, &tmp_str))))) && (((*result).conversation_type_choice = conversation_type_Unset_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Dm", tmp_str.len = sizeof("Dm") - 1, &tmp_str))))) && (((*result).conversation_type_choice = conversation_type_Dm_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"GroupDm", tmp_str.len = sizeof("GroupDm") - 1, &tmp_str))))) && (((*result).conversation_type_choice = conversation_type_GroupDm_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Channel", tmp_str.len = sizeof("Channel") - 1, &tmp_str))))) && (((*result).conversation_type_choice = conversation_type_Channel_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Thread", tmp_str.len = sizeof("Thread") - 1, &tmp_str))))) && (((*result).conversation_type_choice = conversation_type_Thread_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_conversation_info_title(
		zcbor_state_t *state, struct conversation_info_title_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).conversation_info_title_tstr)))) && (((*result).conversation_info_title_choice = conversation_info_title_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).conversation_info_title_choice = conversation_info_title_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_conversation_info_topic(
		zcbor_state_t *state, struct conversation_info_topic_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"topic", tmp_str.len = sizeof("topic") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).conversation_info_topic_tstr)))) && (((*result).conversation_info_topic_choice = conversation_info_topic_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).conversation_info_topic_choice = conversation_info_topic_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_conversation_info_description(
		zcbor_state_t *state, struct conversation_info_description_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"description", tmp_str.len = sizeof("description") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).conversation_info_description_tstr)))) && (((*result).conversation_info_description_choice = conversation_info_description_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).conversation_info_description_choice = conversation_info_description_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_conversation_member_alias(
		zcbor_state_t *state, struct conversation_member_alias_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"alias", tmp_str.len = sizeof("alias") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).conversation_member_alias_tstr)))) && (((*result).conversation_member_alias_choice = conversation_member_alias_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).conversation_member_alias_choice = conversation_member_alias_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_conversation_member_nickname(
		zcbor_state_t *state, struct conversation_member_nickname_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"nickname", tmp_str.len = sizeof("nickname") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).conversation_member_nickname_tstr)))) && (((*result).conversation_member_nickname_choice = conversation_member_nickname_tstr_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).conversation_member_nickname_choice = conversation_member_nickname_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_typing_state(
		zcbor_state_t *state, struct typing_state_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"None", tmp_str.len = sizeof("None") - 1, &tmp_str))))) && (((*result).typing_state_choice = typing_state_None_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Typing", tmp_str.len = sizeof("Typing") - 1, &tmp_str))))) && (((*result).typing_state_choice = typing_state_Typing_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Paused", tmp_str.len = sizeof("Paused") - 1, &tmp_str))))) && (((*result).typing_state_choice = typing_state_Paused_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_conversation_member_typing(
		zcbor_state_t *state, struct conversation_member_typing *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"typing", tmp_str.len = sizeof("typing") - 1, &tmp_str)))))
	&& (decode_typing_state(state, (&(*result).conversation_member_typing)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_conversation_member_role(
		zcbor_state_t *state, struct conversation_member_role *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (decode_member_role(state, (&(*result).conversation_member_role)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_conversation_member_session(
		zcbor_state_t *state, struct conversation_member_session_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).conversation_member_session_session_id_m)))) && (((*result).conversation_member_session_choice = conversation_member_session_session_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).conversation_member_session_choice = conversation_member_session_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_conversation_member(
		zcbor_state_t *state, struct conversation_member *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"contact", tmp_str.len = sizeof("contact") - 1, &tmp_str)))))
	&& (decode_contact_info(state, (&(*result).conversation_member_contact))))
	&& zcbor_present_decode(&((*result).conversation_member_alias_present), (zcbor_decoder_t *)decode_repeated_conversation_member_alias, state, (&(*result).conversation_member_alias))
	&& zcbor_present_decode(&((*result).conversation_member_nickname_present), (zcbor_decoder_t *)decode_repeated_conversation_member_nickname, state, (&(*result).conversation_member_nickname))
	&& zcbor_present_decode(&((*result).conversation_member_typing_present), (zcbor_decoder_t *)decode_repeated_conversation_member_typing, state, (&(*result).conversation_member_typing))
	&& zcbor_present_decode(&((*result).conversation_member_role_present), (zcbor_decoder_t *)decode_repeated_conversation_member_role, state, (&(*result).conversation_member_role))
	&& zcbor_present_decode(&((*result).conversation_member_session_present), (zcbor_decoder_t *)decode_repeated_conversation_member_session, state, (&(*result).conversation_member_session))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_conversation_member_alias(state, (&(*result).conversation_member_alias));
		decode_repeated_conversation_member_nickname(state, (&(*result).conversation_member_nickname));
		decode_repeated_conversation_member_typing(state, (&(*result).conversation_member_typing));
		decode_repeated_conversation_member_role(state, (&(*result).conversation_member_role));
		decode_repeated_conversation_member_session(state, (&(*result).conversation_member_session));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_conversation_info_members(
		zcbor_state_t *state, struct conversation_info_members_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"members", tmp_str.len = sizeof("members") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).conversation_info_members_conversation_member_m_count, (zcbor_decoder_t *)decode_conversation_member, state, (*&(*result).conversation_info_members_conversation_member_m), sizeof(struct conversation_member))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_conversation_member(state, (*&(*result).conversation_info_members_conversation_member_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_conversation_info(
		zcbor_state_t *state, struct conversation_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).conversation_info_transport))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).conversation_info_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (decode_conversation_type(state, (&(*result).conversation_info_kind))))
	&& zcbor_present_decode(&((*result).conversation_info_title_present), (zcbor_decoder_t *)decode_repeated_conversation_info_title, state, (&(*result).conversation_info_title))
	&& zcbor_present_decode(&((*result).conversation_info_topic_present), (zcbor_decoder_t *)decode_repeated_conversation_info_topic, state, (&(*result).conversation_info_topic))
	&& zcbor_present_decode(&((*result).conversation_info_description_present), (zcbor_decoder_t *)decode_repeated_conversation_info_description, state, (&(*result).conversation_info_description))
	&& zcbor_present_decode(&((*result).conversation_info_members_present), (zcbor_decoder_t *)decode_repeated_conversation_info_members, state, (&(*result).conversation_info_members))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_conversation_info_title(state, (&(*result).conversation_info_title));
		decode_repeated_conversation_info_topic(state, (&(*result).conversation_info_topic));
		decode_repeated_conversation_info_description(state, (&(*result).conversation_info_description));
		decode_repeated_conversation_info_members(state, (&(*result).conversation_info_members));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_conversations(
		zcbor_state_t *state, struct response_conversations *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Conversations", tmp_str.len = sizeof("Conversations") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_conversations_Conversations_conversation_info_m_count, (zcbor_decoder_t *)decode_conversation_info, state, (*&(*result).response_conversations_Conversations_conversation_info_m), sizeof(struct conversation_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_conversation_info(state, (*&(*result).response_conversations_Conversations_conversation_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_conversation(
		zcbor_state_t *state, struct response_conversation *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Conversation", tmp_str.len = sizeof("Conversation") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_conversation_info(state, (&(*result).response_conversation_Conversation_conversation_info_m)))) && (((*result).response_conversation_Conversation_choice = response_conversation_Conversation_conversation_info_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).response_conversation_Conversation_choice = response_conversation_Conversation_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_conv_create_details(
		zcbor_state_t *state, struct response_conv_create_details *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvCreateDetails", tmp_str.len = sizeof("ConvCreateDetails") - 1, &tmp_str)))))
	&& (decode_create_conversation_details(state, (&(*result).response_conv_create_details_ConvCreateDetails))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_conv_join_details(
		zcbor_state_t *state, struct response_conv_join_details *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConvJoinDetails", tmp_str.len = sizeof("ConvJoinDetails") - 1, &tmp_str)))))
	&& (decode_channel_join_details(state, (&(*result).response_conv_join_details_ConvJoinDetails))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_contact_profile(
		zcbor_state_t *state, struct response_contact_profile *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ContactProfile", tmp_str.len = sizeof("ContactProfile") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).response_contact_profile_ContactProfile))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_contacts(
		zcbor_state_t *state, struct response_contacts *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Contacts", tmp_str.len = sizeof("Contacts") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_contacts_Contacts_contact_info_m_count, (zcbor_decoder_t *)decode_contact_info, state, (*&(*result).response_contacts_Contacts_contact_info_m), sizeof(struct contact_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_contact_info(state, (*&(*result).response_contacts_Contacts_contact_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_action_menu_items(
		zcbor_state_t *state, struct action_menu_items_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"items", tmp_str.len = sizeof("items") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).action_menu_items_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).action_menu_items_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).action_menu_items_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_action_menu(
		zcbor_state_t *state, struct action_menu *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).action_menu_items_present), (zcbor_decoder_t *)decode_repeated_action_menu_items, state, (&(*result).action_menu_items))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_action_menu_items(state, (&(*result).action_menu_items));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_action_menu(
		zcbor_state_t *state, struct response_action_menu *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ActionMenu", tmp_str.len = sizeof("ActionMenu") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_action_menu(state, (&(*result).response_action_menu_ActionMenu_action_menu_m)))) && (((*result).response_action_menu_ActionMenu_choice = response_action_menu_ActionMenu_action_menu_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).response_action_menu_ActionMenu_choice = response_action_menu_ActionMenu_null_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_api_error_unknown_session(
		zcbor_state_t *state, struct api_error_unknown_session *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"UnknownSession", tmp_str.len = sizeof("UnknownSession") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).api_error_unknown_session_UnknownSession))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_api_error_unsupported(
		zcbor_state_t *state, struct api_error_unsupported *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Unsupported", tmp_str.len = sizeof("Unsupported") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).api_error_unsupported_Unsupported))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_api_error_conflict(
		zcbor_state_t *state, struct api_error_conflict *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Conflict", tmp_str.len = sizeof("Conflict") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).api_error_conflict_Conflict))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_api_error_unauthenticated(
		zcbor_state_t *state, struct api_error_unauthenticated *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Unauthenticated", tmp_str.len = sizeof("Unauthenticated") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).api_error_unauthenticated_Unauthenticated))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_api_error_forbidden(
		zcbor_state_t *state, struct api_error_forbidden *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Forbidden", tmp_str.len = sizeof("Forbidden") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).api_error_forbidden_Forbidden))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_api_error_other(
		zcbor_state_t *state, struct api_error_other *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Other", tmp_str.len = sizeof("Other") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).api_error_other_Other))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_api_error(
		zcbor_state_t *state, struct api_error_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_api_error_unknown_session(state, (&(*result).api_error_unknown_session_m)))) && (((*result).api_error_choice = api_error_unknown_session_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_api_error_unsupported(state, (&(*result).api_error_unsupported_m)))) && (((*result).api_error_choice = api_error_unsupported_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_api_error_conflict(state, (&(*result).api_error_conflict_m)))) && (((*result).api_error_choice = api_error_conflict_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_api_error_unauthenticated(state, (&(*result).api_error_unauthenticated_m)))) && (((*result).api_error_choice = api_error_unauthenticated_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_api_error_forbidden(state, (&(*result).api_error_forbidden_m)))) && (((*result).api_error_choice = api_error_forbidden_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_api_error_other(state, (&(*result).api_error_other_m)))) && (((*result).api_error_choice = api_error_other_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_error(
		zcbor_state_t *state, struct response_error *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Error", tmp_str.len = sizeof("Error") - 1, &tmp_str)))))
	&& (decode_api_error(state, (&(*result).response_error_Error))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_root_kind_t(
		zcbor_state_t *state, struct fs_root_kind_t_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"host", tmp_str.len = sizeof("host") - 1, &tmp_str))))) && (((*result).fs_root_kind_t_choice = fs_root_kind_t_host_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"workspace", tmp_str.len = sizeof("workspace") - 1, &tmp_str))))) && (((*result).fs_root_kind_t_choice = fs_root_kind_t_workspace_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str))))) && (((*result).fs_root_kind_t_choice = fs_root_kind_t_session_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_root_session(
		zcbor_state_t *state, struct fs_root_session_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_decode(state, (&(*result).fs_root_session_session_id_m)))) && (((*result).fs_root_session_choice = fs_root_session_session_id_m_c), true))
	|| (((zcbor_nil_expect(state, NULL))) && (((*result).fs_root_session_choice = fs_root_session_null_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_root(
		zcbor_state_t *state, struct fs_root *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (decode_fs_root_id_t(state, (&(*result).fs_root_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"label", tmp_str.len = sizeof("label") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_root_label))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (decode_fs_root_kind_t(state, (&(*result).fs_root_kind))))
	&& zcbor_present_decode(&((*result).fs_root_session_present), (zcbor_decoder_t *)decode_repeated_fs_root_session, state, (&(*result).fs_root_session))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_fs_root_session(state, (&(*result).fs_root_session));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_fs_roots(
		zcbor_state_t *state, struct response_fs_roots *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsRoots", tmp_str.len = sizeof("FsRoots") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_fs_roots_FsRoots_fs_root_m_count, (zcbor_decoder_t *)decode_fs_root, state, (*&(*result).response_fs_roots_FsRoots_fs_root_m), sizeof(struct fs_root))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_fs_root(state, (*&(*result).response_fs_roots_FsRoots_fs_root_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_entry_kind_t(
		zcbor_state_t *state, struct fs_entry_kind_t_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"file", tmp_str.len = sizeof("file") - 1, &tmp_str))))) && (((*result).fs_entry_kind_t_choice = fs_entry_kind_t_file_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"dir", tmp_str.len = sizeof("dir") - 1, &tmp_str))))) && (((*result).fs_entry_kind_t_choice = fs_entry_kind_t_dir_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"symlink", tmp_str.len = sizeof("symlink") - 1, &tmp_str))))) && (((*result).fs_entry_kind_t_choice = fs_entry_kind_t_symlink_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_entry_ignored(
		zcbor_state_t *state, struct fs_entry_ignored *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ignored", tmp_str.len = sizeof("ignored") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).fs_entry_ignored)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_entry(
		zcbor_state_t *state, struct fs_entry *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_entry_name))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_entry_path))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (decode_fs_entry_kind_t(state, (&(*result).fs_entry_kind))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"size", tmp_str.len = sizeof("size") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).fs_entry_size))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"mtime_ms", tmp_str.len = sizeof("mtime_ms") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).fs_entry_mtime_ms))))
	&& zcbor_present_decode(&((*result).fs_entry_ignored_present), (zcbor_decoder_t *)decode_repeated_fs_entry_ignored, state, (&(*result).fs_entry_ignored))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_fs_entry_ignored(state, (&(*result).fs_entry_ignored));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_fs_list(
		zcbor_state_t *state, struct response_fs_list *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsList", tmp_str.len = sizeof("FsList") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_fs_list_FsList_fs_entry_m_count, (zcbor_decoder_t *)decode_fs_entry, state, (*&(*result).response_fs_list_FsList_fs_entry_m), sizeof(struct fs_entry))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_fs_entry(state, (*&(*result).response_fs_list_FsList_fs_entry_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_fs_stat(
		zcbor_state_t *state, struct response_fs_stat *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsStat", tmp_str.len = sizeof("FsStat") - 1, &tmp_str)))))
	&& (decode_fs_entry(state, (&(*result).response_fs_stat_FsStat))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_content_truncated(
		zcbor_state_t *state, struct fs_content_truncated *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"truncated", tmp_str.len = sizeof("truncated") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).fs_content_truncated)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_content_blob_ref(
		zcbor_state_t *state, struct fs_content_blob_ref_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"blob_ref", tmp_str.len = sizeof("blob_ref") - 1, &tmp_str)))))
	&& (zcbor_union_start_code(state) && (int_res = ((((decode_blob_ref(state, (&(*result).fs_content_blob_ref_blob_ref_m)))) && (((*result).fs_content_blob_ref_choice = fs_content_blob_ref_blob_ref_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_nil_expect(state, NULL))) && (((*result).fs_content_blob_ref_choice = fs_content_blob_ref_null_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_content(
		zcbor_state_t *state, struct fs_content *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"bytes", tmp_str.len = sizeof("bytes") - 1, &tmp_str)))))
	&& (zcbor_bstr_decode(state, (&(*result).fs_content_bytes))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"revision", tmp_str.len = sizeof("revision") - 1, &tmp_str)))))
	&& (decode_fs_revision(state, (&(*result).fs_content_revision))))
	&& zcbor_present_decode(&((*result).fs_content_truncated_present), (zcbor_decoder_t *)decode_repeated_fs_content_truncated, state, (&(*result).fs_content_truncated))
	&& zcbor_present_decode(&((*result).fs_content_blob_ref_present), (zcbor_decoder_t *)decode_repeated_fs_content_blob_ref, state, (&(*result).fs_content_blob_ref))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_fs_content_truncated(state, (&(*result).fs_content_truncated));
		decode_repeated_fs_content_blob_ref(state, (&(*result).fs_content_blob_ref));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_fs_read(
		zcbor_state_t *state, struct response_fs_read *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsRead", tmp_str.len = sizeof("FsRead") - 1, &tmp_str)))))
	&& (decode_fs_content(state, (&(*result).response_fs_read_FsRead))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_fs_write(
		zcbor_state_t *state, struct response_fs_write *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsWrite", tmp_str.len = sizeof("FsWrite") - 1, &tmp_str)))))
	&& (decode_fs_revision(state, (&(*result).response_fs_write_FsWrite))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_search_hit(
		zcbor_state_t *state, struct fs_search_hit *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_search_hit_path))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"line", tmp_str.len = sizeof("line") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).fs_search_hit_line))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"col", tmp_str.len = sizeof("col") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).fs_search_hit_col))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"preview", tmp_str.len = sizeof("preview") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_search_hit_preview))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_fs_search_page_has_more(
		zcbor_state_t *state, struct fs_search_page_has_more *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"has_more", tmp_str.len = sizeof("has_more") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).fs_search_page_has_more)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_search_page(
		zcbor_state_t *state, struct fs_search_page *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"hits", tmp_str.len = sizeof("hits") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).fs_search_page_hits_fs_search_hit_m_count, (zcbor_decoder_t *)decode_fs_search_hit, state, (*&(*result).fs_search_page_hits_fs_search_hit_m), sizeof(struct fs_search_hit))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& zcbor_present_decode(&((*result).fs_search_page_has_more_present), (zcbor_decoder_t *)decode_repeated_fs_search_page_has_more, state, (&(*result).fs_search_page_has_more))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_fs_search_hit(state, (*&(*result).fs_search_page_hits_fs_search_hit_m));
		decode_repeated_fs_search_page_has_more(state, (&(*result).fs_search_page_has_more));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_fs_search(
		zcbor_state_t *state, struct response_fs_search *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsSearch", tmp_str.len = sizeof("FsSearch") - 1, &tmp_str)))))
	&& (decode_fs_search_page(state, (&(*result).response_fs_search_FsSearch))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_change_kind_t(
		zcbor_state_t *state, struct fs_change_kind_t_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"created", tmp_str.len = sizeof("created") - 1, &tmp_str))))) && (((*result).fs_change_kind_t_choice = fs_change_kind_t_created_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"modified", tmp_str.len = sizeof("modified") - 1, &tmp_str))))) && (((*result).fs_change_kind_t_choice = fs_change_kind_t_modified_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"removed", tmp_str.len = sizeof("removed") - 1, &tmp_str))))) && (((*result).fs_change_kind_t_choice = fs_change_kind_t_removed_tstr_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_change(
		zcbor_state_t *state, struct fs_change *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_change_path))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (decode_fs_change_kind_t(state, (&(*result).fs_change_kind))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_fs_watch_page_view(
		zcbor_state_t *state, struct fs_watch_page_view *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"events", tmp_str.len = sizeof("events") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).fs_watch_page_view_events_fs_change_m_count, (zcbor_decoder_t *)decode_fs_change, state, (*&(*result).fs_watch_page_view_events_fs_change_m), sizeof(struct fs_change))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"next_seq", tmp_str.len = sizeof("next_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).fs_watch_page_view_next_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"head_seq", tmp_str.len = sizeof("head_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).fs_watch_page_view_head_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"reset", tmp_str.len = sizeof("reset") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).fs_watch_page_view_reset))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_fs_change(state, (*&(*result).fs_watch_page_view_events_fs_change_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_fs_watch(
		zcbor_state_t *state, struct response_fs_watch *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsWatch", tmp_str.len = sizeof("FsWatch") - 1, &tmp_str)))))
	&& (decode_fs_watch_page_view(state, (&(*result).response_fs_watch_FsWatch))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_blob_put(
		zcbor_state_t *state, struct response_blob_put *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"BlobPut", tmp_str.len = sizeof("BlobPut") - 1, &tmp_str)))))
	&& (decode_blob_ref(state, (&(*result).response_blob_put_BlobPut))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_blob_get(
		zcbor_state_t *state, struct response_blob_get *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"BlobGet", tmp_str.len = sizeof("BlobGet") - 1, &tmp_str)))))
	&& (zcbor_bstr_decode(state, (&(*result).response_blob_get_BlobGet))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_blob_stat(
		zcbor_state_t *state, struct blob_stat *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"size", tmp_str.len = sizeof("size") - 1, &tmp_str)))))
	&& (zcbor_uint64_decode(state, (&(*result).blob_stat_size))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"present", tmp_str.len = sizeof("present") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).blob_stat_present))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_blob_stat(
		zcbor_state_t *state, struct response_blob_stat *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"BlobStat", tmp_str.len = sizeof("BlobStat") - 1, &tmp_str)))))
	&& (decode_blob_stat(state, (&(*result).response_blob_stat_BlobStat))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_access_user(
		zcbor_state_t *state, struct access_user *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).access_user_user_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"username", tmp_str.len = sizeof("username") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).access_user_username))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"disabled", tmp_str.len = sizeof("disabled") - 1, &tmp_str)))))
	&& (zcbor_bool_decode(state, (&(*result).access_user_disabled))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"created_at", tmp_str.len = sizeof("created_at") - 1, &tmp_str)))))
	&& (zcbor_int32_decode(state, (&(*result).access_user_created_at))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"roles", tmp_str.len = sizeof("roles") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).access_user_roles_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).access_user_roles_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).access_user_roles_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_access_user(
		zcbor_state_t *state, struct response_access_user *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AccessUser", tmp_str.len = sizeof("AccessUser") - 1, &tmp_str)))))
	&& (decode_access_user(state, (&(*result).response_access_user_AccessUser))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_access_users(
		zcbor_state_t *state, struct response_access_users *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AccessUsers", tmp_str.len = sizeof("AccessUsers") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_access_users_AccessUsers_access_user_m_count, (zcbor_decoder_t *)decode_access_user, state, (*&(*result).response_access_users_AccessUsers_access_user_m), sizeof(struct access_user))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_access_user(state, (*&(*result).response_access_users_AccessUsers_access_user_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_role_info(
		zcbor_state_t *state, struct role_info *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).role_info_role))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"capabilities", tmp_str.len = sizeof("capabilities") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).role_info_capabilities_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).role_info_capabilities_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).role_info_capabilities_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_access_roles(
		zcbor_state_t *state, struct response_access_roles *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AccessRoles", tmp_str.len = sizeof("AccessRoles") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).response_access_roles_AccessRoles_role_info_m_count, (zcbor_decoder_t *)decode_role_info, state, (*&(*result).response_access_roles_AccessRoles_role_info_m), sizeof(struct role_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_role_info(state, (*&(*result).response_access_roles_AccessRoles_role_info_m));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_principal_view(
		zcbor_state_t *state, struct principal_view *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).principal_view_user_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"username", tmp_str.len = sizeof("username") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).principal_view_username))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"roles", tmp_str.len = sizeof("roles") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).principal_view_roles_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).principal_view_roles_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"capabilities", tmp_str.len = sizeof("capabilities") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 64, &(*result).principal_view_capabilities_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).principal_view_capabilities_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_tstr_decode(state, (*&(*result).principal_view_roles_tstr));
		zcbor_tstr_decode(state, (*&(*result).principal_view_capabilities_tstr));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_response_who_am_i(
		zcbor_state_t *state, struct response_who_am_i *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"WhoAmI", tmp_str.len = sizeof("WhoAmI") - 1, &tmp_str)))))
	&& (decode_principal_view(state, (&(*result).response_who_am_i_WhoAmI))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_api_response(
		zcbor_state_t *state, struct api_response_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Ok", tmp_str.len = sizeof("Ok") - 1, &tmp_str))))) && (((*result).api_response_choice = api_response_response_ok_m_c), true))
	|| (((decode_response_routed(state, (&(*result).api_response_response_routed_m)))) && (((*result).api_response_choice = api_response_response_routed_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_response_drained(state, (&(*result).api_response_response_drained_m)))) && (((*result).api_response_choice = api_response_response_drained_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_health(state, (&(*result).api_response_response_health_m)))) && (((*result).api_response_choice = api_response_response_health_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_stats(state, (&(*result).api_response_response_stats_m)))) && (((*result).api_response_choice = api_response_response_stats_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_telemetry(state, (&(*result).api_response_response_telemetry_m)))) && (((*result).api_response_choice = api_response_response_telemetry_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_sessions(state, (&(*result).api_response_response_sessions_m)))) && (((*result).api_response_choice = api_response_response_sessions_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_approvals(state, (&(*result).api_response_response_approvals_m)))) && (((*result).api_response_choice = api_response_response_approvals_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_fleet(state, (&(*result).api_response_response_fleet_m)))) && (((*result).api_response_choice = api_response_response_fleet_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_tree(state, (&(*result).api_response_response_tree_m)))) && (((*result).api_response_choice = api_response_response_tree_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_unit(state, (&(*result).api_response_response_unit_m)))) && (((*result).api_response_choice = api_response_response_unit_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_unit_events(state, (&(*result).api_response_response_unit_events_m)))) && (((*result).api_response_choice = api_response_response_unit_events_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_journal(state, (&(*result).api_response_response_journal_m)))) && (((*result).api_response_choice = api_response_response_journal_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_log_page(state, (&(*result).api_response_response_log_page_m)))) && (((*result).api_response_choice = api_response_response_log_page_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_events_page(state, (&(*result).api_response_response_events_page_m)))) && (((*result).api_response_choice = api_response_response_events_page_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_delivery_targets(state, (&(*result).api_response_response_delivery_targets_m)))) && (((*result).api_response_choice = api_response_response_delivery_targets_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_delivery_sessions(state, (&(*result).api_response_response_delivery_sessions_m)))) && (((*result).api_response_choice = api_response_response_delivery_sessions_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_verifying_key(state, (&(*result).api_response_response_verifying_key_m)))) && (((*result).api_response_choice = api_response_response_verifying_key_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_model_search(state, (&(*result).api_response_response_model_search_m)))) && (((*result).api_response_choice = api_response_response_model_search_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_model_files(state, (&(*result).api_response_response_model_files_m)))) && (((*result).api_response_choice = api_response_response_model_files_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_model_download_started(state, (&(*result).api_response_response_model_download_started_m)))) && (((*result).api_response_choice = api_response_response_model_download_started_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_model_downloads(state, (&(*result).api_response_response_model_downloads_m)))) && (((*result).api_response_choice = api_response_response_model_downloads_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_model_catalog(state, (&(*result).api_response_response_model_catalog_m)))) && (((*result).api_response_choice = api_response_response_model_catalog_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_model_recommend(state, (&(*result).api_response_response_model_recommend_m)))) && (((*result).api_response_choice = api_response_response_model_recommend_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_model_quantize_started(state, (&(*result).api_response_response_model_quantize_started_m)))) && (((*result).api_response_choice = api_response_response_model_quantize_started_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_model_quantizes(state, (&(*result).api_response_response_model_quantizes_m)))) && (((*result).api_response_choice = api_response_response_model_quantizes_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_model_inspect(state, (&(*result).api_response_response_model_inspect_m)))) && (((*result).api_response_choice = api_response_response_model_inspect_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_profiles(state, (&(*result).api_response_response_profiles_m)))) && (((*result).api_response_choice = api_response_response_profiles_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_profile(state, (&(*result).api_response_response_profile_m)))) && (((*result).api_response_choice = api_response_response_profile_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_credentials(state, (&(*result).api_response_response_credentials_m)))) && (((*result).api_response_choice = api_response_response_credentials_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_models(state, (&(*result).api_response_response_models_m)))) && (((*result).api_response_choice = api_response_response_models_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_model_current(state, (&(*result).api_response_response_model_current_m)))) && (((*result).api_response_choice = api_response_response_model_current_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_distribution(state, (&(*result).api_response_response_distribution_m)))) && (((*result).api_response_choice = api_response_response_distribution_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_profile_id(state, (&(*result).api_response_response_profile_id_m)))) && (((*result).api_response_choice = api_response_response_profile_id_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_revisions(state, (&(*result).api_response_response_revisions_m)))) && (((*result).api_response_choice = api_response_response_revisions_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_skill_bundle(state, (&(*result).api_response_response_skill_bundle_m)))) && (((*result).api_response_choice = api_response_response_skill_bundle_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_curator_skills(state, (&(*result).api_response_response_curator_skills_m)))) && (((*result).api_response_choice = api_response_response_curator_skills_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_curator_run(state, (&(*result).api_response_response_curator_run_m)))) && (((*result).api_response_choice = api_response_response_curator_run_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_auth_begun(state, (&(*result).api_response_response_auth_begun_m)))) && (((*result).api_response_choice = api_response_response_auth_begun_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_auth_completed(state, (&(*result).api_response_response_auth_completed_m)))) && (((*result).api_response_choice = api_response_response_auth_completed_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_auth_providers(state, (&(*result).api_response_response_auth_providers_m)))) && (((*result).api_response_choice = api_response_response_auth_providers_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_checkpoints(state, (&(*result).api_response_response_checkpoints_m)))) && (((*result).api_response_choice = api_response_response_checkpoints_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_session_page(state, (&(*result).api_response_response_session_page_m)))) && (((*result).api_response_choice = api_response_response_session_page_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_session_detail(state, (&(*result).api_response_response_session_detail_m)))) && (((*result).api_response_choice = api_response_response_session_detail_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_sessions_by_profile(state, (&(*result).api_response_response_sessions_by_profile_m)))) && (((*result).api_response_choice = api_response_response_sessions_by_profile_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_session_search(state, (&(*result).api_response_response_session_search_m)))) && (((*result).api_response_choice = api_response_response_session_search_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_acp_catalog(state, (&(*result).api_response_response_acp_catalog_m)))) && (((*result).api_response_choice = api_response_response_acp_catalog_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_providers(state, (&(*result).api_response_response_providers_m)))) && (((*result).api_response_choice = api_response_response_providers_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_tools(state, (&(*result).api_response_response_tools_m)))) && (((*result).api_response_choice = api_response_response_tools_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_commands(state, (&(*result).api_response_response_commands_m)))) && (((*result).api_response_choice = api_response_response_commands_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_command_output(state, (&(*result).api_response_response_command_output_m)))) && (((*result).api_response_choice = api_response_response_command_output_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_config(state, (&(*result).api_response_response_config_m)))) && (((*result).api_response_choice = api_response_response_config_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_cron_jobs(state, (&(*result).api_response_response_cron_jobs_m)))) && (((*result).api_response_choice = api_response_response_cron_jobs_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_cron_id(state, (&(*result).api_response_response_cron_id_m)))) && (((*result).api_response_choice = api_response_response_cron_id_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_cron_runs(state, (&(*result).api_response_response_cron_runs_m)))) && (((*result).api_response_choice = api_response_response_cron_runs_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_cron_suggestions(state, (&(*result).api_response_response_cron_suggestions_m)))) && (((*result).api_response_choice = api_response_response_cron_suggestions_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_chat_routes(state, (&(*result).api_response_response_chat_routes_m)))) && (((*result).api_response_choice = api_response_response_chat_routes_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_chat_route(state, (&(*result).api_response_response_chat_route_m)))) && (((*result).api_response_choice = api_response_response_chat_route_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_rooms(state, (&(*result).api_response_response_rooms_m)))) && (((*result).api_response_choice = api_response_response_rooms_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_adapters(state, (&(*result).api_response_response_adapters_m)))) && (((*result).api_response_choice = api_response_response_adapters_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_transport_instances(state, (&(*result).api_response_response_transport_instances_m)))) && (((*result).api_response_choice = api_response_response_transport_instances_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_conversations(state, (&(*result).api_response_response_conversations_m)))) && (((*result).api_response_choice = api_response_response_conversations_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_conversation(state, (&(*result).api_response_response_conversation_m)))) && (((*result).api_response_choice = api_response_response_conversation_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_conv_create_details(state, (&(*result).api_response_response_conv_create_details_m)))) && (((*result).api_response_choice = api_response_response_conv_create_details_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_conv_join_details(state, (&(*result).api_response_response_conv_join_details_m)))) && (((*result).api_response_choice = api_response_response_conv_join_details_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_contact_profile(state, (&(*result).api_response_response_contact_profile_m)))) && (((*result).api_response_choice = api_response_response_contact_profile_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_contacts(state, (&(*result).api_response_response_contacts_m)))) && (((*result).api_response_choice = api_response_response_contacts_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_action_menu(state, (&(*result).api_response_response_action_menu_m)))) && (((*result).api_response_choice = api_response_response_action_menu_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_error(state, (&(*result).api_response_response_error_m)))) && (((*result).api_response_choice = api_response_response_error_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_fs_roots(state, (&(*result).api_response_response_fs_roots_m)))) && (((*result).api_response_choice = api_response_response_fs_roots_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_fs_list(state, (&(*result).api_response_response_fs_list_m)))) && (((*result).api_response_choice = api_response_response_fs_list_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_fs_stat(state, (&(*result).api_response_response_fs_stat_m)))) && (((*result).api_response_choice = api_response_response_fs_stat_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_fs_read(state, (&(*result).api_response_response_fs_read_m)))) && (((*result).api_response_choice = api_response_response_fs_read_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_fs_write(state, (&(*result).api_response_response_fs_write_m)))) && (((*result).api_response_choice = api_response_response_fs_write_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_fs_search(state, (&(*result).api_response_response_fs_search_m)))) && (((*result).api_response_choice = api_response_response_fs_search_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_fs_watch(state, (&(*result).api_response_response_fs_watch_m)))) && (((*result).api_response_choice = api_response_response_fs_watch_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_blob_put(state, (&(*result).api_response_response_blob_put_m)))) && (((*result).api_response_choice = api_response_response_blob_put_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_blob_get(state, (&(*result).api_response_response_blob_get_m)))) && (((*result).api_response_choice = api_response_response_blob_get_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_blob_stat(state, (&(*result).api_response_response_blob_stat_m)))) && (((*result).api_response_choice = api_response_response_blob_stat_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_access_user(state, (&(*result).api_response_response_access_user_m)))) && (((*result).api_response_choice = api_response_response_access_user_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_access_users(state, (&(*result).api_response_response_access_users_m)))) && (((*result).api_response_choice = api_response_response_access_users_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_access_roles(state, (&(*result).api_response_response_access_roles_m)))) && (((*result).api_response_choice = api_response_response_access_roles_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_who_am_i(state, (&(*result).api_response_response_who_am_i_m)))) && (((*result).api_response_choice = api_response_response_who_am_i_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_api_request(
		zcbor_state_t *state, struct api_request_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_request_submit(state, (&(*result).api_request_request_submit_m)))) && (((*result).api_request_choice = api_request_request_submit_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_submit_routed(state, (&(*result).api_request_request_submit_routed_m)))) && (((*result).api_request_choice = api_request_request_submit_routed_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_poll(state, (&(*result).api_request_request_poll_m)))) && (((*result).api_request_choice = api_request_request_poll_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_respond(state, (&(*result).api_request_request_respond_m)))) && (((*result).api_request_choice = api_request_request_respond_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Health", tmp_str.len = sizeof("Health") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_health_m_c), true)))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Stats", tmp_str.len = sizeof("Stats") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_stats_m_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Telemetry", tmp_str.len = sizeof("Telemetry") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_telemetry_m_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Sessions", tmp_str.len = sizeof("Sessions") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_sessions_m_c), true))
	|| (((decode_request_assign(state, (&(*result).api_request_request_assign_m)))) && (((*result).api_request_choice = api_request_request_assign_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_cancel(state, (&(*result).api_request_request_cancel_m)))) && (((*result).api_request_choice = api_request_request_cancel_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Fleet", tmp_str.len = sizeof("Fleet") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_fleet_m_c), true)))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Tree", tmp_str.len = sizeof("Tree") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_tree_m_c), true))
	|| (((decode_request_unit(state, (&(*result).api_request_request_unit_m)))) && (((*result).api_request_choice = api_request_request_unit_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_unit_events(state, (&(*result).api_request_request_unit_events_m)))) && (((*result).api_request_choice = api_request_request_unit_events_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_unit_outbound(state, (&(*result).api_request_request_unit_outbound_m)))) && (((*result).api_request_choice = api_request_request_unit_outbound_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_session_history(state, (&(*result).api_request_request_session_history_m)))) && (((*result).api_request_choice = api_request_request_session_history_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_subscribe(state, (&(*result).api_request_request_subscribe_m)))) && (((*result).api_request_choice = api_request_request_subscribe_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_events_since(state, (&(*result).api_request_request_events_since_m)))) && (((*result).api_request_choice = api_request_request_events_since_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_delivery_targets(state, (&(*result).api_request_request_delivery_targets_m)))) && (((*result).api_request_choice = api_request_request_delivery_targets_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_delivery_sessions(state, (&(*result).api_request_request_delivery_sessions_m)))) && (((*result).api_request_choice = api_request_request_delivery_sessions_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_handover(state, (&(*result).api_request_request_handover_m)))) && (((*result).api_request_choice = api_request_request_handover_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_record_meta(state, (&(*result).api_request_request_record_meta_m)))) && (((*result).api_request_choice = api_request_request_record_meta_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_unit_history(state, (&(*result).api_request_request_unit_history_m)))) && (((*result).api_request_choice = api_request_request_unit_history_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_pause(state, (&(*result).api_request_request_pause_m)))) && (((*result).api_request_choice = api_request_request_pause_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_resume(state, (&(*result).api_request_request_resume_m)))) && (((*result).api_request_choice = api_request_request_resume_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_scale(state, (&(*result).api_request_request_scale_m)))) && (((*result).api_request_choice = api_request_request_scale_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"VerifyingKey", tmp_str.len = sizeof("VerifyingKey") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_verifying_key_m_c), true)))
	|| (((decode_request_set_session_model(state, (&(*result).api_request_request_set_session_model_m)))) && (((*result).api_request_choice = api_request_request_set_session_model_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_set_session_mode(state, (&(*result).api_request_request_set_session_mode_m)))) && (((*result).api_request_choice = api_request_request_set_session_mode_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_set_session_overlay(state, (&(*result).api_request_request_set_session_overlay_m)))) && (((*result).api_request_choice = api_request_request_set_session_overlay_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_approvals_pending(state, (&(*result).api_request_request_approvals_pending_m)))) && (((*result).api_request_choice = api_request_request_approvals_pending_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_approval_decide(state, (&(*result).api_request_request_approval_decide_m)))) && (((*result).api_request_choice = api_request_request_approval_decide_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileList", tmp_str.len = sizeof("ProfileList") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_profile_list_m_c), true)))
	|| (((decode_request_profile_get(state, (&(*result).api_request_request_profile_get_m)))) && (((*result).api_request_choice = api_request_request_profile_get_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_profile_create(state, (&(*result).api_request_request_profile_create_m)))) && (((*result).api_request_choice = api_request_request_profile_create_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_profile_update(state, (&(*result).api_request_request_profile_update_m)))) && (((*result).api_request_choice = api_request_request_profile_update_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_profile_delete(state, (&(*result).api_request_request_profile_delete_m)))) && (((*result).api_request_choice = api_request_request_profile_delete_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_profile_select(state, (&(*result).api_request_request_profile_select_m)))) && (((*result).api_request_choice = api_request_request_profile_select_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_profile_clone(state, (&(*result).api_request_request_profile_clone_m)))) && (((*result).api_request_choice = api_request_request_profile_clone_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_profile_export(state, (&(*result).api_request_request_profile_export_m)))) && (((*result).api_request_choice = api_request_request_profile_export_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_profile_import(state, (&(*result).api_request_request_profile_import_m)))) && (((*result).api_request_choice = api_request_request_profile_import_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_profile_history(state, (&(*result).api_request_request_profile_history_m)))) && (((*result).api_request_choice = api_request_request_profile_history_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_profile_at(state, (&(*result).api_request_request_profile_at_m)))) && (((*result).api_request_choice = api_request_request_profile_at_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_profile_revert(state, (&(*result).api_request_request_profile_revert_m)))) && (((*result).api_request_choice = api_request_request_profile_revert_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_skill_history(state, (&(*result).api_request_request_skill_history_m)))) && (((*result).api_request_choice = api_request_request_skill_history_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_skill_at(state, (&(*result).api_request_request_skill_at_m)))) && (((*result).api_request_choice = api_request_request_skill_at_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_skill_revert(state, (&(*result).api_request_request_skill_revert_m)))) && (((*result).api_request_choice = api_request_request_skill_revert_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_curator_list(state, (&(*result).api_request_request_curator_list_m)))) && (((*result).api_request_choice = api_request_request_curator_list_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_curator_pin(state, (&(*result).api_request_request_curator_pin_m)))) && (((*result).api_request_choice = api_request_request_curator_pin_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_curator_unpin(state, (&(*result).api_request_request_curator_unpin_m)))) && (((*result).api_request_choice = api_request_request_curator_unpin_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_curator_archive(state, (&(*result).api_request_request_curator_archive_m)))) && (((*result).api_request_choice = api_request_request_curator_archive_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_curator_restore(state, (&(*result).api_request_request_curator_restore_m)))) && (((*result).api_request_choice = api_request_request_curator_restore_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_curator_run(state, (&(*result).api_request_request_curator_run_m)))) && (((*result).api_request_choice = api_request_request_curator_run_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_credential_set(state, (&(*result).api_request_request_credential_set_m)))) && (((*result).api_request_choice = api_request_request_credential_set_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CredentialList", tmp_str.len = sizeof("CredentialList") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_credential_list_m_c), true)))
	|| (((decode_request_credential_remove(state, (&(*result).api_request_request_credential_remove_m)))) && (((*result).api_request_choice = api_request_request_credential_remove_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_model_search(state, (&(*result).api_request_request_model_search_m)))) && (((*result).api_request_choice = api_request_request_model_search_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_model_files(state, (&(*result).api_request_request_model_files_m)))) && (((*result).api_request_choice = api_request_request_model_files_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_model_download(state, (&(*result).api_request_request_model_download_m)))) && (((*result).api_request_choice = api_request_request_model_download_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelDownloads", tmp_str.len = sizeof("ModelDownloads") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_model_downloads_m_c), true)))
	|| (((decode_request_model_cancel(state, (&(*result).api_request_request_model_cancel_m)))) && (((*result).api_request_choice = api_request_request_model_cancel_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_model_pause(state, (&(*result).api_request_request_model_pause_m)))) && (((*result).api_request_choice = api_request_request_model_pause_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_model_resume(state, (&(*result).api_request_request_model_resume_m)))) && (((*result).api_request_choice = api_request_request_model_resume_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelCatalog", tmp_str.len = sizeof("ModelCatalog") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_model_catalog_m_c), true)))
	|| (((decode_request_model_delete(state, (&(*result).api_request_request_model_delete_m)))) && (((*result).api_request_choice = api_request_request_model_delete_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_model_activate(state, (&(*result).api_request_request_model_activate_m)))) && (((*result).api_request_choice = api_request_request_model_activate_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_model_recommend(state, (&(*result).api_request_request_model_recommend_m)))) && (((*result).api_request_choice = api_request_request_model_recommend_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_model_quantize(state, (&(*result).api_request_request_model_quantize_m)))) && (((*result).api_request_choice = api_request_request_model_quantize_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ModelQuantizes", tmp_str.len = sizeof("ModelQuantizes") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_model_quantizes_m_c), true)))
	|| (((decode_request_model_inspect(state, (&(*result).api_request_request_model_inspect_m)))) && (((*result).api_request_choice = api_request_request_model_inspect_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Models", tmp_str.len = sizeof("Models") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_models_m_c), true)))
	|| (((decode_request_model_current(state, (&(*result).api_request_request_model_current_m)))) && (((*result).api_request_choice = api_request_request_model_current_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_auth_begin(state, (&(*result).api_request_request_auth_begin_m)))) && (((*result).api_request_choice = api_request_request_auth_begin_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_auth_complete(state, (&(*result).api_request_request_auth_complete_m)))) && (((*result).api_request_choice = api_request_request_auth_complete_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_auth_cancel(state, (&(*result).api_request_request_auth_cancel_m)))) && (((*result).api_request_choice = api_request_request_auth_cancel_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AuthProviders", tmp_str.len = sizeof("AuthProviders") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_auth_providers_m_c), true)))
	|| (((decode_request_checkpoint_list(state, (&(*result).api_request_request_checkpoint_list_m)))) && (((*result).api_request_choice = api_request_request_checkpoint_list_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_checkpoint_rewind(state, (&(*result).api_request_request_checkpoint_rewind_m)))) && (((*result).api_request_choice = api_request_request_checkpoint_rewind_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_sessions_query(state, (&(*result).api_request_request_sessions_query_m)))) && (((*result).api_request_choice = api_request_request_sessions_query_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_session_get(state, (&(*result).api_request_request_session_get_m)))) && (((*result).api_request_choice = api_request_request_session_get_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"SessionsByProfile", tmp_str.len = sizeof("SessionsByProfile") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_sessions_by_profile_m_c), true)))
	|| (((decode_request_session_search(state, (&(*result).api_request_request_session_search_m)))) && (((*result).api_request_choice = api_request_request_session_search_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_session_update_meta(state, (&(*result).api_request_request_session_update_meta_m)))) && (((*result).api_request_choice = api_request_request_session_update_meta_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_rewind(state, (&(*result).api_request_request_rewind_m)))) && (((*result).api_request_choice = api_request_request_rewind_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AcpDiscover", tmp_str.len = sizeof("AcpDiscover") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_acp_discover_m_c), true)))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"AcpCatalog", tmp_str.len = sizeof("AcpCatalog") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_acp_catalog_m_c), true))
	|| (((decode_request_acp_register(state, (&(*result).api_request_request_acp_register_m)))) && (((*result).api_request_choice = api_request_request_acp_register_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_acp_remove(state, (&(*result).api_request_request_acp_remove_m)))) && (((*result).api_request_choice = api_request_request_acp_remove_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_skill_get(state, (&(*result).api_request_request_skill_get_m)))) && (((*result).api_request_choice = api_request_request_skill_get_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_skill_put(state, (&(*result).api_request_request_skill_put_m)))) && (((*result).api_request_choice = api_request_request_skill_put_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProviderList", tmp_str.len = sizeof("ProviderList") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_provider_list_m_c), true)))
	|| (((decode_request_provider_register(state, (&(*result).api_request_request_provider_register_m)))) && (((*result).api_request_choice = api_request_request_provider_register_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ToolList", tmp_str.len = sizeof("ToolList") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_tool_list_m_c), true)))
	|| (((decode_request_tool_register(state, (&(*result).api_request_request_tool_register_m)))) && (((*result).api_request_choice = api_request_request_tool_register_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CommandList", tmp_str.len = sizeof("CommandList") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_command_list_m_c), true)))
	|| (((decode_request_command_invoke(state, (&(*result).api_request_request_command_invoke_m)))) && (((*result).api_request_choice = api_request_request_command_invoke_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ConfigGet", tmp_str.len = sizeof("ConfigGet") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_config_get_m_c), true)))
	|| (((decode_request_config_set(state, (&(*result).api_request_request_config_set_m)))) && (((*result).api_request_choice = api_request_request_config_set_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronList", tmp_str.len = sizeof("CronList") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_cron_list_m_c), true)))
	|| (((decode_request_cron_create(state, (&(*result).api_request_request_cron_create_m)))) && (((*result).api_request_choice = api_request_request_cron_create_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_cron_update(state, (&(*result).api_request_request_cron_update_m)))) && (((*result).api_request_choice = api_request_request_cron_update_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_cron_delete(state, (&(*result).api_request_request_cron_delete_m)))) && (((*result).api_request_choice = api_request_request_cron_delete_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_cron_trigger(state, (&(*result).api_request_request_cron_trigger_m)))) && (((*result).api_request_choice = api_request_request_cron_trigger_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_cron_runs(state, (&(*result).api_request_request_cron_runs_m)))) && (((*result).api_request_choice = api_request_request_cron_runs_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_cron_pause(state, (&(*result).api_request_request_cron_pause_m)))) && (((*result).api_request_choice = api_request_request_cron_pause_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CronSuggestions", tmp_str.len = sizeof("CronSuggestions") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_cron_suggestions_m_c), true)))
	|| (((decode_request_cron_accept_suggestion(state, (&(*result).api_request_request_cron_accept_suggestion_m)))) && (((*result).api_request_choice = api_request_request_cron_accept_suggestion_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_cron_dismiss_suggestion(state, (&(*result).api_request_request_cron_dismiss_suggestion_m)))) && (((*result).api_request_choice = api_request_request_cron_dismiss_suggestion_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"RoutingListChats", tmp_str.len = sizeof("RoutingListChats") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_routing_list_chats_m_c), true)))
	|| (((decode_request_routing_get(state, (&(*result).api_request_request_routing_get_m)))) && (((*result).api_request_choice = api_request_request_routing_get_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_routing_set(state, (&(*result).api_request_request_routing_set_m)))) && (((*result).api_request_choice = api_request_request_routing_set_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_routing_bind_chat(state, (&(*result).api_request_request_routing_bind_chat_m)))) && (((*result).api_request_choice = api_request_request_routing_bind_chat_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_routing_unbind_chat(state, (&(*result).api_request_request_routing_unbind_chat_m)))) && (((*result).api_request_choice = api_request_request_routing_unbind_chat_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_transport_rooms(state, (&(*result).api_request_request_transport_rooms_m)))) && (((*result).api_request_choice = api_request_request_transport_rooms_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"TransportAdapters", tmp_str.len = sizeof("TransportAdapters") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_transport_adapters_m_c), true)))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"TransportInstances", tmp_str.len = sizeof("TransportInstances") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_transport_instances_m_c), true))
	|| (((decode_request_conv_list(state, (&(*result).api_request_request_conv_list_m)))) && (((*result).api_request_choice = api_request_request_conv_list_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_conv_get(state, (&(*result).api_request_request_conv_get_m)))) && (((*result).api_request_choice = api_request_request_conv_get_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_conv_create_details(state, (&(*result).api_request_request_conv_create_details_m)))) && (((*result).api_request_choice = api_request_request_conv_create_details_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_conv_create(state, (&(*result).api_request_request_conv_create_m)))) && (((*result).api_request_choice = api_request_request_conv_create_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_conv_join_details(state, (&(*result).api_request_request_conv_join_details_m)))) && (((*result).api_request_choice = api_request_request_conv_join_details_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_conv_join(state, (&(*result).api_request_request_conv_join_m)))) && (((*result).api_request_choice = api_request_request_conv_join_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_conv_leave(state, (&(*result).api_request_request_conv_leave_m)))) && (((*result).api_request_choice = api_request_request_conv_leave_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_conv_send(state, (&(*result).api_request_request_conv_send_m)))) && (((*result).api_request_choice = api_request_request_conv_send_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_conv_set_topic(state, (&(*result).api_request_request_conv_set_topic_m)))) && (((*result).api_request_choice = api_request_request_conv_set_topic_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_conv_set_title(state, (&(*result).api_request_request_conv_set_title_m)))) && (((*result).api_request_choice = api_request_request_conv_set_title_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_conv_set_description(state, (&(*result).api_request_request_conv_set_description_m)))) && (((*result).api_request_choice = api_request_request_conv_set_description_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_conv_delete(state, (&(*result).api_request_request_conv_delete_m)))) && (((*result).api_request_choice = api_request_request_conv_delete_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_conv_history(state, (&(*result).api_request_request_conv_history_m)))) && (((*result).api_request_choice = api_request_request_conv_history_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_member_invite(state, (&(*result).api_request_request_member_invite_m)))) && (((*result).api_request_choice = api_request_request_member_invite_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_member_remove(state, (&(*result).api_request_request_member_remove_m)))) && (((*result).api_request_choice = api_request_request_member_remove_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_member_ban(state, (&(*result).api_request_request_member_ban_m)))) && (((*result).api_request_choice = api_request_request_member_ban_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_member_set_role(state, (&(*result).api_request_request_member_set_role_m)))) && (((*result).api_request_choice = api_request_request_member_set_role_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_contact_get_profile(state, (&(*result).api_request_request_contact_get_profile_m)))) && (((*result).api_request_choice = api_request_request_contact_get_profile_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_contact_set_alias(state, (&(*result).api_request_request_contact_set_alias_m)))) && (((*result).api_request_choice = api_request_request_contact_set_alias_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_contact_action_menu(state, (&(*result).api_request_request_contact_action_menu_m)))) && (((*result).api_request_choice = api_request_request_contact_action_menu_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_directory_search(state, (&(*result).api_request_request_directory_search_m)))) && (((*result).api_request_choice = api_request_request_directory_search_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsRoots", tmp_str.len = sizeof("FsRoots") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_fs_roots_m_c), true)))
	|| (((decode_request_fs_list(state, (&(*result).api_request_request_fs_list_m)))) && (((*result).api_request_choice = api_request_request_fs_list_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_fs_stat(state, (&(*result).api_request_request_fs_stat_m)))) && (((*result).api_request_choice = api_request_request_fs_stat_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_fs_read(state, (&(*result).api_request_request_fs_read_m)))) && (((*result).api_request_choice = api_request_request_fs_read_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_fs_write(state, (&(*result).api_request_request_fs_write_m)))) && (((*result).api_request_choice = api_request_request_fs_write_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_fs_search(state, (&(*result).api_request_request_fs_search_m)))) && (((*result).api_request_choice = api_request_request_fs_search_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_fs_watch_poll(state, (&(*result).api_request_request_fs_watch_poll_m)))) && (((*result).api_request_choice = api_request_request_fs_watch_poll_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_blob_put(state, (&(*result).api_request_request_blob_put_m)))) && (((*result).api_request_choice = api_request_request_blob_put_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_blob_get(state, (&(*result).api_request_request_blob_get_m)))) && (((*result).api_request_choice = api_request_request_blob_get_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_blob_stat(state, (&(*result).api_request_request_blob_stat_m)))) && (((*result).api_request_choice = api_request_request_blob_stat_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_fs_write_from_blob(state, (&(*result).api_request_request_fs_write_from_blob_m)))) && (((*result).api_request_choice = api_request_request_fs_write_from_blob_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_user_create(state, (&(*result).api_request_request_user_create_m)))) && (((*result).api_request_choice = api_request_request_user_create_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"UserList", tmp_str.len = sizeof("UserList") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_user_list_m_c), true)))
	|| (((decode_request_user_disable(state, (&(*result).api_request_request_user_disable_m)))) && (((*result).api_request_choice = api_request_request_user_disable_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_user_set_roles(state, (&(*result).api_request_request_user_set_roles_m)))) && (((*result).api_request_choice = api_request_request_user_set_roles_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_user_set_password(state, (&(*result).api_request_request_user_set_password_m)))) && (((*result).api_request_choice = api_request_request_user_set_password_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"RoleList", tmp_str.len = sizeof("RoleList") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_role_list_m_c), true)))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"WhoAmI", tmp_str.len = sizeof("WhoAmI") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_who_am_i_m_c), true))
	|| (((decode_request_session_revoke(state, (&(*result).api_request_request_session_revoke_m)))) && (((*result).api_request_choice = api_request_request_session_revoke_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_resource_grant_create(state, (&(*result).api_request_request_resource_grant_create_m)))) && (((*result).api_request_choice = api_request_request_resource_grant_create_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_resource_grant_list(state, (&(*result).api_request_request_resource_grant_list_m)))) && (((*result).api_request_choice = api_request_request_resource_grant_list_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_resource_grant_revoke(state, (&(*result).api_request_request_resource_grant_revoke_m)))) && (((*result).api_request_choice = api_request_request_resource_grant_revoke_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}



int cbor_decode_api_request(
		const uint8_t *payload, size_t payload_len,
		struct api_request_r *result,
		size_t *payload_len_out)
{
	zcbor_state_t states[12];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
		(zcbor_decoder_t *)decode_api_request, sizeof(states) / sizeof(zcbor_state_t), 1);
}


int cbor_decode_api_response(
		const uint8_t *payload, size_t payload_len,
		struct api_response_r *result,
		size_t *payload_len_out)
{
	zcbor_state_t states[18];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
		(zcbor_decoder_t *)decode_api_response, sizeof(states) / sizeof(zcbor_state_t), 1);
}
