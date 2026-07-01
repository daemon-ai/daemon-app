/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 64
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_encode.h"
#include "daemon_api_client_encode.h"
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

static bool encode_content_hash(zcbor_state_t *state, const struct content_hash *input);
static bool encode_repeated_blob_ref_name(zcbor_state_t *state, const struct blob_ref_name_r *input);
static bool encode_repeated_blob_ref_mime(zcbor_state_t *state, const struct blob_ref_mime_r *input);
static bool encode_blob_ref(zcbor_state_t *state, const struct blob_ref *input);
static bool encode_repeated_user_msg_attachments(zcbor_state_t *state, const struct user_msg_attachments_r *input);
static bool encode_user_msg(zcbor_state_t *state, const struct user_msg *input);
static bool encode_agent_command_start_turn(zcbor_state_t *state, const struct agent_command_start_turn *input);
static bool encode_agent_command_steer(zcbor_state_t *state, const struct agent_command_steer *input);
static bool encode_agent_command_observe(zcbor_state_t *state, const struct agent_command_observe *input);
static bool encode_agent_command_interrupt(zcbor_state_t *state, const struct agent_command_interrupt *input);
static bool encode_agent_command_snapshot(zcbor_state_t *state, const struct agent_command_snapshot *input);
static bool encode_rewind_anchor_user_turn(zcbor_state_t *state, const struct rewind_anchor_user_turn *input);
static bool encode_rewind_anchor_reply_after(zcbor_state_t *state, const struct rewind_anchor_reply_after *input);
static bool encode_rewind_anchor_cursor(zcbor_state_t *state, const struct rewind_anchor_cursor *input);
static bool encode_rewind_anchor(zcbor_state_t *state, const struct rewind_anchor_r *input);
static bool encode_agent_command_rewind_to(zcbor_state_t *state, const struct agent_command_rewind_to *input);
static bool encode_agent_command(zcbor_state_t *state, const struct agent_command_r *input);
static bool encode_origin_scope_dm(zcbor_state_t *state, const struct origin_scope_dm *input);
static bool encode_origin_scope_group(zcbor_state_t *state, const struct origin_scope_group *input);
static bool encode_origin_scope_api(zcbor_state_t *state, const struct origin_scope_api *input);
static bool encode_origin_scope_t(zcbor_state_t *state, const struct origin_scope_t_r *input);
static bool encode_origin(zcbor_state_t *state, const struct origin *input);
static bool encode_repeated_Submit_origin(zcbor_state_t *state, const struct Submit_origin_r *input);
static bool encode_repeated_Submit_profile(zcbor_state_t *state, const struct Submit_profile_r *input);
static bool encode_request_submit(zcbor_state_t *state, const struct request_submit *input);
static bool encode_request_submit_routed(zcbor_state_t *state, const struct request_submit_routed *input);
static bool encode_request_poll(zcbor_state_t *state, const struct request_poll *input);
static bool encode_host_response_body_approved(zcbor_state_t *state, const struct host_response_body_approved *input);
static bool encode_host_response_body_input(zcbor_state_t *state, const struct host_response_body_input *input);
static bool encode_host_response_body_chosen(zcbor_state_t *state, const struct host_response_body_chosen *input);
static bool encode_host_response_body_delegated(zcbor_state_t *state, const struct host_response_body_delegated *input);
static bool encode_host_response_body_spawned(zcbor_state_t *state, const struct host_response_body_spawned *input);
static bool encode_host_response_body_deferred(zcbor_state_t *state, const struct host_response_body_deferred *input);
static bool encode_host_response_body_t(zcbor_state_t *state, const struct host_response_body_t_r *input);
static bool encode_host_response(zcbor_state_t *state, const struct host_response *input);
static bool encode_request_respond(zcbor_state_t *state, const struct request_respond *input);
static bool encode_request_assign(zcbor_state_t *state, const struct request_assign *input);
static bool encode_request_cancel(zcbor_state_t *state, const struct request_cancel *input);
static bool encode_request_unit(zcbor_state_t *state, const struct request_unit *input);
static bool encode_request_unit_events(zcbor_state_t *state, const struct request_unit_events *input);
static bool encode_request_unit_outbound(zcbor_state_t *state, const struct request_unit_outbound *input);
static bool encode_request_session_history(zcbor_state_t *state, const struct request_session_history *input);
static bool encode_request_subscribe(zcbor_state_t *state, const struct request_subscribe *input);
static bool encode_repeated_EventsSince_wait_ms(zcbor_state_t *state, const struct EventsSince_wait_ms_r *input);
static bool encode_request_events_since(zcbor_state_t *state, const struct request_events_since *input);
static bool encode_request_delivery_targets(zcbor_state_t *state, const struct request_delivery_targets *input);
static bool encode_request_delivery_sessions(zcbor_state_t *state, const struct request_delivery_sessions *input);
static bool encode_sink_kind(zcbor_state_t *state, const struct sink_kind_r *input);
static bool encode_delivery_target(zcbor_state_t *state, const struct delivery_target *input);
static bool encode_request_handover(zcbor_state_t *state, const struct request_handover *input);
static bool encode_record_meta_args(zcbor_state_t *state, const struct record_meta_args *input);
static bool encode_request_record_meta(zcbor_state_t *state, const struct request_record_meta *input);
static bool encode_request_unit_history(zcbor_state_t *state, const struct request_unit_history *input);
static bool encode_request_pause(zcbor_state_t *state, const struct request_pause *input);
static bool encode_request_resume(zcbor_state_t *state, const struct request_resume *input);
static bool encode_request_scale(zcbor_state_t *state, const struct request_scale *input);
static bool encode_provider_selector(zcbor_state_t *state, const struct provider_selector_r *input);
static bool encode_repeated_SetSessionModel_provider(zcbor_state_t *state, const struct SetSessionModel_provider_r *input);
static bool encode_request_set_session_model(zcbor_state_t *state, const struct request_set_session_model *input);
static bool encode_approval_mode(zcbor_state_t *state, const struct approval_mode_r *input);
static bool encode_request_set_session_mode(zcbor_state_t *state, const struct request_set_session_mode *input);
static bool encode_repeated_session_overlay_model(zcbor_state_t *state, const struct session_overlay_model_r *input);
static bool encode_repeated_session_overlay_provider(zcbor_state_t *state, const struct session_overlay_provider_r *input);
static bool encode_tools_override_allowlist(zcbor_state_t *state, const struct tools_override_allowlist *input);
static bool encode_tools_override(zcbor_state_t *state, const struct tools_override_r *input);
static bool encode_repeated_session_overlay_tool_allowlist(zcbor_state_t *state, const struct session_overlay_tool_allowlist *input);
static bool encode_repeated_session_overlay_approval_mode(zcbor_state_t *state, const struct session_overlay_approval_mode_r *input);
static bool encode_workspace_binding_bound(zcbor_state_t *state, const struct workspace_binding_bound *input);
static bool encode_workspace_binding(zcbor_state_t *state, const struct workspace_binding_r *input);
static bool encode_repeated_session_overlay_workspace(zcbor_state_t *state, const struct session_overlay_workspace_r *input);
static bool encode_session_overlay(zcbor_state_t *state, const struct session_overlay *input);
static bool encode_request_set_session_overlay(zcbor_state_t *state, const struct request_set_session_overlay *input);
static bool encode_repeated_ApprovalsPending_session(zcbor_state_t *state, const struct ApprovalsPending_session_r *input);
static bool encode_request_approvals_pending(zcbor_state_t *state, const struct request_approvals_pending *input);
static bool encode_request_approval_decide(zcbor_state_t *state, const struct request_approval_decide *input);
static bool encode_request_profile_get(zcbor_state_t *state, const struct request_profile_get *input);
static bool encode_budget(zcbor_state_t *state, const struct budget *input);
static bool encode_engine_tunables(zcbor_state_t *state, const struct engine_tunables *input);
static bool encode_context_engine_sel(zcbor_state_t *state, const struct context_engine_sel_r *input);
static bool encode_memory_provider_sel(zcbor_state_t *state, const struct memory_provider_sel_r *input);
static bool encode_bound_account(zcbor_state_t *state, const struct bound_account *input);
static bool encode_profile_spec(zcbor_state_t *state, const struct profile_spec *input);
static bool encode_request_profile_create(zcbor_state_t *state, const struct request_profile_create *input);
static bool encode_request_profile_update(zcbor_state_t *state, const struct request_profile_update *input);
static bool encode_request_profile_delete(zcbor_state_t *state, const struct request_profile_delete *input);
static bool encode_request_profile_select(zcbor_state_t *state, const struct request_profile_select *input);
static bool encode_request_profile_clone(zcbor_state_t *state, const struct request_profile_clone *input);
static bool encode_request_profile_export(zcbor_state_t *state, const struct request_profile_export *input);
static bool encode_repeated_files_tstrtstr(zcbor_state_t *state, const struct files_tstrtstr *input);
static bool encode_skill_bundle(zcbor_state_t *state, const struct skill_bundle *input);
static bool encode_repeated_distribution_head_seq(zcbor_state_t *state, const struct distribution_head_seq_r *input);
static bool encode_repeated_distribution_source(zcbor_state_t *state, const struct distribution_source_r *input);
static bool encode_distribution(zcbor_state_t *state, const struct distribution *input);
static bool encode_repeated_ProfileImport_new_id(zcbor_state_t *state, const struct ProfileImport_new_id_r *input);
static bool encode_request_profile_import(zcbor_state_t *state, const struct request_profile_import *input);
static bool encode_request_profile_history(zcbor_state_t *state, const struct request_profile_history *input);
static bool encode_request_profile_at(zcbor_state_t *state, const struct request_profile_at *input);
static bool encode_request_profile_revert(zcbor_state_t *state, const struct request_profile_revert *input);
static bool encode_request_skill_history(zcbor_state_t *state, const struct request_skill_history *input);
static bool encode_request_skill_at(zcbor_state_t *state, const struct request_skill_at *input);
static bool encode_request_skill_revert(zcbor_state_t *state, const struct request_skill_revert *input);
static bool encode_repeated_CuratorList_profile(zcbor_state_t *state, const struct CuratorList_profile_r *input);
static bool encode_request_curator_list(zcbor_state_t *state, const struct request_curator_list *input);
static bool encode_repeated_CuratorPin_profile(zcbor_state_t *state, const struct CuratorPin_profile_r *input);
static bool encode_request_curator_pin(zcbor_state_t *state, const struct request_curator_pin *input);
static bool encode_repeated_CuratorUnpin_profile(zcbor_state_t *state, const struct CuratorUnpin_profile_r *input);
static bool encode_request_curator_unpin(zcbor_state_t *state, const struct request_curator_unpin *input);
static bool encode_repeated_CuratorArchive_profile(zcbor_state_t *state, const struct CuratorArchive_profile_r *input);
static bool encode_request_curator_archive(zcbor_state_t *state, const struct request_curator_archive *input);
static bool encode_repeated_CuratorRestore_profile(zcbor_state_t *state, const struct CuratorRestore_profile_r *input);
static bool encode_request_curator_restore(zcbor_state_t *state, const struct request_curator_restore *input);
static bool encode_repeated_CuratorRun_profile(zcbor_state_t *state, const struct CuratorRun_profile_r *input);
static bool encode_request_curator_run(zcbor_state_t *state, const struct request_curator_run *input);
static bool encode_request_credential_set(zcbor_state_t *state, const struct request_credential_set *input);
static bool encode_request_credential_remove(zcbor_state_t *state, const struct request_credential_remove *input);
static bool encode_model_engine(zcbor_state_t *state, const struct model_engine_r *input);
static bool encode_search_sort(zcbor_state_t *state, const struct search_sort_r *input);
static bool encode_search_query(zcbor_state_t *state, const struct search_query *input);
static bool encode_request_model_search(zcbor_state_t *state, const struct request_model_search *input);
static bool encode_request_model_files(zcbor_state_t *state, const struct request_model_files *input);
static bool encode_model_source_hf(zcbor_state_t *state, const struct model_source_hf *input);
static bool encode_model_source_local(zcbor_state_t *state, const struct model_source_local *input);
static bool encode_model_source(zcbor_state_t *state, const struct model_source_r *input);
static bool encode_model_ref(zcbor_state_t *state, const struct model_ref *input);
static bool encode_request_model_download(zcbor_state_t *state, const struct request_model_download *input);
static bool encode_request_model_cancel(zcbor_state_t *state, const struct request_model_cancel *input);
static bool encode_request_model_pause(zcbor_state_t *state, const struct request_model_pause *input);
static bool encode_request_model_resume(zcbor_state_t *state, const struct request_model_resume *input);
static bool encode_request_model_delete(zcbor_state_t *state, const struct request_model_delete *input);
static bool encode_request_model_activate(zcbor_state_t *state, const struct request_model_activate *input);
static bool encode_model_recommend_args(zcbor_state_t *state, const struct model_recommend_args *input);
static bool encode_request_model_recommend(zcbor_state_t *state, const struct request_model_recommend *input);
static bool encode_model_quantize_args(zcbor_state_t *state, const struct model_quantize_args *input);
static bool encode_request_model_quantize(zcbor_state_t *state, const struct request_model_quantize *input);
static bool encode_request_model_inspect(zcbor_state_t *state, const struct request_model_inspect *input);
static bool encode_request_model_current(zcbor_state_t *state, const struct request_model_current *input);
static bool encode_repeated_params_tstrtstr(zcbor_state_t *state, const struct params_tstrtstr *input);
static bool encode_auth_bind_request(zcbor_state_t *state, const struct auth_bind_request *input);
static bool encode_repeated_auth_begin_request_bind(zcbor_state_t *state, const struct auth_begin_request_bind_r *input);
static bool encode_auth_begin_request(zcbor_state_t *state, const struct auth_begin_request *input);
static bool encode_request_auth_begin(zcbor_state_t *state, const struct request_auth_begin *input);
static bool encode_auth_complete_request(zcbor_state_t *state, const struct auth_complete_request *input);
static bool encode_request_auth_complete(zcbor_state_t *state, const struct request_auth_complete *input);
static bool encode_request_auth_cancel(zcbor_state_t *state, const struct request_auth_cancel *input);
static bool encode_repeated_CheckpointList_session(zcbor_state_t *state, const struct CheckpointList_session_r *input);
static bool encode_request_checkpoint_list(zcbor_state_t *state, const struct request_checkpoint_list *input);
static bool encode_request_checkpoint_rewind(zcbor_state_t *state, const struct request_checkpoint_rewind *input);
static bool encode_session_scope_by_profile(zcbor_state_t *state, const struct session_scope_by_profile *input);
static bool encode_session_scope_by_transport(zcbor_state_t *state, const struct session_scope_by_transport *input);
static bool encode_session_scope(zcbor_state_t *state, const struct session_scope_r *input);
static bool encode_repeated_session_query_scope(zcbor_state_t *state, const struct session_query_scope *input);
static bool encode_repeated_session_query_after(zcbor_state_t *state, const struct session_query_after_r *input);
static bool encode_repeated_session_query_limit(zcbor_state_t *state, const struct session_query_limit *input);
static bool encode_repeated_session_query_since_rev(zcbor_state_t *state, const struct session_query_since_rev_r *input);
static bool encode_session_query(zcbor_state_t *state, const struct session_query *input);
static bool encode_request_sessions_query(zcbor_state_t *state, const struct request_sessions_query *input);
static bool encode_request_session_get(zcbor_state_t *state, const struct request_session_get *input);
static bool encode_request_session_search(zcbor_state_t *state, const struct request_session_search *input);
static bool encode_repeated_session_meta_patch_title(zcbor_state_t *state, const struct session_meta_patch_title_r *input);
static bool encode_repeated_session_meta_patch_pinned(zcbor_state_t *state, const struct session_meta_patch_pinned_r *input);
static bool encode_repeated_session_meta_patch_archived(zcbor_state_t *state, const struct session_meta_patch_archived_r *input);
static bool encode_session_meta_patch(zcbor_state_t *state, const struct session_meta_patch *input);
static bool encode_request_session_update_meta(zcbor_state_t *state, const struct request_session_update_meta *input);
static bool encode_repeated_rewind_point_restore_workspace(zcbor_state_t *state, const struct rewind_point_restore_workspace *input);
static bool encode_rewind_point(zcbor_state_t *state, const struct rewind_point *input);
static bool encode_request_rewind(zcbor_state_t *state, const struct request_rewind *input);
static bool encode_repeated_acp_recipe_program(zcbor_state_t *state, const struct acp_recipe_program_r *input);
static bool encode_repeated_acp_recipe_args(zcbor_state_t *state, const struct acp_recipe_args_r *input);
static bool encode_kv_pair(zcbor_state_t *state, const struct kv_pair *input);
static bool encode_repeated_acp_recipe_env(zcbor_state_t *state, const struct acp_recipe_env_r *input);
static bool encode_repeated_acp_recipe_endpoint(zcbor_state_t *state, const struct acp_recipe_endpoint_r *input);
static bool encode_acp_recipe(zcbor_state_t *state, const struct acp_recipe *input);
static bool encode_acp_source(zcbor_state_t *state, const struct acp_source_r *input);
static bool encode_repeated_acp_agent_entry_installed(zcbor_state_t *state, const struct acp_agent_entry_installed *input);
static bool encode_repeated_acp_agent_entry_version(zcbor_state_t *state, const struct acp_agent_entry_version_r *input);
static bool encode_repeated_acp_agent_entry_capabilities(zcbor_state_t *state, const struct acp_agent_entry_capabilities_r *input);
static bool encode_acp_agent_entry(zcbor_state_t *state, const struct acp_agent_entry *input);
static bool encode_request_acp_register(zcbor_state_t *state, const struct request_acp_register *input);
static bool encode_request_acp_remove(zcbor_state_t *state, const struct request_acp_remove *input);
static bool encode_request_skill_get(zcbor_state_t *state, const struct request_skill_get *input);
static bool encode_request_skill_put(zcbor_state_t *state, const struct request_skill_put *input);
static bool encode_repeated_provider_info_base_url(zcbor_state_t *state, const struct provider_info_base_url_r *input);
static bool encode_repeated_provider_info_available(zcbor_state_t *state, const struct provider_info_available *input);
static bool encode_provider_info(zcbor_state_t *state, const struct provider_info *input);
static bool encode_request_provider_register(zcbor_state_t *state, const struct request_provider_register *input);
static bool encode_repeated_tool_info_description(zcbor_state_t *state, const struct tool_info_description_r *input);
static bool encode_tool_info(zcbor_state_t *state, const struct tool_info *input);
static bool encode_request_tool_register(zcbor_state_t *state, const struct request_tool_register *input);
static bool encode_command_invocation(zcbor_state_t *state, const struct command_invocation *input);
static bool encode_request_command_invoke(zcbor_state_t *state, const struct request_command_invoke *input);
static bool encode_node_config_view(zcbor_state_t *state, const struct node_config_view *input);
static bool encode_request_config_set(zcbor_state_t *state, const struct request_config_set *input);
static bool encode_repeated_cron_spec_target(zcbor_state_t *state, const struct cron_spec_target_r *input);
static bool encode_repeated_cron_spec_payload(zcbor_state_t *state, const struct cron_spec_payload *input);
static bool encode_repeated_cron_spec_enabled(zcbor_state_t *state, const struct cron_spec_enabled *input);
static bool encode_repeated_cron_spec_timezone(zcbor_state_t *state, const struct cron_spec_timezone_r *input);
static bool encode_repeated_cron_spec_repeat(zcbor_state_t *state, const struct cron_spec_repeat_r *input);
static bool encode_repeated_cron_spec_jitter_secs(zcbor_state_t *state, const struct cron_spec_jitter_secs_r *input);
static bool encode_overlap_policy(zcbor_state_t *state, const struct overlap_policy_r *input);
static bool encode_repeated_cron_spec_overlap(zcbor_state_t *state, const struct cron_spec_overlap *input);
static bool encode_catch_up_policy(zcbor_state_t *state, const struct catch_up_policy_r *input);
static bool encode_repeated_cron_spec_catch_up(zcbor_state_t *state, const struct cron_spec_catch_up *input);
static bool encode_repeated_cron_spec_script(zcbor_state_t *state, const struct cron_spec_script_r *input);
static bool encode_repeated_cron_spec_no_agent(zcbor_state_t *state, const struct cron_spec_no_agent *input);
static bool encode_repeated_cron_spec_context_from(zcbor_state_t *state, const struct cron_spec_context_from_r *input);
static bool encode_repeated_cron_spec_deliver(zcbor_state_t *state, const struct cron_spec_deliver_r *input);
static bool encode_repeated_cron_spec_enabled_toolsets(zcbor_state_t *state, const struct cron_spec_enabled_toolsets_r *input);
static bool encode_repeated_cron_spec_workdir(zcbor_state_t *state, const struct cron_spec_workdir_r *input);
static bool encode_repeated_cron_spec_model(zcbor_state_t *state, const struct cron_spec_model_r *input);
static bool encode_repeated_cron_spec_provider(zcbor_state_t *state, const struct cron_spec_provider_r *input);
static bool encode_repeated_cron_spec_skills(zcbor_state_t *state, const struct cron_spec_skills_r *input);
static bool encode_repeated_cron_spec_origin(zcbor_state_t *state, const struct cron_spec_origin_r *input);
static bool encode_cron_spec(zcbor_state_t *state, const struct cron_spec *input);
static bool encode_request_cron_create(zcbor_state_t *state, const struct request_cron_create *input);
static bool encode_request_cron_update(zcbor_state_t *state, const struct request_cron_update *input);
static bool encode_request_cron_delete(zcbor_state_t *state, const struct request_cron_delete *input);
static bool encode_request_cron_trigger(zcbor_state_t *state, const struct request_cron_trigger *input);
static bool encode_request_cron_runs(zcbor_state_t *state, const struct request_cron_runs *input);
static bool encode_request_cron_pause(zcbor_state_t *state, const struct request_cron_pause *input);
static bool encode_request_cron_accept_suggestion(zcbor_state_t *state, const struct request_cron_accept_suggestion *input);
static bool encode_request_cron_dismiss_suggestion(zcbor_state_t *state, const struct request_cron_dismiss_suggestion *input);
static bool encode_request_routing_get(zcbor_state_t *state, const struct request_routing_get *input);
static bool encode_repeated_chat_route_profile(zcbor_state_t *state, const struct chat_route_profile_r *input);
static bool encode_isolation_policy(zcbor_state_t *state, const struct isolation_policy_r *input);
static bool encode_repeated_chat_route_isolation(zcbor_state_t *state, const struct chat_route_isolation *input);
static bool encode_chat_route(zcbor_state_t *state, const struct chat_route *input);
static bool encode_request_routing_set(zcbor_state_t *state, const struct request_routing_set *input);
static bool encode_repeated_RoutingBindChat_profile(zcbor_state_t *state, const struct RoutingBindChat_profile_r *input);
static bool encode_request_routing_bind_chat(zcbor_state_t *state, const struct request_routing_bind_chat *input);
static bool encode_request_routing_unbind_chat(zcbor_state_t *state, const struct request_routing_unbind_chat *input);
static bool encode_request_transport_rooms(zcbor_state_t *state, const struct request_transport_rooms *input);
static bool encode_request_conv_list(zcbor_state_t *state, const struct request_conv_list *input);
static bool encode_request_conv_get(zcbor_state_t *state, const struct request_conv_get *input);
static bool encode_request_conv_create_details(zcbor_state_t *state, const struct request_conv_create_details *input);
static bool encode_repeated_create_conversation_details_max_participants(zcbor_state_t *state, const struct create_conversation_details_max_participants *input);
static bool encode_repeated_contact_info_display_name(zcbor_state_t *state, const struct contact_info_display_name_r *input);
static bool encode_presence_primitive_t(zcbor_state_t *state, const struct presence_primitive_t_r *input);
static bool encode_repeated_presence_message(zcbor_state_t *state, const struct presence_message_r *input);
static bool encode_repeated_presence_emoji(zcbor_state_t *state, const struct presence_emoji_r *input);
static bool encode_repeated_presence_mobile(zcbor_state_t *state, const struct presence_mobile *input);
static bool encode_repeated_presence_idle_since(zcbor_state_t *state, const struct presence_idle_since_r *input);
static bool encode_presence(zcbor_state_t *state, const struct presence *input);
static bool encode_repeated_contact_info_presence(zcbor_state_t *state, const struct contact_info_presence *input);
static bool encode_contact_permission(zcbor_state_t *state, const struct contact_permission_r *input);
static bool encode_repeated_contact_info_permission(zcbor_state_t *state, const struct contact_info_permission *input);
static bool encode_contact_info(zcbor_state_t *state, const struct contact_info *input);
static bool encode_repeated_create_conversation_details_participants(zcbor_state_t *state, const struct create_conversation_details_participants_r *input);
static bool encode_auth_param_field(zcbor_state_t *state, const struct auth_param_field *input);
static bool encode_repeated_account_settings_schema_fields(zcbor_state_t *state, const struct account_settings_schema_fields_r *input);
static bool encode_account_settings_schema(zcbor_state_t *state, const struct account_settings_schema *input);
static bool encode_repeated_create_conversation_details_extras_schema(zcbor_state_t *state, const struct create_conversation_details_extras_schema *input);
static bool encode_repeated_values_tstrtstr(zcbor_state_t *state, const struct values_tstrtstr *input);
static bool encode_repeated_account_settings_values_values(zcbor_state_t *state, const struct account_settings_values_values_r *input);
static bool encode_account_settings_values(zcbor_state_t *state, const struct account_settings_values *input);
static bool encode_repeated_create_conversation_details_extras(zcbor_state_t *state, const struct create_conversation_details_extras *input);
static bool encode_create_conversation_details(zcbor_state_t *state, const struct create_conversation_details *input);
static bool encode_request_conv_create(zcbor_state_t *state, const struct request_conv_create *input);
static bool encode_request_conv_join_details(zcbor_state_t *state, const struct request_conv_join_details *input);
static bool encode_repeated_channel_join_details_name(zcbor_state_t *state, const struct channel_join_details_name_r *input);
static bool encode_repeated_channel_join_details_name_max_length(zcbor_state_t *state, const struct channel_join_details_name_max_length *input);
static bool encode_repeated_channel_join_details_nickname(zcbor_state_t *state, const struct channel_join_details_nickname_r *input);
static bool encode_repeated_channel_join_details_nickname_supported(zcbor_state_t *state, const struct channel_join_details_nickname_supported *input);
static bool encode_repeated_channel_join_details_nickname_max_length(zcbor_state_t *state, const struct channel_join_details_nickname_max_length *input);
static bool encode_repeated_channel_join_details_password(zcbor_state_t *state, const struct channel_join_details_password_r *input);
static bool encode_repeated_channel_join_details_password_supported(zcbor_state_t *state, const struct channel_join_details_password_supported *input);
static bool encode_repeated_channel_join_details_password_max_length(zcbor_state_t *state, const struct channel_join_details_password_max_length *input);
static bool encode_repeated_channel_join_details_extras_schema(zcbor_state_t *state, const struct channel_join_details_extras_schema *input);
static bool encode_repeated_channel_join_details_extras(zcbor_state_t *state, const struct channel_join_details_extras *input);
static bool encode_channel_join_details(zcbor_state_t *state, const struct channel_join_details *input);
static bool encode_request_conv_join(zcbor_state_t *state, const struct request_conv_join *input);
static bool encode_request_conv_leave(zcbor_state_t *state, const struct request_conv_leave *input);
static bool encode_participant_contact(zcbor_state_t *state, const struct participant_contact *input);
static bool encode_participant_agent(zcbor_state_t *state, const struct participant_agent *input);
static bool encode_participant(zcbor_state_t *state, const struct participant_r *input);
static bool encode_repeated_conv_send_args_from(zcbor_state_t *state, const struct conv_send_args_from_r *input);
static bool encode_conv_send_args(zcbor_state_t *state, const struct conv_send_args *input);
static bool encode_request_conv_send(zcbor_state_t *state, const struct request_conv_send *input);
static bool encode_repeated_ConvSetTopic_topic(zcbor_state_t *state, const struct ConvSetTopic_topic_r *input);
static bool encode_request_conv_set_topic(zcbor_state_t *state, const struct request_conv_set_topic *input);
static bool encode_repeated_ConvSetTitle_title(zcbor_state_t *state, const struct ConvSetTitle_title_r *input);
static bool encode_request_conv_set_title(zcbor_state_t *state, const struct request_conv_set_title *input);
static bool encode_repeated_ConvSetDescription_description(zcbor_state_t *state, const struct ConvSetDescription_description_r *input);
static bool encode_request_conv_set_description(zcbor_state_t *state, const struct request_conv_set_description *input);
static bool encode_request_conv_delete(zcbor_state_t *state, const struct request_conv_delete *input);
static bool encode_repeated_conv_history_args_after_cursor(zcbor_state_t *state, const struct conv_history_args_after_cursor *input);
static bool encode_repeated_conv_history_args_max(zcbor_state_t *state, const struct conv_history_args_max *input);
static bool encode_conv_history_args(zcbor_state_t *state, const struct conv_history_args *input);
static bool encode_request_conv_history(zcbor_state_t *state, const struct request_conv_history *input);
static bool encode_repeated_member_invite_args_message(zcbor_state_t *state, const struct member_invite_args_message_r *input);
static bool encode_member_invite_args(zcbor_state_t *state, const struct member_invite_args *input);
static bool encode_request_member_invite(zcbor_state_t *state, const struct request_member_invite *input);
static bool encode_repeated_member_remove_args_reason(zcbor_state_t *state, const struct member_remove_args_reason_r *input);
static bool encode_member_remove_args(zcbor_state_t *state, const struct member_remove_args *input);
static bool encode_request_member_remove(zcbor_state_t *state, const struct request_member_remove *input);
static bool encode_repeated_member_ban_args_reason(zcbor_state_t *state, const struct member_ban_args_reason_r *input);
static bool encode_member_ban_args(zcbor_state_t *state, const struct member_ban_args *input);
static bool encode_request_member_ban(zcbor_state_t *state, const struct request_member_ban *input);
static bool encode_member_role(zcbor_state_t *state, const struct member_role_r *input);
static bool encode_member_set_role_args(zcbor_state_t *state, const struct member_set_role_args *input);
static bool encode_request_member_set_role(zcbor_state_t *state, const struct request_member_set_role *input);
static bool encode_request_contact_get_profile(zcbor_state_t *state, const struct request_contact_get_profile *input);
static bool encode_repeated_ContactSetAlias_alias(zcbor_state_t *state, const struct ContactSetAlias_alias_r *input);
static bool encode_request_contact_set_alias(zcbor_state_t *state, const struct request_contact_set_alias *input);
static bool encode_request_contact_action_menu(zcbor_state_t *state, const struct request_contact_action_menu *input);
static bool encode_repeated_DirectorySearch_query(zcbor_state_t *state, const struct DirectorySearch_query_r *input);
static bool encode_request_directory_search(zcbor_state_t *state, const struct request_directory_search *input);
static bool encode_fs_root_id_host(zcbor_state_t *state, const struct fs_root_id_host *input);
static bool encode_fs_root_id_session(zcbor_state_t *state, const struct fs_root_id_session *input);
static bool encode_fs_root_id_t(zcbor_state_t *state, const struct fs_root_id_t_r *input);
static bool encode_repeated_FsList_show_ignored(zcbor_state_t *state, const struct FsList_show_ignored *input);
static bool encode_request_fs_list(zcbor_state_t *state, const struct request_fs_list *input);
static bool encode_request_fs_stat(zcbor_state_t *state, const struct request_fs_stat *input);
static bool encode_repeated_FsRead_max_bytes(zcbor_state_t *state, const struct FsRead_max_bytes *input);
static bool encode_request_fs_read(zcbor_state_t *state, const struct request_fs_read *input);
static bool encode_fs_revision(zcbor_state_t *state, const struct fs_revision *input);
static bool encode_repeated_fs_write_args_base_revision(zcbor_state_t *state, const struct fs_write_args_base_revision_r *input);
static bool encode_repeated_fs_write_args_force(zcbor_state_t *state, const struct fs_write_args_force *input);
static bool encode_fs_write_args(zcbor_state_t *state, const struct fs_write_args *input);
static bool encode_request_fs_write(zcbor_state_t *state, const struct request_fs_write *input);
static bool encode_repeated_fs_search_query_regex(zcbor_state_t *state, const struct fs_search_query_regex *input);
static bool encode_repeated_fs_search_query_case_sensitive(zcbor_state_t *state, const struct fs_search_query_case_sensitive *input);
static bool encode_repeated_fs_search_query_max_results(zcbor_state_t *state, const struct fs_search_query_max_results *input);
static bool encode_repeated_fs_search_query_page(zcbor_state_t *state, const struct fs_search_query_page *input);
static bool encode_fs_search_query(zcbor_state_t *state, const struct fs_search_query *input);
static bool encode_request_fs_search(zcbor_state_t *state, const struct request_fs_search *input);
static bool encode_fs_watch_after_args(zcbor_state_t *state, const struct fs_watch_after_args *input);
static bool encode_request_fs_watch_poll(zcbor_state_t *state, const struct request_fs_watch_poll *input);
static bool encode_request_blob_put(zcbor_state_t *state, const struct request_blob_put *input);
static bool encode_byte_range(zcbor_state_t *state, const struct byte_range *input);
static bool encode_repeated_BlobGet_range(zcbor_state_t *state, const struct BlobGet_range_r *input);
static bool encode_request_blob_get(zcbor_state_t *state, const struct request_blob_get *input);
static bool encode_request_blob_stat(zcbor_state_t *state, const struct request_blob_stat *input);
static bool encode_repeated_fs_write_from_blob_args_base_revision(zcbor_state_t *state, const struct fs_write_from_blob_args_base_revision_r *input);
static bool encode_repeated_fs_write_from_blob_args_force(zcbor_state_t *state, const struct fs_write_from_blob_args_force *input);
static bool encode_fs_write_from_blob_args(zcbor_state_t *state, const struct fs_write_from_blob_args *input);
static bool encode_request_fs_write_from_blob(zcbor_state_t *state, const struct request_fs_write_from_blob *input);
static bool encode_request_user_create(zcbor_state_t *state, const struct request_user_create *input);
static bool encode_request_user_disable(zcbor_state_t *state, const struct request_user_disable *input);
static bool encode_request_user_set_roles(zcbor_state_t *state, const struct request_user_set_roles *input);
static bool encode_request_user_set_password(zcbor_state_t *state, const struct request_user_set_password *input);
static bool encode_request_session_revoke(zcbor_state_t *state, const struct request_session_revoke *input);
static bool encode_request_resource_grant_create(zcbor_state_t *state, const struct request_resource_grant_create *input);
static bool encode_repeated_ResourceGrantList_user_id(zcbor_state_t *state, const struct ResourceGrantList_user_id_r *input);
static bool encode_request_resource_grant_list(zcbor_state_t *state, const struct request_resource_grant_list *input);
static bool encode_request_resource_grant_revoke(zcbor_state_t *state, const struct request_resource_grant_revoke *input);
static bool encode_response_routed(zcbor_state_t *state, const struct response_routed *input);
static bool encode_completion_source_process(zcbor_state_t *state, const struct completion_source_process *input);
static bool encode_completion_source_delegation(zcbor_state_t *state, const struct completion_source_delegation *input);
static bool encode_completion_source(zcbor_state_t *state, const struct completion_source_r *input);
static bool encode_turn_trigger_background(zcbor_state_t *state, const struct turn_trigger_background *input);
static bool encode_turn_trigger_scheduled(zcbor_state_t *state, const struct turn_trigger_scheduled *input);
static bool encode_turn_trigger(zcbor_state_t *state, const struct turn_trigger_r *input);
static bool encode_agent_event_turn_started(zcbor_state_t *state, const struct agent_event_turn_started *input);
static bool encode_agent_event_text_delta(zcbor_state_t *state, const struct agent_event_text_delta *input);
static bool encode_agent_event_reasoning_delta(zcbor_state_t *state, const struct agent_event_reasoning_delta *input);
static bool encode_agent_event_content_delta(zcbor_state_t *state, const struct agent_event_content_delta *input);
static bool encode_tool_detail(zcbor_state_t *state, const struct tool_detail *input);
static bool encode_repeated_tool_call_view_detail(zcbor_state_t *state, const struct tool_call_view_detail_r *input);
static bool encode_tool_call_view(zcbor_state_t *state, const struct tool_call_view *input);
static bool encode_agent_event_tool_started(zcbor_state_t *state, const struct agent_event_tool_started *input);
static bool encode_repeated_tool_result_view_detail(zcbor_state_t *state, const struct tool_result_view_detail_r *input);
static bool encode_tool_result_view(zcbor_state_t *state, const struct tool_result_view *input);
static bool encode_agent_event_tool_finished(zcbor_state_t *state, const struct agent_event_tool_finished *input);
static bool encode_usage_delta(zcbor_state_t *state, const struct usage_delta *input);
static bool encode_agent_event_usage(zcbor_state_t *state, const struct agent_event_usage *input);
static bool encode_context_status(zcbor_state_t *state, const struct context_status *input);
static bool encode_agent_event_context(zcbor_state_t *state, const struct agent_event_context *input);
static bool encode_rate_limit_snapshot(zcbor_state_t *state, const struct rate_limit_snapshot *input);
static bool encode_agent_event_rate_limit(zcbor_state_t *state, const struct agent_event_rate_limit *input);
static bool encode_end_reason(zcbor_state_t *state, const struct end_reason_r *input);
static bool encode_turn_summary(zcbor_state_t *state, const struct turn_summary *input);
static bool encode_agent_event_turn_finished(zcbor_state_t *state, const struct agent_event_turn_finished *input);
static bool encode_agent_event_error(zcbor_state_t *state, const struct agent_event_error *input);
static bool encode_agent_event_steered(zcbor_state_t *state, const struct agent_event_steered *input);
static bool encode_conv_turn_view(zcbor_state_t *state, const struct conv_turn_view *input);
static bool encode_conv_view(zcbor_state_t *state, const struct conv_view *input);
static bool encode_agent_event_snapshot(zcbor_state_t *state, const struct agent_event_snapshot *input);
static bool encode_agent_event_rewound(zcbor_state_t *state, const struct agent_event_rewound *input);
static bool encode_agent_event(zcbor_state_t *state, const struct agent_event_r *input);
static bool encode_outbound_event(zcbor_state_t *state, const struct outbound_event *input);
static bool encode_host_request_kind_approval(zcbor_state_t *state, const struct host_request_kind_approval *input);
static bool encode_host_request_kind_input(zcbor_state_t *state, const struct host_request_kind_input *input);
static bool encode_host_request_kind_choice(zcbor_state_t *state, const struct host_request_kind_choice *input);
static bool encode_host_request_kind_delegate(zcbor_state_t *state, const struct host_request_kind_delegate *input);
static bool encode_spawn_spec(zcbor_state_t *state, const struct spawn_spec *input);
static bool encode_host_request_kind_spawn(zcbor_state_t *state, const struct host_request_kind_spawn *input);
static bool encode_host_request_kind_t(zcbor_state_t *state, const struct host_request_kind_t_r *input);
static bool encode_host_request(zcbor_state_t *state, const struct host_request *input);
static bool encode_outbound_request(zcbor_state_t *state, const struct outbound_request *input);
static bool encode_outbound(zcbor_state_t *state, const struct outbound_r *input);
static bool encode_response_drained(zcbor_state_t *state, const struct response_drained *input);
static bool encode_service_health(zcbor_state_t *state, const struct service_health *input);
static bool encode_health_report(zcbor_state_t *state, const struct health_report *input);
static bool encode_response_health(zcbor_state_t *state, const struct response_health *input);
static bool encode_stats_report(zcbor_state_t *state, const struct stats_report *input);
static bool encode_response_stats(zcbor_state_t *state, const struct response_stats *input);
static bool encode_telemetry_dump(zcbor_state_t *state, const struct telemetry_dump *input);
static bool encode_response_telemetry(zcbor_state_t *state, const struct response_telemetry *input);
static bool encode_session_state_suspended(zcbor_state_t *state, const struct session_state_suspended *input);
static bool encode_session_state(zcbor_state_t *state, const struct session_state_r *input);
static bool encode_repeated_session_info_rewindable(zcbor_state_t *state, const struct session_info_rewindable *input);
static bool encode_repeated_session_info_bound_profile(zcbor_state_t *state, const struct session_info_bound_profile_r *input);
static bool encode_repeated_session_info_title(zcbor_state_t *state, const struct session_info_title_r *input);
static bool encode_repeated_session_info_last_activity_ms(zcbor_state_t *state, const struct session_info_last_activity_ms_r *input);
static bool encode_lifecycle(zcbor_state_t *state, const struct lifecycle_r *input);
static bool encode_repeated_session_info_lifecycle(zcbor_state_t *state, const struct session_info_lifecycle *input);
static bool encode_session_role(zcbor_state_t *state, const struct session_role_r *input);
static bool encode_repeated_session_info_role(zcbor_state_t *state, const struct session_info_role *input);
static bool encode_repeated_session_info_parent(zcbor_state_t *state, const struct session_info_parent_r *input);
static bool encode_repeated_session_info_pinned(zcbor_state_t *state, const struct session_info_pinned *input);
static bool encode_repeated_session_info_archived(zcbor_state_t *state, const struct session_info_archived *input);
static bool encode_session_info(zcbor_state_t *state, const struct session_info *input);
static bool encode_response_sessions(zcbor_state_t *state, const struct response_sessions *input);
static bool encode_repeated_approval_info_path(zcbor_state_t *state, const struct approval_info_path_r *input);
static bool encode_approval_info(zcbor_state_t *state, const struct approval_info *input);
static bool encode_response_approvals(zcbor_state_t *state, const struct response_approvals *input);
static bool encode_fleet_report(zcbor_state_t *state, const struct fleet_report *input);
static bool encode_response_fleet(zcbor_state_t *state, const struct response_fleet *input);
static bool encode_unit_kind(zcbor_state_t *state, const struct unit_kind_r *input);
static bool encode_unit_state_finished(zcbor_state_t *state, const struct unit_state_finished *input);
static bool encode_unit_state(zcbor_state_t *state, const struct unit_state_r *input);
static bool encode_repeated_unit_node_profile(zcbor_state_t *state, const struct unit_node_profile_r *input);
static bool encode_repeated_unit_node_session(zcbor_state_t *state, const struct unit_node_session_r *input);
static bool encode_repeated_unit_node_title(zcbor_state_t *state, const struct unit_node_title_r *input);
static bool encode_repeated_unit_node_role(zcbor_state_t *state, const struct unit_node_role_r *input);
static bool encode_unit_node(zcbor_state_t *state, const struct unit_node *input);
static bool encode_tree_report(zcbor_state_t *state, const struct tree_report *input);
static bool encode_response_tree(zcbor_state_t *state, const struct response_tree *input);
static bool encode_response_unit(zcbor_state_t *state, const struct response_unit *input);
static bool encode_manage_event_view_started(zcbor_state_t *state, const struct manage_event_view_started *input);
static bool encode_manage_event_view_progress(zcbor_state_t *state, const struct manage_event_view_progress *input);
static bool encode_manage_event_view_usage(zcbor_state_t *state, const struct manage_event_view_usage *input);
static bool encode_manage_event_view_finished(zcbor_state_t *state, const struct manage_event_view_finished *input);
static bool encode_manage_event_view_error(zcbor_state_t *state, const struct manage_event_view_error *input);
static bool encode_subagent_phase(zcbor_state_t *state, const struct subagent_phase_r *input);
static bool encode_manage_event_view_subagent(zcbor_state_t *state, const struct manage_event_view_subagent *input);
static bool encode_manage_event_view(zcbor_state_t *state, const struct manage_event_view_r *input);
static bool encode_response_unit_events(zcbor_state_t *state, const struct response_unit_events *input);
static bool encode_journal_record_payload_management(zcbor_state_t *state, const struct journal_record_payload_management *input);
static bool encode_transcript_role(zcbor_state_t *state, const struct transcript_role_r *input);
static bool encode_transcript_block_message(zcbor_state_t *state, const struct transcript_block_message *input);
static bool encode_repeated_ToolCall_detail(zcbor_state_t *state, const struct ToolCall_detail_r *input);
static bool encode_transcript_block_tool_call(zcbor_state_t *state, const struct transcript_block_tool_call *input);
static bool encode_repeated_ToolResult_detail(zcbor_state_t *state, const struct ToolResult_detail_r *input);
static bool encode_transcript_block_tool_result(zcbor_state_t *state, const struct transcript_block_tool_result *input);
static bool encode_transcript_block_request(zcbor_state_t *state, const struct transcript_block_request *input);
static bool encode_transcript_block_content(zcbor_state_t *state, const struct transcript_block_content *input);
static bool encode_transcript_block(zcbor_state_t *state, const struct transcript_block_r *input);
static bool encode_journal_record_payload_block(zcbor_state_t *state, const struct journal_record_payload_block *input);
static bool encode_journal_record_payload_t(zcbor_state_t *state, const struct journal_record_payload_t_r *input);
static bool encode_journal_record(zcbor_state_t *state, const struct journal_record *input);
static bool encode_journal_page_view(zcbor_state_t *state, const struct journal_page_view *input);
static bool encode_response_journal(zcbor_state_t *state, const struct response_journal *input);
static bool encode_direction(zcbor_state_t *state, const struct direction_r *input);
static bool encode_disposition(zcbor_state_t *state, const struct disposition_r *input);
static bool encode_session_payload_event(zcbor_state_t *state, const struct session_payload_event *input);
static bool encode_session_payload_request(zcbor_state_t *state, const struct session_payload_request *input);
static bool encode_session_payload_command(zcbor_state_t *state, const struct session_payload_command *input);
static bool encode_session_payload_response(zcbor_state_t *state, const struct session_payload_response *input);
static bool encode_session_payload_meta(zcbor_state_t *state, const struct session_payload_meta *input);
static bool encode_session_payload(zcbor_state_t *state, const struct session_payload_r *input);
static bool encode_session_log_entry(zcbor_state_t *state, const struct session_log_entry *input);
static bool encode_log_page_view(zcbor_state_t *state, const struct log_page_view *input);
static bool encode_response_log_page(zcbor_state_t *state, const struct response_log_page *input);
static bool encode_node_event_session_advanced(zcbor_state_t *state, const struct node_event_session_advanced *input);
static bool encode_node_event_session_meta_changed(zcbor_state_t *state, const struct node_event_session_meta_changed *input);
static bool encode_node_event_roster_changed(zcbor_state_t *state, const struct node_event_roster_changed *input);
static bool encode_node_event_fleet_changed(zcbor_state_t *state, const struct node_event_fleet_changed *input);
static bool encode_node_event_approval_pending(zcbor_state_t *state, const struct node_event_approval_pending *input);
static bool encode_node_event_download_progress(zcbor_state_t *state, const struct node_event_download_progress *input);
static bool encode_node_event_resync_needed(zcbor_state_t *state, const struct node_event_resync_needed *input);
static bool encode_node_event(zcbor_state_t *state, const struct node_event_r *input);
static bool encode_events_page(zcbor_state_t *state, const struct events_page *input);
static bool encode_response_events_page(zcbor_state_t *state, const struct response_events_page *input);
static bool encode_response_delivery_targets(zcbor_state_t *state, const struct response_delivery_targets *input);
static bool encode_response_delivery_sessions(zcbor_state_t *state, const struct response_delivery_sessions *input);
static bool encode_response_verifying_key(zcbor_state_t *state, const struct response_verifying_key *input);
static bool encode_search_hit(zcbor_state_t *state, const struct search_hit *input);
static bool encode_search_page(zcbor_state_t *state, const struct search_page *input);
static bool encode_response_model_search(zcbor_state_t *state, const struct response_model_search *input);
static bool encode_model_file(zcbor_state_t *state, const struct model_file *input);
static bool encode_response_model_files(zcbor_state_t *state, const struct response_model_files *input);
static bool encode_response_model_download_started(zcbor_state_t *state, const struct response_model_download_started *input);
static bool encode_download_state(zcbor_state_t *state, const struct download_state_r *input);
static bool encode_download_status(zcbor_state_t *state, const struct download_status *input);
static bool encode_response_model_downloads(zcbor_state_t *state, const struct response_model_downloads *input);
static bool encode_repeated_installed_model_arch(zcbor_state_t *state, const struct installed_model_arch_r *input);
static bool encode_repeated_installed_model_context_length(zcbor_state_t *state, const struct installed_model_context_length_r *input);
static bool encode_repeated_installed_model_file_type(zcbor_state_t *state, const struct installed_model_file_type_r *input);
static bool encode_installed_model(zcbor_state_t *state, const struct installed_model *input);
static bool encode_response_model_catalog(zcbor_state_t *state, const struct response_model_catalog *input);
static bool encode_quant_candidate(zcbor_state_t *state, const struct quant_candidate *input);
static bool encode_quant_recommendation(zcbor_state_t *state, const struct quant_recommendation *input);
static bool encode_response_model_recommend(zcbor_state_t *state, const struct response_model_recommend *input);
static bool encode_response_model_quantize_started(zcbor_state_t *state, const struct response_model_quantize_started *input);
static bool encode_quantize_state(zcbor_state_t *state, const struct quantize_state_r *input);
static bool encode_quantize_status(zcbor_state_t *state, const struct quantize_status *input);
static bool encode_response_model_quantizes(zcbor_state_t *state, const struct response_model_quantizes *input);
static bool encode_gguf_info(zcbor_state_t *state, const struct gguf_info *input);
static bool encode_response_model_inspect(zcbor_state_t *state, const struct response_model_inspect *input);
static bool encode_profile_info(zcbor_state_t *state, const struct profile_info *input);
static bool encode_response_profiles(zcbor_state_t *state, const struct response_profiles *input);
static bool encode_response_profile(zcbor_state_t *state, const struct response_profile *input);
static bool encode_credential_info(zcbor_state_t *state, const struct credential_info *input);
static bool encode_response_credentials(zcbor_state_t *state, const struct response_credentials *input);
static bool encode_model_descriptor(zcbor_state_t *state, const struct model_descriptor *input);
static bool encode_response_models(zcbor_state_t *state, const struct response_models *input);
static bool encode_response_model_current(zcbor_state_t *state, const struct response_model_current *input);
static bool encode_response_distribution(zcbor_state_t *state, const struct response_distribution *input);
static bool encode_response_profile_id(zcbor_state_t *state, const struct response_profile_id *input);
static bool encode_author_agent(zcbor_state_t *state, const struct author_agent *input);
static bool encode_author(zcbor_state_t *state, const struct author_r *input);
static bool encode_revision(zcbor_state_t *state, const struct revision *input);
static bool encode_response_revisions(zcbor_state_t *state, const struct response_revisions *input);
static bool encode_response_skill_bundle(zcbor_state_t *state, const struct response_skill_bundle *input);
static bool encode_skill_creator(zcbor_state_t *state, const struct skill_creator_r *input);
static bool encode_skill_state(zcbor_state_t *state, const struct skill_state_r *input);
static bool encode_skill_usage(zcbor_state_t *state, const struct skill_usage *input);
static bool encode_curator_entry(zcbor_state_t *state, const struct curator_entry *input);
static bool encode_response_curator_skills(zcbor_state_t *state, const struct response_curator_skills *input);
static bool encode_curator_change(zcbor_state_t *state, const struct curator_change *input);
static bool encode_response_curator_run(zcbor_state_t *state, const struct response_curator_run *input);
static bool encode_auth_flow_kind(zcbor_state_t *state, const struct auth_flow_kind_r *input);
static bool encode_auth_begin_response(zcbor_state_t *state, const struct auth_begin_response *input);
static bool encode_response_auth_begun(zcbor_state_t *state, const struct response_auth_begun *input);
static bool encode_auth_complete_response(zcbor_state_t *state, const struct auth_complete_response *input);
static bool encode_response_auth_completed(zcbor_state_t *state, const struct response_auth_completed *input);
static bool encode_auth_provider_info(zcbor_state_t *state, const struct auth_provider_info *input);
static bool encode_response_auth_providers(zcbor_state_t *state, const struct response_auth_providers *input);
static bool encode_repeated_checkpoint_info_turn_ordinal(zcbor_state_t *state, const struct checkpoint_info_turn_ordinal_r *input);
static bool encode_repeated_checkpoint_info_cursor(zcbor_state_t *state, const struct checkpoint_info_cursor_r *input);
static bool encode_checkpoint_info(zcbor_state_t *state, const struct checkpoint_info *input);
static bool encode_response_checkpoints(zcbor_state_t *state, const struct response_checkpoints *input);
static bool encode_repeated_session_page_next_cursor(zcbor_state_t *state, const struct session_page_next_cursor_r *input);
static bool encode_repeated_session_page_removed(zcbor_state_t *state, const struct session_page_removed_r *input);
static bool encode_session_page(zcbor_state_t *state, const struct session_page *input);
static bool encode_response_session_page(zcbor_state_t *state, const struct response_session_page *input);
static bool encode_repeated_session_detail_overlay(zcbor_state_t *state, const struct session_detail_overlay_r *input);
static bool encode_repeated_session_detail_model(zcbor_state_t *state, const struct session_detail_model_r *input);
static bool encode_repeated_session_detail_delivery_targets(zcbor_state_t *state, const struct session_detail_delivery_targets_r *input);
static bool encode_repeated_session_detail_children(zcbor_state_t *state, const struct session_detail_children_r *input);
static bool encode_repeated_session_detail_checkpoints(zcbor_state_t *state, const struct session_detail_checkpoints *input);
static bool encode_session_detail(zcbor_state_t *state, const struct session_detail *input);
static bool encode_response_session_detail(zcbor_state_t *state, const struct response_session_detail *input);
static bool encode_repeated_SessionsByProfile_profile_l(zcbor_state_t *state, const struct SessionsByProfile_profile_l *input);
static bool encode_response_sessions_by_profile(zcbor_state_t *state, const struct response_sessions_by_profile *input);
static bool encode_session_search_hit(zcbor_state_t *state, const struct session_search_hit *input);
static bool encode_response_session_search(zcbor_state_t *state, const struct response_session_search *input);
static bool encode_response_acp_catalog(zcbor_state_t *state, const struct response_acp_catalog *input);
static bool encode_response_providers(zcbor_state_t *state, const struct response_providers *input);
static bool encode_response_tools(zcbor_state_t *state, const struct response_tools *input);
static bool encode_command_scope(zcbor_state_t *state, const struct command_scope_r *input);
static bool encode_command_surface(zcbor_state_t *state, const struct command_surface_r *input);
static bool encode_command_access(zcbor_state_t *state, const struct command_access_r *input);
static bool encode_command_spec(zcbor_state_t *state, const struct command_spec *input);
static bool encode_response_commands(zcbor_state_t *state, const struct response_commands *input);
static bool encode_command_output(zcbor_state_t *state, const struct command_output *input);
static bool encode_response_command_output(zcbor_state_t *state, const struct response_command_output *input);
static bool encode_response_config(zcbor_state_t *state, const struct response_config *input);
static bool encode_repeated_cron_job_next_fire_unix(zcbor_state_t *state, const struct cron_job_next_fire_unix_r *input);
static bool encode_repeated_cron_job_paused(zcbor_state_t *state, const struct cron_job_paused *input);
static bool encode_repeated_cron_job_last_run_unix(zcbor_state_t *state, const struct cron_job_last_run_unix_r *input);
static bool encode_repeated_cron_job_last_ok(zcbor_state_t *state, const struct cron_job_last_ok_r *input);
static bool encode_repeated_cron_job_last_detail(zcbor_state_t *state, const struct cron_job_last_detail_r *input);
static bool encode_repeated_cron_job_fire_count(zcbor_state_t *state, const struct cron_job_fire_count *input);
static bool encode_cron_job(zcbor_state_t *state, const struct cron_job *input);
static bool encode_response_cron_jobs(zcbor_state_t *state, const struct response_cron_jobs *input);
static bool encode_response_cron_id(zcbor_state_t *state, const struct response_cron_id *input);
static bool encode_repeated_cron_run_detail(zcbor_state_t *state, const struct cron_run_detail_r *input);
static bool encode_repeated_cron_run_finished_unix(zcbor_state_t *state, const struct cron_run_finished_unix_r *input);
static bool encode_repeated_cron_run_session(zcbor_state_t *state, const struct cron_run_session_r *input);
static bool encode_run_trigger(zcbor_state_t *state, const struct run_trigger_r *input);
static bool encode_repeated_cron_run_trigger(zcbor_state_t *state, const struct cron_run_trigger *input);
static bool encode_cron_run(zcbor_state_t *state, const struct cron_run *input);
static bool encode_response_cron_runs(zcbor_state_t *state, const struct response_cron_runs *input);
static bool encode_repeated_cron_suggestion_description(zcbor_state_t *state, const struct cron_suggestion_description *input);
static bool encode_repeated_cron_suggestion_source(zcbor_state_t *state, const struct cron_suggestion_source *input);
static bool encode_suggestion_status(zcbor_state_t *state, const struct suggestion_status_r *input);
static bool encode_repeated_cron_suggestion_status(zcbor_state_t *state, const struct cron_suggestion_status *input);
static bool encode_cron_suggestion(zcbor_state_t *state, const struct cron_suggestion *input);
static bool encode_response_cron_suggestions(zcbor_state_t *state, const struct response_cron_suggestions *input);
static bool encode_response_chat_routes(zcbor_state_t *state, const struct response_chat_routes *input);
static bool encode_response_chat_route(zcbor_state_t *state, const struct response_chat_route *input);
static bool encode_repeated_room_info_name(zcbor_state_t *state, const struct room_info_name_r *input);
static bool encode_repeated_room_info_session(zcbor_state_t *state, const struct room_info_session_r *input);
static bool encode_room_info(zcbor_state_t *state, const struct room_info *input);
static bool encode_response_rooms(zcbor_state_t *state, const struct response_rooms *input);
static bool encode_adapter_capabilities(zcbor_state_t *state, const struct adapter_capabilities *input);
static bool encode_repeated_adapter_info_account_schema(zcbor_state_t *state, const struct adapter_info_account_schema *input);
static bool encode_adapter_info(zcbor_state_t *state, const struct adapter_info *input);
static bool encode_response_adapters(zcbor_state_t *state, const struct response_adapters *input);
static bool encode_connection_state(zcbor_state_t *state, const struct connection_state_r *input);
static bool encode_repeated_transport_instance_info_connection(zcbor_state_t *state, const struct transport_instance_info_connection *input);
static bool encode_presence_state(zcbor_state_t *state, const struct presence_state_r *input);
static bool encode_repeated_transport_instance_info_presence(zcbor_state_t *state, const struct transport_instance_info_presence *input);
static bool encode_repeated_transport_instance_info_bound_profile(zcbor_state_t *state, const struct transport_instance_info_bound_profile_r *input);
static bool encode_transport_instance_info(zcbor_state_t *state, const struct transport_instance_info *input);
static bool encode_response_transport_instances(zcbor_state_t *state, const struct response_transport_instances *input);
static bool encode_conversation_type(zcbor_state_t *state, const struct conversation_type_r *input);
static bool encode_repeated_conversation_info_title(zcbor_state_t *state, const struct conversation_info_title_r *input);
static bool encode_repeated_conversation_info_topic(zcbor_state_t *state, const struct conversation_info_topic_r *input);
static bool encode_repeated_conversation_info_description(zcbor_state_t *state, const struct conversation_info_description_r *input);
static bool encode_repeated_conversation_member_alias(zcbor_state_t *state, const struct conversation_member_alias_r *input);
static bool encode_repeated_conversation_member_nickname(zcbor_state_t *state, const struct conversation_member_nickname_r *input);
static bool encode_typing_state(zcbor_state_t *state, const struct typing_state_r *input);
static bool encode_repeated_conversation_member_typing(zcbor_state_t *state, const struct conversation_member_typing *input);
static bool encode_repeated_conversation_member_role(zcbor_state_t *state, const struct conversation_member_role *input);
static bool encode_repeated_conversation_member_session(zcbor_state_t *state, const struct conversation_member_session_r *input);
static bool encode_conversation_member(zcbor_state_t *state, const struct conversation_member *input);
static bool encode_repeated_conversation_info_members(zcbor_state_t *state, const struct conversation_info_members_r *input);
static bool encode_conversation_info(zcbor_state_t *state, const struct conversation_info *input);
static bool encode_response_conversations(zcbor_state_t *state, const struct response_conversations *input);
static bool encode_response_conversation(zcbor_state_t *state, const struct response_conversation *input);
static bool encode_response_conv_create_details(zcbor_state_t *state, const struct response_conv_create_details *input);
static bool encode_response_conv_join_details(zcbor_state_t *state, const struct response_conv_join_details *input);
static bool encode_response_contact_profile(zcbor_state_t *state, const struct response_contact_profile *input);
static bool encode_response_contacts(zcbor_state_t *state, const struct response_contacts *input);
static bool encode_repeated_action_menu_items(zcbor_state_t *state, const struct action_menu_items_r *input);
static bool encode_action_menu(zcbor_state_t *state, const struct action_menu *input);
static bool encode_response_action_menu(zcbor_state_t *state, const struct response_action_menu *input);
static bool encode_api_error_unknown_session(zcbor_state_t *state, const struct api_error_unknown_session *input);
static bool encode_api_error_unsupported(zcbor_state_t *state, const struct api_error_unsupported *input);
static bool encode_api_error_conflict(zcbor_state_t *state, const struct api_error_conflict *input);
static bool encode_api_error_unauthenticated(zcbor_state_t *state, const struct api_error_unauthenticated *input);
static bool encode_api_error_forbidden(zcbor_state_t *state, const struct api_error_forbidden *input);
static bool encode_api_error_other(zcbor_state_t *state, const struct api_error_other *input);
static bool encode_api_error(zcbor_state_t *state, const struct api_error_r *input);
static bool encode_response_error(zcbor_state_t *state, const struct response_error *input);
static bool encode_fs_root_kind_t(zcbor_state_t *state, const struct fs_root_kind_t_r *input);
static bool encode_repeated_fs_root_session(zcbor_state_t *state, const struct fs_root_session_r *input);
static bool encode_fs_root(zcbor_state_t *state, const struct fs_root *input);
static bool encode_response_fs_roots(zcbor_state_t *state, const struct response_fs_roots *input);
static bool encode_fs_entry_kind_t(zcbor_state_t *state, const struct fs_entry_kind_t_r *input);
static bool encode_repeated_fs_entry_ignored(zcbor_state_t *state, const struct fs_entry_ignored *input);
static bool encode_fs_entry(zcbor_state_t *state, const struct fs_entry *input);
static bool encode_response_fs_list(zcbor_state_t *state, const struct response_fs_list *input);
static bool encode_response_fs_stat(zcbor_state_t *state, const struct response_fs_stat *input);
static bool encode_repeated_fs_content_truncated(zcbor_state_t *state, const struct fs_content_truncated *input);
static bool encode_repeated_fs_content_blob_ref(zcbor_state_t *state, const struct fs_content_blob_ref_r *input);
static bool encode_fs_content(zcbor_state_t *state, const struct fs_content *input);
static bool encode_response_fs_read(zcbor_state_t *state, const struct response_fs_read *input);
static bool encode_response_fs_write(zcbor_state_t *state, const struct response_fs_write *input);
static bool encode_fs_search_hit(zcbor_state_t *state, const struct fs_search_hit *input);
static bool encode_repeated_fs_search_page_has_more(zcbor_state_t *state, const struct fs_search_page_has_more *input);
static bool encode_fs_search_page(zcbor_state_t *state, const struct fs_search_page *input);
static bool encode_response_fs_search(zcbor_state_t *state, const struct response_fs_search *input);
static bool encode_fs_change_kind_t(zcbor_state_t *state, const struct fs_change_kind_t_r *input);
static bool encode_fs_change(zcbor_state_t *state, const struct fs_change *input);
static bool encode_fs_watch_page_view(zcbor_state_t *state, const struct fs_watch_page_view *input);
static bool encode_response_fs_watch(zcbor_state_t *state, const struct response_fs_watch *input);
static bool encode_response_blob_put(zcbor_state_t *state, const struct response_blob_put *input);
static bool encode_response_blob_get(zcbor_state_t *state, const struct response_blob_get *input);
static bool encode_blob_stat(zcbor_state_t *state, const struct blob_stat *input);
static bool encode_response_blob_stat(zcbor_state_t *state, const struct response_blob_stat *input);
static bool encode_access_user(zcbor_state_t *state, const struct access_user *input);
static bool encode_response_access_user(zcbor_state_t *state, const struct response_access_user *input);
static bool encode_response_access_users(zcbor_state_t *state, const struct response_access_users *input);
static bool encode_role_info(zcbor_state_t *state, const struct role_info *input);
static bool encode_response_access_roles(zcbor_state_t *state, const struct response_access_roles *input);
static bool encode_principal_view(zcbor_state_t *state, const struct principal_view *input);
static bool encode_response_who_am_i(zcbor_state_t *state, const struct response_who_am_i *input);
static bool encode_api_response(zcbor_state_t *state, const struct api_response_r *input);
static bool encode_api_request(zcbor_state_t *state, const struct api_request_r *input);


static bool encode_content_hash(
		zcbor_state_t *state, const struct content_hash *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).content_hash_uint_count, (zcbor_encoder_t *)zcbor_uint32_encode, state, (*&(*input).content_hash_uint), sizeof(uint32_t))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_blob_ref_name(
		zcbor_state_t *state, const struct blob_ref_name_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (((*input).blob_ref_name_choice == blob_ref_name_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).blob_ref_name_tstr))))
	: (((*input).blob_ref_name_choice == blob_ref_name_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_blob_ref_mime(
		zcbor_state_t *state, const struct blob_ref_mime_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"mime", tmp_str.len = sizeof("mime") - 1, &tmp_str)))))
	&& (((*input).blob_ref_mime_choice == blob_ref_mime_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).blob_ref_mime_tstr))))
	: (((*input).blob_ref_mime_choice == blob_ref_mime_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_blob_ref(
		zcbor_state_t *state, const struct blob_ref *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"hash", tmp_str.len = sizeof("hash") - 1, &tmp_str)))))
	&& (encode_content_hash(state, (&(*input).blob_ref_hash))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"size", tmp_str.len = sizeof("size") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).blob_ref_size))))
	&& (!(*input).blob_ref_name_present || encode_repeated_blob_ref_name(state, (&(*input).blob_ref_name)))
	&& (!(*input).blob_ref_mime_present || encode_repeated_blob_ref_mime(state, (&(*input).blob_ref_mime)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_user_msg_attachments(
		zcbor_state_t *state, const struct user_msg_attachments_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"attachments", tmp_str.len = sizeof("attachments") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).user_msg_attachments_blob_ref_m_count, (zcbor_encoder_t *)encode_blob_ref, state, (*&(*input).user_msg_attachments_blob_ref_m), sizeof(struct blob_ref))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_user_msg(
		zcbor_state_t *state, const struct user_msg *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).user_msg_text))))
	&& (!(*input).user_msg_attachments_present || encode_repeated_user_msg_attachments(state, (&(*input).user_msg_attachments)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_command_start_turn(
		zcbor_state_t *state, const struct agent_command_start_turn *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"StartTurn", tmp_str.len = sizeof("StartTurn") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"input", tmp_str.len = sizeof("input") - 1, &tmp_str)))))
	&& (encode_user_msg(state, (&(*input).StartTurn_input))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).StartTurn_request_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_command_steer(
		zcbor_state_t *state, const struct agent_command_steer *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Steer", tmp_str.len = sizeof("Steer") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Steer_text))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Steer_request_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_command_observe(
		zcbor_state_t *state, const struct agent_command_observe *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Observe", tmp_str.len = sizeof("Observe") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"input", tmp_str.len = sizeof("input") - 1, &tmp_str)))))
	&& (encode_user_msg(state, (&(*input).Observe_input))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Observe_request_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_command_interrupt(
		zcbor_state_t *state, const struct agent_command_interrupt *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Interrupt", tmp_str.len = sizeof("Interrupt") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"reason", tmp_str.len = sizeof("reason") - 1, &tmp_str)))))
	&& (((*input).Interrupt_reason_choice == Interrupt_reason_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).Interrupt_reason_tstr))))
	: (((*input).Interrupt_reason_choice == Interrupt_reason_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_command_snapshot(
		zcbor_state_t *state, const struct agent_command_snapshot *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Snapshot", tmp_str.len = sizeof("Snapshot") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Snapshot_request_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_rewind_anchor_user_turn(
		zcbor_state_t *state, const struct rewind_anchor_user_turn *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"UserTurn", tmp_str.len = sizeof("UserTurn") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ordinal", tmp_str.len = sizeof("ordinal") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).UserTurn_ordinal))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_rewind_anchor_reply_after(
		zcbor_state_t *state, const struct rewind_anchor_reply_after *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ReplyAfter", tmp_str.len = sizeof("ReplyAfter") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ordinal", tmp_str.len = sizeof("ordinal") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).ReplyAfter_ordinal))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_rewind_anchor_cursor(
		zcbor_state_t *state, const struct rewind_anchor_cursor *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Cursor", tmp_str.len = sizeof("Cursor") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Cursor_seq))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_rewind_anchor(
		zcbor_state_t *state, const struct rewind_anchor_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).rewind_anchor_choice == rewind_anchor_user_turn_m_c) ? ((encode_rewind_anchor_user_turn(state, (&(*input).rewind_anchor_user_turn_m))))
	: (((*input).rewind_anchor_choice == rewind_anchor_reply_after_m_c) ? ((encode_rewind_anchor_reply_after(state, (&(*input).rewind_anchor_reply_after_m))))
	: (((*input).rewind_anchor_choice == rewind_anchor_cursor_m_c) ? ((encode_rewind_anchor_cursor(state, (&(*input).rewind_anchor_cursor_m))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_command_rewind_to(
		zcbor_state_t *state, const struct agent_command_rewind_to *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"RewindTo", tmp_str.len = sizeof("RewindTo") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"anchor", tmp_str.len = sizeof("anchor") - 1, &tmp_str)))))
	&& (encode_rewind_anchor(state, (&(*input).RewindTo_anchor))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).RewindTo_request_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_command(
		zcbor_state_t *state, const struct agent_command_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).agent_command_choice == agent_command_start_turn_m_c) ? ((encode_agent_command_start_turn(state, (&(*input).agent_command_start_turn_m))))
	: (((*input).agent_command_choice == agent_command_steer_m_c) ? ((encode_agent_command_steer(state, (&(*input).agent_command_steer_m))))
	: (((*input).agent_command_choice == agent_command_observe_m_c) ? ((encode_agent_command_observe(state, (&(*input).agent_command_observe_m))))
	: (((*input).agent_command_choice == agent_command_interrupt_m_c) ? ((encode_agent_command_interrupt(state, (&(*input).agent_command_interrupt_m))))
	: (((*input).agent_command_choice == agent_command_snapshot_m_c) ? ((encode_agent_command_snapshot(state, (&(*input).agent_command_snapshot_m))))
	: (((*input).agent_command_choice == agent_command_rewind_to_m_c) ? ((encode_agent_command_rewind_to(state, (&(*input).agent_command_rewind_to_m))))
	: (((*input).agent_command_choice == agent_command_Shutdown_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Shutdown", tmp_str.len = sizeof("Shutdown") - 1, &tmp_str)))))
	: false)))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_origin_scope_dm(
		zcbor_state_t *state, const struct origin_scope_dm *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Dm", tmp_str.len = sizeof("Dm") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"user", tmp_str.len = sizeof("user") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Dm_user))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_origin_scope_group(
		zcbor_state_t *state, const struct origin_scope_group *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Group", tmp_str.len = sizeof("Group") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"chat", tmp_str.len = sizeof("chat") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Group_chat))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"thread", tmp_str.len = sizeof("thread") - 1, &tmp_str)))))
	&& (((*input).Group_thread_choice == Group_thread_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).Group_thread_tstr))))
	: (((*input).Group_thread_choice == Group_thread_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_origin_scope_api(
		zcbor_state_t *state, const struct origin_scope_api *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Api", tmp_str.len = sizeof("Api") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"key", tmp_str.len = sizeof("key") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Api_key))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_origin_scope_t(
		zcbor_state_t *state, const struct origin_scope_t_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).origin_scope_t_choice == origin_scope_t_origin_scope_dm_m_c) ? ((encode_origin_scope_dm(state, (&(*input).origin_scope_t_origin_scope_dm_m))))
	: (((*input).origin_scope_t_choice == origin_scope_t_origin_scope_group_m_c) ? ((encode_origin_scope_group(state, (&(*input).origin_scope_t_origin_scope_group_m))))
	: (((*input).origin_scope_t_choice == origin_scope_t_origin_scope_api_m_c) ? ((encode_origin_scope_api(state, (&(*input).origin_scope_t_origin_scope_api_m))))
	: (((*input).origin_scope_t_choice == origin_scope_t_Internal_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Internal", tmp_str.len = sizeof("Internal") - 1, &tmp_str)))))
	: false))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_origin(
		zcbor_state_t *state, const struct origin *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).origin_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"scope", tmp_str.len = sizeof("scope") - 1, &tmp_str)))))
	&& (encode_origin_scope_t(state, (&(*input).origin_scope))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_Submit_origin(
		zcbor_state_t *state, const struct Submit_origin_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (((*input).Submit_origin_choice == Submit_origin_origin_m_c) ? ((encode_origin(state, (&(*input).Submit_origin_origin_m))))
	: (((*input).Submit_origin_choice == Submit_origin_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_Submit_profile(
		zcbor_state_t *state, const struct Submit_profile_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (((*input).Submit_profile_choice == Submit_profile_profile_ref_m_c) ? ((zcbor_tstr_encode(state, (&(*input).Submit_profile_profile_ref_m))))
	: (((*input).Submit_profile_choice == Submit_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_submit(
		zcbor_state_t *state, const struct request_submit *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Submit", tmp_str.len = sizeof("Submit") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Submit_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"command", tmp_str.len = sizeof("command") - 1, &tmp_str)))))
	&& (encode_agent_command(state, (&(*input).Submit_command))))
	&& (!(*input).Submit_origin_present || encode_repeated_Submit_origin(state, (&(*input).Submit_origin)))
	&& (!(*input).Submit_profile_present || encode_repeated_Submit_profile(state, (&(*input).Submit_profile)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_submit_routed(
		zcbor_state_t *state, const struct request_submit_routed *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SubmitRouted", tmp_str.len = sizeof("SubmitRouted") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (encode_origin(state, (&(*input).SubmitRouted_origin))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"command", tmp_str.len = sizeof("command") - 1, &tmp_str)))))
	&& (encode_agent_command(state, (&(*input).SubmitRouted_command))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_poll(
		zcbor_state_t *state, const struct request_poll *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Poll", tmp_str.len = sizeof("Poll") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Poll_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).Poll_max))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_response_body_approved(
		zcbor_state_t *state, const struct host_response_body_approved *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Approved", tmp_str.len = sizeof("Approved") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).host_response_body_approved_Approved))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_response_body_input(
		zcbor_state_t *state, const struct host_response_body_input *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Input", tmp_str.len = sizeof("Input") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).host_response_body_input_Input))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_response_body_chosen(
		zcbor_state_t *state, const struct host_response_body_chosen *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Chosen", tmp_str.len = sizeof("Chosen") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).host_response_body_chosen_Chosen))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_response_body_delegated(
		zcbor_state_t *state, const struct host_response_body_delegated *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Delegated", tmp_str.len = sizeof("Delegated") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).host_response_body_delegated_Delegated))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_response_body_spawned(
		zcbor_state_t *state, const struct host_response_body_spawned *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Spawned", tmp_str.len = sizeof("Spawned") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).host_response_body_spawned_Spawned))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_response_body_deferred(
		zcbor_state_t *state, const struct host_response_body_deferred *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Deferred", tmp_str.len = sizeof("Deferred") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).host_response_body_deferred_Deferred))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_response_body_t(
		zcbor_state_t *state, const struct host_response_body_t_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).host_response_body_t_choice == host_response_body_t_host_response_body_approved_m_c) ? ((encode_host_response_body_approved(state, (&(*input).host_response_body_t_host_response_body_approved_m))))
	: (((*input).host_response_body_t_choice == host_response_body_t_host_response_body_input_m_c) ? ((encode_host_response_body_input(state, (&(*input).host_response_body_t_host_response_body_input_m))))
	: (((*input).host_response_body_t_choice == host_response_body_t_host_response_body_chosen_m_c) ? ((encode_host_response_body_chosen(state, (&(*input).host_response_body_t_host_response_body_chosen_m))))
	: (((*input).host_response_body_t_choice == host_response_body_t_host_response_body_delegated_m_c) ? ((encode_host_response_body_delegated(state, (&(*input).host_response_body_t_host_response_body_delegated_m))))
	: (((*input).host_response_body_t_choice == host_response_body_t_host_response_body_spawned_m_c) ? ((encode_host_response_body_spawned(state, (&(*input).host_response_body_t_host_response_body_spawned_m))))
	: (((*input).host_response_body_t_choice == host_response_body_t_host_response_body_deferred_m_c) ? ((encode_host_response_body_deferred(state, (&(*input).host_response_body_t_host_response_body_deferred_m))))
	: false))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_response(
		zcbor_state_t *state, const struct host_response *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).host_response_request_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (encode_host_response_body_t(state, (&(*input).host_response_body))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_respond(
		zcbor_state_t *state, const struct request_respond *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Respond", tmp_str.len = sizeof("Respond") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Respond_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"response", tmp_str.len = sizeof("response") - 1, &tmp_str)))))
	&& (encode_host_response(state, (&(*input).Respond_response))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_assign(
		zcbor_state_t *state, const struct request_assign *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Assign", tmp_str.len = sizeof("Assign") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Assign_session))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_cancel(
		zcbor_state_t *state, const struct request_cancel *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Cancel", tmp_str.len = sizeof("Cancel") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Cancel_session))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_unit(
		zcbor_state_t *state, const struct request_unit *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Unit", tmp_str.len = sizeof("Unit") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Unit_unit))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_unit_events(
		zcbor_state_t *state, const struct request_unit_events *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"UnitEvents", tmp_str.len = sizeof("UnitEvents") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).UnitEvents_unit))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).UnitEvents_max))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_unit_outbound(
		zcbor_state_t *state, const struct request_unit_outbound *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"UnitOutbound", tmp_str.len = sizeof("UnitOutbound") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).UnitOutbound_unit))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).UnitOutbound_max))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_session_history(
		zcbor_state_t *state, const struct request_session_history *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionHistory", tmp_str.len = sizeof("SessionHistory") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SessionHistory_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"after_cursor", tmp_str.len = sizeof("after_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).SessionHistory_after_cursor))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).SessionHistory_max))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_subscribe(
		zcbor_state_t *state, const struct request_subscribe *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Subscribe", tmp_str.len = sizeof("Subscribe") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Subscribe_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"after_seq", tmp_str.len = sizeof("after_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Subscribe_after_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).Subscribe_max))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_EventsSince_wait_ms(
		zcbor_state_t *state, const struct EventsSince_wait_ms_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"wait_ms", tmp_str.len = sizeof("wait_ms") - 1, &tmp_str)))))
	&& (((*input).EventsSince_wait_ms_choice == EventsSince_wait_ms_uint_c) ? ((zcbor_uint32_encode(state, (&(*input).EventsSince_wait_ms_uint))))
	: (((*input).EventsSince_wait_ms_choice == EventsSince_wait_ms_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_events_since(
		zcbor_state_t *state, const struct request_events_since *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"EventsSince", tmp_str.len = sizeof("EventsSince") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"cursor", tmp_str.len = sizeof("cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).EventsSince_cursor))))
	&& (!(*input).EventsSince_wait_ms_present || encode_repeated_EventsSince_wait_ms(state, (&(*input).EventsSince_wait_ms)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_delivery_targets(
		zcbor_state_t *state, const struct request_delivery_targets *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"DeliveryTargets", tmp_str.len = sizeof("DeliveryTargets") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).DeliveryTargets_session))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_delivery_sessions(
		zcbor_state_t *state, const struct request_delivery_sessions *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"DeliverySessions", tmp_str.len = sizeof("DeliverySessions") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).DeliverySessions_transport))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_sink_kind(
		zcbor_state_t *state, const struct sink_kind_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).sink_kind_choice == sink_kind_Primary_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Primary", tmp_str.len = sizeof("Primary") - 1, &tmp_str)))))
	: (((*input).sink_kind_choice == sink_kind_Spectator_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Spectator", tmp_str.len = sizeof("Spectator") - 1, &tmp_str)))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_delivery_target(
		zcbor_state_t *state, const struct delivery_target *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).delivery_target_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"route", tmp_str.len = sizeof("route") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).delivery_target_route))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (encode_sink_kind(state, (&(*input).delivery_target_kind))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_handover(
		zcbor_state_t *state, const struct request_handover *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Handover", tmp_str.len = sizeof("Handover") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Handover_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"target", tmp_str.len = sizeof("target") - 1, &tmp_str)))))
	&& (encode_delivery_target(state, (&(*input).Handover_target))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_record_meta_args(
		zcbor_state_t *state, const struct record_meta_args *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).record_meta_args_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (encode_origin(state, (&(*input).record_meta_args_origin))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).record_meta_args_kind))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (zcbor_bstr_encode(state, (&(*input).record_meta_args_body))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_record_meta(
		zcbor_state_t *state, const struct request_record_meta *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"RecordMeta", tmp_str.len = sizeof("RecordMeta") - 1, &tmp_str)))))
	&& (encode_record_meta_args(state, (&(*input).request_record_meta_RecordMeta))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_unit_history(
		zcbor_state_t *state, const struct request_unit_history *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"UnitHistory", tmp_str.len = sizeof("UnitHistory") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).UnitHistory_unit))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"after_cursor", tmp_str.len = sizeof("after_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).UnitHistory_after_cursor))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).UnitHistory_max))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_pause(
		zcbor_state_t *state, const struct request_pause *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Pause", tmp_str.len = sizeof("Pause") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Pause_unit))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_resume(
		zcbor_state_t *state, const struct request_resume *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Resume", tmp_str.len = sizeof("Resume") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Resume_unit))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_scale(
		zcbor_state_t *state, const struct request_scale *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Scale", tmp_str.len = sizeof("Scale") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"unit", tmp_str.len = sizeof("unit") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Scale_unit))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"n", tmp_str.len = sizeof("n") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).Scale_n))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_provider_selector(
		zcbor_state_t *state, const struct provider_selector_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).provider_selector_choice == provider_selector_mock_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"mock", tmp_str.len = sizeof("mock") - 1, &tmp_str)))))
	: (((*input).provider_selector_choice == provider_selector_genai_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"genai", tmp_str.len = sizeof("genai") - 1, &tmp_str)))))
	: (((*input).provider_selector_choice == provider_selector_daemon_api_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"daemon_api", tmp_str.len = sizeof("daemon_api") - 1, &tmp_str)))))
	: (((*input).provider_selector_choice == provider_selector_llama_cpp_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"llama_cpp", tmp_str.len = sizeof("llama_cpp") - 1, &tmp_str)))))
	: (((*input).provider_selector_choice == provider_selector_mistral_rs_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"mistral_rs", tmp_str.len = sizeof("mistral_rs") - 1, &tmp_str)))))
	: false)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_SetSessionModel_provider(
		zcbor_state_t *state, const struct SetSessionModel_provider_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (((*input).SetSessionModel_provider_choice == SetSessionModel_provider_provider_selector_m_c) ? ((encode_provider_selector(state, (&(*input).SetSessionModel_provider_provider_selector_m))))
	: (((*input).SetSessionModel_provider_choice == SetSessionModel_provider_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_set_session_model(
		zcbor_state_t *state, const struct request_set_session_model *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SetSessionModel", tmp_str.len = sizeof("SetSessionModel") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SetSessionModel_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SetSessionModel_model))))
	&& (!(*input).SetSessionModel_provider_present || encode_repeated_SetSessionModel_provider(state, (&(*input).SetSessionModel_provider)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_approval_mode(
		zcbor_state_t *state, const struct approval_mode_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).approval_mode_choice == approval_mode_ask_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ask", tmp_str.len = sizeof("ask") - 1, &tmp_str)))))
	: (((*input).approval_mode_choice == approval_mode_accept_edits_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"accept_edits", tmp_str.len = sizeof("accept_edits") - 1, &tmp_str)))))
	: (((*input).approval_mode_choice == approval_mode_auto_allow_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"auto_allow", tmp_str.len = sizeof("auto_allow") - 1, &tmp_str)))))
	: (((*input).approval_mode_choice == approval_mode_deny_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"deny", tmp_str.len = sizeof("deny") - 1, &tmp_str)))))
	: false))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_set_session_mode(
		zcbor_state_t *state, const struct request_set_session_mode *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SetSessionMode", tmp_str.len = sizeof("SetSessionMode") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SetSessionMode_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"mode", tmp_str.len = sizeof("mode") - 1, &tmp_str)))))
	&& (encode_approval_mode(state, (&(*input).SetSessionMode_mode))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_overlay_model(
		zcbor_state_t *state, const struct session_overlay_model_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (((*input).session_overlay_model_choice == session_overlay_model_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).session_overlay_model_tstr))))
	: (((*input).session_overlay_model_choice == session_overlay_model_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_overlay_provider(
		zcbor_state_t *state, const struct session_overlay_provider_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (((*input).session_overlay_provider_choice == session_overlay_provider_provider_selector_m_c) ? ((encode_provider_selector(state, (&(*input).session_overlay_provider_provider_selector_m))))
	: (((*input).session_overlay_provider_choice == session_overlay_provider_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_tools_override_allowlist(
		zcbor_state_t *state, const struct tools_override_allowlist *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"allowlist", tmp_str.len = sizeof("allowlist") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).tools_override_allowlist_allowlist_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).tools_override_allowlist_allowlist_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_tools_override(
		zcbor_state_t *state, const struct tools_override_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).tools_override_choice == tools_override_inherit_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"inherit", tmp_str.len = sizeof("inherit") - 1, &tmp_str)))))
	: (((*input).tools_override_choice == tools_override_full_toolset_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"full_toolset", tmp_str.len = sizeof("full_toolset") - 1, &tmp_str)))))
	: (((*input).tools_override_choice == tools_override_allowlist_m_c) ? ((encode_tools_override_allowlist(state, (&(*input).tools_override_allowlist_m))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_overlay_tool_allowlist(
		zcbor_state_t *state, const struct session_overlay_tool_allowlist *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"tool_allowlist", tmp_str.len = sizeof("tool_allowlist") - 1, &tmp_str)))))
	&& (encode_tools_override(state, (&(*input).session_overlay_tool_allowlist)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_overlay_approval_mode(
		zcbor_state_t *state, const struct session_overlay_approval_mode_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"approval_mode", tmp_str.len = sizeof("approval_mode") - 1, &tmp_str)))))
	&& (((*input).session_overlay_approval_mode_choice == session_overlay_approval_mode_approval_mode_m_c) ? ((encode_approval_mode(state, (&(*input).session_overlay_approval_mode_approval_mode_m))))
	: (((*input).session_overlay_approval_mode_choice == session_overlay_approval_mode_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_workspace_binding_bound(
		zcbor_state_t *state, const struct workspace_binding_bound *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"bound", tmp_str.len = sizeof("bound") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).workspace_binding_bound_bound))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_workspace_binding(
		zcbor_state_t *state, const struct workspace_binding_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).workspace_binding_choice == workspace_binding_isolated_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"isolated", tmp_str.len = sizeof("isolated") - 1, &tmp_str)))))
	: (((*input).workspace_binding_choice == workspace_binding_bound_m_c) ? ((encode_workspace_binding_bound(state, (&(*input).workspace_binding_bound_m))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_overlay_workspace(
		zcbor_state_t *state, const struct session_overlay_workspace_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"workspace", tmp_str.len = sizeof("workspace") - 1, &tmp_str)))))
	&& (((*input).session_overlay_workspace_choice == session_overlay_workspace_workspace_binding_m_c) ? ((encode_workspace_binding(state, (&(*input).session_overlay_workspace_workspace_binding_m))))
	: (((*input).session_overlay_workspace_choice == session_overlay_workspace_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_overlay(
		zcbor_state_t *state, const struct session_overlay *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_encode(state, 5) && (((!(*input).session_overlay_model_present || encode_repeated_session_overlay_model(state, (&(*input).session_overlay_model)))
	&& (!(*input).session_overlay_provider_present || encode_repeated_session_overlay_provider(state, (&(*input).session_overlay_provider)))
	&& (!(*input).session_overlay_tool_allowlist_present || encode_repeated_session_overlay_tool_allowlist(state, (&(*input).session_overlay_tool_allowlist)))
	&& (!(*input).session_overlay_approval_mode_present || encode_repeated_session_overlay_approval_mode(state, (&(*input).session_overlay_approval_mode)))
	&& (!(*input).session_overlay_workspace_present || encode_repeated_session_overlay_workspace(state, (&(*input).session_overlay_workspace)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_set_session_overlay(
		zcbor_state_t *state, const struct request_set_session_overlay *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SetSessionOverlay", tmp_str.len = sizeof("SetSessionOverlay") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SetSessionOverlay_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"overlay", tmp_str.len = sizeof("overlay") - 1, &tmp_str)))))
	&& (encode_session_overlay(state, (&(*input).SetSessionOverlay_overlay))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_ApprovalsPending_session(
		zcbor_state_t *state, const struct ApprovalsPending_session_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (((*input).ApprovalsPending_session_choice == ApprovalsPending_session_session_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).ApprovalsPending_session_session_id_m))))
	: (((*input).ApprovalsPending_session_choice == ApprovalsPending_session_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_approvals_pending(
		zcbor_state_t *state, const struct request_approvals_pending *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ApprovalsPending", tmp_str.len = sizeof("ApprovalsPending") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((!(*input).ApprovalsPending_session_present || encode_repeated_ApprovalsPending_session(state, (&(*input).ApprovalsPending_session)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_approval_decide(
		zcbor_state_t *state, const struct request_approval_decide *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ApprovalDecide", tmp_str.len = sizeof("ApprovalDecide") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ApprovalDecide_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ApprovalDecide_request_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"allow", tmp_str.len = sizeof("allow") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).ApprovalDecide_allow))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_profile_get(
		zcbor_state_t *state, const struct request_profile_get *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileGet", tmp_str.len = sizeof("ProfileGet") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ProfileGet_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_budget(
		zcbor_state_t *state, const struct budget *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"tokens", tmp_str.len = sizeof("tokens") - 1, &tmp_str)))))
	&& (((*input).budget_tokens_choice == budget_tokens_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).budget_tokens_uint64_m))))
	: (((*input).budget_tokens_choice == budget_tokens_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"wall_ms", tmp_str.len = sizeof("wall_ms") - 1, &tmp_str)))))
	&& (((*input).budget_wall_ms_choice == budget_wall_ms_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).budget_wall_ms_uint64_m))))
	: (((*input).budget_wall_ms_choice == budget_wall_ms_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_engine_tunables(
		zcbor_state_t *state, const struct engine_tunables *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"model_retry_attempts", tmp_str.len = sizeof("model_retry_attempts") - 1, &tmp_str)))))
	&& (((*input).engine_tunables_model_retry_attempts_choice == engine_tunables_model_retry_attempts_uint_c) ? ((zcbor_uint32_encode(state, (&(*input).engine_tunables_model_retry_attempts_uint))))
	: (((*input).engine_tunables_model_retry_attempts_choice == engine_tunables_model_retry_attempts_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"context_budget_tokens", tmp_str.len = sizeof("context_budget_tokens") - 1, &tmp_str)))))
	&& (((*input).engine_tunables_context_budget_tokens_choice == engine_tunables_context_budget_tokens_uint_c) ? ((zcbor_uint32_encode(state, (&(*input).engine_tunables_context_budget_tokens_uint))))
	: (((*input).engine_tunables_context_budget_tokens_choice == engine_tunables_context_budget_tokens_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max_iterations", tmp_str.len = sizeof("max_iterations") - 1, &tmp_str)))))
	&& (((*input).engine_tunables_max_iterations_choice == engine_tunables_max_iterations_uint_c) ? ((zcbor_uint32_encode(state, (&(*input).engine_tunables_max_iterations_uint))))
	: (((*input).engine_tunables_max_iterations_choice == engine_tunables_max_iterations_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"tool_result_budget", tmp_str.len = sizeof("tool_result_budget") - 1, &tmp_str)))))
	&& (((*input).engine_tunables_tool_result_budget_choice == engine_tunables_tool_result_budget_uint_c) ? ((zcbor_uint32_encode(state, (&(*input).engine_tunables_tool_result_budget_uint))))
	: (((*input).engine_tunables_tool_result_budget_choice == engine_tunables_tool_result_budget_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_context_engine_sel(
		zcbor_state_t *state, const struct context_engine_sel_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).context_engine_sel_choice == context_engine_sel_lcm_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"lcm", tmp_str.len = sizeof("lcm") - 1, &tmp_str)))))
	: (((*input).context_engine_sel_choice == context_engine_sel_budgeted_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"budgeted", tmp_str.len = sizeof("budgeted") - 1, &tmp_str)))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_memory_provider_sel(
		zcbor_state_t *state, const struct memory_provider_sel_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).memory_provider_sel_choice == memory_provider_sel_mnemosyne_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"mnemosyne", tmp_str.len = sizeof("mnemosyne") - 1, &tmp_str)))))
	: (((*input).memory_provider_sel_choice == memory_provider_sel_file_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"file", tmp_str.len = sizeof("file") - 1, &tmp_str)))))
	: (((*input).memory_provider_sel_choice == memory_provider_sel_none_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"none", tmp_str.len = sizeof("none") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_bound_account(
		zcbor_state_t *state, const struct bound_account *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport_instance", tmp_str.len = sizeof("transport_instance") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).bound_account_transport_instance))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"credential_ref", tmp_str.len = sizeof("credential_ref") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).bound_account_credential_ref))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_profile_spec(
		zcbor_state_t *state, const struct profile_spec *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 13) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).profile_spec_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (encode_provider_selector(state, (&(*input).profile_spec_provider))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).profile_spec_model))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"base_url", tmp_str.len = sizeof("base_url") - 1, &tmp_str)))))
	&& (((*input).profile_spec_base_url_choice == profile_spec_base_url_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).profile_spec_base_url_tstr))))
	: (((*input).profile_spec_base_url_choice == profile_spec_base_url_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"system_prompt", tmp_str.len = sizeof("system_prompt") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).profile_spec_system_prompt))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"tool_allowlist", tmp_str.len = sizeof("tool_allowlist") - 1, &tmp_str)))))
	&& (((*input).profile_spec_tool_allowlist_choice == tool_allowlist_tstr_l_c) ? ((zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).tool_allowlist_tstr_l_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).tool_allowlist_tstr_l_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	: (((*input).profile_spec_tool_allowlist_choice == profile_spec_tool_allowlist_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"budget", tmp_str.len = sizeof("budget") - 1, &tmp_str)))))
	&& (encode_budget(state, (&(*input).profile_spec_budget))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"tunables", tmp_str.len = sizeof("tunables") - 1, &tmp_str)))))
	&& (encode_engine_tunables(state, (&(*input).profile_spec_tunables))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"context_engine", tmp_str.len = sizeof("context_engine") - 1, &tmp_str)))))
	&& (encode_context_engine_sel(state, (&(*input).profile_spec_context_engine))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"memory_provider", tmp_str.len = sizeof("memory_provider") - 1, &tmp_str)))))
	&& (encode_memory_provider_sel(state, (&(*input).profile_spec_memory_provider))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"credential_ref", tmp_str.len = sizeof("credential_ref") - 1, &tmp_str)))))
	&& (((*input).profile_spec_credential_ref_choice == profile_spec_credential_ref_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).profile_spec_credential_ref_tstr))))
	: (((*input).profile_spec_credential_ref_choice == profile_spec_credential_ref_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"fallback_credential_ref", tmp_str.len = sizeof("fallback_credential_ref") - 1, &tmp_str)))))
	&& (((*input).profile_spec_fallback_credential_ref_choice == profile_spec_fallback_credential_ref_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).profile_spec_fallback_credential_ref_tstr))))
	: (((*input).profile_spec_fallback_credential_ref_choice == profile_spec_fallback_credential_ref_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"bound_accounts", tmp_str.len = sizeof("bound_accounts") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).profile_spec_bound_accounts_bound_account_m_count, (zcbor_encoder_t *)encode_bound_account, state, (*&(*input).profile_spec_bound_accounts_bound_account_m), sizeof(struct bound_account))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 13))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_profile_create(
		zcbor_state_t *state, const struct request_profile_create *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileCreate", tmp_str.len = sizeof("ProfileCreate") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (encode_profile_spec(state, (&(*input).ProfileCreate_spec))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_profile_update(
		zcbor_state_t *state, const struct request_profile_update *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileUpdate", tmp_str.len = sizeof("ProfileUpdate") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (encode_profile_spec(state, (&(*input).ProfileUpdate_spec))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_profile_delete(
		zcbor_state_t *state, const struct request_profile_delete *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileDelete", tmp_str.len = sizeof("ProfileDelete") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ProfileDelete_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_profile_select(
		zcbor_state_t *state, const struct request_profile_select *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileSelect", tmp_str.len = sizeof("ProfileSelect") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ProfileSelect_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_profile_clone(
		zcbor_state_t *state, const struct request_profile_clone *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileClone", tmp_str.len = sizeof("ProfileClone") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ProfileClone_source))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"new_id", tmp_str.len = sizeof("new_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ProfileClone_new_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_profile_export(
		zcbor_state_t *state, const struct request_profile_export *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileExport", tmp_str.len = sizeof("ProfileExport") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ProfileExport_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_files_tstrtstr(
		zcbor_state_t *state, const struct files_tstrtstr *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_tstr_encode(state, (&(*input).skill_bundle_files_tstrtstr_key))))
	&& (zcbor_tstr_encode(state, (&(*input).files_tstrtstr)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_skill_bundle(
		zcbor_state_t *state, const struct skill_bundle *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).skill_bundle_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"category", tmp_str.len = sizeof("category") - 1, &tmp_str)))))
	&& (((*input).skill_bundle_category_choice == skill_bundle_category_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).skill_bundle_category_tstr))))
	: (((*input).skill_bundle_category_choice == skill_bundle_category_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"files", tmp_str.len = sizeof("files") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).files_tstrtstr_count, (zcbor_encoder_t *)encode_repeated_files_tstrtstr, state, (*&(*input).files_tstrtstr), sizeof(struct files_tstrtstr))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_distribution_head_seq(
		zcbor_state_t *state, const struct distribution_head_seq_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"head_seq", tmp_str.len = sizeof("head_seq") - 1, &tmp_str)))))
	&& (((*input).distribution_head_seq_choice == distribution_head_seq_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).distribution_head_seq_uint64_m))))
	: (((*input).distribution_head_seq_choice == distribution_head_seq_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_distribution_source(
		zcbor_state_t *state, const struct distribution_source_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (((*input).distribution_source_choice == distribution_source_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).distribution_source_tstr))))
	: (((*input).distribution_source_choice == distribution_source_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_distribution(
		zcbor_state_t *state, const struct distribution *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"wire_version", tmp_str.len = sizeof("wire_version") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).distribution_wire_version))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (encode_profile_spec(state, (&(*input).distribution_profile))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"skills", tmp_str.len = sizeof("skills") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).distribution_skills_skill_bundle_m_count, (zcbor_encoder_t *)encode_skill_bundle, state, (*&(*input).distribution_skills_skill_bundle_m), sizeof(struct skill_bundle))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (!(*input).distribution_head_seq_present || encode_repeated_distribution_head_seq(state, (&(*input).distribution_head_seq)))
	&& (!(*input).distribution_source_present || encode_repeated_distribution_source(state, (&(*input).distribution_source)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_ProfileImport_new_id(
		zcbor_state_t *state, const struct ProfileImport_new_id_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"new_id", tmp_str.len = sizeof("new_id") - 1, &tmp_str)))))
	&& (((*input).ProfileImport_new_id_choice == ProfileImport_new_id_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).ProfileImport_new_id_tstr))))
	: (((*input).ProfileImport_new_id_choice == ProfileImport_new_id_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_profile_import(
		zcbor_state_t *state, const struct request_profile_import *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileImport", tmp_str.len = sizeof("ProfileImport") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"dist", tmp_str.len = sizeof("dist") - 1, &tmp_str)))))
	&& (encode_distribution(state, (&(*input).ProfileImport_dist))))
	&& (!(*input).ProfileImport_new_id_present || encode_repeated_ProfileImport_new_id(state, (&(*input).ProfileImport_new_id)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_profile_history(
		zcbor_state_t *state, const struct request_profile_history *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileHistory", tmp_str.len = sizeof("ProfileHistory") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ProfileHistory_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_profile_at(
		zcbor_state_t *state, const struct request_profile_at *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileAt", tmp_str.len = sizeof("ProfileAt") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ProfileAt_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).ProfileAt_seq))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_profile_revert(
		zcbor_state_t *state, const struct request_profile_revert *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileRevert", tmp_str.len = sizeof("ProfileRevert") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ProfileRevert_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).ProfileRevert_seq))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_skill_history(
		zcbor_state_t *state, const struct request_skill_history *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SkillHistory", tmp_str.len = sizeof("SkillHistory") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SkillHistory_name))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_skill_at(
		zcbor_state_t *state, const struct request_skill_at *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SkillAt", tmp_str.len = sizeof("SkillAt") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SkillAt_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).SkillAt_seq))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_skill_revert(
		zcbor_state_t *state, const struct request_skill_revert *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SkillRevert", tmp_str.len = sizeof("SkillRevert") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SkillRevert_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).SkillRevert_seq))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_CuratorList_profile(
		zcbor_state_t *state, const struct CuratorList_profile_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (((*input).CuratorList_profile_choice == CuratorList_profile_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).CuratorList_profile_tstr))))
	: (((*input).CuratorList_profile_choice == CuratorList_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_curator_list(
		zcbor_state_t *state, const struct request_curator_list *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CuratorList", tmp_str.len = sizeof("CuratorList") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((!(*input).CuratorList_profile_present || encode_repeated_CuratorList_profile(state, (&(*input).CuratorList_profile)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_CuratorPin_profile(
		zcbor_state_t *state, const struct CuratorPin_profile_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (((*input).CuratorPin_profile_choice == CuratorPin_profile_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).CuratorPin_profile_tstr))))
	: (((*input).CuratorPin_profile_choice == CuratorPin_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_curator_pin(
		zcbor_state_t *state, const struct request_curator_pin *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CuratorPin", tmp_str.len = sizeof("CuratorPin") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((!(*input).CuratorPin_profile_present || encode_repeated_CuratorPin_profile(state, (&(*input).CuratorPin_profile)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CuratorPin_name))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_CuratorUnpin_profile(
		zcbor_state_t *state, const struct CuratorUnpin_profile_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (((*input).CuratorUnpin_profile_choice == CuratorUnpin_profile_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).CuratorUnpin_profile_tstr))))
	: (((*input).CuratorUnpin_profile_choice == CuratorUnpin_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_curator_unpin(
		zcbor_state_t *state, const struct request_curator_unpin *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CuratorUnpin", tmp_str.len = sizeof("CuratorUnpin") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((!(*input).CuratorUnpin_profile_present || encode_repeated_CuratorUnpin_profile(state, (&(*input).CuratorUnpin_profile)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CuratorUnpin_name))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_CuratorArchive_profile(
		zcbor_state_t *state, const struct CuratorArchive_profile_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (((*input).CuratorArchive_profile_choice == CuratorArchive_profile_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).CuratorArchive_profile_tstr))))
	: (((*input).CuratorArchive_profile_choice == CuratorArchive_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_curator_archive(
		zcbor_state_t *state, const struct request_curator_archive *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CuratorArchive", tmp_str.len = sizeof("CuratorArchive") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((!(*input).CuratorArchive_profile_present || encode_repeated_CuratorArchive_profile(state, (&(*input).CuratorArchive_profile)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CuratorArchive_name))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_CuratorRestore_profile(
		zcbor_state_t *state, const struct CuratorRestore_profile_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (((*input).CuratorRestore_profile_choice == CuratorRestore_profile_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).CuratorRestore_profile_tstr))))
	: (((*input).CuratorRestore_profile_choice == CuratorRestore_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_curator_restore(
		zcbor_state_t *state, const struct request_curator_restore *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CuratorRestore", tmp_str.len = sizeof("CuratorRestore") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((!(*input).CuratorRestore_profile_present || encode_repeated_CuratorRestore_profile(state, (&(*input).CuratorRestore_profile)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CuratorRestore_name))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_CuratorRun_profile(
		zcbor_state_t *state, const struct CuratorRun_profile_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (((*input).CuratorRun_profile_choice == CuratorRun_profile_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).CuratorRun_profile_tstr))))
	: (((*input).CuratorRun_profile_choice == CuratorRun_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_curator_run(
		zcbor_state_t *state, const struct request_curator_run *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CuratorRun", tmp_str.len = sizeof("CuratorRun") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((!(*input).CuratorRun_profile_present || encode_repeated_CuratorRun_profile(state, (&(*input).CuratorRun_profile)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_credential_set(
		zcbor_state_t *state, const struct request_credential_set *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CredentialSet", tmp_str.len = sizeof("CredentialSet") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CredentialSet_profile))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"secret", tmp_str.len = sizeof("secret") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CredentialSet_secret))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_credential_remove(
		zcbor_state_t *state, const struct request_credential_remove *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CredentialRemove", tmp_str.len = sizeof("CredentialRemove") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CredentialRemove_profile))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_model_engine(
		zcbor_state_t *state, const struct model_engine_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).model_engine_choice == model_engine_Llama_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Llama", tmp_str.len = sizeof("Llama") - 1, &tmp_str)))))
	: (((*input).model_engine_choice == model_engine_MistralRs_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"MistralRs", tmp_str.len = sizeof("MistralRs") - 1, &tmp_str)))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_search_sort(
		zcbor_state_t *state, const struct search_sort_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).search_sort_choice == search_sort_Trending_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Trending", tmp_str.len = sizeof("Trending") - 1, &tmp_str)))))
	: (((*input).search_sort_choice == search_sort_Downloads_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Downloads", tmp_str.len = sizeof("Downloads") - 1, &tmp_str)))))
	: (((*input).search_sort_choice == search_sort_Likes_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Likes", tmp_str.len = sizeof("Likes") - 1, &tmp_str)))))
	: (((*input).search_sort_choice == search_sort_Modified_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Modified", tmp_str.len = sizeof("Modified") - 1, &tmp_str)))))
	: (((*input).search_sort_choice == search_sort_Created_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Created", tmp_str.len = sizeof("Created") - 1, &tmp_str)))))
	: false)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_search_query(
		zcbor_state_t *state, const struct search_query *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).search_query_text))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"engine", tmp_str.len = sizeof("engine") - 1, &tmp_str)))))
	&& (encode_model_engine(state, (&(*input).search_query_engine))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"sort", tmp_str.len = sizeof("sort") - 1, &tmp_str)))))
	&& (encode_search_sort(state, (&(*input).search_query_sort))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"page", tmp_str.len = sizeof("page") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).search_query_page))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"limit", tmp_str.len = sizeof("limit") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).search_query_limit))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_model_search(
		zcbor_state_t *state, const struct request_model_search *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelSearch", tmp_str.len = sizeof("ModelSearch") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"query", tmp_str.len = sizeof("query") - 1, &tmp_str)))))
	&& (encode_search_query(state, (&(*input).ModelSearch_query))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_model_files(
		zcbor_state_t *state, const struct request_model_files *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelFiles", tmp_str.len = sizeof("ModelFiles") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ModelFiles_repo))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"revision", tmp_str.len = sizeof("revision") - 1, &tmp_str)))))
	&& (((*input).ModelFiles_revision_choice == ModelFiles_revision_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).ModelFiles_revision_tstr))))
	: (((*input).ModelFiles_revision_choice == ModelFiles_revision_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"engine", tmp_str.len = sizeof("engine") - 1, &tmp_str)))))
	&& (encode_model_engine(state, (&(*input).ModelFiles_engine))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_model_source_hf(
		zcbor_state_t *state, const struct model_source_hf *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Hf", tmp_str.len = sizeof("Hf") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Hf_repo))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"file", tmp_str.len = sizeof("file") - 1, &tmp_str)))))
	&& (((*input).Hf_file_choice == Hf_file_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).Hf_file_tstr))))
	: (((*input).Hf_file_choice == Hf_file_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"revision", tmp_str.len = sizeof("revision") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Hf_revision))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_model_source_local(
		zcbor_state_t *state, const struct model_source_local *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Local", tmp_str.len = sizeof("Local") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Local_path))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_model_source(
		zcbor_state_t *state, const struct model_source_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).model_source_choice == model_source_hf_m_c) ? ((encode_model_source_hf(state, (&(*input).model_source_hf_m))))
	: (((*input).model_source_choice == model_source_local_m_c) ? ((encode_model_source_local(state, (&(*input).model_source_local_m))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_model_ref(
		zcbor_state_t *state, const struct model_ref *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"engine", tmp_str.len = sizeof("engine") - 1, &tmp_str)))))
	&& (encode_model_engine(state, (&(*input).model_ref_engine))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (encode_model_source(state, (&(*input).model_ref_source))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_model_download(
		zcbor_state_t *state, const struct request_model_download *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelDownload", tmp_str.len = sizeof("ModelDownload") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (encode_model_ref(state, (&(*input).ModelDownload_model))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_model_cancel(
		zcbor_state_t *state, const struct request_model_cancel *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelCancel", tmp_str.len = sizeof("ModelCancel") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).ModelCancel_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_model_pause(
		zcbor_state_t *state, const struct request_model_pause *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelPause", tmp_str.len = sizeof("ModelPause") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).ModelPause_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_model_resume(
		zcbor_state_t *state, const struct request_model_resume *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelResume", tmp_str.len = sizeof("ModelResume") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).ModelResume_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_model_delete(
		zcbor_state_t *state, const struct request_model_delete *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelDelete", tmp_str.len = sizeof("ModelDelete") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ModelDelete_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_model_activate(
		zcbor_state_t *state, const struct request_model_activate *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelActivate", tmp_str.len = sizeof("ModelActivate") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ModelActivate_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (((*input).ModelActivate_profile_choice == ModelActivate_profile_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).ModelActivate_profile_tstr))))
	: (((*input).ModelActivate_profile_choice == ModelActivate_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_model_recommend_args(
		zcbor_state_t *state, const struct model_recommend_args *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).model_recommend_args_repo))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"revision", tmp_str.len = sizeof("revision") - 1, &tmp_str)))))
	&& (((*input).model_recommend_args_revision_choice == model_recommend_args_revision_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).model_recommend_args_revision_tstr))))
	: (((*input).model_recommend_args_revision_choice == model_recommend_args_revision_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"engine", tmp_str.len = sizeof("engine") - 1, &tmp_str)))))
	&& (encode_model_engine(state, (&(*input).model_recommend_args_engine))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"budget_bytes", tmp_str.len = sizeof("budget_bytes") - 1, &tmp_str)))))
	&& (((*input).model_recommend_args_budget_bytes_choice == model_recommend_args_budget_bytes_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).model_recommend_args_budget_bytes_uint64_m))))
	: (((*input).model_recommend_args_budget_bytes_choice == model_recommend_args_budget_bytes_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_model_recommend(
		zcbor_state_t *state, const struct request_model_recommend *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelRecommend", tmp_str.len = sizeof("ModelRecommend") - 1, &tmp_str)))))
	&& (encode_model_recommend_args(state, (&(*input).request_model_recommend_ModelRecommend))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_model_quantize_args(
		zcbor_state_t *state, const struct model_quantize_args *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).model_quantize_args_repo))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"revision", tmp_str.len = sizeof("revision") - 1, &tmp_str)))))
	&& (((*input).model_quantize_args_revision_choice == model_quantize_args_revision_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).model_quantize_args_revision_tstr))))
	: (((*input).model_quantize_args_revision_choice == model_quantize_args_revision_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"target_quant", tmp_str.len = sizeof("target_quant") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).model_quantize_args_target_quant))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"source_file", tmp_str.len = sizeof("source_file") - 1, &tmp_str)))))
	&& (((*input).model_quantize_args_source_file_choice == model_quantize_args_source_file_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).model_quantize_args_source_file_tstr))))
	: (((*input).model_quantize_args_source_file_choice == model_quantize_args_source_file_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_model_quantize(
		zcbor_state_t *state, const struct request_model_quantize *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelQuantize", tmp_str.len = sizeof("ModelQuantize") - 1, &tmp_str)))))
	&& (encode_model_quantize_args(state, (&(*input).request_model_quantize_ModelQuantize))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_model_inspect(
		zcbor_state_t *state, const struct request_model_inspect *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelInspect", tmp_str.len = sizeof("ModelInspect") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ModelInspect_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_model_current(
		zcbor_state_t *state, const struct request_model_current *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelCurrent", tmp_str.len = sizeof("ModelCurrent") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (((*input).ModelCurrent_profile_choice == ModelCurrent_profile_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).ModelCurrent_profile_tstr))))
	: (((*input).ModelCurrent_profile_choice == ModelCurrent_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_params_tstrtstr(
		zcbor_state_t *state, const struct params_tstrtstr *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_tstr_encode(state, (&(*input).auth_begin_request_params_tstrtstr_key))))
	&& (zcbor_tstr_encode(state, (&(*input).params_tstrtstr)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_auth_bind_request(
		zcbor_state_t *state, const struct auth_bind_request *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_bind_request_profile))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport_instance", tmp_str.len = sizeof("transport_instance") - 1, &tmp_str)))))
	&& (((*input).auth_bind_request_transport_instance_choice == auth_bind_request_transport_instance_transport_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).auth_bind_request_transport_instance_transport_id_m))))
	: (((*input).auth_bind_request_transport_instance_choice == auth_bind_request_transport_instance_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"credential_ref", tmp_str.len = sizeof("credential_ref") - 1, &tmp_str)))))
	&& (((*input).auth_bind_request_credential_ref_choice == auth_bind_request_credential_ref_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).auth_bind_request_credential_ref_tstr))))
	: (((*input).auth_bind_request_credential_ref_choice == auth_bind_request_credential_ref_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_auth_begin_request_bind(
		zcbor_state_t *state, const struct auth_begin_request_bind_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"bind", tmp_str.len = sizeof("bind") - 1, &tmp_str)))))
	&& (((*input).auth_begin_request_bind_choice == auth_begin_request_bind_auth_bind_request_m_c) ? ((encode_auth_bind_request(state, (&(*input).auth_begin_request_bind_auth_bind_request_m))))
	: (((*input).auth_begin_request_bind_choice == auth_begin_request_bind_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_auth_begin_request(
		zcbor_state_t *state, const struct auth_begin_request *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"family", tmp_str.len = sizeof("family") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_begin_request_family))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"params", tmp_str.len = sizeof("params") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).params_tstrtstr_count, (zcbor_encoder_t *)encode_repeated_params_tstrtstr, state, (*&(*input).params_tstrtstr), sizeof(struct params_tstrtstr))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 64)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"redirect_uri", tmp_str.len = sizeof("redirect_uri") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_begin_request_redirect_uri))))
	&& (!(*input).auth_begin_request_bind_present || encode_repeated_auth_begin_request_bind(state, (&(*input).auth_begin_request_bind)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_auth_begin(
		zcbor_state_t *state, const struct request_auth_begin *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AuthBegin", tmp_str.len = sizeof("AuthBegin") - 1, &tmp_str)))))
	&& (encode_auth_begin_request(state, (&(*input).request_auth_begin_AuthBegin))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_auth_complete_request(
		zcbor_state_t *state, const struct auth_complete_request *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"flow_id", tmp_str.len = sizeof("flow_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_complete_request_flow_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"callback", tmp_str.len = sizeof("callback") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_complete_request_callback))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_auth_complete(
		zcbor_state_t *state, const struct request_auth_complete *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AuthComplete", tmp_str.len = sizeof("AuthComplete") - 1, &tmp_str)))))
	&& (encode_auth_complete_request(state, (&(*input).request_auth_complete_AuthComplete))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_auth_cancel(
		zcbor_state_t *state, const struct request_auth_cancel *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AuthCancel", tmp_str.len = sizeof("AuthCancel") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"flow_id", tmp_str.len = sizeof("flow_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).AuthCancel_flow_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_CheckpointList_session(
		zcbor_state_t *state, const struct CheckpointList_session_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (((*input).CheckpointList_session_choice == CheckpointList_session_session_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).CheckpointList_session_session_id_m))))
	: (((*input).CheckpointList_session_choice == CheckpointList_session_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_checkpoint_list(
		zcbor_state_t *state, const struct request_checkpoint_list *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CheckpointList", tmp_str.len = sizeof("CheckpointList") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((!(*input).CheckpointList_session_present || encode_repeated_CheckpointList_session(state, (&(*input).CheckpointList_session)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_checkpoint_rewind(
		zcbor_state_t *state, const struct request_checkpoint_rewind *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CheckpointRewind", tmp_str.len = sizeof("CheckpointRewind") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CheckpointRewind_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"checkpoint_id", tmp_str.len = sizeof("checkpoint_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CheckpointRewind_checkpoint_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_scope_by_profile(
		zcbor_state_t *state, const struct session_scope_by_profile *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ByProfile", tmp_str.len = sizeof("ByProfile") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).session_scope_by_profile_ByProfile))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_scope_by_transport(
		zcbor_state_t *state, const struct session_scope_by_transport *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ByTransport", tmp_str.len = sizeof("ByTransport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).session_scope_by_transport_ByTransport))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_scope(
		zcbor_state_t *state, const struct session_scope_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).session_scope_choice == session_scope_TopLevel_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"TopLevel", tmp_str.len = sizeof("TopLevel") - 1, &tmp_str)))))
	: (((*input).session_scope_choice == session_scope_by_profile_m_c) ? ((encode_session_scope_by_profile(state, (&(*input).session_scope_by_profile_m))))
	: (((*input).session_scope_choice == session_scope_by_transport_m_c) ? ((encode_session_scope_by_transport(state, (&(*input).session_scope_by_transport_m))))
	: (((*input).session_scope_choice == session_scope_Archived_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Archived", tmp_str.len = sizeof("Archived") - 1, &tmp_str)))))
	: (((*input).session_scope_choice == session_scope_All_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"All", tmp_str.len = sizeof("All") - 1, &tmp_str)))))
	: false)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_query_scope(
		zcbor_state_t *state, const struct session_query_scope *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"scope", tmp_str.len = sizeof("scope") - 1, &tmp_str)))))
	&& (encode_session_scope(state, (&(*input).session_query_scope)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_query_after(
		zcbor_state_t *state, const struct session_query_after_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"after", tmp_str.len = sizeof("after") - 1, &tmp_str)))))
	&& (((*input).session_query_after_choice == session_query_after_session_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).session_query_after_session_id_m))))
	: (((*input).session_query_after_choice == session_query_after_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_query_limit(
		zcbor_state_t *state, const struct session_query_limit *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"limit", tmp_str.len = sizeof("limit") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).session_query_limit)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_query_since_rev(
		zcbor_state_t *state, const struct session_query_since_rev_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"since_rev", tmp_str.len = sizeof("since_rev") - 1, &tmp_str)))))
	&& (((*input).session_query_since_rev_choice == session_query_since_rev_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).session_query_since_rev_uint64_m))))
	: (((*input).session_query_since_rev_choice == session_query_since_rev_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_query(
		zcbor_state_t *state, const struct session_query *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_encode(state, 4) && (((!(*input).session_query_scope_present || encode_repeated_session_query_scope(state, (&(*input).session_query_scope)))
	&& (!(*input).session_query_after_present || encode_repeated_session_query_after(state, (&(*input).session_query_after)))
	&& (!(*input).session_query_limit_present || encode_repeated_session_query_limit(state, (&(*input).session_query_limit)))
	&& (!(*input).session_query_since_rev_present || encode_repeated_session_query_since_rev(state, (&(*input).session_query_since_rev)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_sessions_query(
		zcbor_state_t *state, const struct request_sessions_query *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionsQuery", tmp_str.len = sizeof("SessionsQuery") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"query", tmp_str.len = sizeof("query") - 1, &tmp_str)))))
	&& (encode_session_query(state, (&(*input).SessionsQuery_query))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_session_get(
		zcbor_state_t *state, const struct request_session_get *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionGet", tmp_str.len = sizeof("SessionGet") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SessionGet_session))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_session_search(
		zcbor_state_t *state, const struct request_session_search *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionSearch", tmp_str.len = sizeof("SessionSearch") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"query", tmp_str.len = sizeof("query") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SessionSearch_query))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"limit", tmp_str.len = sizeof("limit") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).SessionSearch_limit))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_meta_patch_title(
		zcbor_state_t *state, const struct session_meta_patch_title_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (((*input).session_meta_patch_title_choice == session_meta_patch_title_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).session_meta_patch_title_tstr))))
	: (((*input).session_meta_patch_title_choice == session_meta_patch_title_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_meta_patch_pinned(
		zcbor_state_t *state, const struct session_meta_patch_pinned_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"pinned", tmp_str.len = sizeof("pinned") - 1, &tmp_str)))))
	&& (((*input).session_meta_patch_pinned_choice == session_meta_patch_pinned_bool_c) ? ((zcbor_bool_encode(state, (&(*input).session_meta_patch_pinned_bool))))
	: (((*input).session_meta_patch_pinned_choice == session_meta_patch_pinned_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_meta_patch_archived(
		zcbor_state_t *state, const struct session_meta_patch_archived_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"archived", tmp_str.len = sizeof("archived") - 1, &tmp_str)))))
	&& (((*input).session_meta_patch_archived_choice == session_meta_patch_archived_bool_c) ? ((zcbor_bool_encode(state, (&(*input).session_meta_patch_archived_bool))))
	: (((*input).session_meta_patch_archived_choice == session_meta_patch_archived_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_meta_patch(
		zcbor_state_t *state, const struct session_meta_patch *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_encode(state, 3) && (((!(*input).session_meta_patch_title_present || encode_repeated_session_meta_patch_title(state, (&(*input).session_meta_patch_title)))
	&& (!(*input).session_meta_patch_pinned_present || encode_repeated_session_meta_patch_pinned(state, (&(*input).session_meta_patch_pinned)))
	&& (!(*input).session_meta_patch_archived_present || encode_repeated_session_meta_patch_archived(state, (&(*input).session_meta_patch_archived)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_session_update_meta(
		zcbor_state_t *state, const struct request_session_update_meta *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionUpdateMeta", tmp_str.len = sizeof("SessionUpdateMeta") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SessionUpdateMeta_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"patch", tmp_str.len = sizeof("patch") - 1, &tmp_str)))))
	&& (encode_session_meta_patch(state, (&(*input).SessionUpdateMeta_patch))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_rewind_point_restore_workspace(
		zcbor_state_t *state, const struct rewind_point_restore_workspace *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"restore_workspace", tmp_str.len = sizeof("restore_workspace") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).rewind_point_restore_workspace)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_rewind_point(
		zcbor_state_t *state, const struct rewind_point *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"anchor", tmp_str.len = sizeof("anchor") - 1, &tmp_str)))))
	&& (encode_rewind_anchor(state, (&(*input).rewind_point_anchor))))
	&& (!(*input).rewind_point_restore_workspace_present || encode_repeated_rewind_point_restore_workspace(state, (&(*input).rewind_point_restore_workspace)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_rewind(
		zcbor_state_t *state, const struct request_rewind *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Rewind", tmp_str.len = sizeof("Rewind") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Rewind_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"point", tmp_str.len = sizeof("point") - 1, &tmp_str)))))
	&& (encode_rewind_point(state, (&(*input).Rewind_point))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_acp_recipe_program(
		zcbor_state_t *state, const struct acp_recipe_program_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"program", tmp_str.len = sizeof("program") - 1, &tmp_str)))))
	&& (((*input).acp_recipe_program_choice == acp_recipe_program_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).acp_recipe_program_tstr))))
	: (((*input).acp_recipe_program_choice == acp_recipe_program_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_acp_recipe_args(
		zcbor_state_t *state, const struct acp_recipe_args_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"args", tmp_str.len = sizeof("args") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).acp_recipe_args_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).acp_recipe_args_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_kv_pair(
		zcbor_state_t *state, const struct kv_pair *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_encode(state, 2) && ((((zcbor_tstr_encode(state, (&(*input).kv_pair_k))))
	&& ((zcbor_tstr_encode(state, (&(*input).kv_pair_v))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_acp_recipe_env(
		zcbor_state_t *state, const struct acp_recipe_env_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"env", tmp_str.len = sizeof("env") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).acp_recipe_env_kv_pair_m_count, (zcbor_encoder_t *)encode_kv_pair, state, (*&(*input).acp_recipe_env_kv_pair_m), sizeof(struct kv_pair))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_acp_recipe_endpoint(
		zcbor_state_t *state, const struct acp_recipe_endpoint_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"endpoint", tmp_str.len = sizeof("endpoint") - 1, &tmp_str)))))
	&& (((*input).acp_recipe_endpoint_choice == acp_recipe_endpoint_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).acp_recipe_endpoint_tstr))))
	: (((*input).acp_recipe_endpoint_choice == acp_recipe_endpoint_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_acp_recipe(
		zcbor_state_t *state, const struct acp_recipe *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_encode(state, 4) && (((!(*input).acp_recipe_program_present || encode_repeated_acp_recipe_program(state, (&(*input).acp_recipe_program)))
	&& (!(*input).acp_recipe_args_present || encode_repeated_acp_recipe_args(state, (&(*input).acp_recipe_args)))
	&& (!(*input).acp_recipe_env_present || encode_repeated_acp_recipe_env(state, (&(*input).acp_recipe_env)))
	&& (!(*input).acp_recipe_endpoint_present || encode_repeated_acp_recipe_endpoint(state, (&(*input).acp_recipe_endpoint)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_acp_source(
		zcbor_state_t *state, const struct acp_source_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).acp_source_choice == acp_source_Builtin_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Builtin", tmp_str.len = sizeof("Builtin") - 1, &tmp_str)))))
	: (((*input).acp_source_choice == acp_source_Manual_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Manual", tmp_str.len = sizeof("Manual") - 1, &tmp_str)))))
	: (((*input).acp_source_choice == acp_source_Endpoint_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Endpoint", tmp_str.len = sizeof("Endpoint") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_acp_agent_entry_installed(
		zcbor_state_t *state, const struct acp_agent_entry_installed *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"installed", tmp_str.len = sizeof("installed") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).acp_agent_entry_installed)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_acp_agent_entry_version(
		zcbor_state_t *state, const struct acp_agent_entry_version_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"version", tmp_str.len = sizeof("version") - 1, &tmp_str)))))
	&& (((*input).acp_agent_entry_version_choice == acp_agent_entry_version_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).acp_agent_entry_version_tstr))))
	: (((*input).acp_agent_entry_version_choice == acp_agent_entry_version_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_acp_agent_entry_capabilities(
		zcbor_state_t *state, const struct acp_agent_entry_capabilities_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"capabilities", tmp_str.len = sizeof("capabilities") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).acp_agent_entry_capabilities_kv_pair_m_count, (zcbor_encoder_t *)encode_kv_pair, state, (*&(*input).acp_agent_entry_capabilities_kv_pair_m), sizeof(struct kv_pair))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_acp_agent_entry(
		zcbor_state_t *state, const struct acp_agent_entry *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 6) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).acp_agent_entry_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"recipe", tmp_str.len = sizeof("recipe") - 1, &tmp_str)))))
	&& (encode_acp_recipe(state, (&(*input).acp_agent_entry_recipe))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (encode_acp_source(state, (&(*input).acp_agent_entry_source))))
	&& (!(*input).acp_agent_entry_installed_present || encode_repeated_acp_agent_entry_installed(state, (&(*input).acp_agent_entry_installed)))
	&& (!(*input).acp_agent_entry_version_present || encode_repeated_acp_agent_entry_version(state, (&(*input).acp_agent_entry_version)))
	&& (!(*input).acp_agent_entry_capabilities_present || encode_repeated_acp_agent_entry_capabilities(state, (&(*input).acp_agent_entry_capabilities)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 6))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_acp_register(
		zcbor_state_t *state, const struct request_acp_register *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AcpRegister", tmp_str.len = sizeof("AcpRegister") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"entry", tmp_str.len = sizeof("entry") - 1, &tmp_str)))))
	&& (encode_acp_agent_entry(state, (&(*input).AcpRegister_entry))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_acp_remove(
		zcbor_state_t *state, const struct request_acp_remove *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AcpRemove", tmp_str.len = sizeof("AcpRemove") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).AcpRemove_name))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_skill_get(
		zcbor_state_t *state, const struct request_skill_get *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SkillGet", tmp_str.len = sizeof("SkillGet") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SkillGet_name))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_skill_put(
		zcbor_state_t *state, const struct request_skill_put *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SkillPut", tmp_str.len = sizeof("SkillPut") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"bundle", tmp_str.len = sizeof("bundle") - 1, &tmp_str)))))
	&& (encode_skill_bundle(state, (&(*input).SkillPut_bundle))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_provider_info_base_url(
		zcbor_state_t *state, const struct provider_info_base_url_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"base_url", tmp_str.len = sizeof("base_url") - 1, &tmp_str)))))
	&& (((*input).provider_info_base_url_choice == provider_info_base_url_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).provider_info_base_url_tstr))))
	: (((*input).provider_info_base_url_choice == provider_info_base_url_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_provider_info_available(
		zcbor_state_t *state, const struct provider_info_available *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"available", tmp_str.len = sizeof("available") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).provider_info_available)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_provider_info(
		zcbor_state_t *state, const struct provider_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).provider_info_name))))
	&& (!(*input).provider_info_base_url_present || encode_repeated_provider_info_base_url(state, (&(*input).provider_info_base_url)))
	&& (!(*input).provider_info_available_present || encode_repeated_provider_info_available(state, (&(*input).provider_info_available)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_provider_register(
		zcbor_state_t *state, const struct request_provider_register *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProviderRegister", tmp_str.len = sizeof("ProviderRegister") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (encode_provider_info(state, (&(*input).ProviderRegister_provider))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_tool_info_description(
		zcbor_state_t *state, const struct tool_info_description_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"description", tmp_str.len = sizeof("description") - 1, &tmp_str)))))
	&& (((*input).tool_info_description_choice == tool_info_description_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).tool_info_description_tstr))))
	: (((*input).tool_info_description_choice == tool_info_description_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_tool_info(
		zcbor_state_t *state, const struct tool_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).tool_info_name))))
	&& (!(*input).tool_info_description_present || encode_repeated_tool_info_description(state, (&(*input).tool_info_description)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_tool_register(
		zcbor_state_t *state, const struct request_tool_register *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ToolRegister", tmp_str.len = sizeof("ToolRegister") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"tool", tmp_str.len = sizeof("tool") - 1, &tmp_str)))))
	&& (encode_tool_info(state, (&(*input).ToolRegister_tool))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_command_invocation(
		zcbor_state_t *state, const struct command_invocation *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).command_invocation_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"args", tmp_str.len = sizeof("args") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).command_invocation_args))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (((*input).command_invocation_session_choice == command_invocation_session_session_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).command_invocation_session_session_id_m))))
	: (((*input).command_invocation_session_choice == command_invocation_session_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (((*input).command_invocation_origin_choice == command_invocation_origin_origin_m_c) ? ((encode_origin(state, (&(*input).command_invocation_origin_origin_m))))
	: (((*input).command_invocation_origin_choice == command_invocation_origin_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_command_invoke(
		zcbor_state_t *state, const struct request_command_invoke *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CommandInvoke", tmp_str.len = sizeof("CommandInvoke") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"invocation", tmp_str.len = sizeof("invocation") - 1, &tmp_str)))))
	&& (encode_command_invocation(state, (&(*input).CommandInvoke_invocation))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_node_config_view(
		zcbor_state_t *state, const struct node_config_view *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"format", tmp_str.len = sizeof("format") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).node_config_view_format))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).node_config_view_body))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_config_set(
		zcbor_state_t *state, const struct request_config_set *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConfigSet", tmp_str.len = sizeof("ConfigSet") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"config", tmp_str.len = sizeof("config") - 1, &tmp_str)))))
	&& (encode_node_config_view(state, (&(*input).ConfigSet_config))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_target(
		zcbor_state_t *state, const struct cron_spec_target_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"target", tmp_str.len = sizeof("target") - 1, &tmp_str)))))
	&& (((*input).cron_spec_target_choice == cron_spec_target_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).cron_spec_target_tstr))))
	: (((*input).cron_spec_target_choice == cron_spec_target_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_payload(
		zcbor_state_t *state, const struct cron_spec_payload *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"payload", tmp_str.len = sizeof("payload") - 1, &tmp_str)))))
	&& (zcbor_bstr_encode(state, (&(*input).cron_spec_payload)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_enabled(
		zcbor_state_t *state, const struct cron_spec_enabled *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"enabled", tmp_str.len = sizeof("enabled") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).cron_spec_enabled)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_timezone(
		zcbor_state_t *state, const struct cron_spec_timezone_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"timezone", tmp_str.len = sizeof("timezone") - 1, &tmp_str)))))
	&& (((*input).cron_spec_timezone_choice == cron_spec_timezone_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).cron_spec_timezone_tstr))))
	: (((*input).cron_spec_timezone_choice == cron_spec_timezone_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_repeat(
		zcbor_state_t *state, const struct cron_spec_repeat_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"repeat", tmp_str.len = sizeof("repeat") - 1, &tmp_str)))))
	&& (((*input).cron_spec_repeat_choice == cron_spec_repeat_uint_c) ? ((zcbor_uint32_encode(state, (&(*input).cron_spec_repeat_uint))))
	: (((*input).cron_spec_repeat_choice == cron_spec_repeat_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_jitter_secs(
		zcbor_state_t *state, const struct cron_spec_jitter_secs_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"jitter_secs", tmp_str.len = sizeof("jitter_secs") - 1, &tmp_str)))))
	&& (((*input).cron_spec_jitter_secs_choice == cron_spec_jitter_secs_uint_c) ? ((zcbor_uint32_encode(state, (&(*input).cron_spec_jitter_secs_uint))))
	: (((*input).cron_spec_jitter_secs_choice == cron_spec_jitter_secs_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_overlap_policy(
		zcbor_state_t *state, const struct overlap_policy_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).overlap_policy_choice == overlap_policy_Skip_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Skip", tmp_str.len = sizeof("Skip") - 1, &tmp_str)))))
	: (((*input).overlap_policy_choice == overlap_policy_Allow_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Allow", tmp_str.len = sizeof("Allow") - 1, &tmp_str)))))
	: (((*input).overlap_policy_choice == overlap_policy_Queue_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Queue", tmp_str.len = sizeof("Queue") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_overlap(
		zcbor_state_t *state, const struct cron_spec_overlap *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"overlap", tmp_str.len = sizeof("overlap") - 1, &tmp_str)))))
	&& (encode_overlap_policy(state, (&(*input).cron_spec_overlap)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_catch_up_policy(
		zcbor_state_t *state, const struct catch_up_policy_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).catch_up_policy_choice == catch_up_policy_Grace_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Grace", tmp_str.len = sizeof("Grace") - 1, &tmp_str)))))
	: (((*input).catch_up_policy_choice == catch_up_policy_Skip_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Skip", tmp_str.len = sizeof("Skip") - 1, &tmp_str)))))
	: (((*input).catch_up_policy_choice == catch_up_policy_Always_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Always", tmp_str.len = sizeof("Always") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_catch_up(
		zcbor_state_t *state, const struct cron_spec_catch_up *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"catch_up", tmp_str.len = sizeof("catch_up") - 1, &tmp_str)))))
	&& (encode_catch_up_policy(state, (&(*input).cron_spec_catch_up)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_script(
		zcbor_state_t *state, const struct cron_spec_script_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"script", tmp_str.len = sizeof("script") - 1, &tmp_str)))))
	&& (((*input).cron_spec_script_choice == cron_spec_script_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).cron_spec_script_tstr))))
	: (((*input).cron_spec_script_choice == cron_spec_script_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_no_agent(
		zcbor_state_t *state, const struct cron_spec_no_agent *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"no_agent", tmp_str.len = sizeof("no_agent") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).cron_spec_no_agent)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_context_from(
		zcbor_state_t *state, const struct cron_spec_context_from_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"context_from", tmp_str.len = sizeof("context_from") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).cron_spec_context_from_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).cron_spec_context_from_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_deliver(
		zcbor_state_t *state, const struct cron_spec_deliver_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"deliver", tmp_str.len = sizeof("deliver") - 1, &tmp_str)))))
	&& (((*input).cron_spec_deliver_choice == cron_spec_deliver_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).cron_spec_deliver_tstr))))
	: (((*input).cron_spec_deliver_choice == cron_spec_deliver_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_enabled_toolsets(
		zcbor_state_t *state, const struct cron_spec_enabled_toolsets_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"enabled_toolsets", tmp_str.len = sizeof("enabled_toolsets") - 1, &tmp_str)))))
	&& (((*input).cron_spec_enabled_toolsets_choice == enabled_toolsets_tstr_l_c) ? ((zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).enabled_toolsets_tstr_l_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).enabled_toolsets_tstr_l_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	: (((*input).cron_spec_enabled_toolsets_choice == cron_spec_enabled_toolsets_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_workdir(
		zcbor_state_t *state, const struct cron_spec_workdir_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"workdir", tmp_str.len = sizeof("workdir") - 1, &tmp_str)))))
	&& (((*input).cron_spec_workdir_choice == cron_spec_workdir_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).cron_spec_workdir_tstr))))
	: (((*input).cron_spec_workdir_choice == cron_spec_workdir_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_model(
		zcbor_state_t *state, const struct cron_spec_model_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (((*input).cron_spec_model_choice == cron_spec_model_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).cron_spec_model_tstr))))
	: (((*input).cron_spec_model_choice == cron_spec_model_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_provider(
		zcbor_state_t *state, const struct cron_spec_provider_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (((*input).cron_spec_provider_choice == cron_spec_provider_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).cron_spec_provider_tstr))))
	: (((*input).cron_spec_provider_choice == cron_spec_provider_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_skills(
		zcbor_state_t *state, const struct cron_spec_skills_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"skills", tmp_str.len = sizeof("skills") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).cron_spec_skills_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).cron_spec_skills_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_spec_origin(
		zcbor_state_t *state, const struct cron_spec_origin_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (((*input).cron_spec_origin_choice == cron_spec_origin_origin_m_c) ? ((encode_origin(state, (&(*input).cron_spec_origin_origin_m))))
	: (((*input).cron_spec_origin_choice == cron_spec_origin_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_cron_spec(
		zcbor_state_t *state, const struct cron_spec *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 20) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).cron_spec_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"schedule", tmp_str.len = sizeof("schedule") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).cron_spec_schedule))))
	&& (!(*input).cron_spec_target_present || encode_repeated_cron_spec_target(state, (&(*input).cron_spec_target)))
	&& (!(*input).cron_spec_payload_present || encode_repeated_cron_spec_payload(state, (&(*input).cron_spec_payload)))
	&& (!(*input).cron_spec_enabled_present || encode_repeated_cron_spec_enabled(state, (&(*input).cron_spec_enabled)))
	&& (!(*input).cron_spec_timezone_present || encode_repeated_cron_spec_timezone(state, (&(*input).cron_spec_timezone)))
	&& (!(*input).cron_spec_repeat_present || encode_repeated_cron_spec_repeat(state, (&(*input).cron_spec_repeat)))
	&& (!(*input).cron_spec_jitter_secs_present || encode_repeated_cron_spec_jitter_secs(state, (&(*input).cron_spec_jitter_secs)))
	&& (!(*input).cron_spec_overlap_present || encode_repeated_cron_spec_overlap(state, (&(*input).cron_spec_overlap)))
	&& (!(*input).cron_spec_catch_up_present || encode_repeated_cron_spec_catch_up(state, (&(*input).cron_spec_catch_up)))
	&& (!(*input).cron_spec_script_present || encode_repeated_cron_spec_script(state, (&(*input).cron_spec_script)))
	&& (!(*input).cron_spec_no_agent_present || encode_repeated_cron_spec_no_agent(state, (&(*input).cron_spec_no_agent)))
	&& (!(*input).cron_spec_context_from_present || encode_repeated_cron_spec_context_from(state, (&(*input).cron_spec_context_from)))
	&& (!(*input).cron_spec_deliver_present || encode_repeated_cron_spec_deliver(state, (&(*input).cron_spec_deliver)))
	&& (!(*input).cron_spec_enabled_toolsets_present || encode_repeated_cron_spec_enabled_toolsets(state, (&(*input).cron_spec_enabled_toolsets)))
	&& (!(*input).cron_spec_workdir_present || encode_repeated_cron_spec_workdir(state, (&(*input).cron_spec_workdir)))
	&& (!(*input).cron_spec_model_present || encode_repeated_cron_spec_model(state, (&(*input).cron_spec_model)))
	&& (!(*input).cron_spec_provider_present || encode_repeated_cron_spec_provider(state, (&(*input).cron_spec_provider)))
	&& (!(*input).cron_spec_skills_present || encode_repeated_cron_spec_skills(state, (&(*input).cron_spec_skills)))
	&& (!(*input).cron_spec_origin_present || encode_repeated_cron_spec_origin(state, (&(*input).cron_spec_origin)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 20))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_cron_create(
		zcbor_state_t *state, const struct request_cron_create *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronCreate", tmp_str.len = sizeof("CronCreate") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (encode_cron_spec(state, (&(*input).CronCreate_spec))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_cron_update(
		zcbor_state_t *state, const struct request_cron_update *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronUpdate", tmp_str.len = sizeof("CronUpdate") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CronUpdate_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (encode_cron_spec(state, (&(*input).CronUpdate_spec))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_cron_delete(
		zcbor_state_t *state, const struct request_cron_delete *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronDelete", tmp_str.len = sizeof("CronDelete") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CronDelete_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_cron_trigger(
		zcbor_state_t *state, const struct request_cron_trigger *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronTrigger", tmp_str.len = sizeof("CronTrigger") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CronTrigger_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_cron_runs(
		zcbor_state_t *state, const struct request_cron_runs *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronRuns", tmp_str.len = sizeof("CronRuns") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CronRuns_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_cron_pause(
		zcbor_state_t *state, const struct request_cron_pause *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronPause", tmp_str.len = sizeof("CronPause") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CronPause_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"paused", tmp_str.len = sizeof("paused") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).CronPause_paused))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_cron_accept_suggestion(
		zcbor_state_t *state, const struct request_cron_accept_suggestion *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronAcceptSuggestion", tmp_str.len = sizeof("CronAcceptSuggestion") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CronAcceptSuggestion_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_cron_dismiss_suggestion(
		zcbor_state_t *state, const struct request_cron_dismiss_suggestion *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronDismissSuggestion", tmp_str.len = sizeof("CronDismissSuggestion") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).CronDismissSuggestion_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_routing_get(
		zcbor_state_t *state, const struct request_routing_get *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"RoutingGet", tmp_str.len = sizeof("RoutingGet") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (encode_origin(state, (&(*input).RoutingGet_origin))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_chat_route_profile(
		zcbor_state_t *state, const struct chat_route_profile_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (((*input).chat_route_profile_choice == chat_route_profile_profile_ref_m_c) ? ((zcbor_tstr_encode(state, (&(*input).chat_route_profile_profile_ref_m))))
	: (((*input).chat_route_profile_choice == chat_route_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_isolation_policy(
		zcbor_state_t *state, const struct isolation_policy_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).isolation_policy_choice == isolation_policy_PerUser_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"PerUser", tmp_str.len = sizeof("PerUser") - 1, &tmp_str)))))
	: (((*input).isolation_policy_choice == isolation_policy_PerChat_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"PerChat", tmp_str.len = sizeof("PerChat") - 1, &tmp_str)))))
	: (((*input).isolation_policy_choice == isolation_policy_PerThread_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"PerThread", tmp_str.len = sizeof("PerThread") - 1, &tmp_str)))))
	: (((*input).isolation_policy_choice == isolation_policy_Shared_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Shared", tmp_str.len = sizeof("Shared") - 1, &tmp_str)))))
	: false))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_chat_route_isolation(
		zcbor_state_t *state, const struct chat_route_isolation *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"isolation", tmp_str.len = sizeof("isolation") - 1, &tmp_str)))))
	&& (encode_isolation_policy(state, (&(*input).chat_route_isolation)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_chat_route(
		zcbor_state_t *state, const struct chat_route *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (encode_origin(state, (&(*input).chat_route_origin))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).chat_route_session))))
	&& (!(*input).chat_route_profile_present || encode_repeated_chat_route_profile(state, (&(*input).chat_route_profile)))
	&& (!(*input).chat_route_isolation_present || encode_repeated_chat_route_isolation(state, (&(*input).chat_route_isolation)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_routing_set(
		zcbor_state_t *state, const struct request_routing_set *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"RoutingSet", tmp_str.len = sizeof("RoutingSet") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"route", tmp_str.len = sizeof("route") - 1, &tmp_str)))))
	&& (encode_chat_route(state, (&(*input).RoutingSet_route))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_RoutingBindChat_profile(
		zcbor_state_t *state, const struct RoutingBindChat_profile_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (((*input).RoutingBindChat_profile_choice == RoutingBindChat_profile_profile_ref_m_c) ? ((zcbor_tstr_encode(state, (&(*input).RoutingBindChat_profile_profile_ref_m))))
	: (((*input).RoutingBindChat_profile_choice == RoutingBindChat_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_routing_bind_chat(
		zcbor_state_t *state, const struct request_routing_bind_chat *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"RoutingBindChat", tmp_str.len = sizeof("RoutingBindChat") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (encode_origin(state, (&(*input).RoutingBindChat_origin))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).RoutingBindChat_session))))
	&& (!(*input).RoutingBindChat_profile_present || encode_repeated_RoutingBindChat_profile(state, (&(*input).RoutingBindChat_profile)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_routing_unbind_chat(
		zcbor_state_t *state, const struct request_routing_unbind_chat *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"RoutingUnbindChat", tmp_str.len = sizeof("RoutingUnbindChat") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (encode_origin(state, (&(*input).RoutingUnbindChat_origin))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_transport_rooms(
		zcbor_state_t *state, const struct request_transport_rooms *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"TransportRooms", tmp_str.len = sizeof("TransportRooms") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).TransportRooms_transport))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_list(
		zcbor_state_t *state, const struct request_conv_list *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvList", tmp_str.len = sizeof("ConvList") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvList_transport))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_get(
		zcbor_state_t *state, const struct request_conv_get *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvGet", tmp_str.len = sizeof("ConvGet") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvGet_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvGet_conv))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_create_details(
		zcbor_state_t *state, const struct request_conv_create_details *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvCreateDetails", tmp_str.len = sizeof("ConvCreateDetails") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvCreateDetails_transport))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_create_conversation_details_max_participants(
		zcbor_state_t *state, const struct create_conversation_details_max_participants *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max_participants", tmp_str.len = sizeof("max_participants") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).create_conversation_details_max_participants)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_contact_info_display_name(
		zcbor_state_t *state, const struct contact_info_display_name_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"display_name", tmp_str.len = sizeof("display_name") - 1, &tmp_str)))))
	&& (((*input).contact_info_display_name_choice == contact_info_display_name_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).contact_info_display_name_tstr))))
	: (((*input).contact_info_display_name_choice == contact_info_display_name_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_presence_primitive_t(
		zcbor_state_t *state, const struct presence_primitive_t_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).presence_primitive_t_choice == presence_primitive_t_Offline_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Offline", tmp_str.len = sizeof("Offline") - 1, &tmp_str)))))
	: (((*input).presence_primitive_t_choice == presence_primitive_t_Available_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Available", tmp_str.len = sizeof("Available") - 1, &tmp_str)))))
	: (((*input).presence_primitive_t_choice == presence_primitive_t_Idle_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Idle", tmp_str.len = sizeof("Idle") - 1, &tmp_str)))))
	: (((*input).presence_primitive_t_choice == presence_primitive_t_Invisible_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Invisible", tmp_str.len = sizeof("Invisible") - 1, &tmp_str)))))
	: (((*input).presence_primitive_t_choice == presence_primitive_t_Away_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Away", tmp_str.len = sizeof("Away") - 1, &tmp_str)))))
	: (((*input).presence_primitive_t_choice == presence_primitive_t_DoNotDisturb_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"DoNotDisturb", tmp_str.len = sizeof("DoNotDisturb") - 1, &tmp_str)))))
	: (((*input).presence_primitive_t_choice == presence_primitive_t_Streaming_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Streaming", tmp_str.len = sizeof("Streaming") - 1, &tmp_str)))))
	: (((*input).presence_primitive_t_choice == presence_primitive_t_OutOfOffice_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"OutOfOffice", tmp_str.len = sizeof("OutOfOffice") - 1, &tmp_str)))))
	: false))))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_presence_message(
		zcbor_state_t *state, const struct presence_message_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"message", tmp_str.len = sizeof("message") - 1, &tmp_str)))))
	&& (((*input).presence_message_choice == presence_message_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).presence_message_tstr))))
	: (((*input).presence_message_choice == presence_message_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_presence_emoji(
		zcbor_state_t *state, const struct presence_emoji_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"emoji", tmp_str.len = sizeof("emoji") - 1, &tmp_str)))))
	&& (((*input).presence_emoji_choice == presence_emoji_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).presence_emoji_tstr))))
	: (((*input).presence_emoji_choice == presence_emoji_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_presence_mobile(
		zcbor_state_t *state, const struct presence_mobile *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"mobile", tmp_str.len = sizeof("mobile") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).presence_mobile)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_presence_idle_since(
		zcbor_state_t *state, const struct presence_idle_since_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"idle_since", tmp_str.len = sizeof("idle_since") - 1, &tmp_str)))))
	&& (((*input).presence_idle_since_choice == presence_idle_since_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).presence_idle_since_uint64_m))))
	: (((*input).presence_idle_since_choice == presence_idle_since_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_presence(
		zcbor_state_t *state, const struct presence *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"primitive", tmp_str.len = sizeof("primitive") - 1, &tmp_str)))))
	&& (encode_presence_primitive_t(state, (&(*input).presence_primitive))))
	&& (!(*input).presence_message_present || encode_repeated_presence_message(state, (&(*input).presence_message)))
	&& (!(*input).presence_emoji_present || encode_repeated_presence_emoji(state, (&(*input).presence_emoji)))
	&& (!(*input).presence_mobile_present || encode_repeated_presence_mobile(state, (&(*input).presence_mobile)))
	&& (!(*input).presence_idle_since_present || encode_repeated_presence_idle_since(state, (&(*input).presence_idle_since)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_contact_info_presence(
		zcbor_state_t *state, const struct contact_info_presence *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"presence", tmp_str.len = sizeof("presence") - 1, &tmp_str)))))
	&& (encode_presence(state, (&(*input).contact_info_presence)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_contact_permission(
		zcbor_state_t *state, const struct contact_permission_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).contact_permission_choice == contact_permission_Unset_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Unset", tmp_str.len = sizeof("Unset") - 1, &tmp_str)))))
	: (((*input).contact_permission_choice == contact_permission_Allow_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Allow", tmp_str.len = sizeof("Allow") - 1, &tmp_str)))))
	: (((*input).contact_permission_choice == contact_permission_Deny_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Deny", tmp_str.len = sizeof("Deny") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_contact_info_permission(
		zcbor_state_t *state, const struct contact_info_permission *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"permission", tmp_str.len = sizeof("permission") - 1, &tmp_str)))))
	&& (encode_contact_permission(state, (&(*input).contact_info_permission)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_contact_info(
		zcbor_state_t *state, const struct contact_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).contact_info_id))))
	&& (!(*input).contact_info_display_name_present || encode_repeated_contact_info_display_name(state, (&(*input).contact_info_display_name)))
	&& (!(*input).contact_info_presence_present || encode_repeated_contact_info_presence(state, (&(*input).contact_info_presence)))
	&& (!(*input).contact_info_permission_present || encode_repeated_contact_info_permission(state, (&(*input).contact_info_permission)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_create_conversation_details_participants(
		zcbor_state_t *state, const struct create_conversation_details_participants_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"participants", tmp_str.len = sizeof("participants") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).create_conversation_details_participants_contact_info_m_count, (zcbor_encoder_t *)encode_contact_info, state, (*&(*input).create_conversation_details_participants_contact_info_m), sizeof(struct contact_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_auth_param_field(
		zcbor_state_t *state, const struct auth_param_field *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"key", tmp_str.len = sizeof("key") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_param_field_key))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"label", tmp_str.len = sizeof("label") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_param_field_label))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"required", tmp_str.len = sizeof("required") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).auth_param_field_required))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_account_settings_schema_fields(
		zcbor_state_t *state, const struct account_settings_schema_fields_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"fields", tmp_str.len = sizeof("fields") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).account_settings_schema_fields_auth_param_field_m_count, (zcbor_encoder_t *)encode_auth_param_field, state, (*&(*input).account_settings_schema_fields_auth_param_field_m), sizeof(struct auth_param_field))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_account_settings_schema(
		zcbor_state_t *state, const struct account_settings_schema *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_encode(state, 1) && (((!(*input).account_settings_schema_fields_present || encode_repeated_account_settings_schema_fields(state, (&(*input).account_settings_schema_fields)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_create_conversation_details_extras_schema(
		zcbor_state_t *state, const struct create_conversation_details_extras_schema *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"extras_schema", tmp_str.len = sizeof("extras_schema") - 1, &tmp_str)))))
	&& (encode_account_settings_schema(state, (&(*input).create_conversation_details_extras_schema)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_values_tstrtstr(
		zcbor_state_t *state, const struct values_tstrtstr *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_tstr_encode(state, (&(*input).account_settings_values_values_tstrtstr_key))))
	&& (zcbor_tstr_encode(state, (&(*input).values_tstrtstr)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_account_settings_values_values(
		zcbor_state_t *state, const struct account_settings_values_values_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"values", tmp_str.len = sizeof("values") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).values_tstrtstr_count, (zcbor_encoder_t *)encode_repeated_values_tstrtstr, state, (*&(*input).values_tstrtstr), sizeof(struct values_tstrtstr))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_account_settings_values(
		zcbor_state_t *state, const struct account_settings_values *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_encode(state, 1) && (((!(*input).account_settings_values_values_present || encode_repeated_account_settings_values_values(state, (&(*input).account_settings_values_values)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_create_conversation_details_extras(
		zcbor_state_t *state, const struct create_conversation_details_extras *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"extras", tmp_str.len = sizeof("extras") - 1, &tmp_str)))))
	&& (encode_account_settings_values(state, (&(*input).create_conversation_details_extras)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_create_conversation_details(
		zcbor_state_t *state, const struct create_conversation_details *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_encode(state, 4) && (((!(*input).create_conversation_details_max_participants_present || encode_repeated_create_conversation_details_max_participants(state, (&(*input).create_conversation_details_max_participants)))
	&& (!(*input).create_conversation_details_participants_present || encode_repeated_create_conversation_details_participants(state, (&(*input).create_conversation_details_participants)))
	&& (!(*input).create_conversation_details_extras_schema_present || encode_repeated_create_conversation_details_extras_schema(state, (&(*input).create_conversation_details_extras_schema)))
	&& (!(*input).create_conversation_details_extras_present || encode_repeated_create_conversation_details_extras(state, (&(*input).create_conversation_details_extras)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_create(
		zcbor_state_t *state, const struct request_conv_create *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvCreate", tmp_str.len = sizeof("ConvCreate") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvCreate_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"details", tmp_str.len = sizeof("details") - 1, &tmp_str)))))
	&& (encode_create_conversation_details(state, (&(*input).ConvCreate_details))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_join_details(
		zcbor_state_t *state, const struct request_conv_join_details *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvJoinDetails", tmp_str.len = sizeof("ConvJoinDetails") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvJoinDetails_transport))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_channel_join_details_name(
		zcbor_state_t *state, const struct channel_join_details_name_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (((*input).channel_join_details_name_choice == channel_join_details_name_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).channel_join_details_name_tstr))))
	: (((*input).channel_join_details_name_choice == channel_join_details_name_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_channel_join_details_name_max_length(
		zcbor_state_t *state, const struct channel_join_details_name_max_length *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name_max_length", tmp_str.len = sizeof("name_max_length") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).channel_join_details_name_max_length)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_channel_join_details_nickname(
		zcbor_state_t *state, const struct channel_join_details_nickname_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"nickname", tmp_str.len = sizeof("nickname") - 1, &tmp_str)))))
	&& (((*input).channel_join_details_nickname_choice == channel_join_details_nickname_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).channel_join_details_nickname_tstr))))
	: (((*input).channel_join_details_nickname_choice == channel_join_details_nickname_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_channel_join_details_nickname_supported(
		zcbor_state_t *state, const struct channel_join_details_nickname_supported *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"nickname_supported", tmp_str.len = sizeof("nickname_supported") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).channel_join_details_nickname_supported)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_channel_join_details_nickname_max_length(
		zcbor_state_t *state, const struct channel_join_details_nickname_max_length *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"nickname_max_length", tmp_str.len = sizeof("nickname_max_length") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).channel_join_details_nickname_max_length)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_channel_join_details_password(
		zcbor_state_t *state, const struct channel_join_details_password_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"password", tmp_str.len = sizeof("password") - 1, &tmp_str)))))
	&& (((*input).channel_join_details_password_choice == channel_join_details_password_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).channel_join_details_password_tstr))))
	: (((*input).channel_join_details_password_choice == channel_join_details_password_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_channel_join_details_password_supported(
		zcbor_state_t *state, const struct channel_join_details_password_supported *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"password_supported", tmp_str.len = sizeof("password_supported") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).channel_join_details_password_supported)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_channel_join_details_password_max_length(
		zcbor_state_t *state, const struct channel_join_details_password_max_length *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"password_max_length", tmp_str.len = sizeof("password_max_length") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).channel_join_details_password_max_length)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_channel_join_details_extras_schema(
		zcbor_state_t *state, const struct channel_join_details_extras_schema *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"extras_schema", tmp_str.len = sizeof("extras_schema") - 1, &tmp_str)))))
	&& (encode_account_settings_schema(state, (&(*input).channel_join_details_extras_schema)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_channel_join_details_extras(
		zcbor_state_t *state, const struct channel_join_details_extras *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"extras", tmp_str.len = sizeof("extras") - 1, &tmp_str)))))
	&& (encode_account_settings_values(state, (&(*input).channel_join_details_extras)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_channel_join_details(
		zcbor_state_t *state, const struct channel_join_details *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_encode(state, 10) && (((!(*input).channel_join_details_name_present || encode_repeated_channel_join_details_name(state, (&(*input).channel_join_details_name)))
	&& (!(*input).channel_join_details_name_max_length_present || encode_repeated_channel_join_details_name_max_length(state, (&(*input).channel_join_details_name_max_length)))
	&& (!(*input).channel_join_details_nickname_present || encode_repeated_channel_join_details_nickname(state, (&(*input).channel_join_details_nickname)))
	&& (!(*input).channel_join_details_nickname_supported_present || encode_repeated_channel_join_details_nickname_supported(state, (&(*input).channel_join_details_nickname_supported)))
	&& (!(*input).channel_join_details_nickname_max_length_present || encode_repeated_channel_join_details_nickname_max_length(state, (&(*input).channel_join_details_nickname_max_length)))
	&& (!(*input).channel_join_details_password_present || encode_repeated_channel_join_details_password(state, (&(*input).channel_join_details_password)))
	&& (!(*input).channel_join_details_password_supported_present || encode_repeated_channel_join_details_password_supported(state, (&(*input).channel_join_details_password_supported)))
	&& (!(*input).channel_join_details_password_max_length_present || encode_repeated_channel_join_details_password_max_length(state, (&(*input).channel_join_details_password_max_length)))
	&& (!(*input).channel_join_details_extras_schema_present || encode_repeated_channel_join_details_extras_schema(state, (&(*input).channel_join_details_extras_schema)))
	&& (!(*input).channel_join_details_extras_present || encode_repeated_channel_join_details_extras(state, (&(*input).channel_join_details_extras)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 10))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_join(
		zcbor_state_t *state, const struct request_conv_join *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvJoin", tmp_str.len = sizeof("ConvJoin") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvJoin_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"details", tmp_str.len = sizeof("details") - 1, &tmp_str)))))
	&& (encode_channel_join_details(state, (&(*input).ConvJoin_details))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_leave(
		zcbor_state_t *state, const struct request_conv_leave *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvLeave", tmp_str.len = sizeof("ConvLeave") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvLeave_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvLeave_conv))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_participant_contact(
		zcbor_state_t *state, const struct participant_contact *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Contact", tmp_str.len = sizeof("Contact") - 1, &tmp_str)))))
	&& (encode_contact_info(state, (&(*input).participant_contact_Contact))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_participant_agent(
		zcbor_state_t *state, const struct participant_agent *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Agent", tmp_str.len = sizeof("Agent") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Agent_profile))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"member", tmp_str.len = sizeof("member") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Agent_member))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_participant(
		zcbor_state_t *state, const struct participant_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).participant_choice == participant_contact_m_c) ? ((encode_participant_contact(state, (&(*input).participant_contact_m))))
	: (((*input).participant_choice == participant_agent_m_c) ? ((encode_participant_agent(state, (&(*input).participant_agent_m))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_conv_send_args_from(
		zcbor_state_t *state, const struct conv_send_args_from_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"from", tmp_str.len = sizeof("from") - 1, &tmp_str)))))
	&& (((*input).conv_send_args_from_choice == conv_send_args_from_participant_m_c) ? ((encode_participant(state, (&(*input).conv_send_args_from_participant_m))))
	: (((*input).conv_send_args_from_choice == conv_send_args_from_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_conv_send_args(
		zcbor_state_t *state, const struct conv_send_args *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).conv_send_args_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).conv_send_args_conv))))
	&& (!(*input).conv_send_args_from_present || encode_repeated_conv_send_args_from(state, (&(*input).conv_send_args_from)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"message", tmp_str.len = sizeof("message") - 1, &tmp_str)))))
	&& (encode_user_msg(state, (&(*input).conv_send_args_message))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_send(
		zcbor_state_t *state, const struct request_conv_send *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvSend", tmp_str.len = sizeof("ConvSend") - 1, &tmp_str)))))
	&& (encode_conv_send_args(state, (&(*input).request_conv_send_ConvSend))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_ConvSetTopic_topic(
		zcbor_state_t *state, const struct ConvSetTopic_topic_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"topic", tmp_str.len = sizeof("topic") - 1, &tmp_str)))))
	&& (((*input).ConvSetTopic_topic_choice == ConvSetTopic_topic_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).ConvSetTopic_topic_tstr))))
	: (((*input).ConvSetTopic_topic_choice == ConvSetTopic_topic_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_set_topic(
		zcbor_state_t *state, const struct request_conv_set_topic *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvSetTopic", tmp_str.len = sizeof("ConvSetTopic") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvSetTopic_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvSetTopic_conv))))
	&& (!(*input).ConvSetTopic_topic_present || encode_repeated_ConvSetTopic_topic(state, (&(*input).ConvSetTopic_topic)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_ConvSetTitle_title(
		zcbor_state_t *state, const struct ConvSetTitle_title_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (((*input).ConvSetTitle_title_choice == ConvSetTitle_title_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).ConvSetTitle_title_tstr))))
	: (((*input).ConvSetTitle_title_choice == ConvSetTitle_title_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_set_title(
		zcbor_state_t *state, const struct request_conv_set_title *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvSetTitle", tmp_str.len = sizeof("ConvSetTitle") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvSetTitle_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvSetTitle_conv))))
	&& (!(*input).ConvSetTitle_title_present || encode_repeated_ConvSetTitle_title(state, (&(*input).ConvSetTitle_title)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_ConvSetDescription_description(
		zcbor_state_t *state, const struct ConvSetDescription_description_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"description", tmp_str.len = sizeof("description") - 1, &tmp_str)))))
	&& (((*input).ConvSetDescription_description_choice == ConvSetDescription_description_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).ConvSetDescription_description_tstr))))
	: (((*input).ConvSetDescription_description_choice == ConvSetDescription_description_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_set_description(
		zcbor_state_t *state, const struct request_conv_set_description *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvSetDescription", tmp_str.len = sizeof("ConvSetDescription") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvSetDescription_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvSetDescription_conv))))
	&& (!(*input).ConvSetDescription_description_present || encode_repeated_ConvSetDescription_description(state, (&(*input).ConvSetDescription_description)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_delete(
		zcbor_state_t *state, const struct request_conv_delete *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvDelete", tmp_str.len = sizeof("ConvDelete") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvDelete_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ConvDelete_conv))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_conv_history_args_after_cursor(
		zcbor_state_t *state, const struct conv_history_args_after_cursor *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"after_cursor", tmp_str.len = sizeof("after_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).conv_history_args_after_cursor)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_conv_history_args_max(
		zcbor_state_t *state, const struct conv_history_args_max *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).conv_history_args_max)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_conv_history_args(
		zcbor_state_t *state, const struct conv_history_args *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).conv_history_args_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).conv_history_args_conv))))
	&& (!(*input).conv_history_args_after_cursor_present || encode_repeated_conv_history_args_after_cursor(state, (&(*input).conv_history_args_after_cursor)))
	&& (!(*input).conv_history_args_max_present || encode_repeated_conv_history_args_max(state, (&(*input).conv_history_args_max)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_conv_history(
		zcbor_state_t *state, const struct request_conv_history *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvHistory", tmp_str.len = sizeof("ConvHistory") - 1, &tmp_str)))))
	&& (encode_conv_history_args(state, (&(*input).request_conv_history_ConvHistory))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_member_invite_args_message(
		zcbor_state_t *state, const struct member_invite_args_message_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"message", tmp_str.len = sizeof("message") - 1, &tmp_str)))))
	&& (((*input).member_invite_args_message_choice == member_invite_args_message_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).member_invite_args_message_tstr))))
	: (((*input).member_invite_args_message_choice == member_invite_args_message_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_member_invite_args(
		zcbor_state_t *state, const struct member_invite_args *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).member_invite_args_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).member_invite_args_conv))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"who", tmp_str.len = sizeof("who") - 1, &tmp_str)))))
	&& (encode_participant(state, (&(*input).member_invite_args_who))))
	&& (!(*input).member_invite_args_message_present || encode_repeated_member_invite_args_message(state, (&(*input).member_invite_args_message)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_member_invite(
		zcbor_state_t *state, const struct request_member_invite *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"MemberInvite", tmp_str.len = sizeof("MemberInvite") - 1, &tmp_str)))))
	&& (encode_member_invite_args(state, (&(*input).request_member_invite_MemberInvite))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_member_remove_args_reason(
		zcbor_state_t *state, const struct member_remove_args_reason_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"reason", tmp_str.len = sizeof("reason") - 1, &tmp_str)))))
	&& (((*input).member_remove_args_reason_choice == member_remove_args_reason_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).member_remove_args_reason_tstr))))
	: (((*input).member_remove_args_reason_choice == member_remove_args_reason_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_member_remove_args(
		zcbor_state_t *state, const struct member_remove_args *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).member_remove_args_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).member_remove_args_conv))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"who", tmp_str.len = sizeof("who") - 1, &tmp_str)))))
	&& (encode_participant(state, (&(*input).member_remove_args_who))))
	&& (!(*input).member_remove_args_reason_present || encode_repeated_member_remove_args_reason(state, (&(*input).member_remove_args_reason)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_member_remove(
		zcbor_state_t *state, const struct request_member_remove *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"MemberRemove", tmp_str.len = sizeof("MemberRemove") - 1, &tmp_str)))))
	&& (encode_member_remove_args(state, (&(*input).request_member_remove_MemberRemove))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_member_ban_args_reason(
		zcbor_state_t *state, const struct member_ban_args_reason_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"reason", tmp_str.len = sizeof("reason") - 1, &tmp_str)))))
	&& (((*input).member_ban_args_reason_choice == member_ban_args_reason_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).member_ban_args_reason_tstr))))
	: (((*input).member_ban_args_reason_choice == member_ban_args_reason_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_member_ban_args(
		zcbor_state_t *state, const struct member_ban_args *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).member_ban_args_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).member_ban_args_conv))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"who", tmp_str.len = sizeof("who") - 1, &tmp_str)))))
	&& (encode_participant(state, (&(*input).member_ban_args_who))))
	&& (!(*input).member_ban_args_reason_present || encode_repeated_member_ban_args_reason(state, (&(*input).member_ban_args_reason)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_member_ban(
		zcbor_state_t *state, const struct request_member_ban *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"MemberBan", tmp_str.len = sizeof("MemberBan") - 1, &tmp_str)))))
	&& (encode_member_ban_args(state, (&(*input).request_member_ban_MemberBan))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_member_role(
		zcbor_state_t *state, const struct member_role_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).member_role_choice == member_role_None_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"None", tmp_str.len = sizeof("None") - 1, &tmp_str)))))
	: (((*input).member_role_choice == member_role_Voice_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Voice", tmp_str.len = sizeof("Voice") - 1, &tmp_str)))))
	: (((*input).member_role_choice == member_role_HalfOp_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"HalfOp", tmp_str.len = sizeof("HalfOp") - 1, &tmp_str)))))
	: (((*input).member_role_choice == member_role_Op_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Op", tmp_str.len = sizeof("Op") - 1, &tmp_str)))))
	: (((*input).member_role_choice == member_role_Founder_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Founder", tmp_str.len = sizeof("Founder") - 1, &tmp_str)))))
	: false)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_member_set_role_args(
		zcbor_state_t *state, const struct member_set_role_args *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).member_set_role_args_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"conv", tmp_str.len = sizeof("conv") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).member_set_role_args_conv))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"who", tmp_str.len = sizeof("who") - 1, &tmp_str)))))
	&& (encode_participant(state, (&(*input).member_set_role_args_who))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (encode_member_role(state, (&(*input).member_set_role_args_role))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_member_set_role(
		zcbor_state_t *state, const struct request_member_set_role *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"MemberSetRole", tmp_str.len = sizeof("MemberSetRole") - 1, &tmp_str)))))
	&& (encode_member_set_role_args(state, (&(*input).request_member_set_role_MemberSetRole))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_contact_get_profile(
		zcbor_state_t *state, const struct request_contact_get_profile *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ContactGetProfile", tmp_str.len = sizeof("ContactGetProfile") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ContactGetProfile_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"contact", tmp_str.len = sizeof("contact") - 1, &tmp_str)))))
	&& (encode_contact_info(state, (&(*input).ContactGetProfile_contact))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_ContactSetAlias_alias(
		zcbor_state_t *state, const struct ContactSetAlias_alias_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"alias", tmp_str.len = sizeof("alias") - 1, &tmp_str)))))
	&& (((*input).ContactSetAlias_alias_choice == ContactSetAlias_alias_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).ContactSetAlias_alias_tstr))))
	: (((*input).ContactSetAlias_alias_choice == ContactSetAlias_alias_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_contact_set_alias(
		zcbor_state_t *state, const struct request_contact_set_alias *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ContactSetAlias", tmp_str.len = sizeof("ContactSetAlias") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ContactSetAlias_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"contact", tmp_str.len = sizeof("contact") - 1, &tmp_str)))))
	&& (encode_contact_info(state, (&(*input).ContactSetAlias_contact))))
	&& (!(*input).ContactSetAlias_alias_present || encode_repeated_ContactSetAlias_alias(state, (&(*input).ContactSetAlias_alias)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_contact_action_menu(
		zcbor_state_t *state, const struct request_contact_action_menu *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ContactActionMenu", tmp_str.len = sizeof("ContactActionMenu") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ContactActionMenu_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"contact", tmp_str.len = sizeof("contact") - 1, &tmp_str)))))
	&& (encode_contact_info(state, (&(*input).ContactActionMenu_contact))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_DirectorySearch_query(
		zcbor_state_t *state, const struct DirectorySearch_query_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"query", tmp_str.len = sizeof("query") - 1, &tmp_str)))))
	&& (((*input).DirectorySearch_query_choice == DirectorySearch_query_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).DirectorySearch_query_tstr))))
	: (((*input).DirectorySearch_query_choice == DirectorySearch_query_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_directory_search(
		zcbor_state_t *state, const struct request_directory_search *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"DirectorySearch", tmp_str.len = sizeof("DirectorySearch") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).DirectorySearch_transport))))
	&& (!(*input).DirectorySearch_query_present || encode_repeated_DirectorySearch_query(state, (&(*input).DirectorySearch_query)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_root_id_host(
		zcbor_state_t *state, const struct fs_root_id_host *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"host", tmp_str.len = sizeof("host") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_root_id_host_host))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_root_id_session(
		zcbor_state_t *state, const struct fs_root_id_session *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_root_id_session_session))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_root_id_t(
		zcbor_state_t *state, const struct fs_root_id_t_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).fs_root_id_t_choice == fs_root_id_t_fs_root_id_host_m_c) ? ((encode_fs_root_id_host(state, (&(*input).fs_root_id_t_fs_root_id_host_m))))
	: (((*input).fs_root_id_t_choice == fs_root_id_t_workspace_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"workspace", tmp_str.len = sizeof("workspace") - 1, &tmp_str)))))
	: (((*input).fs_root_id_t_choice == fs_root_id_t_fs_root_id_session_m_c) ? ((encode_fs_root_id_session(state, (&(*input).fs_root_id_t_fs_root_id_session_m))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_FsList_show_ignored(
		zcbor_state_t *state, const struct FsList_show_ignored *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"show_ignored", tmp_str.len = sizeof("show_ignored") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).FsList_show_ignored)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_fs_list(
		zcbor_state_t *state, const struct request_fs_list *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsList", tmp_str.len = sizeof("FsList") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (encode_fs_root_id_t(state, (&(*input).FsList_root))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"dir", tmp_str.len = sizeof("dir") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).FsList_dir))))
	&& (!(*input).FsList_show_ignored_present || encode_repeated_FsList_show_ignored(state, (&(*input).FsList_show_ignored)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_fs_stat(
		zcbor_state_t *state, const struct request_fs_stat *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsStat", tmp_str.len = sizeof("FsStat") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (encode_fs_root_id_t(state, (&(*input).FsStat_root))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).FsStat_path))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_FsRead_max_bytes(
		zcbor_state_t *state, const struct FsRead_max_bytes *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max_bytes", tmp_str.len = sizeof("max_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).FsRead_max_bytes)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_fs_read(
		zcbor_state_t *state, const struct request_fs_read *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsRead", tmp_str.len = sizeof("FsRead") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (encode_fs_root_id_t(state, (&(*input).FsRead_root))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).FsRead_path))))
	&& (!(*input).FsRead_max_bytes_present || encode_repeated_FsRead_max_bytes(state, (&(*input).FsRead_max_bytes)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_revision(
		zcbor_state_t *state, const struct fs_revision *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"mtime_ms", tmp_str.len = sizeof("mtime_ms") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).fs_revision_mtime_ms))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"size", tmp_str.len = sizeof("size") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).fs_revision_size))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_write_args_base_revision(
		zcbor_state_t *state, const struct fs_write_args_base_revision_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"base_revision", tmp_str.len = sizeof("base_revision") - 1, &tmp_str)))))
	&& (((*input).fs_write_args_base_revision_choice == fs_write_args_base_revision_fs_revision_m_c) ? ((encode_fs_revision(state, (&(*input).fs_write_args_base_revision_fs_revision_m))))
	: (((*input).fs_write_args_base_revision_choice == fs_write_args_base_revision_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_write_args_force(
		zcbor_state_t *state, const struct fs_write_args_force *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"force", tmp_str.len = sizeof("force") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).fs_write_args_force)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_write_args(
		zcbor_state_t *state, const struct fs_write_args *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (encode_fs_root_id_t(state, (&(*input).fs_write_args_root))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_write_args_path))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"bytes", tmp_str.len = sizeof("bytes") - 1, &tmp_str)))))
	&& (zcbor_bstr_encode(state, (&(*input).fs_write_args_bytes))))
	&& (!(*input).fs_write_args_base_revision_present || encode_repeated_fs_write_args_base_revision(state, (&(*input).fs_write_args_base_revision)))
	&& (!(*input).fs_write_args_force_present || encode_repeated_fs_write_args_force(state, (&(*input).fs_write_args_force)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_fs_write(
		zcbor_state_t *state, const struct request_fs_write *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsWrite", tmp_str.len = sizeof("FsWrite") - 1, &tmp_str)))))
	&& (encode_fs_write_args(state, (&(*input).request_fs_write_FsWrite))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_search_query_regex(
		zcbor_state_t *state, const struct fs_search_query_regex *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"regex", tmp_str.len = sizeof("regex") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).fs_search_query_regex)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_search_query_case_sensitive(
		zcbor_state_t *state, const struct fs_search_query_case_sensitive *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"case_sensitive", tmp_str.len = sizeof("case_sensitive") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).fs_search_query_case_sensitive)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_search_query_max_results(
		zcbor_state_t *state, const struct fs_search_query_max_results *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max_results", tmp_str.len = sizeof("max_results") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).fs_search_query_max_results)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_search_query_page(
		zcbor_state_t *state, const struct fs_search_query_page *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"page", tmp_str.len = sizeof("page") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).fs_search_query_page)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_search_query(
		zcbor_state_t *state, const struct fs_search_query *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"query", tmp_str.len = sizeof("query") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_search_query_query))))
	&& (!(*input).fs_search_query_regex_present || encode_repeated_fs_search_query_regex(state, (&(*input).fs_search_query_regex)))
	&& (!(*input).fs_search_query_case_sensitive_present || encode_repeated_fs_search_query_case_sensitive(state, (&(*input).fs_search_query_case_sensitive)))
	&& (!(*input).fs_search_query_max_results_present || encode_repeated_fs_search_query_max_results(state, (&(*input).fs_search_query_max_results)))
	&& (!(*input).fs_search_query_page_present || encode_repeated_fs_search_query_page(state, (&(*input).fs_search_query_page)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_fs_search(
		zcbor_state_t *state, const struct request_fs_search *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsSearch", tmp_str.len = sizeof("FsSearch") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (encode_fs_root_id_t(state, (&(*input).FsSearch_root))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"query", tmp_str.len = sizeof("query") - 1, &tmp_str)))))
	&& (encode_fs_search_query(state, (&(*input).FsSearch_query))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_watch_after_args(
		zcbor_state_t *state, const struct fs_watch_after_args *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (encode_fs_root_id_t(state, (&(*input).fs_watch_after_args_root))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"dir", tmp_str.len = sizeof("dir") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_watch_after_args_dir))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"after_seq", tmp_str.len = sizeof("after_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).fs_watch_after_args_after_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).fs_watch_after_args_max))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_fs_watch_poll(
		zcbor_state_t *state, const struct request_fs_watch_poll *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsWatchPoll", tmp_str.len = sizeof("FsWatchPoll") - 1, &tmp_str)))))
	&& (encode_fs_watch_after_args(state, (&(*input).request_fs_watch_poll_FsWatchPoll))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_blob_put(
		zcbor_state_t *state, const struct request_blob_put *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"BlobPut", tmp_str.len = sizeof("BlobPut") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"bytes", tmp_str.len = sizeof("bytes") - 1, &tmp_str)))))
	&& (zcbor_bstr_encode(state, (&(*input).BlobPut_bytes))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_byte_range(
		zcbor_state_t *state, const struct byte_range *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"offset", tmp_str.len = sizeof("offset") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).byte_range_offset))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"len", tmp_str.len = sizeof("len") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).byte_range_len))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_BlobGet_range(
		zcbor_state_t *state, const struct BlobGet_range_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"range", tmp_str.len = sizeof("range") - 1, &tmp_str)))))
	&& (((*input).BlobGet_range_choice == BlobGet_range_byte_range_m_c) ? ((encode_byte_range(state, (&(*input).BlobGet_range_byte_range_m))))
	: (((*input).BlobGet_range_choice == BlobGet_range_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_blob_get(
		zcbor_state_t *state, const struct request_blob_get *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"BlobGet", tmp_str.len = sizeof("BlobGet") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"hash", tmp_str.len = sizeof("hash") - 1, &tmp_str)))))
	&& (encode_content_hash(state, (&(*input).BlobGet_hash))))
	&& (!(*input).BlobGet_range_present || encode_repeated_BlobGet_range(state, (&(*input).BlobGet_range)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_blob_stat(
		zcbor_state_t *state, const struct request_blob_stat *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"BlobStat", tmp_str.len = sizeof("BlobStat") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"hash", tmp_str.len = sizeof("hash") - 1, &tmp_str)))))
	&& (encode_content_hash(state, (&(*input).BlobStat_hash))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_write_from_blob_args_base_revision(
		zcbor_state_t *state, const struct fs_write_from_blob_args_base_revision_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"base_revision", tmp_str.len = sizeof("base_revision") - 1, &tmp_str)))))
	&& (((*input).fs_write_from_blob_args_base_revision_choice == fs_write_from_blob_args_base_revision_fs_revision_m_c) ? ((encode_fs_revision(state, (&(*input).fs_write_from_blob_args_base_revision_fs_revision_m))))
	: (((*input).fs_write_from_blob_args_base_revision_choice == fs_write_from_blob_args_base_revision_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_write_from_blob_args_force(
		zcbor_state_t *state, const struct fs_write_from_blob_args_force *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"force", tmp_str.len = sizeof("force") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).fs_write_from_blob_args_force)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_write_from_blob_args(
		zcbor_state_t *state, const struct fs_write_from_blob_args *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (encode_fs_root_id_t(state, (&(*input).fs_write_from_blob_args_root))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_write_from_blob_args_path))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"hash", tmp_str.len = sizeof("hash") - 1, &tmp_str)))))
	&& (encode_content_hash(state, (&(*input).fs_write_from_blob_args_hash))))
	&& (!(*input).fs_write_from_blob_args_base_revision_present || encode_repeated_fs_write_from_blob_args_base_revision(state, (&(*input).fs_write_from_blob_args_base_revision)))
	&& (!(*input).fs_write_from_blob_args_force_present || encode_repeated_fs_write_from_blob_args_force(state, (&(*input).fs_write_from_blob_args_force)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_fs_write_from_blob(
		zcbor_state_t *state, const struct request_fs_write_from_blob *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsWriteFromBlob", tmp_str.len = sizeof("FsWriteFromBlob") - 1, &tmp_str)))))
	&& (encode_fs_write_from_blob_args(state, (&(*input).request_fs_write_from_blob_FsWriteFromBlob))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_user_create(
		zcbor_state_t *state, const struct request_user_create *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"UserCreate", tmp_str.len = sizeof("UserCreate") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"username", tmp_str.len = sizeof("username") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).UserCreate_username))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"password", tmp_str.len = sizeof("password") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).UserCreate_password))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"roles", tmp_str.len = sizeof("roles") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).UserCreate_roles_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).UserCreate_roles_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_user_disable(
		zcbor_state_t *state, const struct request_user_disable *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"UserDisable", tmp_str.len = sizeof("UserDisable") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).UserDisable_user_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"disabled", tmp_str.len = sizeof("disabled") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).UserDisable_disabled))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_user_set_roles(
		zcbor_state_t *state, const struct request_user_set_roles *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"UserSetRoles", tmp_str.len = sizeof("UserSetRoles") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).UserSetRoles_user_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"roles", tmp_str.len = sizeof("roles") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).UserSetRoles_roles_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).UserSetRoles_roles_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_user_set_password(
		zcbor_state_t *state, const struct request_user_set_password *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"UserSetPassword", tmp_str.len = sizeof("UserSetPassword") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).UserSetPassword_user_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"password", tmp_str.len = sizeof("password") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).UserSetPassword_password))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_session_revoke(
		zcbor_state_t *state, const struct request_session_revoke *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionRevoke", tmp_str.len = sizeof("SessionRevoke") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SessionRevoke_user_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_resource_grant_create(
		zcbor_state_t *state, const struct request_resource_grant_create *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ResourceGrantCreate", tmp_str.len = sizeof("ResourceGrantCreate") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ResourceGrantCreate_user_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"resource_kind", tmp_str.len = sizeof("resource_kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ResourceGrantCreate_resource_kind))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"resource_id", tmp_str.len = sizeof("resource_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ResourceGrantCreate_resource_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"capability", tmp_str.len = sizeof("capability") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ResourceGrantCreate_capability))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_ResourceGrantList_user_id(
		zcbor_state_t *state, const struct ResourceGrantList_user_id_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (((*input).ResourceGrantList_user_id_choice == ResourceGrantList_user_id_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).ResourceGrantList_user_id_tstr))))
	: (((*input).ResourceGrantList_user_id_choice == ResourceGrantList_user_id_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_resource_grant_list(
		zcbor_state_t *state, const struct request_resource_grant_list *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ResourceGrantList", tmp_str.len = sizeof("ResourceGrantList") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((!(*input).ResourceGrantList_user_id_present || encode_repeated_ResourceGrantList_user_id(state, (&(*input).ResourceGrantList_user_id)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_request_resource_grant_revoke(
		zcbor_state_t *state, const struct request_resource_grant_revoke *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ResourceGrantRevoke", tmp_str.len = sizeof("ResourceGrantRevoke") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ResourceGrantRevoke_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_routed(
		zcbor_state_t *state, const struct response_routed *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Routed", tmp_str.len = sizeof("Routed") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Routed_session))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_completion_source_process(
		zcbor_state_t *state, const struct completion_source_process *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Process", tmp_str.len = sizeof("Process") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).completion_source_process_Process))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_completion_source_delegation(
		zcbor_state_t *state, const struct completion_source_delegation *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Delegation", tmp_str.len = sizeof("Delegation") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).completion_source_delegation_Delegation))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_completion_source(
		zcbor_state_t *state, const struct completion_source_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).completion_source_choice == completion_source_process_m_c) ? ((encode_completion_source_process(state, (&(*input).completion_source_process_m))))
	: (((*input).completion_source_choice == completion_source_delegation_m_c) ? ((encode_completion_source_delegation(state, (&(*input).completion_source_delegation_m))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_turn_trigger_background(
		zcbor_state_t *state, const struct turn_trigger_background *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"BackgroundCompletion", tmp_str.len = sizeof("BackgroundCompletion") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (encode_completion_source(state, (&(*input).BackgroundCompletion_source))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_turn_trigger_scheduled(
		zcbor_state_t *state, const struct turn_trigger_scheduled *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Scheduled", tmp_str.len = sizeof("Scheduled") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"job", tmp_str.len = sizeof("job") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Scheduled_job))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_turn_trigger(
		zcbor_state_t *state, const struct turn_trigger_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).turn_trigger_choice == turn_trigger_User_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"User", tmp_str.len = sizeof("User") - 1, &tmp_str)))))
	: (((*input).turn_trigger_choice == turn_trigger_Steer_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Steer", tmp_str.len = sizeof("Steer") - 1, &tmp_str)))))
	: (((*input).turn_trigger_choice == turn_trigger_background_m_c) ? ((encode_turn_trigger_background(state, (&(*input).turn_trigger_background_m))))
	: (((*input).turn_trigger_choice == turn_trigger_scheduled_m_c) ? ((encode_turn_trigger_scheduled(state, (&(*input).turn_trigger_scheduled_m))))
	: false))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_turn_started(
		zcbor_state_t *state, const struct agent_event_turn_started *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"TurnStarted", tmp_str.len = sizeof("TurnStarted") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).TurnStarted_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"trigger", tmp_str.len = sizeof("trigger") - 1, &tmp_str)))))
	&& (encode_turn_trigger(state, (&(*input).TurnStarted_trigger))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_text_delta(
		zcbor_state_t *state, const struct agent_event_text_delta *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"TextDelta", tmp_str.len = sizeof("TextDelta") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).TextDelta_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).TextDelta_text))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_reasoning_delta(
		zcbor_state_t *state, const struct agent_event_reasoning_delta *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ReasoningDelta", tmp_str.len = sizeof("ReasoningDelta") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).ReasoningDelta_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ReasoningDelta_text))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_content_delta(
		zcbor_state_t *state, const struct agent_event_content_delta *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ContentDelta", tmp_str.len = sizeof("ContentDelta") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).ContentDelta_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ContentDelta_kind))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (zcbor_bstr_encode(state, (&(*input).ContentDelta_body))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_tool_detail(
		zcbor_state_t *state, const struct tool_detail *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).tool_detail_kind))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (zcbor_bstr_encode(state, (&(*input).tool_detail_body))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_tool_call_view_detail(
		zcbor_state_t *state, const struct tool_call_view_detail_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (((*input).tool_call_view_detail_choice == tool_call_view_detail_tool_detail_m_c) ? ((encode_tool_detail(state, (&(*input).tool_call_view_detail_tool_detail_m))))
	: (((*input).tool_call_view_detail_choice == tool_call_view_detail_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_tool_call_view(
		zcbor_state_t *state, const struct tool_call_view *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"call_id", tmp_str.len = sizeof("call_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).tool_call_view_call_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).tool_call_view_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"args_summary", tmp_str.len = sizeof("args_summary") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).tool_call_view_args_summary))))
	&& (!(*input).tool_call_view_detail_present || encode_repeated_tool_call_view_detail(state, (&(*input).tool_call_view_detail)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_tool_started(
		zcbor_state_t *state, const struct agent_event_tool_started *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ToolStarted", tmp_str.len = sizeof("ToolStarted") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).ToolStarted_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"call", tmp_str.len = sizeof("call") - 1, &tmp_str)))))
	&& (encode_tool_call_view(state, (&(*input).ToolStarted_call))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_tool_result_view_detail(
		zcbor_state_t *state, const struct tool_result_view_detail_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (((*input).tool_result_view_detail_choice == tool_result_view_detail_tool_detail_m_c) ? ((encode_tool_detail(state, (&(*input).tool_result_view_detail_tool_detail_m))))
	: (((*input).tool_result_view_detail_choice == tool_result_view_detail_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_tool_result_view(
		zcbor_state_t *state, const struct tool_result_view *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"call_id", tmp_str.len = sizeof("call_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).tool_result_view_call_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ok", tmp_str.len = sizeof("ok") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).tool_result_view_ok))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"summary", tmp_str.len = sizeof("summary") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).tool_result_view_summary))))
	&& (!(*input).tool_result_view_detail_present || encode_repeated_tool_result_view_detail(state, (&(*input).tool_result_view_detail)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_tool_finished(
		zcbor_state_t *state, const struct agent_event_tool_finished *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ToolFinished", tmp_str.len = sizeof("ToolFinished") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).ToolFinished_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"result", tmp_str.len = sizeof("result") - 1, &tmp_str)))))
	&& (encode_tool_result_view(state, (&(*input).ToolFinished_result))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_usage_delta(
		zcbor_state_t *state, const struct usage_delta *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 7) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"input_tokens", tmp_str.len = sizeof("input_tokens") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).usage_delta_input_tokens))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"output_tokens", tmp_str.len = sizeof("output_tokens") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).usage_delta_output_tokens))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"api_calls", tmp_str.len = sizeof("api_calls") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).usage_delta_api_calls))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"cache_read_tokens", tmp_str.len = sizeof("cache_read_tokens") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).usage_delta_cache_read_tokens))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"cache_write_tokens", tmp_str.len = sizeof("cache_write_tokens") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).usage_delta_cache_write_tokens))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"reasoning_tokens", tmp_str.len = sizeof("reasoning_tokens") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).usage_delta_reasoning_tokens))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"cost_micros", tmp_str.len = sizeof("cost_micros") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).usage_delta_cost_micros))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 7))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_usage(
		zcbor_state_t *state, const struct agent_event_usage *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Usage", tmp_str.len = sizeof("Usage") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Usage_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"delta", tmp_str.len = sizeof("delta") - 1, &tmp_str)))))
	&& (encode_usage_delta(state, (&(*input).Usage_delta))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_context_status(
		zcbor_state_t *state, const struct context_status *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"used_tokens", tmp_str.len = sizeof("used_tokens") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).context_status_used_tokens))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max_tokens", tmp_str.len = sizeof("max_tokens") - 1, &tmp_str)))))
	&& (((*input).context_status_max_tokens_choice == context_status_max_tokens_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).context_status_max_tokens_uint64_m))))
	: (((*input).context_status_max_tokens_choice == context_status_max_tokens_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"budget_tokens", tmp_str.len = sizeof("budget_tokens") - 1, &tmp_str)))))
	&& (((*input).context_status_budget_tokens_choice == context_status_budget_tokens_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).context_status_budget_tokens_uint64_m))))
	: (((*input).context_status_budget_tokens_choice == context_status_budget_tokens_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"compacted", tmp_str.len = sizeof("compacted") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).context_status_compacted))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"dropped_turns", tmp_str.len = sizeof("dropped_turns") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).context_status_dropped_turns))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_context(
		zcbor_state_t *state, const struct agent_event_context *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Context", tmp_str.len = sizeof("Context") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Context_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"status", tmp_str.len = sizeof("status") - 1, &tmp_str)))))
	&& (encode_context_status(state, (&(*input).Context_status))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_rate_limit_snapshot(
		zcbor_state_t *state, const struct rate_limit_snapshot *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"remaining", tmp_str.len = sizeof("remaining") - 1, &tmp_str)))))
	&& (((*input).rate_limit_snapshot_remaining_choice == rate_limit_snapshot_remaining_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).rate_limit_snapshot_remaining_uint64_m))))
	: (((*input).rate_limit_snapshot_remaining_choice == rate_limit_snapshot_remaining_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"limit", tmp_str.len = sizeof("limit") - 1, &tmp_str)))))
	&& (((*input).rate_limit_snapshot_limit_choice == rate_limit_snapshot_limit_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).rate_limit_snapshot_limit_uint64_m))))
	: (((*input).rate_limit_snapshot_limit_choice == rate_limit_snapshot_limit_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"reset_ms", tmp_str.len = sizeof("reset_ms") - 1, &tmp_str)))))
	&& (((*input).rate_limit_snapshot_reset_ms_choice == rate_limit_snapshot_reset_ms_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).rate_limit_snapshot_reset_ms_uint64_m))))
	: (((*input).rate_limit_snapshot_reset_ms_choice == rate_limit_snapshot_reset_ms_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_rate_limit(
		zcbor_state_t *state, const struct agent_event_rate_limit *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"RateLimit", tmp_str.len = sizeof("RateLimit") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).RateLimit_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"snapshot", tmp_str.len = sizeof("snapshot") - 1, &tmp_str)))))
	&& (encode_rate_limit_snapshot(state, (&(*input).RateLimit_snapshot))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_end_reason(
		zcbor_state_t *state, const struct end_reason_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).end_reason_choice == end_reason_Completed_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Completed", tmp_str.len = sizeof("Completed") - 1, &tmp_str)))))
	: (((*input).end_reason_choice == end_reason_Suspended_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Suspended", tmp_str.len = sizeof("Suspended") - 1, &tmp_str)))))
	: (((*input).end_reason_choice == end_reason_Interrupted_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Interrupted", tmp_str.len = sizeof("Interrupted") - 1, &tmp_str)))))
	: (((*input).end_reason_choice == end_reason_BudgetExhausted_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"BudgetExhausted", tmp_str.len = sizeof("BudgetExhausted") - 1, &tmp_str)))))
	: (((*input).end_reason_choice == end_reason_NoProgress_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"NoProgress", tmp_str.len = sizeof("NoProgress") - 1, &tmp_str)))))
	: (((*input).end_reason_choice == end_reason_Failed_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Failed", tmp_str.len = sizeof("Failed") - 1, &tmp_str)))))
	: false))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_turn_summary(
		zcbor_state_t *state, const struct turn_summary *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"end_reason", tmp_str.len = sizeof("end_reason") - 1, &tmp_str)))))
	&& (encode_end_reason(state, (&(*input).turn_summary_end_reason))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"final_text", tmp_str.len = sizeof("final_text") - 1, &tmp_str)))))
	&& (((*input).turn_summary_final_text_choice == turn_summary_final_text_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).turn_summary_final_text_tstr))))
	: (((*input).turn_summary_final_text_choice == turn_summary_final_text_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"usage", tmp_str.len = sizeof("usage") - 1, &tmp_str)))))
	&& (encode_usage_delta(state, (&(*input).turn_summary_usage))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_turn_finished(
		zcbor_state_t *state, const struct agent_event_turn_finished *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"TurnFinished", tmp_str.len = sizeof("TurnFinished") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).TurnFinished_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"summary", tmp_str.len = sizeof("summary") - 1, &tmp_str)))))
	&& (encode_turn_summary(state, (&(*input).TurnFinished_summary))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_error(
		zcbor_state_t *state, const struct agent_event_error *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Error", tmp_str.len = sizeof("Error") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Error_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"failure", tmp_str.len = sizeof("failure") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Error_failure))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_steered(
		zcbor_state_t *state, const struct agent_event_steered *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Steered", tmp_str.len = sizeof("Steered") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Steered_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Steered_request_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"accepted", tmp_str.len = sizeof("accepted") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).Steered_accepted))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_conv_turn_view(
		zcbor_state_t *state, const struct conv_turn_view *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).conv_turn_view_role))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).conv_turn_view_text))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"tools", tmp_str.len = sizeof("tools") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).conv_turn_view_tools_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).conv_turn_view_tools_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_conv_view(
		zcbor_state_t *state, const struct conv_view *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"epoch", tmp_str.len = sizeof("epoch") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).conv_view_epoch))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"turns", tmp_str.len = sizeof("turns") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).conv_view_turns_conv_turn_view_m_count, (zcbor_encoder_t *)encode_conv_turn_view, state, (*&(*input).conv_view_turns_conv_turn_view_m), sizeof(struct conv_turn_view))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"waiting_for", tmp_str.len = sizeof("waiting_for") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).conv_view_waiting_for_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).conv_view_waiting_for_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_snapshot(
		zcbor_state_t *state, const struct agent_event_snapshot *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Snapshot", tmp_str.len = sizeof("Snapshot") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Snapshot_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Snapshot_request_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"view", tmp_str.len = sizeof("view") - 1, &tmp_str)))))
	&& (encode_conv_view(state, (&(*input).Snapshot_view))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event_rewound(
		zcbor_state_t *state, const struct agent_event_rewound *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Rewound", tmp_str.len = sizeof("Rewound") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Rewound_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Rewound_request_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"to_cursor", tmp_str.len = sizeof("to_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Rewound_to_cursor))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"epoch", tmp_str.len = sizeof("epoch") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Rewound_epoch))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_agent_event(
		zcbor_state_t *state, const struct agent_event_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).agent_event_choice == agent_event_turn_started_m_c) ? ((encode_agent_event_turn_started(state, (&(*input).agent_event_turn_started_m))))
	: (((*input).agent_event_choice == agent_event_text_delta_m_c) ? ((encode_agent_event_text_delta(state, (&(*input).agent_event_text_delta_m))))
	: (((*input).agent_event_choice == agent_event_reasoning_delta_m_c) ? ((encode_agent_event_reasoning_delta(state, (&(*input).agent_event_reasoning_delta_m))))
	: (((*input).agent_event_choice == agent_event_content_delta_m_c) ? ((encode_agent_event_content_delta(state, (&(*input).agent_event_content_delta_m))))
	: (((*input).agent_event_choice == agent_event_tool_started_m_c) ? ((encode_agent_event_tool_started(state, (&(*input).agent_event_tool_started_m))))
	: (((*input).agent_event_choice == agent_event_tool_finished_m_c) ? ((encode_agent_event_tool_finished(state, (&(*input).agent_event_tool_finished_m))))
	: (((*input).agent_event_choice == agent_event_usage_m_c) ? ((encode_agent_event_usage(state, (&(*input).agent_event_usage_m))))
	: (((*input).agent_event_choice == agent_event_context_m_c) ? ((encode_agent_event_context(state, (&(*input).agent_event_context_m))))
	: (((*input).agent_event_choice == agent_event_rate_limit_m_c) ? ((encode_agent_event_rate_limit(state, (&(*input).agent_event_rate_limit_m))))
	: (((*input).agent_event_choice == agent_event_turn_finished_m_c) ? ((encode_agent_event_turn_finished(state, (&(*input).agent_event_turn_finished_m))))
	: (((*input).agent_event_choice == agent_event_error_m_c) ? ((encode_agent_event_error(state, (&(*input).agent_event_error_m))))
	: (((*input).agent_event_choice == agent_event_steered_m_c) ? ((encode_agent_event_steered(state, (&(*input).agent_event_steered_m))))
	: (((*input).agent_event_choice == agent_event_snapshot_m_c) ? ((encode_agent_event_snapshot(state, (&(*input).agent_event_snapshot_m))))
	: (((*input).agent_event_choice == agent_event_rewound_m_c) ? ((encode_agent_event_rewound(state, (&(*input).agent_event_rewound_m))))
	: false))))))))))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_outbound_event(
		zcbor_state_t *state, const struct outbound_event *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Event", tmp_str.len = sizeof("Event") - 1, &tmp_str)))))
	&& (encode_agent_event(state, (&(*input).outbound_event_Event))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_request_kind_approval(
		zcbor_state_t *state, const struct host_request_kind_approval *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Approval", tmp_str.len = sizeof("Approval") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"prompt", tmp_str.len = sizeof("prompt") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Approval_prompt))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_request_kind_input(
		zcbor_state_t *state, const struct host_request_kind_input *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Input", tmp_str.len = sizeof("Input") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"prompt", tmp_str.len = sizeof("prompt") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Input_prompt))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_request_kind_choice(
		zcbor_state_t *state, const struct host_request_kind_choice *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Choice", tmp_str.len = sizeof("Choice") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"prompt", tmp_str.len = sizeof("prompt") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Choice_prompt))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"options", tmp_str.len = sizeof("options") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).Choice_options_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).Choice_options_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_request_kind_delegate(
		zcbor_state_t *state, const struct host_request_kind_delegate *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Delegate", tmp_str.len = sizeof("Delegate") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"label", tmp_str.len = sizeof("label") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Delegate_label))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"budget", tmp_str.len = sizeof("budget") - 1, &tmp_str)))))
	&& (encode_budget(state, (&(*input).Delegate_budget))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_spawn_spec(
		zcbor_state_t *state, const struct spawn_spec *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).spawn_spec_kind))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seed", tmp_str.len = sizeof("seed") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FromConversation", tmp_str.len = sizeof("FromConversation") - 1, &tmp_str)))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_request_kind_spawn(
		zcbor_state_t *state, const struct host_request_kind_spawn *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Spawn", tmp_str.len = sizeof("Spawn") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (encode_spawn_spec(state, (&(*input).Spawn_spec))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_request_kind_t(
		zcbor_state_t *state, const struct host_request_kind_t_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).host_request_kind_t_choice == host_request_kind_t_host_request_kind_approval_m_c) ? ((encode_host_request_kind_approval(state, (&(*input).host_request_kind_t_host_request_kind_approval_m))))
	: (((*input).host_request_kind_t_choice == host_request_kind_t_host_request_kind_input_m_c) ? ((encode_host_request_kind_input(state, (&(*input).host_request_kind_t_host_request_kind_input_m))))
	: (((*input).host_request_kind_t_choice == host_request_kind_t_host_request_kind_choice_m_c) ? ((encode_host_request_kind_choice(state, (&(*input).host_request_kind_t_host_request_kind_choice_m))))
	: (((*input).host_request_kind_t_choice == host_request_kind_t_host_request_kind_delegate_m_c) ? ((encode_host_request_kind_delegate(state, (&(*input).host_request_kind_t_host_request_kind_delegate_m))))
	: (((*input).host_request_kind_t_choice == host_request_kind_t_host_request_kind_spawn_m_c) ? ((encode_host_request_kind_spawn(state, (&(*input).host_request_kind_t_host_request_kind_spawn_m))))
	: false)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_host_request(
		zcbor_state_t *state, const struct host_request *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).host_request_request_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (encode_host_request_kind_t(state, (&(*input).host_request_kind))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_outbound_request(
		zcbor_state_t *state, const struct outbound_request *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Request", tmp_str.len = sizeof("Request") - 1, &tmp_str)))))
	&& (encode_host_request(state, (&(*input).outbound_request_Request))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_outbound(
		zcbor_state_t *state, const struct outbound_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).outbound_choice == outbound_event_m_c) ? ((encode_outbound_event(state, (&(*input).outbound_event_m))))
	: (((*input).outbound_choice == outbound_request_m_c) ? ((encode_outbound_request(state, (&(*input).outbound_request_m))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_drained(
		zcbor_state_t *state, const struct response_drained *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Drained", tmp_str.len = sizeof("Drained") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_drained_Drained_outbound_m_count, (zcbor_encoder_t *)encode_outbound, state, (*&(*input).response_drained_Drained_outbound_m), sizeof(struct outbound_r))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_service_health(
		zcbor_state_t *state, const struct service_health *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).service_health_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ok", tmp_str.len = sizeof("ok") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).service_health_ok))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"restarts", tmp_str.len = sizeof("restarts") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).service_health_restarts))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (((*input).service_health_detail_choice == service_health_detail_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).service_health_detail_tstr))))
	: (((*input).service_health_detail_choice == service_health_detail_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_health_report(
		zcbor_state_t *state, const struct health_report *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"all_ok", tmp_str.len = sizeof("all_ok") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).health_report_all_ok))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"services", tmp_str.len = sizeof("services") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).health_report_services_service_health_m_count, (zcbor_encoder_t *)encode_service_health, state, (*&(*input).health_report_services_service_health_m), sizeof(struct service_health))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_health(
		zcbor_state_t *state, const struct response_health *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Health", tmp_str.len = sizeof("Health") - 1, &tmp_str)))))
	&& (encode_health_report(state, (&(*input).response_health_Health))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_stats_report(
		zcbor_state_t *state, const struct stats_report *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"pending_jobs", tmp_str.len = sizeof("pending_jobs") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).stats_report_pending_jobs))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"pending_wakes", tmp_str.len = sizeof("pending_wakes") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).stats_report_pending_wakes))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"sessions", tmp_str.len = sizeof("sessions") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).stats_report_sessions))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"active", tmp_str.len = sizeof("active") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).stats_report_active))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"usage", tmp_str.len = sizeof("usage") - 1, &tmp_str)))))
	&& (encode_usage_delta(state, (&(*input).stats_report_usage))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_stats(
		zcbor_state_t *state, const struct response_stats *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Stats", tmp_str.len = sizeof("Stats") - 1, &tmp_str)))))
	&& (encode_stats_report(state, (&(*input).response_stats_Stats))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_telemetry_dump(
		zcbor_state_t *state, const struct telemetry_dump *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 7) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"usage", tmp_str.len = sizeof("usage") - 1, &tmp_str)))))
	&& (encode_usage_delta(state, (&(*input).telemetry_dump_usage))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"events", tmp_str.len = sizeof("events") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).telemetry_dump_events))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"healthy", tmp_str.len = sizeof("healthy") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).telemetry_dump_healthy))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"pending_jobs", tmp_str.len = sizeof("pending_jobs") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).telemetry_dump_pending_jobs))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"pending_wakes", tmp_str.len = sizeof("pending_wakes") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).telemetry_dump_pending_wakes))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"sessions", tmp_str.len = sizeof("sessions") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).telemetry_dump_sessions))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"active", tmp_str.len = sizeof("active") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).telemetry_dump_active))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 7))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_telemetry(
		zcbor_state_t *state, const struct response_telemetry *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Telemetry", tmp_str.len = sizeof("Telemetry") - 1, &tmp_str)))))
	&& (encode_telemetry_dump(state, (&(*input).response_telemetry_Telemetry))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_state_suspended(
		zcbor_state_t *state, const struct session_state_suspended *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Suspended", tmp_str.len = sizeof("Suspended") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"job_id", tmp_str.len = sizeof("job_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Suspended_job_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_state(
		zcbor_state_t *state, const struct session_state_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).session_state_choice == session_state_Active_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Active", tmp_str.len = sizeof("Active") - 1, &tmp_str)))))
	: (((*input).session_state_choice == session_state_suspended_m_c) ? ((encode_session_state_suspended(state, (&(*input).session_state_suspended_m))))
	: (((*input).session_state_choice == session_state_Ready_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Ready", tmp_str.len = sizeof("Ready") - 1, &tmp_str)))))
	: (((*input).session_state_choice == session_state_Completed_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Completed", tmp_str.len = sizeof("Completed") - 1, &tmp_str)))))
	: (((*input).session_state_choice == session_state_Unknown_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Unknown", tmp_str.len = sizeof("Unknown") - 1, &tmp_str)))))
	: false)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_info_rewindable(
		zcbor_state_t *state, const struct session_info_rewindable *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"rewindable", tmp_str.len = sizeof("rewindable") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).session_info_rewindable)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_info_bound_profile(
		zcbor_state_t *state, const struct session_info_bound_profile_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"bound_profile", tmp_str.len = sizeof("bound_profile") - 1, &tmp_str)))))
	&& (((*input).session_info_bound_profile_choice == session_info_bound_profile_profile_ref_m_c) ? ((zcbor_tstr_encode(state, (&(*input).session_info_bound_profile_profile_ref_m))))
	: (((*input).session_info_bound_profile_choice == session_info_bound_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_info_title(
		zcbor_state_t *state, const struct session_info_title_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (((*input).session_info_title_choice == session_info_title_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).session_info_title_tstr))))
	: (((*input).session_info_title_choice == session_info_title_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_info_last_activity_ms(
		zcbor_state_t *state, const struct session_info_last_activity_ms_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"last_activity_ms", tmp_str.len = sizeof("last_activity_ms") - 1, &tmp_str)))))
	&& (((*input).session_info_last_activity_ms_choice == session_info_last_activity_ms_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).session_info_last_activity_ms_uint64_m))))
	: (((*input).session_info_last_activity_ms_choice == session_info_last_activity_ms_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_lifecycle(
		zcbor_state_t *state, const struct lifecycle_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).lifecycle_choice == lifecycle_Durable_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Durable", tmp_str.len = sizeof("Durable") - 1, &tmp_str)))))
	: (((*input).lifecycle_choice == lifecycle_Live_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Live", tmp_str.len = sizeof("Live") - 1, &tmp_str)))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_info_lifecycle(
		zcbor_state_t *state, const struct session_info_lifecycle *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"lifecycle", tmp_str.len = sizeof("lifecycle") - 1, &tmp_str)))))
	&& (encode_lifecycle(state, (&(*input).session_info_lifecycle)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_role(
		zcbor_state_t *state, const struct session_role_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).session_role_choice == session_role_Primary_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Primary", tmp_str.len = sizeof("Primary") - 1, &tmp_str)))))
	: (((*input).session_role_choice == session_role_ManagedChild_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ManagedChild", tmp_str.len = sizeof("ManagedChild") - 1, &tmp_str)))))
	: (((*input).session_role_choice == session_role_EphemeralSubagent_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"EphemeralSubagent", tmp_str.len = sizeof("EphemeralSubagent") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_info_role(
		zcbor_state_t *state, const struct session_info_role *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (encode_session_role(state, (&(*input).session_info_role)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_info_parent(
		zcbor_state_t *state, const struct session_info_parent_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"parent", tmp_str.len = sizeof("parent") - 1, &tmp_str)))))
	&& (((*input).session_info_parent_choice == session_info_parent_session_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).session_info_parent_session_id_m))))
	: (((*input).session_info_parent_choice == session_info_parent_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_info_pinned(
		zcbor_state_t *state, const struct session_info_pinned *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"pinned", tmp_str.len = sizeof("pinned") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).session_info_pinned)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_info_archived(
		zcbor_state_t *state, const struct session_info_archived *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"archived", tmp_str.len = sizeof("archived") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).session_info_archived)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_info(
		zcbor_state_t *state, const struct session_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 11) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).session_info_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (encode_session_state(state, (&(*input).session_info_state))))
	&& (!(*input).session_info_rewindable_present || encode_repeated_session_info_rewindable(state, (&(*input).session_info_rewindable)))
	&& (!(*input).session_info_bound_profile_present || encode_repeated_session_info_bound_profile(state, (&(*input).session_info_bound_profile)))
	&& (!(*input).session_info_title_present || encode_repeated_session_info_title(state, (&(*input).session_info_title)))
	&& (!(*input).session_info_last_activity_ms_present || encode_repeated_session_info_last_activity_ms(state, (&(*input).session_info_last_activity_ms)))
	&& (!(*input).session_info_lifecycle_present || encode_repeated_session_info_lifecycle(state, (&(*input).session_info_lifecycle)))
	&& (!(*input).session_info_role_present || encode_repeated_session_info_role(state, (&(*input).session_info_role)))
	&& (!(*input).session_info_parent_present || encode_repeated_session_info_parent(state, (&(*input).session_info_parent)))
	&& (!(*input).session_info_pinned_present || encode_repeated_session_info_pinned(state, (&(*input).session_info_pinned)))
	&& (!(*input).session_info_archived_present || encode_repeated_session_info_archived(state, (&(*input).session_info_archived)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 11))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_sessions(
		zcbor_state_t *state, const struct response_sessions *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Sessions", tmp_str.len = sizeof("Sessions") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_sessions_Sessions_session_info_m_count, (zcbor_encoder_t *)encode_session_info, state, (*&(*input).response_sessions_Sessions_session_info_m), sizeof(struct session_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_approval_info_path(
		zcbor_state_t *state, const struct approval_info_path_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (((*input).approval_info_path_choice == approval_info_path_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).approval_info_path_tstr))))
	: (((*input).approval_info_path_choice == approval_info_path_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_approval_info(
		zcbor_state_t *state, const struct approval_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).approval_info_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).approval_info_request_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"prompt", tmp_str.len = sizeof("prompt") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).approval_info_prompt))))
	&& (!(*input).approval_info_path_present || encode_repeated_approval_info_path(state, (&(*input).approval_info_path)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_approvals(
		zcbor_state_t *state, const struct response_approvals *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Approvals", tmp_str.len = sizeof("Approvals") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_approvals_Approvals_approval_info_m_count, (zcbor_encoder_t *)encode_approval_info, state, (*&(*input).response_approvals_Approvals_approval_info_m), sizeof(struct approval_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fleet_report(
		zcbor_state_t *state, const struct fleet_report *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"children", tmp_str.len = sizeof("children") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).fleet_report_children_unit_id_m_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).fleet_report_children_unit_id_m), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"usage", tmp_str.len = sizeof("usage") - 1, &tmp_str)))))
	&& (encode_usage_delta(state, (&(*input).fleet_report_usage))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_fleet(
		zcbor_state_t *state, const struct response_fleet *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Fleet", tmp_str.len = sizeof("Fleet") - 1, &tmp_str)))))
	&& (encode_fleet_report(state, (&(*input).response_fleet_Fleet))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_unit_kind(
		zcbor_state_t *state, const struct unit_kind_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).unit_kind_choice == unit_kind_Engine_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Engine", tmp_str.len = sizeof("Engine") - 1, &tmp_str)))))
	: (((*input).unit_kind_choice == unit_kind_Host_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Host", tmp_str.len = sizeof("Host") - 1, &tmp_str)))))
	: (((*input).unit_kind_choice == unit_kind_Orchestrator_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Orchestrator", tmp_str.len = sizeof("Orchestrator") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_unit_state_finished(
		zcbor_state_t *state, const struct unit_state_finished *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Finished", tmp_str.len = sizeof("Finished") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"end_reason", tmp_str.len = sizeof("end_reason") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Finished_end_reason))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_unit_state(
		zcbor_state_t *state, const struct unit_state_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).unit_state_choice == unit_state_Running_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Running", tmp_str.len = sizeof("Running") - 1, &tmp_str)))))
	: (((*input).unit_state_choice == unit_state_finished_m_c) ? ((encode_unit_state_finished(state, (&(*input).unit_state_finished_m))))
	: (((*input).unit_state_choice == unit_state_Unknown_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Unknown", tmp_str.len = sizeof("Unknown") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_unit_node_profile(
		zcbor_state_t *state, const struct unit_node_profile_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (((*input).unit_node_profile_choice == unit_node_profile_profile_ref_m_c) ? ((zcbor_tstr_encode(state, (&(*input).unit_node_profile_profile_ref_m))))
	: (((*input).unit_node_profile_choice == unit_node_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_unit_node_session(
		zcbor_state_t *state, const struct unit_node_session_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (((*input).unit_node_session_choice == unit_node_session_session_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).unit_node_session_session_id_m))))
	: (((*input).unit_node_session_choice == unit_node_session_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_unit_node_title(
		zcbor_state_t *state, const struct unit_node_title_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (((*input).unit_node_title_choice == unit_node_title_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).unit_node_title_tstr))))
	: (((*input).unit_node_title_choice == unit_node_title_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_unit_node_role(
		zcbor_state_t *state, const struct unit_node_role_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (((*input).unit_node_role_choice == unit_node_role_session_role_m_c) ? ((encode_session_role(state, (&(*input).unit_node_role_session_role_m))))
	: (((*input).unit_node_role_choice == unit_node_role_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_unit_node(
		zcbor_state_t *state, const struct unit_node *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 10) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).unit_node_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (encode_unit_kind(state, (&(*input).unit_node_kind))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (encode_unit_state(state, (&(*input).unit_node_state))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"work", tmp_str.len = sizeof("work") - 1, &tmp_str)))))
	&& (((*input).unit_node_work_choice == unit_node_work_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).unit_node_work_tstr))))
	: (((*input).unit_node_work_choice == unit_node_work_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"usage", tmp_str.len = sizeof("usage") - 1, &tmp_str)))))
	&& (encode_usage_delta(state, (&(*input).unit_node_usage))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"children", tmp_str.len = sizeof("children") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).unit_node_children_unit_id_m_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).unit_node_children_unit_id_m), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (!(*input).unit_node_profile_present || encode_repeated_unit_node_profile(state, (&(*input).unit_node_profile)))
	&& (!(*input).unit_node_session_present || encode_repeated_unit_node_session(state, (&(*input).unit_node_session)))
	&& (!(*input).unit_node_title_present || encode_repeated_unit_node_title(state, (&(*input).unit_node_title)))
	&& (!(*input).unit_node_role_present || encode_repeated_unit_node_role(state, (&(*input).unit_node_role)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 10))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_tree_report(
		zcbor_state_t *state, const struct tree_report *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"root", tmp_str.len = sizeof("root") - 1, &tmp_str)))))
	&& (((*input).tree_report_root_choice == tree_report_root_unit_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).tree_report_root_unit_id_m))))
	: (((*input).tree_report_root_choice == tree_report_root_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"nodes", tmp_str.len = sizeof("nodes") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).tree_report_nodes_unit_node_m_count, (zcbor_encoder_t *)encode_unit_node, state, (*&(*input).tree_report_nodes_unit_node_m), sizeof(struct unit_node))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_tree(
		zcbor_state_t *state, const struct response_tree *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Tree", tmp_str.len = sizeof("Tree") - 1, &tmp_str)))))
	&& (encode_tree_report(state, (&(*input).response_tree_Tree))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_unit(
		zcbor_state_t *state, const struct response_unit *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Unit", tmp_str.len = sizeof("Unit") - 1, &tmp_str)))))
	&& (((*input).response_unit_Unit_choice == response_unit_Unit_unit_node_m_c) ? ((encode_unit_node(state, (&(*input).response_unit_Unit_unit_node_m))))
	: (((*input).response_unit_Unit_choice == response_unit_Unit_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_manage_event_view_started(
		zcbor_state_t *state, const struct manage_event_view_started *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Started", tmp_str.len = sizeof("Started") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Started_seq))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_manage_event_view_progress(
		zcbor_state_t *state, const struct manage_event_view_progress *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Progress", tmp_str.len = sizeof("Progress") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Progress_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (((*input).Progress_text_choice == Progress_text_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).Progress_text_tstr))))
	: (((*input).Progress_text_choice == Progress_text_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_manage_event_view_usage(
		zcbor_state_t *state, const struct manage_event_view_usage *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Usage", tmp_str.len = sizeof("Usage") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Usage_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"delta", tmp_str.len = sizeof("delta") - 1, &tmp_str)))))
	&& (encode_usage_delta(state, (&(*input).Usage_delta))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_manage_event_view_finished(
		zcbor_state_t *state, const struct manage_event_view_finished *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Finished", tmp_str.len = sizeof("Finished") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Finished_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"end_reason", tmp_str.len = sizeof("end_reason") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Finished_end_reason))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"summary", tmp_str.len = sizeof("summary") - 1, &tmp_str)))))
	&& (((*input).Finished_summary_choice == Finished_summary_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).Finished_summary_tstr))))
	: (((*input).Finished_summary_choice == Finished_summary_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_manage_event_view_error(
		zcbor_state_t *state, const struct manage_event_view_error *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Error", tmp_str.len = sizeof("Error") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Error_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"message", tmp_str.len = sizeof("message") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Error_message))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_subagent_phase(
		zcbor_state_t *state, const struct subagent_phase_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).subagent_phase_choice == subagent_phase_Spawned_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Spawned", tmp_str.len = sizeof("Spawned") - 1, &tmp_str)))))
	: (((*input).subagent_phase_choice == subagent_phase_Finished_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Finished", tmp_str.len = sizeof("Finished") - 1, &tmp_str)))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_manage_event_view_subagent(
		zcbor_state_t *state, const struct manage_event_view_subagent *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Subagent", tmp_str.len = sizeof("Subagent") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Subagent_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"child", tmp_str.len = sizeof("child") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Subagent_child))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (encode_session_role(state, (&(*input).Subagent_role))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"phase", tmp_str.len = sizeof("phase") - 1, &tmp_str)))))
	&& (encode_subagent_phase(state, (&(*input).Subagent_phase))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"active_children", tmp_str.len = sizeof("active_children") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).Subagent_active_children))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_manage_event_view(
		zcbor_state_t *state, const struct manage_event_view_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).manage_event_view_choice == manage_event_view_started_m_c) ? ((encode_manage_event_view_started(state, (&(*input).manage_event_view_started_m))))
	: (((*input).manage_event_view_choice == manage_event_view_progress_m_c) ? ((encode_manage_event_view_progress(state, (&(*input).manage_event_view_progress_m))))
	: (((*input).manage_event_view_choice == manage_event_view_usage_m_c) ? ((encode_manage_event_view_usage(state, (&(*input).manage_event_view_usage_m))))
	: (((*input).manage_event_view_choice == manage_event_view_finished_m_c) ? ((encode_manage_event_view_finished(state, (&(*input).manage_event_view_finished_m))))
	: (((*input).manage_event_view_choice == manage_event_view_error_m_c) ? ((encode_manage_event_view_error(state, (&(*input).manage_event_view_error_m))))
	: (((*input).manage_event_view_choice == manage_event_view_subagent_m_c) ? ((encode_manage_event_view_subagent(state, (&(*input).manage_event_view_subagent_m))))
	: false))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_unit_events(
		zcbor_state_t *state, const struct response_unit_events *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"UnitEvents", tmp_str.len = sizeof("UnitEvents") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_unit_events_UnitEvents_manage_event_view_m_count, (zcbor_encoder_t *)encode_manage_event_view, state, (*&(*input).response_unit_events_UnitEvents_manage_event_view_m), sizeof(struct manage_event_view_r))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_journal_record_payload_management(
		zcbor_state_t *state, const struct journal_record_payload_management *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Management", tmp_str.len = sizeof("Management") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Management_detail))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_transcript_role(
		zcbor_state_t *state, const struct transcript_role_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).transcript_role_choice == transcript_role_Assistant_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Assistant", tmp_str.len = sizeof("Assistant") - 1, &tmp_str)))))
	: (((*input).transcript_role_choice == transcript_role_User_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"User", tmp_str.len = sizeof("User") - 1, &tmp_str)))))
	: (((*input).transcript_role_choice == transcript_role_System_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"System", tmp_str.len = sizeof("System") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_transcript_block_message(
		zcbor_state_t *state, const struct transcript_block_message *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Message", tmp_str.len = sizeof("Message") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (encode_transcript_role(state, (&(*input).Message_role))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Message_text))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_ToolCall_detail(
		zcbor_state_t *state, const struct ToolCall_detail_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (((*input).ToolCall_detail_choice == ToolCall_detail_tool_detail_m_c) ? ((encode_tool_detail(state, (&(*input).ToolCall_detail_tool_detail_m))))
	: (((*input).ToolCall_detail_choice == ToolCall_detail_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_transcript_block_tool_call(
		zcbor_state_t *state, const struct transcript_block_tool_call *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ToolCall", tmp_str.len = sizeof("ToolCall") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"call_id", tmp_str.len = sizeof("call_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ToolCall_call_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ToolCall_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"args_summary", tmp_str.len = sizeof("args_summary") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ToolCall_args_summary))))
	&& (!(*input).ToolCall_detail_present || encode_repeated_ToolCall_detail(state, (&(*input).ToolCall_detail)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_ToolResult_detail(
		zcbor_state_t *state, const struct ToolResult_detail_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (((*input).ToolResult_detail_choice == ToolResult_detail_tool_detail_m_c) ? ((encode_tool_detail(state, (&(*input).ToolResult_detail_tool_detail_m))))
	: (((*input).ToolResult_detail_choice == ToolResult_detail_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_transcript_block_tool_result(
		zcbor_state_t *state, const struct transcript_block_tool_result *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ToolResult", tmp_str.len = sizeof("ToolResult") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"call_id", tmp_str.len = sizeof("call_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ToolResult_call_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ok", tmp_str.len = sizeof("ok") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).ToolResult_ok))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"summary", tmp_str.len = sizeof("summary") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ToolResult_summary))))
	&& (!(*input).ToolResult_detail_present || encode_repeated_ToolResult_detail(state, (&(*input).ToolResult_detail)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_transcript_block_request(
		zcbor_state_t *state, const struct transcript_block_request *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Request", tmp_str.len = sizeof("Request") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).Request_request_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (encode_host_request_kind_t(state, (&(*input).Request_kind))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_transcript_block_content(
		zcbor_state_t *state, const struct transcript_block_content *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Content", tmp_str.len = sizeof("Content") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Content_kind))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (zcbor_bstr_encode(state, (&(*input).Content_body))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_transcript_block(
		zcbor_state_t *state, const struct transcript_block_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).transcript_block_choice == transcript_block_message_m_c) ? ((encode_transcript_block_message(state, (&(*input).transcript_block_message_m))))
	: (((*input).transcript_block_choice == transcript_block_tool_call_m_c) ? ((encode_transcript_block_tool_call(state, (&(*input).transcript_block_tool_call_m))))
	: (((*input).transcript_block_choice == transcript_block_tool_result_m_c) ? ((encode_transcript_block_tool_result(state, (&(*input).transcript_block_tool_result_m))))
	: (((*input).transcript_block_choice == transcript_block_request_m_c) ? ((encode_transcript_block_request(state, (&(*input).transcript_block_request_m))))
	: (((*input).transcript_block_choice == transcript_block_content_m_c) ? ((encode_transcript_block_content(state, (&(*input).transcript_block_content_m))))
	: false)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_journal_record_payload_block(
		zcbor_state_t *state, const struct journal_record_payload_block *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Block", tmp_str.len = sizeof("Block") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"block", tmp_str.len = sizeof("block") - 1, &tmp_str)))))
	&& (encode_transcript_block(state, (&(*input).Block_block))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_journal_record_payload_t(
		zcbor_state_t *state, const struct journal_record_payload_t_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).journal_record_payload_t_choice == journal_record_payload_t_journal_record_payload_management_m_c) ? ((encode_journal_record_payload_management(state, (&(*input).journal_record_payload_t_journal_record_payload_management_m))))
	: (((*input).journal_record_payload_t_choice == journal_record_payload_t_journal_record_payload_block_m_c) ? ((encode_journal_record_payload_block(state, (&(*input).journal_record_payload_t_journal_record_payload_block_m))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_journal_record(
		zcbor_state_t *state, const struct journal_record *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 9) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"cursor", tmp_str.len = sizeof("cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).journal_record_cursor))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"segment", tmp_str.len = sizeof("segment") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).journal_record_segment))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).journal_record_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"epoch", tmp_str.len = sizeof("epoch") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).journal_record_epoch))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"trace", tmp_str.len = sizeof("trace") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).journal_record_trace))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).journal_record_kind))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"timestamp_ms", tmp_str.len = sizeof("timestamp_ms") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).journal_record_timestamp_ms))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"verified", tmp_str.len = sizeof("verified") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).journal_record_verified))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"payload", tmp_str.len = sizeof("payload") - 1, &tmp_str)))))
	&& (encode_journal_record_payload_t(state, (&(*input).journal_record_payload))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 9))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_journal_page_view(
		zcbor_state_t *state, const struct journal_page_view *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"entries", tmp_str.len = sizeof("entries") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).journal_page_view_entries_journal_record_m_count, (zcbor_encoder_t *)encode_journal_record, state, (*&(*input).journal_page_view_entries_journal_record_m), sizeof(struct journal_record))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"next_cursor", tmp_str.len = sizeof("next_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).journal_page_view_next_cursor))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"head_cursor", tmp_str.len = sizeof("head_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).journal_page_view_head_cursor))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"sealed_after", tmp_str.len = sizeof("sealed_after") - 1, &tmp_str)))))
	&& (((*input).journal_page_view_sealed_after_choice == journal_page_view_sealed_after_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).journal_page_view_sealed_after_uint64_m))))
	: (((*input).journal_page_view_sealed_after_choice == journal_page_view_sealed_after_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_journal(
		zcbor_state_t *state, const struct response_journal *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Journal", tmp_str.len = sizeof("Journal") - 1, &tmp_str)))))
	&& (encode_journal_page_view(state, (&(*input).response_journal_Journal))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_direction(
		zcbor_state_t *state, const struct direction_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).direction_choice == direction_Inbound_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Inbound", tmp_str.len = sizeof("Inbound") - 1, &tmp_str)))))
	: (((*input).direction_choice == direction_Outbound_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Outbound", tmp_str.len = sizeof("Outbound") - 1, &tmp_str)))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_disposition(
		zcbor_state_t *state, const struct disposition_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).disposition_choice == disposition_Context_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Context", tmp_str.len = sizeof("Context") - 1, &tmp_str)))))
	: (((*input).disposition_choice == disposition_Transport_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Transport", tmp_str.len = sizeof("Transport") - 1, &tmp_str)))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_payload_event(
		zcbor_state_t *state, const struct session_payload_event *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Event", tmp_str.len = sizeof("Event") - 1, &tmp_str)))))
	&& (encode_agent_event(state, (&(*input).session_payload_event_Event))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_payload_request(
		zcbor_state_t *state, const struct session_payload_request *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Request", tmp_str.len = sizeof("Request") - 1, &tmp_str)))))
	&& (encode_host_request(state, (&(*input).session_payload_request_Request))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_payload_command(
		zcbor_state_t *state, const struct session_payload_command *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Command", tmp_str.len = sizeof("Command") - 1, &tmp_str)))))
	&& (encode_agent_command(state, (&(*input).session_payload_command_Command))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_payload_response(
		zcbor_state_t *state, const struct session_payload_response *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Response", tmp_str.len = sizeof("Response") - 1, &tmp_str)))))
	&& (encode_host_response(state, (&(*input).session_payload_response_Response))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_payload_meta(
		zcbor_state_t *state, const struct session_payload_meta *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Meta", tmp_str.len = sizeof("Meta") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Meta_kind))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"body", tmp_str.len = sizeof("body") - 1, &tmp_str)))))
	&& (zcbor_bstr_encode(state, (&(*input).Meta_body))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_payload(
		zcbor_state_t *state, const struct session_payload_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).session_payload_choice == session_payload_event_m_c) ? ((encode_session_payload_event(state, (&(*input).session_payload_event_m))))
	: (((*input).session_payload_choice == session_payload_request_m_c) ? ((encode_session_payload_request(state, (&(*input).session_payload_request_m))))
	: (((*input).session_payload_choice == session_payload_command_m_c) ? ((encode_session_payload_command(state, (&(*input).session_payload_command_m))))
	: (((*input).session_payload_choice == session_payload_response_m_c) ? ((encode_session_payload_response(state, (&(*input).session_payload_response_m))))
	: (((*input).session_payload_choice == session_payload_meta_m_c) ? ((encode_session_payload_meta(state, (&(*input).session_payload_meta_m))))
	: false)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_log_entry(
		zcbor_state_t *state, const struct session_log_entry *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).session_log_entry_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"direction", tmp_str.len = sizeof("direction") - 1, &tmp_str)))))
	&& (encode_direction(state, (&(*input).session_log_entry_direction))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"origin", tmp_str.len = sizeof("origin") - 1, &tmp_str)))))
	&& (encode_origin(state, (&(*input).session_log_entry_origin))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"disposition", tmp_str.len = sizeof("disposition") - 1, &tmp_str)))))
	&& (encode_disposition(state, (&(*input).session_log_entry_disposition))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"payload", tmp_str.len = sizeof("payload") - 1, &tmp_str)))))
	&& (encode_session_payload(state, (&(*input).session_log_entry_payload))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_log_page_view(
		zcbor_state_t *state, const struct log_page_view *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"entries", tmp_str.len = sizeof("entries") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).log_page_view_entries_session_log_entry_m_count, (zcbor_encoder_t *)encode_session_log_entry, state, (*&(*input).log_page_view_entries_session_log_entry_m), sizeof(struct session_log_entry))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"next_seq", tmp_str.len = sizeof("next_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).log_page_view_next_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"head_seq", tmp_str.len = sizeof("head_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).log_page_view_head_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"epoch", tmp_str.len = sizeof("epoch") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).log_page_view_epoch))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_log_page(
		zcbor_state_t *state, const struct response_log_page *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"LogPage", tmp_str.len = sizeof("LogPage") - 1, &tmp_str)))))
	&& (encode_log_page_view(state, (&(*input).response_log_page_LogPage))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_node_event_session_advanced(
		zcbor_state_t *state, const struct node_event_session_advanced *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionAdvanced", tmp_str.len = sizeof("SessionAdvanced") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SessionAdvanced_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"epoch", tmp_str.len = sizeof("epoch") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).SessionAdvanced_epoch))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"head_seq", tmp_str.len = sizeof("head_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).SessionAdvanced_head_seq))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_node_event_session_meta_changed(
		zcbor_state_t *state, const struct node_event_session_meta_changed *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionMetaChanged", tmp_str.len = sizeof("SessionMetaChanged") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).SessionMetaChanged_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"rev", tmp_str.len = sizeof("rev") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).SessionMetaChanged_rev))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_node_event_roster_changed(
		zcbor_state_t *state, const struct node_event_roster_changed *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"RosterChanged", tmp_str.len = sizeof("RosterChanged") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"rev", tmp_str.len = sizeof("rev") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).RosterChanged_rev))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_node_event_fleet_changed(
		zcbor_state_t *state, const struct node_event_fleet_changed *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FleetChanged", tmp_str.len = sizeof("FleetChanged") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"rev", tmp_str.len = sizeof("rev") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).FleetChanged_rev))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_node_event_approval_pending(
		zcbor_state_t *state, const struct node_event_approval_pending *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ApprovalPending", tmp_str.len = sizeof("ApprovalPending") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ApprovalPending_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"request_id", tmp_str.len = sizeof("request_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ApprovalPending_request_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_node_event_download_progress(
		zcbor_state_t *state, const struct node_event_download_progress *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"DownloadProgress", tmp_str.len = sizeof("DownloadProgress") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).DownloadProgress_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"pct", tmp_str.len = sizeof("pct") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).DownloadProgress_pct))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).DownloadProgress_state))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_node_event_resync_needed(
		zcbor_state_t *state, const struct node_event_resync_needed *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ResyncNeeded", tmp_str.len = sizeof("ResyncNeeded") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"scope", tmp_str.len = sizeof("scope") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).ResyncNeeded_scope))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_node_event(
		zcbor_state_t *state, const struct node_event_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).node_event_choice == node_event_session_advanced_m_c) ? ((encode_node_event_session_advanced(state, (&(*input).node_event_session_advanced_m))))
	: (((*input).node_event_choice == node_event_session_meta_changed_m_c) ? ((encode_node_event_session_meta_changed(state, (&(*input).node_event_session_meta_changed_m))))
	: (((*input).node_event_choice == node_event_roster_changed_m_c) ? ((encode_node_event_roster_changed(state, (&(*input).node_event_roster_changed_m))))
	: (((*input).node_event_choice == node_event_fleet_changed_m_c) ? ((encode_node_event_fleet_changed(state, (&(*input).node_event_fleet_changed_m))))
	: (((*input).node_event_choice == node_event_approval_pending_m_c) ? ((encode_node_event_approval_pending(state, (&(*input).node_event_approval_pending_m))))
	: (((*input).node_event_choice == node_event_download_progress_m_c) ? ((encode_node_event_download_progress(state, (&(*input).node_event_download_progress_m))))
	: (((*input).node_event_choice == node_event_resync_needed_m_c) ? ((encode_node_event_resync_needed(state, (&(*input).node_event_resync_needed_m))))
	: false)))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_events_page(
		zcbor_state_t *state, const struct events_page *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"events", tmp_str.len = sizeof("events") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).events_page_events_node_event_m_count, (zcbor_encoder_t *)encode_node_event, state, (*&(*input).events_page_events_node_event_m), sizeof(struct node_event_r))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"next_cursor", tmp_str.len = sizeof("next_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).events_page_next_cursor))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"head_cursor", tmp_str.len = sizeof("head_cursor") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).events_page_head_cursor))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_events_page(
		zcbor_state_t *state, const struct response_events_page *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"EventsPage", tmp_str.len = sizeof("EventsPage") - 1, &tmp_str)))))
	&& (encode_events_page(state, (&(*input).response_events_page_EventsPage))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_delivery_targets(
		zcbor_state_t *state, const struct response_delivery_targets *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"DeliveryTargets", tmp_str.len = sizeof("DeliveryTargets") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_delivery_targets_DeliveryTargets_delivery_target_m_count, (zcbor_encoder_t *)encode_delivery_target, state, (*&(*input).response_delivery_targets_DeliveryTargets_delivery_target_m), sizeof(struct delivery_target))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_delivery_sessions(
		zcbor_state_t *state, const struct response_delivery_sessions *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"DeliverySessions", tmp_str.len = sizeof("DeliverySessions") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_delivery_sessions_DeliverySessions_session_id_m_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).response_delivery_sessions_DeliverySessions_session_id_m), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_verifying_key(
		zcbor_state_t *state, const struct response_verifying_key *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"VerifyingKey", tmp_str.len = sizeof("VerifyingKey") - 1, &tmp_str)))))
	&& (((*input).response_verifying_key_VerifyingKey_choice == response_verifying_key_VerifyingKey_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).response_verifying_key_VerifyingKey_tstr))))
	: (((*input).response_verifying_key_VerifyingKey_choice == response_verifying_key_VerifyingKey_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_search_hit(
		zcbor_state_t *state, const struct search_hit *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 9) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).search_hit_repo))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"author", tmp_str.len = sizeof("author") - 1, &tmp_str)))))
	&& (((*input).search_hit_author_choice == search_hit_author_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).search_hit_author_tstr))))
	: (((*input).search_hit_author_choice == search_hit_author_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"downloads", tmp_str.len = sizeof("downloads") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).search_hit_downloads))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"likes", tmp_str.len = sizeof("likes") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).search_hit_likes))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"num_parameters", tmp_str.len = sizeof("num_parameters") - 1, &tmp_str)))))
	&& (((*input).search_hit_num_parameters_choice == search_hit_num_parameters_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).search_hit_num_parameters_uint64_m))))
	: (((*input).search_hit_num_parameters_choice == search_hit_num_parameters_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"pipeline_tag", tmp_str.len = sizeof("pipeline_tag") - 1, &tmp_str)))))
	&& (((*input).search_hit_pipeline_tag_choice == search_hit_pipeline_tag_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).search_hit_pipeline_tag_tstr))))
	: (((*input).search_hit_pipeline_tag_choice == search_hit_pipeline_tag_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"last_modified", tmp_str.len = sizeof("last_modified") - 1, &tmp_str)))))
	&& (((*input).search_hit_last_modified_choice == search_hit_last_modified_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).search_hit_last_modified_tstr))))
	: (((*input).search_hit_last_modified_choice == search_hit_last_modified_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"gated", tmp_str.len = sizeof("gated") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).search_hit_gated))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"private", tmp_str.len = sizeof("private") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).search_hit_private))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 9))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_search_page(
		zcbor_state_t *state, const struct search_page *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"page", tmp_str.len = sizeof("page") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).search_page_page))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"results", tmp_str.len = sizeof("results") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).search_page_results_search_hit_m_count, (zcbor_encoder_t *)encode_search_hit, state, (*&(*input).search_page_results_search_hit_m), sizeof(struct search_hit))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"has_more", tmp_str.len = sizeof("has_more") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).search_page_has_more))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_model_search(
		zcbor_state_t *state, const struct response_model_search *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelSearch", tmp_str.len = sizeof("ModelSearch") - 1, &tmp_str)))))
	&& (encode_search_page(state, (&(*input).response_model_search_ModelSearch))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_model_file(
		zcbor_state_t *state, const struct model_file *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).model_file_path))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"size_bytes", tmp_str.len = sizeof("size_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).model_file_size_bytes))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"quant", tmp_str.len = sizeof("quant") - 1, &tmp_str)))))
	&& (((*input).model_file_quant_choice == model_file_quant_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).model_file_quant_tstr))))
	: (((*input).model_file_quant_choice == model_file_quant_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"is_split", tmp_str.len = sizeof("is_split") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).model_file_is_split))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"is_first_shard", tmp_str.len = sizeof("is_first_shard") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).model_file_is_first_shard))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_model_files(
		zcbor_state_t *state, const struct response_model_files *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelFiles", tmp_str.len = sizeof("ModelFiles") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_model_files_ModelFiles_model_file_m_count, (zcbor_encoder_t *)encode_model_file, state, (*&(*input).response_model_files_ModelFiles_model_file_m), sizeof(struct model_file))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_model_download_started(
		zcbor_state_t *state, const struct response_model_download_started *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelDownloadStarted", tmp_str.len = sizeof("ModelDownloadStarted") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).response_model_download_started_ModelDownloadStarted))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_download_state(
		zcbor_state_t *state, const struct download_state_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).download_state_choice == download_state_Queued_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Queued", tmp_str.len = sizeof("Queued") - 1, &tmp_str)))))
	: (((*input).download_state_choice == download_state_Downloading_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Downloading", tmp_str.len = sizeof("Downloading") - 1, &tmp_str)))))
	: (((*input).download_state_choice == download_state_Completed_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Completed", tmp_str.len = sizeof("Completed") - 1, &tmp_str)))))
	: (((*input).download_state_choice == download_state_Paused_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Paused", tmp_str.len = sizeof("Paused") - 1, &tmp_str)))))
	: (((*input).download_state_choice == download_state_Cancelled_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Cancelled", tmp_str.len = sizeof("Cancelled") - 1, &tmp_str)))))
	: (((*input).download_state_choice == download_state_Failed_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Failed", tmp_str.len = sizeof("Failed") - 1, &tmp_str)))))
	: false))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_download_status(
		zcbor_state_t *state, const struct download_status *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 8) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).download_status_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (encode_model_ref(state, (&(*input).download_status_model))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (encode_download_state(state, (&(*input).download_status_state))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"downloaded_bytes", tmp_str.len = sizeof("downloaded_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).download_status_downloaded_bytes))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"total_bytes", tmp_str.len = sizeof("total_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).download_status_total_bytes))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"files_done", tmp_str.len = sizeof("files_done") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).download_status_files_done))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"files_total", tmp_str.len = sizeof("files_total") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).download_status_files_total))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"error", tmp_str.len = sizeof("error") - 1, &tmp_str)))))
	&& (((*input).download_status_error_choice == download_status_error_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).download_status_error_tstr))))
	: (((*input).download_status_error_choice == download_status_error_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 8))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_model_downloads(
		zcbor_state_t *state, const struct response_model_downloads *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelDownloads", tmp_str.len = sizeof("ModelDownloads") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_model_downloads_ModelDownloads_download_status_m_count, (zcbor_encoder_t *)encode_download_status, state, (*&(*input).response_model_downloads_ModelDownloads_download_status_m), sizeof(struct download_status))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_installed_model_arch(
		zcbor_state_t *state, const struct installed_model_arch_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"arch", tmp_str.len = sizeof("arch") - 1, &tmp_str)))))
	&& (((*input).installed_model_arch_choice == installed_model_arch_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).installed_model_arch_tstr))))
	: (((*input).installed_model_arch_choice == installed_model_arch_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_installed_model_context_length(
		zcbor_state_t *state, const struct installed_model_context_length_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"context_length", tmp_str.len = sizeof("context_length") - 1, &tmp_str)))))
	&& (((*input).installed_model_context_length_choice == installed_model_context_length_uint_c) ? ((zcbor_uint32_encode(state, (&(*input).installed_model_context_length_uint))))
	: (((*input).installed_model_context_length_choice == installed_model_context_length_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_installed_model_file_type(
		zcbor_state_t *state, const struct installed_model_file_type_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"file_type", tmp_str.len = sizeof("file_type") - 1, &tmp_str)))))
	&& (((*input).installed_model_file_type_choice == installed_model_file_type_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).installed_model_file_type_tstr))))
	: (((*input).installed_model_file_type_choice == installed_model_file_type_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_installed_model(
		zcbor_state_t *state, const struct installed_model *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 10) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).installed_model_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (encode_model_ref(state, (&(*input).installed_model_model))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"display_name", tmp_str.len = sizeof("display_name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).installed_model_display_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"local_path", tmp_str.len = sizeof("local_path") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).installed_model_local_path))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"size_bytes", tmp_str.len = sizeof("size_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).installed_model_size_bytes))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"quant", tmp_str.len = sizeof("quant") - 1, &tmp_str)))))
	&& (((*input).installed_model_quant_choice == installed_model_quant_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).installed_model_quant_tstr))))
	: (((*input).installed_model_quant_choice == installed_model_quant_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"installed_at_ms", tmp_str.len = sizeof("installed_at_ms") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).installed_model_installed_at_ms))))
	&& (!(*input).installed_model_arch_present || encode_repeated_installed_model_arch(state, (&(*input).installed_model_arch)))
	&& (!(*input).installed_model_context_length_present || encode_repeated_installed_model_context_length(state, (&(*input).installed_model_context_length)))
	&& (!(*input).installed_model_file_type_present || encode_repeated_installed_model_file_type(state, (&(*input).installed_model_file_type)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 10))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_model_catalog(
		zcbor_state_t *state, const struct response_model_catalog *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelCatalog", tmp_str.len = sizeof("ModelCatalog") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_model_catalog_ModelCatalog_installed_model_m_count, (zcbor_encoder_t *)encode_installed_model, state, (*&(*input).response_model_catalog_ModelCatalog_installed_model_m), sizeof(struct installed_model))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_quant_candidate(
		zcbor_state_t *state, const struct quant_candidate *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"quant", tmp_str.len = sizeof("quant") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).quant_candidate_quant))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"file", tmp_str.len = sizeof("file") - 1, &tmp_str)))))
	&& (((*input).quant_candidate_file_choice == quant_candidate_file_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).quant_candidate_file_tstr))))
	: (((*input).quant_candidate_file_choice == quant_candidate_file_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"size_bytes", tmp_str.len = sizeof("size_bytes") - 1, &tmp_str)))))
	&& (((*input).quant_candidate_size_bytes_choice == quant_candidate_size_bytes_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).quant_candidate_size_bytes_uint64_m))))
	: (((*input).quant_candidate_size_bytes_choice == quant_candidate_size_bytes_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"fits", tmp_str.len = sizeof("fits") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).quant_candidate_fits))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_quant_recommendation(
		zcbor_state_t *state, const struct quant_recommendation *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 9) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"engine", tmp_str.len = sizeof("engine") - 1, &tmp_str)))))
	&& (encode_model_engine(state, (&(*input).quant_recommendation_engine))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).quant_recommendation_repo))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"file", tmp_str.len = sizeof("file") - 1, &tmp_str)))))
	&& (((*input).quant_recommendation_file_choice == quant_recommendation_file_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).quant_recommendation_file_tstr))))
	: (((*input).quant_recommendation_file_choice == quant_recommendation_file_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"quant", tmp_str.len = sizeof("quant") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).quant_recommendation_quant))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"size_bytes", tmp_str.len = sizeof("size_bytes") - 1, &tmp_str)))))
	&& (((*input).quant_recommendation_size_bytes_choice == quant_recommendation_size_bytes_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).quant_recommendation_size_bytes_uint64_m))))
	: (((*input).quant_recommendation_size_bytes_choice == quant_recommendation_size_bytes_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"budget_bytes", tmp_str.len = sizeof("budget_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).quant_recommendation_budget_bytes))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"fits", tmp_str.len = sizeof("fits") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).quant_recommendation_fits))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"reason", tmp_str.len = sizeof("reason") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).quant_recommendation_reason))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"candidates", tmp_str.len = sizeof("candidates") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).quant_recommendation_candidates_quant_candidate_m_count, (zcbor_encoder_t *)encode_quant_candidate, state, (*&(*input).quant_recommendation_candidates_quant_candidate_m), sizeof(struct quant_candidate))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 9))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_model_recommend(
		zcbor_state_t *state, const struct response_model_recommend *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelRecommend", tmp_str.len = sizeof("ModelRecommend") - 1, &tmp_str)))))
	&& (encode_quant_recommendation(state, (&(*input).response_model_recommend_ModelRecommend))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_model_quantize_started(
		zcbor_state_t *state, const struct response_model_quantize_started *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelQuantizeStarted", tmp_str.len = sizeof("ModelQuantizeStarted") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).response_model_quantize_started_ModelQuantizeStarted))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_quantize_state(
		zcbor_state_t *state, const struct quantize_state_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).quantize_state_choice == quantize_state_Queued_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Queued", tmp_str.len = sizeof("Queued") - 1, &tmp_str)))))
	: (((*input).quantize_state_choice == quantize_state_Preparing_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Preparing", tmp_str.len = sizeof("Preparing") - 1, &tmp_str)))))
	: (((*input).quantize_state_choice == quantize_state_Quantizing_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Quantizing", tmp_str.len = sizeof("Quantizing") - 1, &tmp_str)))))
	: (((*input).quantize_state_choice == quantize_state_Completed_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Completed", tmp_str.len = sizeof("Completed") - 1, &tmp_str)))))
	: (((*input).quantize_state_choice == quantize_state_Failed_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Failed", tmp_str.len = sizeof("Failed") - 1, &tmp_str)))))
	: false)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_quantize_status(
		zcbor_state_t *state, const struct quantize_status *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 8) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).quantize_status_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"repo", tmp_str.len = sizeof("repo") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).quantize_status_repo))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"source_file", tmp_str.len = sizeof("source_file") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).quantize_status_source_file))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"target_quant", tmp_str.len = sizeof("target_quant") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).quantize_status_target_quant))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (encode_quantize_state(state, (&(*input).quantize_status_state))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"output_path", tmp_str.len = sizeof("output_path") - 1, &tmp_str)))))
	&& (((*input).quantize_status_output_path_choice == quantize_status_output_path_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).quantize_status_output_path_tstr))))
	: (((*input).quantize_status_output_path_choice == quantize_status_output_path_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"model_id", tmp_str.len = sizeof("model_id") - 1, &tmp_str)))))
	&& (((*input).quantize_status_model_id_choice == quantize_status_model_id_model_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).quantize_status_model_id_model_id_m))))
	: (((*input).quantize_status_model_id_choice == quantize_status_model_id_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"error", tmp_str.len = sizeof("error") - 1, &tmp_str)))))
	&& (((*input).quantize_status_error_choice == quantize_status_error_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).quantize_status_error_tstr))))
	: (((*input).quantize_status_error_choice == quantize_status_error_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 8))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_model_quantizes(
		zcbor_state_t *state, const struct response_model_quantizes *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelQuantizes", tmp_str.len = sizeof("ModelQuantizes") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_model_quantizes_ModelQuantizes_quantize_status_m_count, (zcbor_encoder_t *)encode_quantize_status, state, (*&(*input).response_model_quantizes_ModelQuantizes_quantize_status_m), sizeof(struct quantize_status))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_gguf_info(
		zcbor_state_t *state, const struct gguf_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 8) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"architecture", tmp_str.len = sizeof("architecture") - 1, &tmp_str)))))
	&& (((*input).gguf_info_architecture_choice == gguf_info_architecture_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).gguf_info_architecture_tstr))))
	: (((*input).gguf_info_architecture_choice == gguf_info_architecture_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (((*input).gguf_info_name_choice == gguf_info_name_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).gguf_info_name_tstr))))
	: (((*input).gguf_info_name_choice == gguf_info_name_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"file_type", tmp_str.len = sizeof("file_type") - 1, &tmp_str)))))
	&& (((*input).gguf_info_file_type_choice == gguf_info_file_type_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).gguf_info_file_type_tstr))))
	: (((*input).gguf_info_file_type_choice == gguf_info_file_type_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"context_length", tmp_str.len = sizeof("context_length") - 1, &tmp_str)))))
	&& (((*input).gguf_info_context_length_choice == gguf_info_context_length_uint_c) ? ((zcbor_uint32_encode(state, (&(*input).gguf_info_context_length_uint))))
	: (((*input).gguf_info_context_length_choice == gguf_info_context_length_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"block_count", tmp_str.len = sizeof("block_count") - 1, &tmp_str)))))
	&& (((*input).gguf_info_block_count_choice == gguf_info_block_count_uint_c) ? ((zcbor_uint32_encode(state, (&(*input).gguf_info_block_count_uint))))
	: (((*input).gguf_info_block_count_choice == gguf_info_block_count_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"quantization_version", tmp_str.len = sizeof("quantization_version") - 1, &tmp_str)))))
	&& (((*input).gguf_info_quantization_version_choice == gguf_info_quantization_version_uint_c) ? ((zcbor_uint32_encode(state, (&(*input).gguf_info_quantization_version_uint))))
	: (((*input).gguf_info_quantization_version_choice == gguf_info_quantization_version_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"parameter_count", tmp_str.len = sizeof("parameter_count") - 1, &tmp_str)))))
	&& (((*input).gguf_info_parameter_count_choice == gguf_info_parameter_count_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).gguf_info_parameter_count_uint64_m))))
	: (((*input).gguf_info_parameter_count_choice == gguf_info_parameter_count_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"size_bytes", tmp_str.len = sizeof("size_bytes") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).gguf_info_size_bytes))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 8))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_model_inspect(
		zcbor_state_t *state, const struct response_model_inspect *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelInspect", tmp_str.len = sizeof("ModelInspect") - 1, &tmp_str)))))
	&& (encode_gguf_info(state, (&(*input).response_model_inspect_ModelInspect))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_profile_info(
		zcbor_state_t *state, const struct profile_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).profile_info_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (encode_provider_selector(state, (&(*input).profile_info_provider))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).profile_info_model))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"is_active", tmp_str.len = sizeof("is_active") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).profile_info_is_active))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"bound_accounts", tmp_str.len = sizeof("bound_accounts") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).profile_info_bound_accounts_bound_account_m_count, (zcbor_encoder_t *)encode_bound_account, state, (*&(*input).profile_info_bound_accounts_bound_account_m), sizeof(struct bound_account))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_profiles(
		zcbor_state_t *state, const struct response_profiles *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Profiles", tmp_str.len = sizeof("Profiles") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_profiles_Profiles_profile_info_m_count, (zcbor_encoder_t *)encode_profile_info, state, (*&(*input).response_profiles_Profiles_profile_info_m), sizeof(struct profile_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_profile(
		zcbor_state_t *state, const struct response_profile *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Profile", tmp_str.len = sizeof("Profile") - 1, &tmp_str)))))
	&& (((*input).response_profile_Profile_choice == response_profile_Profile_profile_spec_m_c) ? ((encode_profile_spec(state, (&(*input).response_profile_Profile_profile_spec_m))))
	: (((*input).response_profile_Profile_choice == response_profile_Profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_credential_info(
		zcbor_state_t *state, const struct credential_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"profile", tmp_str.len = sizeof("profile") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).credential_info_profile))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"present", tmp_str.len = sizeof("present") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).credential_info_present))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"hint", tmp_str.len = sizeof("hint") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).credential_info_hint))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_credentials(
		zcbor_state_t *state, const struct response_credentials *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Credentials", tmp_str.len = sizeof("Credentials") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_credentials_Credentials_credential_info_m_count, (zcbor_encoder_t *)encode_credential_info, state, (*&(*input).response_credentials_Credentials_credential_info_m), sizeof(struct credential_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_model_descriptor(
		zcbor_state_t *state, const struct model_descriptor *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 6) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).model_descriptor_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"provider", tmp_str.len = sizeof("provider") - 1, &tmp_str)))))
	&& (encode_provider_selector(state, (&(*input).model_descriptor_provider))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"context_length", tmp_str.len = sizeof("context_length") - 1, &tmp_str)))))
	&& (((*input).model_descriptor_context_length_choice == model_descriptor_context_length_uint_c) ? ((zcbor_uint32_encode(state, (&(*input).model_descriptor_context_length_uint))))
	: (((*input).model_descriptor_context_length_choice == model_descriptor_context_length_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"input_price_micros_per_mtok", tmp_str.len = sizeof("input_price_micros_per_mtok") - 1, &tmp_str)))))
	&& (((*input).model_descriptor_input_price_micros_per_mtok_choice == model_descriptor_input_price_micros_per_mtok_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).model_descriptor_input_price_micros_per_mtok_uint64_m))))
	: (((*input).model_descriptor_input_price_micros_per_mtok_choice == model_descriptor_input_price_micros_per_mtok_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"output_price_micros_per_mtok", tmp_str.len = sizeof("output_price_micros_per_mtok") - 1, &tmp_str)))))
	&& (((*input).model_descriptor_output_price_micros_per_mtok_choice == model_descriptor_output_price_micros_per_mtok_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).model_descriptor_output_price_micros_per_mtok_uint64_m))))
	: (((*input).model_descriptor_output_price_micros_per_mtok_choice == model_descriptor_output_price_micros_per_mtok_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"local", tmp_str.len = sizeof("local") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).model_descriptor_local))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 6))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_models(
		zcbor_state_t *state, const struct response_models *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Models", tmp_str.len = sizeof("Models") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_models_Models_model_descriptor_m_count, (zcbor_encoder_t *)encode_model_descriptor, state, (*&(*input).response_models_Models_model_descriptor_m), sizeof(struct model_descriptor))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_model_current(
		zcbor_state_t *state, const struct response_model_current *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelCurrent", tmp_str.len = sizeof("ModelCurrent") - 1, &tmp_str)))))
	&& (((*input).response_model_current_ModelCurrent_choice == response_model_current_ModelCurrent_model_descriptor_m_c) ? ((encode_model_descriptor(state, (&(*input).response_model_current_ModelCurrent_model_descriptor_m))))
	: (((*input).response_model_current_ModelCurrent_choice == response_model_current_ModelCurrent_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_distribution(
		zcbor_state_t *state, const struct response_distribution *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Distribution", tmp_str.len = sizeof("Distribution") - 1, &tmp_str)))))
	&& (encode_distribution(state, (&(*input).response_distribution_Distribution))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_profile_id(
		zcbor_state_t *state, const struct response_profile_id *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileId", tmp_str.len = sizeof("ProfileId") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).response_profile_id_ProfileId))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_author_agent(
		zcbor_state_t *state, const struct author_agent *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"agent", tmp_str.len = sizeof("agent") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).author_agent_agent))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_author(
		zcbor_state_t *state, const struct author_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).author_choice == author_operator_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"operator", tmp_str.len = sizeof("operator") - 1, &tmp_str)))))
	: (((*input).author_choice == author_agent_m_c) ? ((encode_author_agent(state, (&(*input).author_agent_m))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_revision(
		zcbor_state_t *state, const struct revision *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 6) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).revision_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"parent", tmp_str.len = sizeof("parent") - 1, &tmp_str)))))
	&& (((*input).revision_parent_choice == revision_parent_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).revision_parent_uint64_m))))
	: (((*input).revision_parent_choice == revision_parent_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"content_hash", tmp_str.len = sizeof("content_hash") - 1, &tmp_str)))))
	&& (encode_content_hash(state, (&(*input).revision_content_hash))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"author", tmp_str.len = sizeof("author") - 1, &tmp_str)))))
	&& (encode_author(state, (&(*input).revision_author))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"reason", tmp_str.len = sizeof("reason") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).revision_reason))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ts_ms", tmp_str.len = sizeof("ts_ms") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).revision_ts_ms))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 6))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_revisions(
		zcbor_state_t *state, const struct response_revisions *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Revisions", tmp_str.len = sizeof("Revisions") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_revisions_Revisions_revision_m_count, (zcbor_encoder_t *)encode_revision, state, (*&(*input).response_revisions_Revisions_revision_m), sizeof(struct revision))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_skill_bundle(
		zcbor_state_t *state, const struct response_skill_bundle *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SkillBundle", tmp_str.len = sizeof("SkillBundle") - 1, &tmp_str)))))
	&& (encode_skill_bundle(state, (&(*input).response_skill_bundle_SkillBundle))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_skill_creator(
		zcbor_state_t *state, const struct skill_creator_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).skill_creator_choice == skill_creator_agent_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"agent", tmp_str.len = sizeof("agent") - 1, &tmp_str)))))
	: (((*input).skill_creator_choice == skill_creator_user_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"user", tmp_str.len = sizeof("user") - 1, &tmp_str)))))
	: (((*input).skill_creator_choice == skill_creator_bundled_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"bundled", tmp_str.len = sizeof("bundled") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_skill_state(
		zcbor_state_t *state, const struct skill_state_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).skill_state_choice == skill_state_active_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"active", tmp_str.len = sizeof("active") - 1, &tmp_str)))))
	: (((*input).skill_state_choice == skill_state_stale_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"stale", tmp_str.len = sizeof("stale") - 1, &tmp_str)))))
	: (((*input).skill_state_choice == skill_state_archived_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"archived", tmp_str.len = sizeof("archived") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_skill_usage(
		zcbor_state_t *state, const struct skill_usage *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 10) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"created_by", tmp_str.len = sizeof("created_by") - 1, &tmp_str)))))
	&& (encode_skill_creator(state, (&(*input).skill_usage_created_by))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (encode_skill_state(state, (&(*input).skill_usage_state))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"pinned", tmp_str.len = sizeof("pinned") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).skill_usage_pinned))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"use_count", tmp_str.len = sizeof("use_count") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).skill_usage_use_count))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"view_count", tmp_str.len = sizeof("view_count") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).skill_usage_view_count))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"patch_count", tmp_str.len = sizeof("patch_count") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).skill_usage_patch_count))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"created_at_ms", tmp_str.len = sizeof("created_at_ms") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).skill_usage_created_at_ms))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"last_used_ms", tmp_str.len = sizeof("last_used_ms") - 1, &tmp_str)))))
	&& (((*input).skill_usage_last_used_ms_choice == skill_usage_last_used_ms_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).skill_usage_last_used_ms_uint64_m))))
	: (((*input).skill_usage_last_used_ms_choice == skill_usage_last_used_ms_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"last_viewed_ms", tmp_str.len = sizeof("last_viewed_ms") - 1, &tmp_str)))))
	&& (((*input).skill_usage_last_viewed_ms_choice == skill_usage_last_viewed_ms_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).skill_usage_last_viewed_ms_uint64_m))))
	: (((*input).skill_usage_last_viewed_ms_choice == skill_usage_last_viewed_ms_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"last_patched_ms", tmp_str.len = sizeof("last_patched_ms") - 1, &tmp_str)))))
	&& (((*input).skill_usage_last_patched_ms_choice == skill_usage_last_patched_ms_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).skill_usage_last_patched_ms_uint64_m))))
	: (((*input).skill_usage_last_patched_ms_choice == skill_usage_last_patched_ms_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 10))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_curator_entry(
		zcbor_state_t *state, const struct curator_entry *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).curator_entry_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"category", tmp_str.len = sizeof("category") - 1, &tmp_str)))))
	&& (((*input).curator_entry_category_choice == curator_entry_category_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).curator_entry_category_tstr))))
	: (((*input).curator_entry_category_choice == curator_entry_category_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"is_bundled", tmp_str.len = sizeof("is_bundled") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).curator_entry_is_bundled))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"usage", tmp_str.len = sizeof("usage") - 1, &tmp_str)))))
	&& (encode_skill_usage(state, (&(*input).curator_entry_usage))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_curator_skills(
		zcbor_state_t *state, const struct response_curator_skills *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CuratorSkills", tmp_str.len = sizeof("CuratorSkills") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_curator_skills_CuratorSkills_curator_entry_m_count, (zcbor_encoder_t *)encode_curator_entry, state, (*&(*input).response_curator_skills_CuratorSkills_curator_entry_m), sizeof(struct curator_entry))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_curator_change(
		zcbor_state_t *state, const struct curator_change *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).curator_change_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"from", tmp_str.len = sizeof("from") - 1, &tmp_str)))))
	&& (encode_skill_state(state, (&(*input).curator_change_from))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"to", tmp_str.len = sizeof("to") - 1, &tmp_str)))))
	&& (encode_skill_state(state, (&(*input).curator_change_to))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_curator_run(
		zcbor_state_t *state, const struct response_curator_run *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CuratorRun", tmp_str.len = sizeof("CuratorRun") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_curator_run_CuratorRun_curator_change_m_count, (zcbor_encoder_t *)encode_curator_change, state, (*&(*input).response_curator_run_CuratorRun_curator_change_m), sizeof(struct curator_change))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_auth_flow_kind(
		zcbor_state_t *state, const struct auth_flow_kind_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).auth_flow_kind_choice == auth_flow_kind_MatrixSso_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"MatrixSso", tmp_str.len = sizeof("MatrixSso") - 1, &tmp_str)))))
	: (((*input).auth_flow_kind_choice == auth_flow_kind_OAuth2Pkce_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"OAuth2Pkce", tmp_str.len = sizeof("OAuth2Pkce") - 1, &tmp_str)))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_auth_begin_response(
		zcbor_state_t *state, const struct auth_begin_response *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"flow_id", tmp_str.len = sizeof("flow_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_begin_response_flow_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"authorization_url", tmp_str.len = sizeof("authorization_url") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_begin_response_authorization_url))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"redirect_uri", tmp_str.len = sizeof("redirect_uri") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_begin_response_redirect_uri))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"expires_at", tmp_str.len = sizeof("expires_at") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).auth_begin_response_expires_at))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"flow_kind", tmp_str.len = sizeof("flow_kind") - 1, &tmp_str)))))
	&& (encode_auth_flow_kind(state, (&(*input).auth_begin_response_flow_kind))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_auth_begun(
		zcbor_state_t *state, const struct response_auth_begun *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AuthBegun", tmp_str.len = sizeof("AuthBegun") - 1, &tmp_str)))))
	&& (encode_auth_begin_response(state, (&(*input).response_auth_begun_AuthBegun))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_auth_complete_response(
		zcbor_state_t *state, const struct auth_complete_response *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"credential_ref", tmp_str.len = sizeof("credential_ref") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_complete_response_credential_ref))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"account_label", tmp_str.len = sizeof("account_label") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_complete_response_account_label))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport_instance", tmp_str.len = sizeof("transport_instance") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_complete_response_transport_instance))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"bound_profile", tmp_str.len = sizeof("bound_profile") - 1, &tmp_str)))))
	&& (((*input).auth_complete_response_bound_profile_choice == auth_complete_response_bound_profile_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).auth_complete_response_bound_profile_tstr))))
	: (((*input).auth_complete_response_bound_profile_choice == auth_complete_response_bound_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_auth_completed(
		zcbor_state_t *state, const struct response_auth_completed *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AuthCompleted", tmp_str.len = sizeof("AuthCompleted") - 1, &tmp_str)))))
	&& (encode_auth_complete_response(state, (&(*input).response_auth_completed_AuthCompleted))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_auth_provider_info(
		zcbor_state_t *state, const struct auth_provider_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"family", tmp_str.len = sizeof("family") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_provider_info_family))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"flow_kind", tmp_str.len = sizeof("flow_kind") - 1, &tmp_str)))))
	&& (encode_auth_flow_kind(state, (&(*input).auth_provider_info_flow_kind))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"display_name", tmp_str.len = sizeof("display_name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).auth_provider_info_display_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"params_schema", tmp_str.len = sizeof("params_schema") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).auth_provider_info_params_schema_auth_param_field_m_count, (zcbor_encoder_t *)encode_auth_param_field, state, (*&(*input).auth_provider_info_params_schema_auth_param_field_m), sizeof(struct auth_param_field))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_auth_providers(
		zcbor_state_t *state, const struct response_auth_providers *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AuthProviders", tmp_str.len = sizeof("AuthProviders") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_auth_providers_AuthProviders_auth_provider_info_m_count, (zcbor_encoder_t *)encode_auth_provider_info, state, (*&(*input).response_auth_providers_AuthProviders_auth_provider_info_m), sizeof(struct auth_provider_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_checkpoint_info_turn_ordinal(
		zcbor_state_t *state, const struct checkpoint_info_turn_ordinal_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"turn_ordinal", tmp_str.len = sizeof("turn_ordinal") - 1, &tmp_str)))))
	&& (((*input).checkpoint_info_turn_ordinal_choice == checkpoint_info_turn_ordinal_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).checkpoint_info_turn_ordinal_uint64_m))))
	: (((*input).checkpoint_info_turn_ordinal_choice == checkpoint_info_turn_ordinal_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_checkpoint_info_cursor(
		zcbor_state_t *state, const struct checkpoint_info_cursor_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"cursor", tmp_str.len = sizeof("cursor") - 1, &tmp_str)))))
	&& (((*input).checkpoint_info_cursor_choice == checkpoint_info_cursor_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).checkpoint_info_cursor_uint64_m))))
	: (((*input).checkpoint_info_cursor_choice == checkpoint_info_cursor_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_checkpoint_info(
		zcbor_state_t *state, const struct checkpoint_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 6) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).checkpoint_info_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).checkpoint_info_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"tool", tmp_str.len = sizeof("tool") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).checkpoint_info_tool))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"created_unix", tmp_str.len = sizeof("created_unix") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).checkpoint_info_created_unix))))
	&& (!(*input).checkpoint_info_turn_ordinal_present || encode_repeated_checkpoint_info_turn_ordinal(state, (&(*input).checkpoint_info_turn_ordinal)))
	&& (!(*input).checkpoint_info_cursor_present || encode_repeated_checkpoint_info_cursor(state, (&(*input).checkpoint_info_cursor)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 6))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_checkpoints(
		zcbor_state_t *state, const struct response_checkpoints *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Checkpoints", tmp_str.len = sizeof("Checkpoints") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_checkpoints_Checkpoints_checkpoint_info_m_count, (zcbor_encoder_t *)encode_checkpoint_info, state, (*&(*input).response_checkpoints_Checkpoints_checkpoint_info_m), sizeof(struct checkpoint_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_page_next_cursor(
		zcbor_state_t *state, const struct session_page_next_cursor_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"next_cursor", tmp_str.len = sizeof("next_cursor") - 1, &tmp_str)))))
	&& (((*input).session_page_next_cursor_choice == session_page_next_cursor_session_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).session_page_next_cursor_session_id_m))))
	: (((*input).session_page_next_cursor_choice == session_page_next_cursor_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_page_removed(
		zcbor_state_t *state, const struct session_page_removed_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"removed", tmp_str.len = sizeof("removed") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).session_page_removed_session_id_m_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).session_page_removed_session_id_m), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_page(
		zcbor_state_t *state, const struct session_page *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"sessions", tmp_str.len = sizeof("sessions") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).session_page_sessions_session_info_m_count, (zcbor_encoder_t *)encode_session_info, state, (*&(*input).session_page_sessions_session_info_m), sizeof(struct session_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (!(*input).session_page_next_cursor_present || encode_repeated_session_page_next_cursor(state, (&(*input).session_page_next_cursor)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"rev", tmp_str.len = sizeof("rev") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).session_page_rev))))
	&& (!(*input).session_page_removed_present || encode_repeated_session_page_removed(state, (&(*input).session_page_removed)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_session_page(
		zcbor_state_t *state, const struct response_session_page *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionPage", tmp_str.len = sizeof("SessionPage") - 1, &tmp_str)))))
	&& (encode_session_page(state, (&(*input).response_session_page_SessionPage))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_detail_overlay(
		zcbor_state_t *state, const struct session_detail_overlay_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"overlay", tmp_str.len = sizeof("overlay") - 1, &tmp_str)))))
	&& (((*input).session_detail_overlay_choice == session_detail_overlay_session_overlay_m_c) ? ((encode_session_overlay(state, (&(*input).session_detail_overlay_session_overlay_m))))
	: (((*input).session_detail_overlay_choice == session_detail_overlay_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_detail_model(
		zcbor_state_t *state, const struct session_detail_model_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"model", tmp_str.len = sizeof("model") - 1, &tmp_str)))))
	&& (((*input).session_detail_model_choice == session_detail_model_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).session_detail_model_tstr))))
	: (((*input).session_detail_model_choice == session_detail_model_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_detail_delivery_targets(
		zcbor_state_t *state, const struct session_detail_delivery_targets_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"delivery_targets", tmp_str.len = sizeof("delivery_targets") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).session_detail_delivery_targets_delivery_target_m_count, (zcbor_encoder_t *)encode_delivery_target, state, (*&(*input).session_detail_delivery_targets_delivery_target_m), sizeof(struct delivery_target))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_detail_children(
		zcbor_state_t *state, const struct session_detail_children_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"children", tmp_str.len = sizeof("children") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).session_detail_children_session_id_m_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).session_detail_children_session_id_m), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_session_detail_checkpoints(
		zcbor_state_t *state, const struct session_detail_checkpoints *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"checkpoints", tmp_str.len = sizeof("checkpoints") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).session_detail_checkpoints)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_detail(
		zcbor_state_t *state, const struct session_detail *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 6) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"info", tmp_str.len = sizeof("info") - 1, &tmp_str)))))
	&& (encode_session_info(state, (&(*input).session_detail_info))))
	&& (!(*input).session_detail_overlay_present || encode_repeated_session_detail_overlay(state, (&(*input).session_detail_overlay)))
	&& (!(*input).session_detail_model_present || encode_repeated_session_detail_model(state, (&(*input).session_detail_model)))
	&& (!(*input).session_detail_delivery_targets_present || encode_repeated_session_detail_delivery_targets(state, (&(*input).session_detail_delivery_targets)))
	&& (!(*input).session_detail_children_present || encode_repeated_session_detail_children(state, (&(*input).session_detail_children)))
	&& (!(*input).session_detail_checkpoints_present || encode_repeated_session_detail_checkpoints(state, (&(*input).session_detail_checkpoints)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 6))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_session_detail(
		zcbor_state_t *state, const struct response_session_detail *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionDetail", tmp_str.len = sizeof("SessionDetail") - 1, &tmp_str)))))
	&& (((*input).response_session_detail_SessionDetail_choice == response_session_detail_SessionDetail_session_detail_m_c) ? ((encode_session_detail(state, (&(*input).response_session_detail_SessionDetail_session_detail_m))))
	: (((*input).response_session_detail_SessionDetail_choice == response_session_detail_SessionDetail_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_SessionsByProfile_profile_l(
		zcbor_state_t *state, const struct SessionsByProfile_profile_l *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_encode(state, 2) && ((((zcbor_tstr_encode(state, (&(*input).SessionsByProfile_profile_l_profile))))
	&& ((zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).profile_l_sessions_session_info_m_count, (zcbor_encoder_t *)encode_session_info, state, (*&(*input).profile_l_sessions_session_info_m), sizeof(struct session_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_sessions_by_profile(
		zcbor_state_t *state, const struct response_sessions_by_profile *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionsByProfile", tmp_str.len = sizeof("SessionsByProfile") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).SessionsByProfile_profile_l_count, (zcbor_encoder_t *)encode_repeated_SessionsByProfile_profile_l, state, (*&(*input).SessionsByProfile_profile_l), sizeof(struct SessionsByProfile_profile_l))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_search_hit(
		zcbor_state_t *state, const struct session_search_hit *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).session_search_hit_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).session_search_hit_title))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"snippet", tmp_str.len = sizeof("snippet") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).session_search_hit_snippet))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_session_search(
		zcbor_state_t *state, const struct response_session_search *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionSearch", tmp_str.len = sizeof("SessionSearch") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_session_search_SessionSearch_session_search_hit_m_count, (zcbor_encoder_t *)encode_session_search_hit, state, (*&(*input).response_session_search_SessionSearch_session_search_hit_m), sizeof(struct session_search_hit))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_acp_catalog(
		zcbor_state_t *state, const struct response_acp_catalog *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AcpCatalog", tmp_str.len = sizeof("AcpCatalog") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_acp_catalog_AcpCatalog_acp_agent_entry_m_count, (zcbor_encoder_t *)encode_acp_agent_entry, state, (*&(*input).response_acp_catalog_AcpCatalog_acp_agent_entry_m), sizeof(struct acp_agent_entry))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_providers(
		zcbor_state_t *state, const struct response_providers *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Providers", tmp_str.len = sizeof("Providers") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_providers_Providers_provider_info_m_count, (zcbor_encoder_t *)encode_provider_info, state, (*&(*input).response_providers_Providers_provider_info_m), sizeof(struct provider_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_tools(
		zcbor_state_t *state, const struct response_tools *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Tools", tmp_str.len = sizeof("Tools") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_tools_Tools_tool_info_m_count, (zcbor_encoder_t *)encode_tool_info, state, (*&(*input).response_tools_Tools_tool_info_m), sizeof(struct tool_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_command_scope(
		zcbor_state_t *state, const struct command_scope_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).command_scope_choice == command_scope_Session_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Session", tmp_str.len = sizeof("Session") - 1, &tmp_str)))))
	: (((*input).command_scope_choice == command_scope_Node_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Node", tmp_str.len = sizeof("Node") - 1, &tmp_str)))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_command_surface(
		zcbor_state_t *state, const struct command_surface_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).command_surface_choice == command_surface_Cli_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Cli", tmp_str.len = sizeof("Cli") - 1, &tmp_str)))))
	: (((*input).command_surface_choice == command_surface_Gui_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Gui", tmp_str.len = sizeof("Gui") - 1, &tmp_str)))))
	: (((*input).command_surface_choice == command_surface_Chat_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Chat", tmp_str.len = sizeof("Chat") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_command_access(
		zcbor_state_t *state, const struct command_access_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).command_access_choice == command_access_User_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"User", tmp_str.len = sizeof("User") - 1, &tmp_str)))))
	: (((*input).command_access_choice == command_access_Admin_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Admin", tmp_str.len = sizeof("Admin") - 1, &tmp_str)))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_command_spec(
		zcbor_state_t *state, const struct command_spec *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 12) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).command_spec_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"aliases", tmp_str.len = sizeof("aliases") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).command_spec_aliases_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).command_spec_aliases_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"summary", tmp_str.len = sizeof("summary") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).command_spec_summary))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"category", tmp_str.len = sizeof("category") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).command_spec_category))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"args_hint", tmp_str.len = sizeof("args_hint") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).command_spec_args_hint))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"subcommands", tmp_str.len = sizeof("subcommands") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).command_spec_subcommands_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).command_spec_subcommands_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"scope", tmp_str.len = sizeof("scope") - 1, &tmp_str)))))
	&& (encode_command_scope(state, (&(*input).command_spec_scope))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"surfaces", tmp_str.len = sizeof("surfaces") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).command_spec_surfaces_command_surface_m_count, (zcbor_encoder_t *)encode_command_surface, state, (*&(*input).command_spec_surfaces_command_surface_m), sizeof(struct command_surface_r))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"side_effecting", tmp_str.len = sizeof("side_effecting") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).command_spec_side_effecting))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"confirm", tmp_str.len = sizeof("confirm") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).command_spec_confirm))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"min_access", tmp_str.len = sizeof("min_access") - 1, &tmp_str)))))
	&& (encode_command_access(state, (&(*input).command_spec_min_access))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).command_spec_source))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 12))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_commands(
		zcbor_state_t *state, const struct response_commands *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Commands", tmp_str.len = sizeof("Commands") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_commands_Commands_command_spec_m_count, (zcbor_encoder_t *)encode_command_spec, state, (*&(*input).response_commands_Commands_command_spec_m), sizeof(struct command_spec))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_command_output(
		zcbor_state_t *state, const struct command_output *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"text", tmp_str.len = sizeof("text") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).command_output_text))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ephemeral", tmp_str.len = sizeof("ephemeral") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).command_output_ephemeral))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_command_output(
		zcbor_state_t *state, const struct response_command_output *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CommandOutput", tmp_str.len = sizeof("CommandOutput") - 1, &tmp_str)))))
	&& (encode_command_output(state, (&(*input).response_command_output_CommandOutput))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_config(
		zcbor_state_t *state, const struct response_config *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Config", tmp_str.len = sizeof("Config") - 1, &tmp_str)))))
	&& (encode_node_config_view(state, (&(*input).response_config_Config))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_job_next_fire_unix(
		zcbor_state_t *state, const struct cron_job_next_fire_unix_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"next_fire_unix", tmp_str.len = sizeof("next_fire_unix") - 1, &tmp_str)))))
	&& (((*input).cron_job_next_fire_unix_choice == cron_job_next_fire_unix_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).cron_job_next_fire_unix_uint64_m))))
	: (((*input).cron_job_next_fire_unix_choice == cron_job_next_fire_unix_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_job_paused(
		zcbor_state_t *state, const struct cron_job_paused *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"paused", tmp_str.len = sizeof("paused") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).cron_job_paused)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_job_last_run_unix(
		zcbor_state_t *state, const struct cron_job_last_run_unix_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"last_run_unix", tmp_str.len = sizeof("last_run_unix") - 1, &tmp_str)))))
	&& (((*input).cron_job_last_run_unix_choice == cron_job_last_run_unix_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).cron_job_last_run_unix_uint64_m))))
	: (((*input).cron_job_last_run_unix_choice == cron_job_last_run_unix_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_job_last_ok(
		zcbor_state_t *state, const struct cron_job_last_ok_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"last_ok", tmp_str.len = sizeof("last_ok") - 1, &tmp_str)))))
	&& (((*input).cron_job_last_ok_choice == cron_job_last_ok_bool_c) ? ((zcbor_bool_encode(state, (&(*input).cron_job_last_ok_bool))))
	: (((*input).cron_job_last_ok_choice == cron_job_last_ok_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_job_last_detail(
		zcbor_state_t *state, const struct cron_job_last_detail_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"last_detail", tmp_str.len = sizeof("last_detail") - 1, &tmp_str)))))
	&& (((*input).cron_job_last_detail_choice == cron_job_last_detail_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).cron_job_last_detail_tstr))))
	: (((*input).cron_job_last_detail_choice == cron_job_last_detail_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_job_fire_count(
		zcbor_state_t *state, const struct cron_job_fire_count *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"fire_count", tmp_str.len = sizeof("fire_count") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).cron_job_fire_count)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_cron_job(
		zcbor_state_t *state, const struct cron_job *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 8) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).cron_job_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (encode_cron_spec(state, (&(*input).cron_job_spec))))
	&& (!(*input).cron_job_next_fire_unix_present || encode_repeated_cron_job_next_fire_unix(state, (&(*input).cron_job_next_fire_unix)))
	&& (!(*input).cron_job_paused_present || encode_repeated_cron_job_paused(state, (&(*input).cron_job_paused)))
	&& (!(*input).cron_job_last_run_unix_present || encode_repeated_cron_job_last_run_unix(state, (&(*input).cron_job_last_run_unix)))
	&& (!(*input).cron_job_last_ok_present || encode_repeated_cron_job_last_ok(state, (&(*input).cron_job_last_ok)))
	&& (!(*input).cron_job_last_detail_present || encode_repeated_cron_job_last_detail(state, (&(*input).cron_job_last_detail)))
	&& (!(*input).cron_job_fire_count_present || encode_repeated_cron_job_fire_count(state, (&(*input).cron_job_fire_count)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 8))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_cron_jobs(
		zcbor_state_t *state, const struct response_cron_jobs *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronJobs", tmp_str.len = sizeof("CronJobs") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_cron_jobs_CronJobs_cron_job_m_count, (zcbor_encoder_t *)encode_cron_job, state, (*&(*input).response_cron_jobs_CronJobs_cron_job_m), sizeof(struct cron_job))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_cron_id(
		zcbor_state_t *state, const struct response_cron_id *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronId", tmp_str.len = sizeof("CronId") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).response_cron_id_CronId))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_run_detail(
		zcbor_state_t *state, const struct cron_run_detail_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"detail", tmp_str.len = sizeof("detail") - 1, &tmp_str)))))
	&& (((*input).cron_run_detail_choice == cron_run_detail_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).cron_run_detail_tstr))))
	: (((*input).cron_run_detail_choice == cron_run_detail_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_run_finished_unix(
		zcbor_state_t *state, const struct cron_run_finished_unix_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"finished_unix", tmp_str.len = sizeof("finished_unix") - 1, &tmp_str)))))
	&& (((*input).cron_run_finished_unix_choice == cron_run_finished_unix_uint64_m_c) ? ((zcbor_uint64_encode(state, (&(*input).cron_run_finished_unix_uint64_m))))
	: (((*input).cron_run_finished_unix_choice == cron_run_finished_unix_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_run_session(
		zcbor_state_t *state, const struct cron_run_session_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (((*input).cron_run_session_choice == cron_run_session_session_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).cron_run_session_session_id_m))))
	: (((*input).cron_run_session_choice == cron_run_session_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_run_trigger(
		zcbor_state_t *state, const struct run_trigger_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).run_trigger_choice == run_trigger_Scheduled_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Scheduled", tmp_str.len = sizeof("Scheduled") - 1, &tmp_str)))))
	: (((*input).run_trigger_choice == run_trigger_Manual_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Manual", tmp_str.len = sizeof("Manual") - 1, &tmp_str)))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_run_trigger(
		zcbor_state_t *state, const struct cron_run_trigger *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"trigger", tmp_str.len = sizeof("trigger") - 1, &tmp_str)))))
	&& (encode_run_trigger(state, (&(*input).cron_run_trigger)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_cron_run(
		zcbor_state_t *state, const struct cron_run *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 6) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"started_unix", tmp_str.len = sizeof("started_unix") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).cron_run_started_unix))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ok", tmp_str.len = sizeof("ok") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).cron_run_ok))))
	&& (!(*input).cron_run_detail_present || encode_repeated_cron_run_detail(state, (&(*input).cron_run_detail)))
	&& (!(*input).cron_run_finished_unix_present || encode_repeated_cron_run_finished_unix(state, (&(*input).cron_run_finished_unix)))
	&& (!(*input).cron_run_session_present || encode_repeated_cron_run_session(state, (&(*input).cron_run_session)))
	&& (!(*input).cron_run_trigger_present || encode_repeated_cron_run_trigger(state, (&(*input).cron_run_trigger)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 6))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_cron_runs(
		zcbor_state_t *state, const struct response_cron_runs *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronRuns", tmp_str.len = sizeof("CronRuns") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_cron_runs_CronRuns_cron_run_m_count, (zcbor_encoder_t *)encode_cron_run, state, (*&(*input).response_cron_runs_CronRuns_cron_run_m), sizeof(struct cron_run))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_suggestion_description(
		zcbor_state_t *state, const struct cron_suggestion_description *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"description", tmp_str.len = sizeof("description") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).cron_suggestion_description)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_suggestion_source(
		zcbor_state_t *state, const struct cron_suggestion_source *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"source", tmp_str.len = sizeof("source") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).cron_suggestion_source)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_suggestion_status(
		zcbor_state_t *state, const struct suggestion_status_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).suggestion_status_choice == suggestion_status_Pending_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Pending", tmp_str.len = sizeof("Pending") - 1, &tmp_str)))))
	: (((*input).suggestion_status_choice == suggestion_status_Accepted_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Accepted", tmp_str.len = sizeof("Accepted") - 1, &tmp_str)))))
	: (((*input).suggestion_status_choice == suggestion_status_Dismissed_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Dismissed", tmp_str.len = sizeof("Dismissed") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_cron_suggestion_status(
		zcbor_state_t *state, const struct cron_suggestion_status *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"status", tmp_str.len = sizeof("status") - 1, &tmp_str)))))
	&& (encode_suggestion_status(state, (&(*input).cron_suggestion_status)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_cron_suggestion(
		zcbor_state_t *state, const struct cron_suggestion *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 7) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).cron_suggestion_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).cron_suggestion_title))))
	&& (!(*input).cron_suggestion_description_present || encode_repeated_cron_suggestion_description(state, (&(*input).cron_suggestion_description)))
	&& (!(*input).cron_suggestion_source_present || encode_repeated_cron_suggestion_source(state, (&(*input).cron_suggestion_source)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"spec", tmp_str.len = sizeof("spec") - 1, &tmp_str)))))
	&& (encode_cron_spec(state, (&(*input).cron_suggestion_spec))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"dedup_key", tmp_str.len = sizeof("dedup_key") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).cron_suggestion_dedup_key))))
	&& (!(*input).cron_suggestion_status_present || encode_repeated_cron_suggestion_status(state, (&(*input).cron_suggestion_status)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 7))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_cron_suggestions(
		zcbor_state_t *state, const struct response_cron_suggestions *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronSuggestions", tmp_str.len = sizeof("CronSuggestions") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_cron_suggestions_CronSuggestions_cron_suggestion_m_count, (zcbor_encoder_t *)encode_cron_suggestion, state, (*&(*input).response_cron_suggestions_CronSuggestions_cron_suggestion_m), sizeof(struct cron_suggestion))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_chat_routes(
		zcbor_state_t *state, const struct response_chat_routes *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ChatRoutes", tmp_str.len = sizeof("ChatRoutes") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_chat_routes_ChatRoutes_chat_route_m_count, (zcbor_encoder_t *)encode_chat_route, state, (*&(*input).response_chat_routes_ChatRoutes_chat_route_m), sizeof(struct chat_route))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_chat_route(
		zcbor_state_t *state, const struct response_chat_route *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ChatRoute", tmp_str.len = sizeof("ChatRoute") - 1, &tmp_str)))))
	&& (((*input).response_chat_route_ChatRoute_choice == response_chat_route_ChatRoute_chat_route_m_c) ? ((encode_chat_route(state, (&(*input).response_chat_route_ChatRoute_chat_route_m))))
	: (((*input).response_chat_route_ChatRoute_choice == response_chat_route_ChatRoute_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_room_info_name(
		zcbor_state_t *state, const struct room_info_name_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (((*input).room_info_name_choice == room_info_name_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).room_info_name_tstr))))
	: (((*input).room_info_name_choice == room_info_name_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_room_info_session(
		zcbor_state_t *state, const struct room_info_session_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (((*input).room_info_session_choice == room_info_session_session_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).room_info_session_session_id_m))))
	: (((*input).room_info_session_choice == room_info_session_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_room_info(
		zcbor_state_t *state, const struct room_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).room_info_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"room", tmp_str.len = sizeof("room") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).room_info_room))))
	&& (!(*input).room_info_name_present || encode_repeated_room_info_name(state, (&(*input).room_info_name)))
	&& (!(*input).room_info_session_present || encode_repeated_room_info_session(state, (&(*input).room_info_session)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_rooms(
		zcbor_state_t *state, const struct response_rooms *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Rooms", tmp_str.len = sizeof("Rooms") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_rooms_Rooms_room_info_m_count, (zcbor_encoder_t *)encode_room_info, state, (*&(*input).response_rooms_Rooms_room_info_m), sizeof(struct room_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_adapter_capabilities(
		zcbor_state_t *state, const struct adapter_capabilities *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 6) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"rooms", tmp_str.len = sizeof("rooms") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).adapter_capabilities_rooms))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"direct_messages", tmp_str.len = sizeof("direct_messages") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).adapter_capabilities_direct_messages))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"presence", tmp_str.len = sizeof("presence") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).adapter_capabilities_presence))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"room_enumeration", tmp_str.len = sizeof("room_enumeration") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).adapter_capabilities_room_enumeration))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"file_transfer", tmp_str.len = sizeof("file_transfer") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).adapter_capabilities_file_transfer))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"interactive_auth", tmp_str.len = sizeof("interactive_auth") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).adapter_capabilities_interactive_auth))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 6))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_adapter_info_account_schema(
		zcbor_state_t *state, const struct adapter_info_account_schema *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"account_schema", tmp_str.len = sizeof("account_schema") - 1, &tmp_str)))))
	&& (encode_account_settings_schema(state, (&(*input).adapter_info_account_schema)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_adapter_info(
		zcbor_state_t *state, const struct adapter_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"family", tmp_str.len = sizeof("family") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).adapter_info_family))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"display_name", tmp_str.len = sizeof("display_name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).adapter_info_display_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"capabilities", tmp_str.len = sizeof("capabilities") - 1, &tmp_str)))))
	&& (encode_adapter_capabilities(state, (&(*input).adapter_info_capabilities))))
	&& (!(*input).adapter_info_account_schema_present || encode_repeated_adapter_info_account_schema(state, (&(*input).adapter_info_account_schema)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_adapters(
		zcbor_state_t *state, const struct response_adapters *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Adapters", tmp_str.len = sizeof("Adapters") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_adapters_Adapters_adapter_info_m_count, (zcbor_encoder_t *)encode_adapter_info, state, (*&(*input).response_adapters_Adapters_adapter_info_m), sizeof(struct adapter_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_connection_state(
		zcbor_state_t *state, const struct connection_state_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).connection_state_choice == connection_state_Offline_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Offline", tmp_str.len = sizeof("Offline") - 1, &tmp_str)))))
	: (((*input).connection_state_choice == connection_state_Connecting_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Connecting", tmp_str.len = sizeof("Connecting") - 1, &tmp_str)))))
	: (((*input).connection_state_choice == connection_state_Connected_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Connected", tmp_str.len = sizeof("Connected") - 1, &tmp_str)))))
	: (((*input).connection_state_choice == connection_state_Error_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Error", tmp_str.len = sizeof("Error") - 1, &tmp_str)))))
	: false))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_transport_instance_info_connection(
		zcbor_state_t *state, const struct transport_instance_info_connection *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"connection", tmp_str.len = sizeof("connection") - 1, &tmp_str)))))
	&& (encode_connection_state(state, (&(*input).transport_instance_info_connection)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_presence_state(
		zcbor_state_t *state, const struct presence_state_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).presence_state_choice == presence_state_Unknown_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Unknown", tmp_str.len = sizeof("Unknown") - 1, &tmp_str)))))
	: (((*input).presence_state_choice == presence_state_Offline_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Offline", tmp_str.len = sizeof("Offline") - 1, &tmp_str)))))
	: (((*input).presence_state_choice == presence_state_Available_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Available", tmp_str.len = sizeof("Available") - 1, &tmp_str)))))
	: (((*input).presence_state_choice == presence_state_Idle_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Idle", tmp_str.len = sizeof("Idle") - 1, &tmp_str)))))
	: (((*input).presence_state_choice == presence_state_Away_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Away", tmp_str.len = sizeof("Away") - 1, &tmp_str)))))
	: (((*input).presence_state_choice == presence_state_Busy_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Busy", tmp_str.len = sizeof("Busy") - 1, &tmp_str)))))
	: false))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_transport_instance_info_presence(
		zcbor_state_t *state, const struct transport_instance_info_presence *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"presence", tmp_str.len = sizeof("presence") - 1, &tmp_str)))))
	&& (encode_presence_state(state, (&(*input).transport_instance_info_presence)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_transport_instance_info_bound_profile(
		zcbor_state_t *state, const struct transport_instance_info_bound_profile_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"bound_profile", tmp_str.len = sizeof("bound_profile") - 1, &tmp_str)))))
	&& (((*input).transport_instance_info_bound_profile_choice == transport_instance_info_bound_profile_profile_ref_m_c) ? ((zcbor_tstr_encode(state, (&(*input).transport_instance_info_bound_profile_profile_ref_m))))
	: (((*input).transport_instance_info_bound_profile_choice == transport_instance_info_bound_profile_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_transport_instance_info(
		zcbor_state_t *state, const struct transport_instance_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 6) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).transport_instance_info_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"family", tmp_str.len = sizeof("family") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).transport_instance_info_family))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"display_name", tmp_str.len = sizeof("display_name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).transport_instance_info_display_name))))
	&& (!(*input).transport_instance_info_connection_present || encode_repeated_transport_instance_info_connection(state, (&(*input).transport_instance_info_connection)))
	&& (!(*input).transport_instance_info_presence_present || encode_repeated_transport_instance_info_presence(state, (&(*input).transport_instance_info_presence)))
	&& (!(*input).transport_instance_info_bound_profile_present || encode_repeated_transport_instance_info_bound_profile(state, (&(*input).transport_instance_info_bound_profile)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 6))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_transport_instances(
		zcbor_state_t *state, const struct response_transport_instances *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"TransportInstances", tmp_str.len = sizeof("TransportInstances") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_transport_instances_TransportInstances_transport_instance_info_m_count, (zcbor_encoder_t *)encode_transport_instance_info, state, (*&(*input).response_transport_instances_TransportInstances_transport_instance_info_m), sizeof(struct transport_instance_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_conversation_type(
		zcbor_state_t *state, const struct conversation_type_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).conversation_type_choice == conversation_type_Unset_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Unset", tmp_str.len = sizeof("Unset") - 1, &tmp_str)))))
	: (((*input).conversation_type_choice == conversation_type_Dm_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Dm", tmp_str.len = sizeof("Dm") - 1, &tmp_str)))))
	: (((*input).conversation_type_choice == conversation_type_GroupDm_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"GroupDm", tmp_str.len = sizeof("GroupDm") - 1, &tmp_str)))))
	: (((*input).conversation_type_choice == conversation_type_Channel_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Channel", tmp_str.len = sizeof("Channel") - 1, &tmp_str)))))
	: (((*input).conversation_type_choice == conversation_type_Thread_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Thread", tmp_str.len = sizeof("Thread") - 1, &tmp_str)))))
	: false)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_conversation_info_title(
		zcbor_state_t *state, const struct conversation_info_title_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"title", tmp_str.len = sizeof("title") - 1, &tmp_str)))))
	&& (((*input).conversation_info_title_choice == conversation_info_title_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).conversation_info_title_tstr))))
	: (((*input).conversation_info_title_choice == conversation_info_title_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_conversation_info_topic(
		zcbor_state_t *state, const struct conversation_info_topic_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"topic", tmp_str.len = sizeof("topic") - 1, &tmp_str)))))
	&& (((*input).conversation_info_topic_choice == conversation_info_topic_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).conversation_info_topic_tstr))))
	: (((*input).conversation_info_topic_choice == conversation_info_topic_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_conversation_info_description(
		zcbor_state_t *state, const struct conversation_info_description_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"description", tmp_str.len = sizeof("description") - 1, &tmp_str)))))
	&& (((*input).conversation_info_description_choice == conversation_info_description_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).conversation_info_description_tstr))))
	: (((*input).conversation_info_description_choice == conversation_info_description_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_conversation_member_alias(
		zcbor_state_t *state, const struct conversation_member_alias_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"alias", tmp_str.len = sizeof("alias") - 1, &tmp_str)))))
	&& (((*input).conversation_member_alias_choice == conversation_member_alias_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).conversation_member_alias_tstr))))
	: (((*input).conversation_member_alias_choice == conversation_member_alias_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_conversation_member_nickname(
		zcbor_state_t *state, const struct conversation_member_nickname_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"nickname", tmp_str.len = sizeof("nickname") - 1, &tmp_str)))))
	&& (((*input).conversation_member_nickname_choice == conversation_member_nickname_tstr_c) ? ((zcbor_tstr_encode(state, (&(*input).conversation_member_nickname_tstr))))
	: (((*input).conversation_member_nickname_choice == conversation_member_nickname_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_typing_state(
		zcbor_state_t *state, const struct typing_state_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).typing_state_choice == typing_state_None_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"None", tmp_str.len = sizeof("None") - 1, &tmp_str)))))
	: (((*input).typing_state_choice == typing_state_Typing_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Typing", tmp_str.len = sizeof("Typing") - 1, &tmp_str)))))
	: (((*input).typing_state_choice == typing_state_Paused_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Paused", tmp_str.len = sizeof("Paused") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_conversation_member_typing(
		zcbor_state_t *state, const struct conversation_member_typing *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"typing", tmp_str.len = sizeof("typing") - 1, &tmp_str)))))
	&& (encode_typing_state(state, (&(*input).conversation_member_typing)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_conversation_member_role(
		zcbor_state_t *state, const struct conversation_member_role *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (encode_member_role(state, (&(*input).conversation_member_role)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_conversation_member_session(
		zcbor_state_t *state, const struct conversation_member_session_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (((*input).conversation_member_session_choice == conversation_member_session_session_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).conversation_member_session_session_id_m))))
	: (((*input).conversation_member_session_choice == conversation_member_session_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_conversation_member(
		zcbor_state_t *state, const struct conversation_member *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 6) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"contact", tmp_str.len = sizeof("contact") - 1, &tmp_str)))))
	&& (encode_contact_info(state, (&(*input).conversation_member_contact))))
	&& (!(*input).conversation_member_alias_present || encode_repeated_conversation_member_alias(state, (&(*input).conversation_member_alias)))
	&& (!(*input).conversation_member_nickname_present || encode_repeated_conversation_member_nickname(state, (&(*input).conversation_member_nickname)))
	&& (!(*input).conversation_member_typing_present || encode_repeated_conversation_member_typing(state, (&(*input).conversation_member_typing)))
	&& (!(*input).conversation_member_role_present || encode_repeated_conversation_member_role(state, (&(*input).conversation_member_role)))
	&& (!(*input).conversation_member_session_present || encode_repeated_conversation_member_session(state, (&(*input).conversation_member_session)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 6))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_conversation_info_members(
		zcbor_state_t *state, const struct conversation_info_members_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"members", tmp_str.len = sizeof("members") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).conversation_info_members_conversation_member_m_count, (zcbor_encoder_t *)encode_conversation_member, state, (*&(*input).conversation_info_members_conversation_member_m), sizeof(struct conversation_member))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_conversation_info(
		zcbor_state_t *state, const struct conversation_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 7) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"transport", tmp_str.len = sizeof("transport") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).conversation_info_transport))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).conversation_info_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (encode_conversation_type(state, (&(*input).conversation_info_kind))))
	&& (!(*input).conversation_info_title_present || encode_repeated_conversation_info_title(state, (&(*input).conversation_info_title)))
	&& (!(*input).conversation_info_topic_present || encode_repeated_conversation_info_topic(state, (&(*input).conversation_info_topic)))
	&& (!(*input).conversation_info_description_present || encode_repeated_conversation_info_description(state, (&(*input).conversation_info_description)))
	&& (!(*input).conversation_info_members_present || encode_repeated_conversation_info_members(state, (&(*input).conversation_info_members)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 7))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_conversations(
		zcbor_state_t *state, const struct response_conversations *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Conversations", tmp_str.len = sizeof("Conversations") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_conversations_Conversations_conversation_info_m_count, (zcbor_encoder_t *)encode_conversation_info, state, (*&(*input).response_conversations_Conversations_conversation_info_m), sizeof(struct conversation_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_conversation(
		zcbor_state_t *state, const struct response_conversation *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Conversation", tmp_str.len = sizeof("Conversation") - 1, &tmp_str)))))
	&& (((*input).response_conversation_Conversation_choice == response_conversation_Conversation_conversation_info_m_c) ? ((encode_conversation_info(state, (&(*input).response_conversation_Conversation_conversation_info_m))))
	: (((*input).response_conversation_Conversation_choice == response_conversation_Conversation_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_conv_create_details(
		zcbor_state_t *state, const struct response_conv_create_details *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvCreateDetails", tmp_str.len = sizeof("ConvCreateDetails") - 1, &tmp_str)))))
	&& (encode_create_conversation_details(state, (&(*input).response_conv_create_details_ConvCreateDetails))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_conv_join_details(
		zcbor_state_t *state, const struct response_conv_join_details *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConvJoinDetails", tmp_str.len = sizeof("ConvJoinDetails") - 1, &tmp_str)))))
	&& (encode_channel_join_details(state, (&(*input).response_conv_join_details_ConvJoinDetails))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_contact_profile(
		zcbor_state_t *state, const struct response_contact_profile *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ContactProfile", tmp_str.len = sizeof("ContactProfile") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).response_contact_profile_ContactProfile))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_contacts(
		zcbor_state_t *state, const struct response_contacts *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Contacts", tmp_str.len = sizeof("Contacts") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_contacts_Contacts_contact_info_m_count, (zcbor_encoder_t *)encode_contact_info, state, (*&(*input).response_contacts_Contacts_contact_info_m), sizeof(struct contact_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_action_menu_items(
		zcbor_state_t *state, const struct action_menu_items_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"items", tmp_str.len = sizeof("items") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).action_menu_items_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).action_menu_items_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_action_menu(
		zcbor_state_t *state, const struct action_menu *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_encode(state, 1) && (((!(*input).action_menu_items_present || encode_repeated_action_menu_items(state, (&(*input).action_menu_items)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_action_menu(
		zcbor_state_t *state, const struct response_action_menu *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ActionMenu", tmp_str.len = sizeof("ActionMenu") - 1, &tmp_str)))))
	&& (((*input).response_action_menu_ActionMenu_choice == response_action_menu_ActionMenu_action_menu_m_c) ? ((encode_action_menu(state, (&(*input).response_action_menu_ActionMenu_action_menu_m))))
	: (((*input).response_action_menu_ActionMenu_choice == response_action_menu_ActionMenu_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_api_error_unknown_session(
		zcbor_state_t *state, const struct api_error_unknown_session *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"UnknownSession", tmp_str.len = sizeof("UnknownSession") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).api_error_unknown_session_UnknownSession))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_api_error_unsupported(
		zcbor_state_t *state, const struct api_error_unsupported *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Unsupported", tmp_str.len = sizeof("Unsupported") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).api_error_unsupported_Unsupported))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_api_error_conflict(
		zcbor_state_t *state, const struct api_error_conflict *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Conflict", tmp_str.len = sizeof("Conflict") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).api_error_conflict_Conflict))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_api_error_unauthenticated(
		zcbor_state_t *state, const struct api_error_unauthenticated *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Unauthenticated", tmp_str.len = sizeof("Unauthenticated") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).api_error_unauthenticated_Unauthenticated))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_api_error_forbidden(
		zcbor_state_t *state, const struct api_error_forbidden *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Forbidden", tmp_str.len = sizeof("Forbidden") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).api_error_forbidden_Forbidden))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_api_error_other(
		zcbor_state_t *state, const struct api_error_other *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Other", tmp_str.len = sizeof("Other") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).api_error_other_Other))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_api_error(
		zcbor_state_t *state, const struct api_error_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).api_error_choice == api_error_unknown_session_m_c) ? ((encode_api_error_unknown_session(state, (&(*input).api_error_unknown_session_m))))
	: (((*input).api_error_choice == api_error_unsupported_m_c) ? ((encode_api_error_unsupported(state, (&(*input).api_error_unsupported_m))))
	: (((*input).api_error_choice == api_error_conflict_m_c) ? ((encode_api_error_conflict(state, (&(*input).api_error_conflict_m))))
	: (((*input).api_error_choice == api_error_unauthenticated_m_c) ? ((encode_api_error_unauthenticated(state, (&(*input).api_error_unauthenticated_m))))
	: (((*input).api_error_choice == api_error_forbidden_m_c) ? ((encode_api_error_forbidden(state, (&(*input).api_error_forbidden_m))))
	: (((*input).api_error_choice == api_error_other_m_c) ? ((encode_api_error_other(state, (&(*input).api_error_other_m))))
	: false))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_error(
		zcbor_state_t *state, const struct response_error *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Error", tmp_str.len = sizeof("Error") - 1, &tmp_str)))))
	&& (encode_api_error(state, (&(*input).response_error_Error))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_root_kind_t(
		zcbor_state_t *state, const struct fs_root_kind_t_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).fs_root_kind_t_choice == fs_root_kind_t_host_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"host", tmp_str.len = sizeof("host") - 1, &tmp_str)))))
	: (((*input).fs_root_kind_t_choice == fs_root_kind_t_workspace_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"workspace", tmp_str.len = sizeof("workspace") - 1, &tmp_str)))))
	: (((*input).fs_root_kind_t_choice == fs_root_kind_t_session_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_root_session(
		zcbor_state_t *state, const struct fs_root_session_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (((*input).fs_root_session_choice == fs_root_session_session_id_m_c) ? ((zcbor_tstr_encode(state, (&(*input).fs_root_session_session_id_m))))
	: (((*input).fs_root_session_choice == fs_root_session_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_root(
		zcbor_state_t *state, const struct fs_root *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"id", tmp_str.len = sizeof("id") - 1, &tmp_str)))))
	&& (encode_fs_root_id_t(state, (&(*input).fs_root_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"label", tmp_str.len = sizeof("label") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_root_label))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (encode_fs_root_kind_t(state, (&(*input).fs_root_kind))))
	&& (!(*input).fs_root_session_present || encode_repeated_fs_root_session(state, (&(*input).fs_root_session)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_fs_roots(
		zcbor_state_t *state, const struct response_fs_roots *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsRoots", tmp_str.len = sizeof("FsRoots") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_fs_roots_FsRoots_fs_root_m_count, (zcbor_encoder_t *)encode_fs_root, state, (*&(*input).response_fs_roots_FsRoots_fs_root_m), sizeof(struct fs_root))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_entry_kind_t(
		zcbor_state_t *state, const struct fs_entry_kind_t_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).fs_entry_kind_t_choice == fs_entry_kind_t_file_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"file", tmp_str.len = sizeof("file") - 1, &tmp_str)))))
	: (((*input).fs_entry_kind_t_choice == fs_entry_kind_t_dir_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"dir", tmp_str.len = sizeof("dir") - 1, &tmp_str)))))
	: (((*input).fs_entry_kind_t_choice == fs_entry_kind_t_symlink_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"symlink", tmp_str.len = sizeof("symlink") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_entry_ignored(
		zcbor_state_t *state, const struct fs_entry_ignored *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ignored", tmp_str.len = sizeof("ignored") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).fs_entry_ignored)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_entry(
		zcbor_state_t *state, const struct fs_entry *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 6) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"name", tmp_str.len = sizeof("name") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_entry_name))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_entry_path))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (encode_fs_entry_kind_t(state, (&(*input).fs_entry_kind))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"size", tmp_str.len = sizeof("size") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).fs_entry_size))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"mtime_ms", tmp_str.len = sizeof("mtime_ms") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).fs_entry_mtime_ms))))
	&& (!(*input).fs_entry_ignored_present || encode_repeated_fs_entry_ignored(state, (&(*input).fs_entry_ignored)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 6))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_fs_list(
		zcbor_state_t *state, const struct response_fs_list *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsList", tmp_str.len = sizeof("FsList") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_fs_list_FsList_fs_entry_m_count, (zcbor_encoder_t *)encode_fs_entry, state, (*&(*input).response_fs_list_FsList_fs_entry_m), sizeof(struct fs_entry))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_fs_stat(
		zcbor_state_t *state, const struct response_fs_stat *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsStat", tmp_str.len = sizeof("FsStat") - 1, &tmp_str)))))
	&& (encode_fs_entry(state, (&(*input).response_fs_stat_FsStat))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_content_truncated(
		zcbor_state_t *state, const struct fs_content_truncated *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"truncated", tmp_str.len = sizeof("truncated") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).fs_content_truncated)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_content_blob_ref(
		zcbor_state_t *state, const struct fs_content_blob_ref_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"blob_ref", tmp_str.len = sizeof("blob_ref") - 1, &tmp_str)))))
	&& (((*input).fs_content_blob_ref_choice == fs_content_blob_ref_blob_ref_m_c) ? ((encode_blob_ref(state, (&(*input).fs_content_blob_ref_blob_ref_m))))
	: (((*input).fs_content_blob_ref_choice == fs_content_blob_ref_null_m_c) ? ((zcbor_nil_put(state, NULL)))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_content(
		zcbor_state_t *state, const struct fs_content *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"bytes", tmp_str.len = sizeof("bytes") - 1, &tmp_str)))))
	&& (zcbor_bstr_encode(state, (&(*input).fs_content_bytes))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"revision", tmp_str.len = sizeof("revision") - 1, &tmp_str)))))
	&& (encode_fs_revision(state, (&(*input).fs_content_revision))))
	&& (!(*input).fs_content_truncated_present || encode_repeated_fs_content_truncated(state, (&(*input).fs_content_truncated)))
	&& (!(*input).fs_content_blob_ref_present || encode_repeated_fs_content_blob_ref(state, (&(*input).fs_content_blob_ref)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_fs_read(
		zcbor_state_t *state, const struct response_fs_read *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsRead", tmp_str.len = sizeof("FsRead") - 1, &tmp_str)))))
	&& (encode_fs_content(state, (&(*input).response_fs_read_FsRead))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_fs_write(
		zcbor_state_t *state, const struct response_fs_write *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsWrite", tmp_str.len = sizeof("FsWrite") - 1, &tmp_str)))))
	&& (encode_fs_revision(state, (&(*input).response_fs_write_FsWrite))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_search_hit(
		zcbor_state_t *state, const struct fs_search_hit *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_search_hit_path))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"line", tmp_str.len = sizeof("line") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).fs_search_hit_line))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"col", tmp_str.len = sizeof("col") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).fs_search_hit_col))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"preview", tmp_str.len = sizeof("preview") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_search_hit_preview))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_fs_search_page_has_more(
		zcbor_state_t *state, const struct fs_search_page_has_more *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"has_more", tmp_str.len = sizeof("has_more") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).fs_search_page_has_more)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_search_page(
		zcbor_state_t *state, const struct fs_search_page *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"hits", tmp_str.len = sizeof("hits") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).fs_search_page_hits_fs_search_hit_m_count, (zcbor_encoder_t *)encode_fs_search_hit, state, (*&(*input).fs_search_page_hits_fs_search_hit_m), sizeof(struct fs_search_hit))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (!(*input).fs_search_page_has_more_present || encode_repeated_fs_search_page_has_more(state, (&(*input).fs_search_page_has_more)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_fs_search(
		zcbor_state_t *state, const struct response_fs_search *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsSearch", tmp_str.len = sizeof("FsSearch") - 1, &tmp_str)))))
	&& (encode_fs_search_page(state, (&(*input).response_fs_search_FsSearch))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_change_kind_t(
		zcbor_state_t *state, const struct fs_change_kind_t_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).fs_change_kind_t_choice == fs_change_kind_t_created_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"created", tmp_str.len = sizeof("created") - 1, &tmp_str)))))
	: (((*input).fs_change_kind_t_choice == fs_change_kind_t_modified_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"modified", tmp_str.len = sizeof("modified") - 1, &tmp_str)))))
	: (((*input).fs_change_kind_t_choice == fs_change_kind_t_removed_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"removed", tmp_str.len = sizeof("removed") - 1, &tmp_str)))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_change(
		zcbor_state_t *state, const struct fs_change *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"path", tmp_str.len = sizeof("path") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_change_path))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (encode_fs_change_kind_t(state, (&(*input).fs_change_kind))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_fs_watch_page_view(
		zcbor_state_t *state, const struct fs_watch_page_view *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"events", tmp_str.len = sizeof("events") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).fs_watch_page_view_events_fs_change_m_count, (zcbor_encoder_t *)encode_fs_change, state, (*&(*input).fs_watch_page_view_events_fs_change_m), sizeof(struct fs_change))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"next_seq", tmp_str.len = sizeof("next_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).fs_watch_page_view_next_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"head_seq", tmp_str.len = sizeof("head_seq") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).fs_watch_page_view_head_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"reset", tmp_str.len = sizeof("reset") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).fs_watch_page_view_reset))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_fs_watch(
		zcbor_state_t *state, const struct response_fs_watch *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsWatch", tmp_str.len = sizeof("FsWatch") - 1, &tmp_str)))))
	&& (encode_fs_watch_page_view(state, (&(*input).response_fs_watch_FsWatch))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_blob_put(
		zcbor_state_t *state, const struct response_blob_put *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"BlobPut", tmp_str.len = sizeof("BlobPut") - 1, &tmp_str)))))
	&& (encode_blob_ref(state, (&(*input).response_blob_put_BlobPut))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_blob_get(
		zcbor_state_t *state, const struct response_blob_get *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"BlobGet", tmp_str.len = sizeof("BlobGet") - 1, &tmp_str)))))
	&& (zcbor_bstr_encode(state, (&(*input).response_blob_get_BlobGet))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_blob_stat(
		zcbor_state_t *state, const struct blob_stat *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"size", tmp_str.len = sizeof("size") - 1, &tmp_str)))))
	&& (zcbor_uint64_encode(state, (&(*input).blob_stat_size))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"present", tmp_str.len = sizeof("present") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).blob_stat_present))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_blob_stat(
		zcbor_state_t *state, const struct response_blob_stat *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"BlobStat", tmp_str.len = sizeof("BlobStat") - 1, &tmp_str)))))
	&& (encode_blob_stat(state, (&(*input).response_blob_stat_BlobStat))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_access_user(
		zcbor_state_t *state, const struct access_user *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).access_user_user_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"username", tmp_str.len = sizeof("username") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).access_user_username))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"disabled", tmp_str.len = sizeof("disabled") - 1, &tmp_str)))))
	&& (zcbor_bool_encode(state, (&(*input).access_user_disabled))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"created_at", tmp_str.len = sizeof("created_at") - 1, &tmp_str)))))
	&& (zcbor_int32_encode(state, (&(*input).access_user_created_at))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"roles", tmp_str.len = sizeof("roles") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).access_user_roles_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).access_user_roles_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_access_user(
		zcbor_state_t *state, const struct response_access_user *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AccessUser", tmp_str.len = sizeof("AccessUser") - 1, &tmp_str)))))
	&& (encode_access_user(state, (&(*input).response_access_user_AccessUser))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_access_users(
		zcbor_state_t *state, const struct response_access_users *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AccessUsers", tmp_str.len = sizeof("AccessUsers") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_access_users_AccessUsers_access_user_m_count, (zcbor_encoder_t *)encode_access_user, state, (*&(*input).response_access_users_AccessUsers_access_user_m), sizeof(struct access_user))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_role_info(
		zcbor_state_t *state, const struct role_info *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"role", tmp_str.len = sizeof("role") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).role_info_role))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"capabilities", tmp_str.len = sizeof("capabilities") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).role_info_capabilities_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).role_info_capabilities_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_access_roles(
		zcbor_state_t *state, const struct response_access_roles *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AccessRoles", tmp_str.len = sizeof("AccessRoles") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).response_access_roles_AccessRoles_role_info_m_count, (zcbor_encoder_t *)encode_role_info, state, (*&(*input).response_access_roles_AccessRoles_role_info_m), sizeof(struct role_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_principal_view(
		zcbor_state_t *state, const struct principal_view *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 4) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"user_id", tmp_str.len = sizeof("user_id") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).principal_view_user_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"username", tmp_str.len = sizeof("username") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).principal_view_username))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"roles", tmp_str.len = sizeof("roles") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).principal_view_roles_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).principal_view_roles_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"capabilities", tmp_str.len = sizeof("capabilities") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 64) && ((zcbor_multi_encode_minmax(0, 64, &(*input).principal_view_capabilities_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).principal_view_capabilities_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 64)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 4))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_response_who_am_i(
		zcbor_state_t *state, const struct response_who_am_i *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"WhoAmI", tmp_str.len = sizeof("WhoAmI") - 1, &tmp_str)))))
	&& (encode_principal_view(state, (&(*input).response_who_am_i_WhoAmI))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_api_response(
		zcbor_state_t *state, const struct api_response_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).api_response_choice == api_response_response_ok_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Ok", tmp_str.len = sizeof("Ok") - 1, &tmp_str)))))
	: (((*input).api_response_choice == api_response_response_routed_m_c) ? ((encode_response_routed(state, (&(*input).api_response_response_routed_m))))
	: (((*input).api_response_choice == api_response_response_drained_m_c) ? ((encode_response_drained(state, (&(*input).api_response_response_drained_m))))
	: (((*input).api_response_choice == api_response_response_health_m_c) ? ((encode_response_health(state, (&(*input).api_response_response_health_m))))
	: (((*input).api_response_choice == api_response_response_stats_m_c) ? ((encode_response_stats(state, (&(*input).api_response_response_stats_m))))
	: (((*input).api_response_choice == api_response_response_telemetry_m_c) ? ((encode_response_telemetry(state, (&(*input).api_response_response_telemetry_m))))
	: (((*input).api_response_choice == api_response_response_sessions_m_c) ? ((encode_response_sessions(state, (&(*input).api_response_response_sessions_m))))
	: (((*input).api_response_choice == api_response_response_approvals_m_c) ? ((encode_response_approvals(state, (&(*input).api_response_response_approvals_m))))
	: (((*input).api_response_choice == api_response_response_fleet_m_c) ? ((encode_response_fleet(state, (&(*input).api_response_response_fleet_m))))
	: (((*input).api_response_choice == api_response_response_tree_m_c) ? ((encode_response_tree(state, (&(*input).api_response_response_tree_m))))
	: (((*input).api_response_choice == api_response_response_unit_m_c) ? ((encode_response_unit(state, (&(*input).api_response_response_unit_m))))
	: (((*input).api_response_choice == api_response_response_unit_events_m_c) ? ((encode_response_unit_events(state, (&(*input).api_response_response_unit_events_m))))
	: (((*input).api_response_choice == api_response_response_journal_m_c) ? ((encode_response_journal(state, (&(*input).api_response_response_journal_m))))
	: (((*input).api_response_choice == api_response_response_log_page_m_c) ? ((encode_response_log_page(state, (&(*input).api_response_response_log_page_m))))
	: (((*input).api_response_choice == api_response_response_events_page_m_c) ? ((encode_response_events_page(state, (&(*input).api_response_response_events_page_m))))
	: (((*input).api_response_choice == api_response_response_delivery_targets_m_c) ? ((encode_response_delivery_targets(state, (&(*input).api_response_response_delivery_targets_m))))
	: (((*input).api_response_choice == api_response_response_delivery_sessions_m_c) ? ((encode_response_delivery_sessions(state, (&(*input).api_response_response_delivery_sessions_m))))
	: (((*input).api_response_choice == api_response_response_verifying_key_m_c) ? ((encode_response_verifying_key(state, (&(*input).api_response_response_verifying_key_m))))
	: (((*input).api_response_choice == api_response_response_model_search_m_c) ? ((encode_response_model_search(state, (&(*input).api_response_response_model_search_m))))
	: (((*input).api_response_choice == api_response_response_model_files_m_c) ? ((encode_response_model_files(state, (&(*input).api_response_response_model_files_m))))
	: (((*input).api_response_choice == api_response_response_model_download_started_m_c) ? ((encode_response_model_download_started(state, (&(*input).api_response_response_model_download_started_m))))
	: (((*input).api_response_choice == api_response_response_model_downloads_m_c) ? ((encode_response_model_downloads(state, (&(*input).api_response_response_model_downloads_m))))
	: (((*input).api_response_choice == api_response_response_model_catalog_m_c) ? ((encode_response_model_catalog(state, (&(*input).api_response_response_model_catalog_m))))
	: (((*input).api_response_choice == api_response_response_model_recommend_m_c) ? ((encode_response_model_recommend(state, (&(*input).api_response_response_model_recommend_m))))
	: (((*input).api_response_choice == api_response_response_model_quantize_started_m_c) ? ((encode_response_model_quantize_started(state, (&(*input).api_response_response_model_quantize_started_m))))
	: (((*input).api_response_choice == api_response_response_model_quantizes_m_c) ? ((encode_response_model_quantizes(state, (&(*input).api_response_response_model_quantizes_m))))
	: (((*input).api_response_choice == api_response_response_model_inspect_m_c) ? ((encode_response_model_inspect(state, (&(*input).api_response_response_model_inspect_m))))
	: (((*input).api_response_choice == api_response_response_profiles_m_c) ? ((encode_response_profiles(state, (&(*input).api_response_response_profiles_m))))
	: (((*input).api_response_choice == api_response_response_profile_m_c) ? ((encode_response_profile(state, (&(*input).api_response_response_profile_m))))
	: (((*input).api_response_choice == api_response_response_credentials_m_c) ? ((encode_response_credentials(state, (&(*input).api_response_response_credentials_m))))
	: (((*input).api_response_choice == api_response_response_models_m_c) ? ((encode_response_models(state, (&(*input).api_response_response_models_m))))
	: (((*input).api_response_choice == api_response_response_model_current_m_c) ? ((encode_response_model_current(state, (&(*input).api_response_response_model_current_m))))
	: (((*input).api_response_choice == api_response_response_distribution_m_c) ? ((encode_response_distribution(state, (&(*input).api_response_response_distribution_m))))
	: (((*input).api_response_choice == api_response_response_profile_id_m_c) ? ((encode_response_profile_id(state, (&(*input).api_response_response_profile_id_m))))
	: (((*input).api_response_choice == api_response_response_revisions_m_c) ? ((encode_response_revisions(state, (&(*input).api_response_response_revisions_m))))
	: (((*input).api_response_choice == api_response_response_skill_bundle_m_c) ? ((encode_response_skill_bundle(state, (&(*input).api_response_response_skill_bundle_m))))
	: (((*input).api_response_choice == api_response_response_curator_skills_m_c) ? ((encode_response_curator_skills(state, (&(*input).api_response_response_curator_skills_m))))
	: (((*input).api_response_choice == api_response_response_curator_run_m_c) ? ((encode_response_curator_run(state, (&(*input).api_response_response_curator_run_m))))
	: (((*input).api_response_choice == api_response_response_auth_begun_m_c) ? ((encode_response_auth_begun(state, (&(*input).api_response_response_auth_begun_m))))
	: (((*input).api_response_choice == api_response_response_auth_completed_m_c) ? ((encode_response_auth_completed(state, (&(*input).api_response_response_auth_completed_m))))
	: (((*input).api_response_choice == api_response_response_auth_providers_m_c) ? ((encode_response_auth_providers(state, (&(*input).api_response_response_auth_providers_m))))
	: (((*input).api_response_choice == api_response_response_checkpoints_m_c) ? ((encode_response_checkpoints(state, (&(*input).api_response_response_checkpoints_m))))
	: (((*input).api_response_choice == api_response_response_session_page_m_c) ? ((encode_response_session_page(state, (&(*input).api_response_response_session_page_m))))
	: (((*input).api_response_choice == api_response_response_session_detail_m_c) ? ((encode_response_session_detail(state, (&(*input).api_response_response_session_detail_m))))
	: (((*input).api_response_choice == api_response_response_sessions_by_profile_m_c) ? ((encode_response_sessions_by_profile(state, (&(*input).api_response_response_sessions_by_profile_m))))
	: (((*input).api_response_choice == api_response_response_session_search_m_c) ? ((encode_response_session_search(state, (&(*input).api_response_response_session_search_m))))
	: (((*input).api_response_choice == api_response_response_acp_catalog_m_c) ? ((encode_response_acp_catalog(state, (&(*input).api_response_response_acp_catalog_m))))
	: (((*input).api_response_choice == api_response_response_providers_m_c) ? ((encode_response_providers(state, (&(*input).api_response_response_providers_m))))
	: (((*input).api_response_choice == api_response_response_tools_m_c) ? ((encode_response_tools(state, (&(*input).api_response_response_tools_m))))
	: (((*input).api_response_choice == api_response_response_commands_m_c) ? ((encode_response_commands(state, (&(*input).api_response_response_commands_m))))
	: (((*input).api_response_choice == api_response_response_command_output_m_c) ? ((encode_response_command_output(state, (&(*input).api_response_response_command_output_m))))
	: (((*input).api_response_choice == api_response_response_config_m_c) ? ((encode_response_config(state, (&(*input).api_response_response_config_m))))
	: (((*input).api_response_choice == api_response_response_cron_jobs_m_c) ? ((encode_response_cron_jobs(state, (&(*input).api_response_response_cron_jobs_m))))
	: (((*input).api_response_choice == api_response_response_cron_id_m_c) ? ((encode_response_cron_id(state, (&(*input).api_response_response_cron_id_m))))
	: (((*input).api_response_choice == api_response_response_cron_runs_m_c) ? ((encode_response_cron_runs(state, (&(*input).api_response_response_cron_runs_m))))
	: (((*input).api_response_choice == api_response_response_cron_suggestions_m_c) ? ((encode_response_cron_suggestions(state, (&(*input).api_response_response_cron_suggestions_m))))
	: (((*input).api_response_choice == api_response_response_chat_routes_m_c) ? ((encode_response_chat_routes(state, (&(*input).api_response_response_chat_routes_m))))
	: (((*input).api_response_choice == api_response_response_chat_route_m_c) ? ((encode_response_chat_route(state, (&(*input).api_response_response_chat_route_m))))
	: (((*input).api_response_choice == api_response_response_rooms_m_c) ? ((encode_response_rooms(state, (&(*input).api_response_response_rooms_m))))
	: (((*input).api_response_choice == api_response_response_adapters_m_c) ? ((encode_response_adapters(state, (&(*input).api_response_response_adapters_m))))
	: (((*input).api_response_choice == api_response_response_transport_instances_m_c) ? ((encode_response_transport_instances(state, (&(*input).api_response_response_transport_instances_m))))
	: (((*input).api_response_choice == api_response_response_conversations_m_c) ? ((encode_response_conversations(state, (&(*input).api_response_response_conversations_m))))
	: (((*input).api_response_choice == api_response_response_conversation_m_c) ? ((encode_response_conversation(state, (&(*input).api_response_response_conversation_m))))
	: (((*input).api_response_choice == api_response_response_conv_create_details_m_c) ? ((encode_response_conv_create_details(state, (&(*input).api_response_response_conv_create_details_m))))
	: (((*input).api_response_choice == api_response_response_conv_join_details_m_c) ? ((encode_response_conv_join_details(state, (&(*input).api_response_response_conv_join_details_m))))
	: (((*input).api_response_choice == api_response_response_contact_profile_m_c) ? ((encode_response_contact_profile(state, (&(*input).api_response_response_contact_profile_m))))
	: (((*input).api_response_choice == api_response_response_contacts_m_c) ? ((encode_response_contacts(state, (&(*input).api_response_response_contacts_m))))
	: (((*input).api_response_choice == api_response_response_action_menu_m_c) ? ((encode_response_action_menu(state, (&(*input).api_response_response_action_menu_m))))
	: (((*input).api_response_choice == api_response_response_error_m_c) ? ((encode_response_error(state, (&(*input).api_response_response_error_m))))
	: (((*input).api_response_choice == api_response_response_fs_roots_m_c) ? ((encode_response_fs_roots(state, (&(*input).api_response_response_fs_roots_m))))
	: (((*input).api_response_choice == api_response_response_fs_list_m_c) ? ((encode_response_fs_list(state, (&(*input).api_response_response_fs_list_m))))
	: (((*input).api_response_choice == api_response_response_fs_stat_m_c) ? ((encode_response_fs_stat(state, (&(*input).api_response_response_fs_stat_m))))
	: (((*input).api_response_choice == api_response_response_fs_read_m_c) ? ((encode_response_fs_read(state, (&(*input).api_response_response_fs_read_m))))
	: (((*input).api_response_choice == api_response_response_fs_write_m_c) ? ((encode_response_fs_write(state, (&(*input).api_response_response_fs_write_m))))
	: (((*input).api_response_choice == api_response_response_fs_search_m_c) ? ((encode_response_fs_search(state, (&(*input).api_response_response_fs_search_m))))
	: (((*input).api_response_choice == api_response_response_fs_watch_m_c) ? ((encode_response_fs_watch(state, (&(*input).api_response_response_fs_watch_m))))
	: (((*input).api_response_choice == api_response_response_blob_put_m_c) ? ((encode_response_blob_put(state, (&(*input).api_response_response_blob_put_m))))
	: (((*input).api_response_choice == api_response_response_blob_get_m_c) ? ((encode_response_blob_get(state, (&(*input).api_response_response_blob_get_m))))
	: (((*input).api_response_choice == api_response_response_blob_stat_m_c) ? ((encode_response_blob_stat(state, (&(*input).api_response_response_blob_stat_m))))
	: (((*input).api_response_choice == api_response_response_access_user_m_c) ? ((encode_response_access_user(state, (&(*input).api_response_response_access_user_m))))
	: (((*input).api_response_choice == api_response_response_access_users_m_c) ? ((encode_response_access_users(state, (&(*input).api_response_response_access_users_m))))
	: (((*input).api_response_choice == api_response_response_access_roles_m_c) ? ((encode_response_access_roles(state, (&(*input).api_response_response_access_roles_m))))
	: (((*input).api_response_choice == api_response_response_who_am_i_m_c) ? ((encode_response_who_am_i(state, (&(*input).api_response_response_who_am_i_m))))
	: false)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_api_request(
		zcbor_state_t *state, const struct api_request_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).api_request_choice == api_request_request_submit_m_c) ? ((encode_request_submit(state, (&(*input).api_request_request_submit_m))))
	: (((*input).api_request_choice == api_request_request_submit_routed_m_c) ? ((encode_request_submit_routed(state, (&(*input).api_request_request_submit_routed_m))))
	: (((*input).api_request_choice == api_request_request_poll_m_c) ? ((encode_request_poll(state, (&(*input).api_request_request_poll_m))))
	: (((*input).api_request_choice == api_request_request_respond_m_c) ? ((encode_request_respond(state, (&(*input).api_request_request_respond_m))))
	: (((*input).api_request_choice == api_request_request_health_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Health", tmp_str.len = sizeof("Health") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_stats_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Stats", tmp_str.len = sizeof("Stats") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_telemetry_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Telemetry", tmp_str.len = sizeof("Telemetry") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_sessions_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Sessions", tmp_str.len = sizeof("Sessions") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_assign_m_c) ? ((encode_request_assign(state, (&(*input).api_request_request_assign_m))))
	: (((*input).api_request_choice == api_request_request_cancel_m_c) ? ((encode_request_cancel(state, (&(*input).api_request_request_cancel_m))))
	: (((*input).api_request_choice == api_request_request_fleet_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Fleet", tmp_str.len = sizeof("Fleet") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_tree_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Tree", tmp_str.len = sizeof("Tree") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_unit_m_c) ? ((encode_request_unit(state, (&(*input).api_request_request_unit_m))))
	: (((*input).api_request_choice == api_request_request_unit_events_m_c) ? ((encode_request_unit_events(state, (&(*input).api_request_request_unit_events_m))))
	: (((*input).api_request_choice == api_request_request_unit_outbound_m_c) ? ((encode_request_unit_outbound(state, (&(*input).api_request_request_unit_outbound_m))))
	: (((*input).api_request_choice == api_request_request_session_history_m_c) ? ((encode_request_session_history(state, (&(*input).api_request_request_session_history_m))))
	: (((*input).api_request_choice == api_request_request_subscribe_m_c) ? ((encode_request_subscribe(state, (&(*input).api_request_request_subscribe_m))))
	: (((*input).api_request_choice == api_request_request_events_since_m_c) ? ((encode_request_events_since(state, (&(*input).api_request_request_events_since_m))))
	: (((*input).api_request_choice == api_request_request_delivery_targets_m_c) ? ((encode_request_delivery_targets(state, (&(*input).api_request_request_delivery_targets_m))))
	: (((*input).api_request_choice == api_request_request_delivery_sessions_m_c) ? ((encode_request_delivery_sessions(state, (&(*input).api_request_request_delivery_sessions_m))))
	: (((*input).api_request_choice == api_request_request_handover_m_c) ? ((encode_request_handover(state, (&(*input).api_request_request_handover_m))))
	: (((*input).api_request_choice == api_request_request_record_meta_m_c) ? ((encode_request_record_meta(state, (&(*input).api_request_request_record_meta_m))))
	: (((*input).api_request_choice == api_request_request_unit_history_m_c) ? ((encode_request_unit_history(state, (&(*input).api_request_request_unit_history_m))))
	: (((*input).api_request_choice == api_request_request_pause_m_c) ? ((encode_request_pause(state, (&(*input).api_request_request_pause_m))))
	: (((*input).api_request_choice == api_request_request_resume_m_c) ? ((encode_request_resume(state, (&(*input).api_request_request_resume_m))))
	: (((*input).api_request_choice == api_request_request_scale_m_c) ? ((encode_request_scale(state, (&(*input).api_request_request_scale_m))))
	: (((*input).api_request_choice == api_request_request_verifying_key_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"VerifyingKey", tmp_str.len = sizeof("VerifyingKey") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_set_session_model_m_c) ? ((encode_request_set_session_model(state, (&(*input).api_request_request_set_session_model_m))))
	: (((*input).api_request_choice == api_request_request_set_session_mode_m_c) ? ((encode_request_set_session_mode(state, (&(*input).api_request_request_set_session_mode_m))))
	: (((*input).api_request_choice == api_request_request_set_session_overlay_m_c) ? ((encode_request_set_session_overlay(state, (&(*input).api_request_request_set_session_overlay_m))))
	: (((*input).api_request_choice == api_request_request_approvals_pending_m_c) ? ((encode_request_approvals_pending(state, (&(*input).api_request_request_approvals_pending_m))))
	: (((*input).api_request_choice == api_request_request_approval_decide_m_c) ? ((encode_request_approval_decide(state, (&(*input).api_request_request_approval_decide_m))))
	: (((*input).api_request_choice == api_request_request_profile_list_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileList", tmp_str.len = sizeof("ProfileList") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_profile_get_m_c) ? ((encode_request_profile_get(state, (&(*input).api_request_request_profile_get_m))))
	: (((*input).api_request_choice == api_request_request_profile_create_m_c) ? ((encode_request_profile_create(state, (&(*input).api_request_request_profile_create_m))))
	: (((*input).api_request_choice == api_request_request_profile_update_m_c) ? ((encode_request_profile_update(state, (&(*input).api_request_request_profile_update_m))))
	: (((*input).api_request_choice == api_request_request_profile_delete_m_c) ? ((encode_request_profile_delete(state, (&(*input).api_request_request_profile_delete_m))))
	: (((*input).api_request_choice == api_request_request_profile_select_m_c) ? ((encode_request_profile_select(state, (&(*input).api_request_request_profile_select_m))))
	: (((*input).api_request_choice == api_request_request_profile_clone_m_c) ? ((encode_request_profile_clone(state, (&(*input).api_request_request_profile_clone_m))))
	: (((*input).api_request_choice == api_request_request_profile_export_m_c) ? ((encode_request_profile_export(state, (&(*input).api_request_request_profile_export_m))))
	: (((*input).api_request_choice == api_request_request_profile_import_m_c) ? ((encode_request_profile_import(state, (&(*input).api_request_request_profile_import_m))))
	: (((*input).api_request_choice == api_request_request_profile_history_m_c) ? ((encode_request_profile_history(state, (&(*input).api_request_request_profile_history_m))))
	: (((*input).api_request_choice == api_request_request_profile_at_m_c) ? ((encode_request_profile_at(state, (&(*input).api_request_request_profile_at_m))))
	: (((*input).api_request_choice == api_request_request_profile_revert_m_c) ? ((encode_request_profile_revert(state, (&(*input).api_request_request_profile_revert_m))))
	: (((*input).api_request_choice == api_request_request_skill_history_m_c) ? ((encode_request_skill_history(state, (&(*input).api_request_request_skill_history_m))))
	: (((*input).api_request_choice == api_request_request_skill_at_m_c) ? ((encode_request_skill_at(state, (&(*input).api_request_request_skill_at_m))))
	: (((*input).api_request_choice == api_request_request_skill_revert_m_c) ? ((encode_request_skill_revert(state, (&(*input).api_request_request_skill_revert_m))))
	: (((*input).api_request_choice == api_request_request_curator_list_m_c) ? ((encode_request_curator_list(state, (&(*input).api_request_request_curator_list_m))))
	: (((*input).api_request_choice == api_request_request_curator_pin_m_c) ? ((encode_request_curator_pin(state, (&(*input).api_request_request_curator_pin_m))))
	: (((*input).api_request_choice == api_request_request_curator_unpin_m_c) ? ((encode_request_curator_unpin(state, (&(*input).api_request_request_curator_unpin_m))))
	: (((*input).api_request_choice == api_request_request_curator_archive_m_c) ? ((encode_request_curator_archive(state, (&(*input).api_request_request_curator_archive_m))))
	: (((*input).api_request_choice == api_request_request_curator_restore_m_c) ? ((encode_request_curator_restore(state, (&(*input).api_request_request_curator_restore_m))))
	: (((*input).api_request_choice == api_request_request_curator_run_m_c) ? ((encode_request_curator_run(state, (&(*input).api_request_request_curator_run_m))))
	: (((*input).api_request_choice == api_request_request_credential_set_m_c) ? ((encode_request_credential_set(state, (&(*input).api_request_request_credential_set_m))))
	: (((*input).api_request_choice == api_request_request_credential_list_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CredentialList", tmp_str.len = sizeof("CredentialList") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_credential_remove_m_c) ? ((encode_request_credential_remove(state, (&(*input).api_request_request_credential_remove_m))))
	: (((*input).api_request_choice == api_request_request_model_search_m_c) ? ((encode_request_model_search(state, (&(*input).api_request_request_model_search_m))))
	: (((*input).api_request_choice == api_request_request_model_files_m_c) ? ((encode_request_model_files(state, (&(*input).api_request_request_model_files_m))))
	: (((*input).api_request_choice == api_request_request_model_download_m_c) ? ((encode_request_model_download(state, (&(*input).api_request_request_model_download_m))))
	: (((*input).api_request_choice == api_request_request_model_downloads_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelDownloads", tmp_str.len = sizeof("ModelDownloads") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_model_cancel_m_c) ? ((encode_request_model_cancel(state, (&(*input).api_request_request_model_cancel_m))))
	: (((*input).api_request_choice == api_request_request_model_pause_m_c) ? ((encode_request_model_pause(state, (&(*input).api_request_request_model_pause_m))))
	: (((*input).api_request_choice == api_request_request_model_resume_m_c) ? ((encode_request_model_resume(state, (&(*input).api_request_request_model_resume_m))))
	: (((*input).api_request_choice == api_request_request_model_catalog_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelCatalog", tmp_str.len = sizeof("ModelCatalog") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_model_delete_m_c) ? ((encode_request_model_delete(state, (&(*input).api_request_request_model_delete_m))))
	: (((*input).api_request_choice == api_request_request_model_activate_m_c) ? ((encode_request_model_activate(state, (&(*input).api_request_request_model_activate_m))))
	: (((*input).api_request_choice == api_request_request_model_recommend_m_c) ? ((encode_request_model_recommend(state, (&(*input).api_request_request_model_recommend_m))))
	: (((*input).api_request_choice == api_request_request_model_quantize_m_c) ? ((encode_request_model_quantize(state, (&(*input).api_request_request_model_quantize_m))))
	: (((*input).api_request_choice == api_request_request_model_quantizes_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ModelQuantizes", tmp_str.len = sizeof("ModelQuantizes") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_model_inspect_m_c) ? ((encode_request_model_inspect(state, (&(*input).api_request_request_model_inspect_m))))
	: (((*input).api_request_choice == api_request_request_models_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Models", tmp_str.len = sizeof("Models") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_model_current_m_c) ? ((encode_request_model_current(state, (&(*input).api_request_request_model_current_m))))
	: (((*input).api_request_choice == api_request_request_auth_begin_m_c) ? ((encode_request_auth_begin(state, (&(*input).api_request_request_auth_begin_m))))
	: (((*input).api_request_choice == api_request_request_auth_complete_m_c) ? ((encode_request_auth_complete(state, (&(*input).api_request_request_auth_complete_m))))
	: (((*input).api_request_choice == api_request_request_auth_cancel_m_c) ? ((encode_request_auth_cancel(state, (&(*input).api_request_request_auth_cancel_m))))
	: (((*input).api_request_choice == api_request_request_auth_providers_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AuthProviders", tmp_str.len = sizeof("AuthProviders") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_checkpoint_list_m_c) ? ((encode_request_checkpoint_list(state, (&(*input).api_request_request_checkpoint_list_m))))
	: (((*input).api_request_choice == api_request_request_checkpoint_rewind_m_c) ? ((encode_request_checkpoint_rewind(state, (&(*input).api_request_request_checkpoint_rewind_m))))
	: (((*input).api_request_choice == api_request_request_sessions_query_m_c) ? ((encode_request_sessions_query(state, (&(*input).api_request_request_sessions_query_m))))
	: (((*input).api_request_choice == api_request_request_session_get_m_c) ? ((encode_request_session_get(state, (&(*input).api_request_request_session_get_m))))
	: (((*input).api_request_choice == api_request_request_sessions_by_profile_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"SessionsByProfile", tmp_str.len = sizeof("SessionsByProfile") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_session_search_m_c) ? ((encode_request_session_search(state, (&(*input).api_request_request_session_search_m))))
	: (((*input).api_request_choice == api_request_request_session_update_meta_m_c) ? ((encode_request_session_update_meta(state, (&(*input).api_request_request_session_update_meta_m))))
	: (((*input).api_request_choice == api_request_request_rewind_m_c) ? ((encode_request_rewind(state, (&(*input).api_request_request_rewind_m))))
	: (((*input).api_request_choice == api_request_request_acp_discover_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AcpDiscover", tmp_str.len = sizeof("AcpDiscover") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_acp_catalog_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"AcpCatalog", tmp_str.len = sizeof("AcpCatalog") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_acp_register_m_c) ? ((encode_request_acp_register(state, (&(*input).api_request_request_acp_register_m))))
	: (((*input).api_request_choice == api_request_request_acp_remove_m_c) ? ((encode_request_acp_remove(state, (&(*input).api_request_request_acp_remove_m))))
	: (((*input).api_request_choice == api_request_request_skill_get_m_c) ? ((encode_request_skill_get(state, (&(*input).api_request_request_skill_get_m))))
	: (((*input).api_request_choice == api_request_request_skill_put_m_c) ? ((encode_request_skill_put(state, (&(*input).api_request_request_skill_put_m))))
	: (((*input).api_request_choice == api_request_request_provider_list_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProviderList", tmp_str.len = sizeof("ProviderList") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_provider_register_m_c) ? ((encode_request_provider_register(state, (&(*input).api_request_request_provider_register_m))))
	: (((*input).api_request_choice == api_request_request_tool_list_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ToolList", tmp_str.len = sizeof("ToolList") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_tool_register_m_c) ? ((encode_request_tool_register(state, (&(*input).api_request_request_tool_register_m))))
	: (((*input).api_request_choice == api_request_request_command_list_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CommandList", tmp_str.len = sizeof("CommandList") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_command_invoke_m_c) ? ((encode_request_command_invoke(state, (&(*input).api_request_request_command_invoke_m))))
	: (((*input).api_request_choice == api_request_request_config_get_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ConfigGet", tmp_str.len = sizeof("ConfigGet") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_config_set_m_c) ? ((encode_request_config_set(state, (&(*input).api_request_request_config_set_m))))
	: (((*input).api_request_choice == api_request_request_cron_list_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronList", tmp_str.len = sizeof("CronList") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_cron_create_m_c) ? ((encode_request_cron_create(state, (&(*input).api_request_request_cron_create_m))))
	: (((*input).api_request_choice == api_request_request_cron_update_m_c) ? ((encode_request_cron_update(state, (&(*input).api_request_request_cron_update_m))))
	: (((*input).api_request_choice == api_request_request_cron_delete_m_c) ? ((encode_request_cron_delete(state, (&(*input).api_request_request_cron_delete_m))))
	: (((*input).api_request_choice == api_request_request_cron_trigger_m_c) ? ((encode_request_cron_trigger(state, (&(*input).api_request_request_cron_trigger_m))))
	: (((*input).api_request_choice == api_request_request_cron_runs_m_c) ? ((encode_request_cron_runs(state, (&(*input).api_request_request_cron_runs_m))))
	: (((*input).api_request_choice == api_request_request_cron_pause_m_c) ? ((encode_request_cron_pause(state, (&(*input).api_request_request_cron_pause_m))))
	: (((*input).api_request_choice == api_request_request_cron_suggestions_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CronSuggestions", tmp_str.len = sizeof("CronSuggestions") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_cron_accept_suggestion_m_c) ? ((encode_request_cron_accept_suggestion(state, (&(*input).api_request_request_cron_accept_suggestion_m))))
	: (((*input).api_request_choice == api_request_request_cron_dismiss_suggestion_m_c) ? ((encode_request_cron_dismiss_suggestion(state, (&(*input).api_request_request_cron_dismiss_suggestion_m))))
	: (((*input).api_request_choice == api_request_request_routing_list_chats_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"RoutingListChats", tmp_str.len = sizeof("RoutingListChats") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_routing_get_m_c) ? ((encode_request_routing_get(state, (&(*input).api_request_request_routing_get_m))))
	: (((*input).api_request_choice == api_request_request_routing_set_m_c) ? ((encode_request_routing_set(state, (&(*input).api_request_request_routing_set_m))))
	: (((*input).api_request_choice == api_request_request_routing_bind_chat_m_c) ? ((encode_request_routing_bind_chat(state, (&(*input).api_request_request_routing_bind_chat_m))))
	: (((*input).api_request_choice == api_request_request_routing_unbind_chat_m_c) ? ((encode_request_routing_unbind_chat(state, (&(*input).api_request_request_routing_unbind_chat_m))))
	: (((*input).api_request_choice == api_request_request_transport_rooms_m_c) ? ((encode_request_transport_rooms(state, (&(*input).api_request_request_transport_rooms_m))))
	: (((*input).api_request_choice == api_request_request_transport_adapters_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"TransportAdapters", tmp_str.len = sizeof("TransportAdapters") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_transport_instances_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"TransportInstances", tmp_str.len = sizeof("TransportInstances") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_conv_list_m_c) ? ((encode_request_conv_list(state, (&(*input).api_request_request_conv_list_m))))
	: (((*input).api_request_choice == api_request_request_conv_get_m_c) ? ((encode_request_conv_get(state, (&(*input).api_request_request_conv_get_m))))
	: (((*input).api_request_choice == api_request_request_conv_create_details_m_c) ? ((encode_request_conv_create_details(state, (&(*input).api_request_request_conv_create_details_m))))
	: (((*input).api_request_choice == api_request_request_conv_create_m_c) ? ((encode_request_conv_create(state, (&(*input).api_request_request_conv_create_m))))
	: (((*input).api_request_choice == api_request_request_conv_join_details_m_c) ? ((encode_request_conv_join_details(state, (&(*input).api_request_request_conv_join_details_m))))
	: (((*input).api_request_choice == api_request_request_conv_join_m_c) ? ((encode_request_conv_join(state, (&(*input).api_request_request_conv_join_m))))
	: (((*input).api_request_choice == api_request_request_conv_leave_m_c) ? ((encode_request_conv_leave(state, (&(*input).api_request_request_conv_leave_m))))
	: (((*input).api_request_choice == api_request_request_conv_send_m_c) ? ((encode_request_conv_send(state, (&(*input).api_request_request_conv_send_m))))
	: (((*input).api_request_choice == api_request_request_conv_set_topic_m_c) ? ((encode_request_conv_set_topic(state, (&(*input).api_request_request_conv_set_topic_m))))
	: (((*input).api_request_choice == api_request_request_conv_set_title_m_c) ? ((encode_request_conv_set_title(state, (&(*input).api_request_request_conv_set_title_m))))
	: (((*input).api_request_choice == api_request_request_conv_set_description_m_c) ? ((encode_request_conv_set_description(state, (&(*input).api_request_request_conv_set_description_m))))
	: (((*input).api_request_choice == api_request_request_conv_delete_m_c) ? ((encode_request_conv_delete(state, (&(*input).api_request_request_conv_delete_m))))
	: (((*input).api_request_choice == api_request_request_conv_history_m_c) ? ((encode_request_conv_history(state, (&(*input).api_request_request_conv_history_m))))
	: (((*input).api_request_choice == api_request_request_member_invite_m_c) ? ((encode_request_member_invite(state, (&(*input).api_request_request_member_invite_m))))
	: (((*input).api_request_choice == api_request_request_member_remove_m_c) ? ((encode_request_member_remove(state, (&(*input).api_request_request_member_remove_m))))
	: (((*input).api_request_choice == api_request_request_member_ban_m_c) ? ((encode_request_member_ban(state, (&(*input).api_request_request_member_ban_m))))
	: (((*input).api_request_choice == api_request_request_member_set_role_m_c) ? ((encode_request_member_set_role(state, (&(*input).api_request_request_member_set_role_m))))
	: (((*input).api_request_choice == api_request_request_contact_get_profile_m_c) ? ((encode_request_contact_get_profile(state, (&(*input).api_request_request_contact_get_profile_m))))
	: (((*input).api_request_choice == api_request_request_contact_set_alias_m_c) ? ((encode_request_contact_set_alias(state, (&(*input).api_request_request_contact_set_alias_m))))
	: (((*input).api_request_choice == api_request_request_contact_action_menu_m_c) ? ((encode_request_contact_action_menu(state, (&(*input).api_request_request_contact_action_menu_m))))
	: (((*input).api_request_choice == api_request_request_directory_search_m_c) ? ((encode_request_directory_search(state, (&(*input).api_request_request_directory_search_m))))
	: (((*input).api_request_choice == api_request_request_fs_roots_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsRoots", tmp_str.len = sizeof("FsRoots") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_fs_list_m_c) ? ((encode_request_fs_list(state, (&(*input).api_request_request_fs_list_m))))
	: (((*input).api_request_choice == api_request_request_fs_stat_m_c) ? ((encode_request_fs_stat(state, (&(*input).api_request_request_fs_stat_m))))
	: (((*input).api_request_choice == api_request_request_fs_read_m_c) ? ((encode_request_fs_read(state, (&(*input).api_request_request_fs_read_m))))
	: (((*input).api_request_choice == api_request_request_fs_write_m_c) ? ((encode_request_fs_write(state, (&(*input).api_request_request_fs_write_m))))
	: (((*input).api_request_choice == api_request_request_fs_search_m_c) ? ((encode_request_fs_search(state, (&(*input).api_request_request_fs_search_m))))
	: (((*input).api_request_choice == api_request_request_fs_watch_poll_m_c) ? ((encode_request_fs_watch_poll(state, (&(*input).api_request_request_fs_watch_poll_m))))
	: (((*input).api_request_choice == api_request_request_blob_put_m_c) ? ((encode_request_blob_put(state, (&(*input).api_request_request_blob_put_m))))
	: (((*input).api_request_choice == api_request_request_blob_get_m_c) ? ((encode_request_blob_get(state, (&(*input).api_request_request_blob_get_m))))
	: (((*input).api_request_choice == api_request_request_blob_stat_m_c) ? ((encode_request_blob_stat(state, (&(*input).api_request_request_blob_stat_m))))
	: (((*input).api_request_choice == api_request_request_fs_write_from_blob_m_c) ? ((encode_request_fs_write_from_blob(state, (&(*input).api_request_request_fs_write_from_blob_m))))
	: (((*input).api_request_choice == api_request_request_user_create_m_c) ? ((encode_request_user_create(state, (&(*input).api_request_request_user_create_m))))
	: (((*input).api_request_choice == api_request_request_user_list_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"UserList", tmp_str.len = sizeof("UserList") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_user_disable_m_c) ? ((encode_request_user_disable(state, (&(*input).api_request_request_user_disable_m))))
	: (((*input).api_request_choice == api_request_request_user_set_roles_m_c) ? ((encode_request_user_set_roles(state, (&(*input).api_request_request_user_set_roles_m))))
	: (((*input).api_request_choice == api_request_request_user_set_password_m_c) ? ((encode_request_user_set_password(state, (&(*input).api_request_request_user_set_password_m))))
	: (((*input).api_request_choice == api_request_request_role_list_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"RoleList", tmp_str.len = sizeof("RoleList") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_who_am_i_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"WhoAmI", tmp_str.len = sizeof("WhoAmI") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_session_revoke_m_c) ? ((encode_request_session_revoke(state, (&(*input).api_request_request_session_revoke_m))))
	: (((*input).api_request_choice == api_request_request_resource_grant_create_m_c) ? ((encode_request_resource_grant_create(state, (&(*input).api_request_request_resource_grant_create_m))))
	: (((*input).api_request_choice == api_request_request_resource_grant_list_m_c) ? ((encode_request_resource_grant_list(state, (&(*input).api_request_request_resource_grant_list_m))))
	: (((*input).api_request_choice == api_request_request_resource_grant_revoke_m_c) ? ((encode_request_resource_grant_revoke(state, (&(*input).api_request_request_resource_grant_revoke_m))))
	: false)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))));

	log_result(state, res, __func__);
	return res;
}



int cbor_encode_api_request(
		uint8_t *payload, size_t payload_len,
		const struct api_request_r *input,
		size_t *payload_len_out)
{
	zcbor_state_t states[12];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
		(zcbor_decoder_t *)encode_api_request, sizeof(states) / sizeof(zcbor_state_t), 1);
}


int cbor_encode_api_response(
		uint8_t *payload, size_t payload_len,
		const struct api_response_r *input,
		size_t *payload_len_out)
{
	zcbor_state_t states[18];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
		(zcbor_decoder_t *)encode_api_response, sizeof(states) / sizeof(zcbor_state_t), 1);
}
