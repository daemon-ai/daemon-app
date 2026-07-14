/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 64
 */

#ifndef DAEMON_API_CLIENT_TYPES_H__
#define DAEMON_API_CLIENT_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY 64

struct content_hash {
	uint32_t content_hash_uint[64];
	size_t content_hash_uint_count;
};

struct blob_ref_name_r {
	union {
		struct zcbor_string blob_ref_name_tstr;
	};
	enum {
		blob_ref_name_tstr_c,
		blob_ref_name_null_m_c,
	} blob_ref_name_choice;
};

struct blob_ref_mime_r {
	union {
		struct zcbor_string blob_ref_mime_tstr;
	};
	enum {
		blob_ref_mime_tstr_c,
		blob_ref_mime_null_m_c,
	} blob_ref_mime_choice;
};

struct blob_ref {
	struct content_hash blob_ref_hash;
	uint64_t blob_ref_size;
	struct blob_ref_name_r blob_ref_name;
	bool blob_ref_name_present;
	struct blob_ref_mime_r blob_ref_mime;
	bool blob_ref_mime_present;
};

struct user_msg_attachments_r {
	struct blob_ref user_msg_attachments_blob_ref_m[64];
	size_t user_msg_attachments_blob_ref_m_count;
};

struct completion_notice_ref_call_id_r {
	union {
		struct zcbor_string completion_notice_ref_call_id_tstr;
	};
	enum {
		completion_notice_ref_call_id_tstr_c,
		completion_notice_ref_call_id_null_m_c,
	} completion_notice_ref_call_id_choice;
};

struct completion_notice_ref {
	struct zcbor_string completion_notice_ref_child;
	struct completion_notice_ref_call_id_r completion_notice_ref_call_id;
	bool completion_notice_ref_call_id_present;
};

struct user_msg_notice_r {
	union {
		struct completion_notice_ref user_msg_notice_completion_notice_ref_m;
	};
	enum {
		user_msg_notice_completion_notice_ref_m_c,
		user_msg_notice_null_m_c,
	} user_msg_notice_choice;
};

struct user_msg {
	struct zcbor_string user_msg_text;
	struct user_msg_attachments_r user_msg_attachments;
	bool user_msg_attachments_present;
	struct user_msg_notice_r user_msg_notice;
	bool user_msg_notice_present;
};

struct agent_command_start_turn {
	struct user_msg StartTurn_input;
	uint64_t StartTurn_request_id;
};

struct agent_command_steer {
	struct zcbor_string Steer_text;
	uint64_t Steer_request_id;
};

struct agent_command_observe {
	struct user_msg Observe_input;
	uint64_t Observe_request_id;
};

struct agent_command_interrupt {
	union {
		struct zcbor_string Interrupt_reason_tstr;
	};
	enum {
		Interrupt_reason_tstr_c,
		Interrupt_reason_null_m_c,
	} Interrupt_reason_choice;
};

struct agent_command_snapshot {
	uint64_t Snapshot_request_id;
};

struct rewind_anchor_user_turn {
	uint64_t UserTurn_ordinal;
};

struct rewind_anchor_reply_after {
	uint64_t ReplyAfter_ordinal;
};

struct rewind_anchor_cursor {
	uint64_t Cursor_seq;
};

struct rewind_anchor_r {
	union {
		struct rewind_anchor_user_turn rewind_anchor_user_turn_m;
		struct rewind_anchor_reply_after rewind_anchor_reply_after_m;
		struct rewind_anchor_cursor rewind_anchor_cursor_m;
	};
	enum {
		rewind_anchor_user_turn_m_c,
		rewind_anchor_reply_after_m_c,
		rewind_anchor_cursor_m_c,
	} rewind_anchor_choice;
};

struct agent_command_rewind_to {
	struct rewind_anchor_r RewindTo_anchor;
	uint64_t RewindTo_request_id;
};

struct agent_command_r {
	union {
		struct agent_command_start_turn agent_command_start_turn_m;
		struct agent_command_steer agent_command_steer_m;
		struct agent_command_observe agent_command_observe_m;
		struct agent_command_interrupt agent_command_interrupt_m;
		struct agent_command_snapshot agent_command_snapshot_m;
		struct agent_command_rewind_to agent_command_rewind_to_m;
	};
	enum {
		agent_command_start_turn_m_c,
		agent_command_steer_m_c,
		agent_command_observe_m_c,
		agent_command_interrupt_m_c,
		agent_command_snapshot_m_c,
		agent_command_rewind_to_m_c,
		agent_command_Shutdown_tstr_c,
	} agent_command_choice;
};

struct origin_scope_dm {
	struct zcbor_string Dm_user;
};

struct origin_scope_group {
	struct zcbor_string Group_chat;
	union {
		struct zcbor_string Group_thread_tstr;
	};
	enum {
		Group_thread_tstr_c,
		Group_thread_null_m_c,
	} Group_thread_choice;
};

struct origin_scope_api {
	struct zcbor_string Api_key;
};

struct origin_scope_t_r {
	union {
		struct origin_scope_dm origin_scope_t_origin_scope_dm_m;
		struct origin_scope_group origin_scope_t_origin_scope_group_m;
		struct origin_scope_api origin_scope_t_origin_scope_api_m;
	};
	enum {
		origin_scope_t_origin_scope_dm_m_c,
		origin_scope_t_origin_scope_group_m_c,
		origin_scope_t_origin_scope_api_m_c,
		origin_scope_t_Internal_tstr_c,
	} origin_scope_t_choice;
};

struct origin_sender_r {
	union {
		struct zcbor_string origin_sender_sender_id_m;
	};
	enum {
		origin_sender_sender_id_m_c,
		origin_sender_null_m_c,
	} origin_sender_choice;
};

struct origin {
	struct zcbor_string origin_transport;
	struct origin_scope_t_r origin_scope;
	struct origin_sender_r origin_sender;
	bool origin_sender_present;
};

struct Submit_origin_r {
	union {
		struct origin Submit_origin_origin_m;
	};
	enum {
		Submit_origin_origin_m_c,
		Submit_origin_null_m_c,
	} Submit_origin_choice;
};

struct Submit_profile_r {
	union {
		struct zcbor_string Submit_profile_profile_ref_m;
	};
	enum {
		Submit_profile_profile_ref_m_c,
		Submit_profile_null_m_c,
	} Submit_profile_choice;
};

struct request_submit {
	struct zcbor_string Submit_session;
	struct agent_command_r Submit_command;
	struct Submit_origin_r Submit_origin;
	bool Submit_origin_present;
	struct Submit_profile_r Submit_profile;
	bool Submit_profile_present;
};

struct request_submit_routed {
	struct origin SubmitRouted_origin;
	struct agent_command_r SubmitRouted_command;
};

struct request_poll {
	struct zcbor_string Poll_session;
	uint32_t Poll_max;
};

struct Approved_allow_permanent {
	bool Approved_allow_permanent;
};

struct Approved_reason_r {
	union {
		struct zcbor_string Approved_reason_tstr;
	};
	enum {
		Approved_reason_tstr_c,
		Approved_reason_null_m_c,
	} Approved_reason_choice;
};

struct host_response_body_approved {
	bool Approved_approved;
	struct Approved_allow_permanent Approved_allow_permanent;
	bool Approved_allow_permanent_present;
	struct Approved_reason_r Approved_reason;
	bool Approved_reason_present;
};

struct host_response_body_input {
	struct zcbor_string host_response_body_input_Input;
};

struct host_response_body_chosen {
	uint32_t host_response_body_chosen_Chosen;
};

struct host_response_body_delegated {
	struct zcbor_string host_response_body_delegated_Delegated;
};

struct host_response_body_spawned {
	struct zcbor_string host_response_body_spawned_Spawned;
};

struct host_response_body_deferred {
	struct zcbor_string host_response_body_deferred_Deferred;
};

struct host_response_body_t_r {
	union {
		struct host_response_body_approved host_response_body_t_host_response_body_approved_m;
		struct host_response_body_input host_response_body_t_host_response_body_input_m;
		struct host_response_body_chosen host_response_body_t_host_response_body_chosen_m;
		struct host_response_body_delegated host_response_body_t_host_response_body_delegated_m;
		struct host_response_body_spawned host_response_body_t_host_response_body_spawned_m;
		struct host_response_body_deferred host_response_body_t_host_response_body_deferred_m;
	};
	enum {
		host_response_body_t_host_response_body_approved_m_c,
		host_response_body_t_host_response_body_input_m_c,
		host_response_body_t_host_response_body_chosen_m_c,
		host_response_body_t_host_response_body_delegated_m_c,
		host_response_body_t_host_response_body_spawned_m_c,
		host_response_body_t_host_response_body_deferred_m_c,
	} host_response_body_t_choice;
};

struct host_response {
	uint64_t host_response_request_id;
	struct host_response_body_t_r host_response_body;
};

struct request_respond {
	struct zcbor_string Respond_session;
	struct host_response Respond_response;
};

struct request_assign {
	struct zcbor_string Assign_session;
};

struct SessionCreate_session_r {
	union {
		struct zcbor_string SessionCreate_session_session_id_m;
	};
	enum {
		SessionCreate_session_session_id_m_c,
		SessionCreate_session_null_m_c,
	} SessionCreate_session_choice;
};

struct SessionCreate_profile_r {
	union {
		struct zcbor_string SessionCreate_profile_profile_ref_m;
	};
	enum {
		SessionCreate_profile_profile_ref_m_c,
		SessionCreate_profile_null_m_c,
	} SessionCreate_profile_choice;
};

struct request_session_create {
	struct SessionCreate_session_r SessionCreate_session;
	bool SessionCreate_session_present;
	struct SessionCreate_profile_r SessionCreate_profile;
	bool SessionCreate_profile_present;
};

struct request_cancel {
	struct zcbor_string Cancel_session;
};

struct Tree_after_r {
	union {
		struct zcbor_string Tree_after_tstr;
	};
	enum {
		Tree_after_tstr_c,
		Tree_after_null_m_c,
	} Tree_after_choice;
};

struct request_tree {
	struct Tree_after_r Tree_after;
	bool Tree_after_present;
};

struct request_unit {
	struct zcbor_string Unit_unit;
};

struct request_unit_events {
	struct zcbor_string UnitEvents_unit;
	uint32_t UnitEvents_max;
};

struct request_unit_outbound {
	struct zcbor_string UnitOutbound_unit;
	uint32_t UnitOutbound_max;
};

struct SessionHistory_after_cursor {
	uint64_t SessionHistory_after_cursor;
};

struct SessionHistory_before_cursor {
	uint64_t SessionHistory_before_cursor;
};

struct request_session_history {
	struct zcbor_string SessionHistory_session;
	struct SessionHistory_after_cursor SessionHistory_after_cursor;
	bool SessionHistory_after_cursor_present;
	struct SessionHistory_before_cursor SessionHistory_before_cursor;
	bool SessionHistory_before_cursor_present;
	uint32_t SessionHistory_max;
};

struct request_subscribe {
	struct zcbor_string Subscribe_session;
	uint64_t Subscribe_after_seq;
	uint32_t Subscribe_max;
};

struct EventsSince_wait_ms_r {
	union {
		uint32_t EventsSince_wait_ms_uint;
	};
	enum {
		EventsSince_wait_ms_uint_c,
		EventsSince_wait_ms_null_m_c,
	} EventsSince_wait_ms_choice;
};

struct request_events_since {
	uint64_t EventsSince_cursor;
	struct EventsSince_wait_ms_r EventsSince_wait_ms;
	bool EventsSince_wait_ms_present;
};

struct request_delivery_targets {
	struct zcbor_string DeliveryTargets_session;
};

struct DeliverySessions_after_r {
	union {
		struct zcbor_string DeliverySessions_after_tstr;
	};
	enum {
		DeliverySessions_after_tstr_c,
		DeliverySessions_after_null_m_c,
	} DeliverySessions_after_choice;
};

struct request_delivery_sessions {
	struct zcbor_string DeliverySessions_transport;
	struct DeliverySessions_after_r DeliverySessions_after;
	bool DeliverySessions_after_present;
};

struct sink_kind_r {
	enum {
		sink_kind_Primary_tstr_c,
		sink_kind_Spectator_tstr_c,
	} sink_kind_choice;
};

struct delivery_target {
	struct zcbor_string delivery_target_transport;
	struct zcbor_string delivery_target_route;
	struct sink_kind_r delivery_target_kind;
};

struct request_handover {
	struct zcbor_string Handover_session;
	struct delivery_target Handover_target;
};

struct record_meta_args {
	struct zcbor_string record_meta_args_session;
	struct origin record_meta_args_origin;
	struct zcbor_string record_meta_args_kind;
	struct zcbor_string record_meta_args_body;
};

struct request_record_meta {
	struct record_meta_args request_record_meta_RecordMeta;
};

struct UnitHistory_after_cursor {
	uint64_t UnitHistory_after_cursor;
};

struct UnitHistory_before_cursor {
	uint64_t UnitHistory_before_cursor;
};

struct request_unit_history {
	struct zcbor_string UnitHistory_unit;
	struct UnitHistory_after_cursor UnitHistory_after_cursor;
	bool UnitHistory_after_cursor_present;
	struct UnitHistory_before_cursor UnitHistory_before_cursor;
	bool UnitHistory_before_cursor_present;
	uint32_t UnitHistory_max;
};

struct request_pause {
	struct zcbor_string Pause_unit;
};

struct request_resume {
	struct zcbor_string Resume_unit;
};

struct request_scale {
	struct zcbor_string Scale_unit;
	uint32_t Scale_n;
};

struct provider_selector_r {
	enum {
		provider_selector_mock_tstr_c,
		provider_selector_genai_tstr_c,
		provider_selector_daemon_api_tstr_c,
		provider_selector_llama_cpp_tstr_c,
		provider_selector_mistral_rs_tstr_c,
	} provider_selector_choice;
};

struct SetSessionModel_provider_r {
	union {
		struct provider_selector_r SetSessionModel_provider_provider_selector_m;
	};
	enum {
		SetSessionModel_provider_provider_selector_m_c,
		SetSessionModel_provider_null_m_c,
	} SetSessionModel_provider_choice;
};

struct request_set_session_model {
	struct zcbor_string SetSessionModel_session;
	struct zcbor_string SetSessionModel_model;
	struct SetSessionModel_provider_r SetSessionModel_provider;
	bool SetSessionModel_provider_present;
};

struct approval_mode_r {
	enum {
		approval_mode_ask_tstr_c,
		approval_mode_accept_edits_tstr_c,
		approval_mode_auto_allow_tstr_c,
		approval_mode_deny_tstr_c,
	} approval_mode_choice;
};

struct request_set_session_mode {
	struct zcbor_string SetSessionMode_session;
	struct approval_mode_r SetSessionMode_mode;
};

struct session_overlay_model_r {
	union {
		struct zcbor_string session_overlay_model_tstr;
	};
	enum {
		session_overlay_model_tstr_c,
		session_overlay_model_null_m_c,
	} session_overlay_model_choice;
};

struct session_overlay_provider_r {
	union {
		struct provider_selector_r session_overlay_provider_provider_selector_m;
	};
	enum {
		session_overlay_provider_provider_selector_m_c,
		session_overlay_provider_null_m_c,
	} session_overlay_provider_choice;
};

struct tools_override_allowlist {
	struct zcbor_string tools_override_allowlist_allowlist_tstr[64];
	size_t tools_override_allowlist_allowlist_tstr_count;
};

struct tools_override_r {
	union {
		struct tools_override_allowlist tools_override_allowlist_m;
	};
	enum {
		tools_override_inherit_tstr_c,
		tools_override_full_toolset_tstr_c,
		tools_override_allowlist_m_c,
	} tools_override_choice;
};

struct session_overlay_tool_allowlist {
	struct tools_override_r session_overlay_tool_allowlist;
};

struct session_overlay_approval_mode_r {
	union {
		struct approval_mode_r session_overlay_approval_mode_approval_mode_m;
	};
	enum {
		session_overlay_approval_mode_approval_mode_m_c,
		session_overlay_approval_mode_null_m_c,
	} session_overlay_approval_mode_choice;
};

struct workspace_binding_bound {
	struct zcbor_string workspace_binding_bound_bound;
};

struct workspace_binding_r {
	union {
		struct workspace_binding_bound workspace_binding_bound_m;
	};
	enum {
		workspace_binding_isolated_tstr_c,
		workspace_binding_bound_m_c,
	} workspace_binding_choice;
};

struct session_overlay_workspace_r {
	union {
		struct workspace_binding_r session_overlay_workspace_workspace_binding_m;
	};
	enum {
		session_overlay_workspace_workspace_binding_m_c,
		session_overlay_workspace_null_m_c,
	} session_overlay_workspace_choice;
};

struct session_overlay {
	struct session_overlay_model_r session_overlay_model;
	bool session_overlay_model_present;
	struct session_overlay_provider_r session_overlay_provider;
	bool session_overlay_provider_present;
	struct session_overlay_tool_allowlist session_overlay_tool_allowlist;
	bool session_overlay_tool_allowlist_present;
	struct session_overlay_approval_mode_r session_overlay_approval_mode;
	bool session_overlay_approval_mode_present;
	struct session_overlay_workspace_r session_overlay_workspace;
	bool session_overlay_workspace_present;
};

struct request_set_session_overlay {
	struct zcbor_string SetSessionOverlay_session;
	struct session_overlay SetSessionOverlay_overlay;
};

struct ApprovalsPending_session_r {
	union {
		struct zcbor_string ApprovalsPending_session_session_id_m;
	};
	enum {
		ApprovalsPending_session_session_id_m_c,
		ApprovalsPending_session_null_m_c,
	} ApprovalsPending_session_choice;
};

struct ApprovalsPending_after_r {
	union {
		struct zcbor_string ApprovalsPending_after_tstr;
	};
	enum {
		ApprovalsPending_after_tstr_c,
		ApprovalsPending_after_null_m_c,
	} ApprovalsPending_after_choice;
};

struct request_approvals_pending {
	struct ApprovalsPending_session_r ApprovalsPending_session;
	bool ApprovalsPending_session_present;
	struct ApprovalsPending_after_r ApprovalsPending_after;
	bool ApprovalsPending_after_present;
};

struct ApprovalDecide_allow_permanent {
	bool ApprovalDecide_allow_permanent;
};

struct ApprovalDecide_reason_r {
	union {
		struct zcbor_string ApprovalDecide_reason_tstr;
	};
	enum {
		ApprovalDecide_reason_tstr_c,
		ApprovalDecide_reason_null_m_c,
	} ApprovalDecide_reason_choice;
};

struct request_approval_decide {
	struct zcbor_string ApprovalDecide_session;
	struct zcbor_string ApprovalDecide_request_id;
	bool ApprovalDecide_allow;
	struct ApprovalDecide_allow_permanent ApprovalDecide_allow_permanent;
	bool ApprovalDecide_allow_permanent_present;
	struct ApprovalDecide_reason_r ApprovalDecide_reason;
	bool ApprovalDecide_reason_present;
};

struct request_fingerprint_list {
	struct zcbor_string FingerprintList_session;
};

struct request_fingerprint_revoke {
	struct zcbor_string FingerprintRevoke_session;
	struct zcbor_string FingerprintRevoke_fingerprint;
};

struct request_profile_get {
	struct zcbor_string ProfileGet_id;
};

struct budget {
	union {
		uint64_t budget_tokens_uint64_m;
	};
	enum {
		budget_tokens_uint64_m_c,
		budget_tokens_null_m_c,
	} budget_tokens_choice;
	union {
		uint64_t budget_wall_ms_uint64_m;
	};
	enum {
		budget_wall_ms_uint64_m_c,
		budget_wall_ms_null_m_c,
	} budget_wall_ms_choice;
};

struct engine_tunables {
	union {
		uint32_t engine_tunables_model_retry_attempts_uint;
	};
	enum {
		engine_tunables_model_retry_attempts_uint_c,
		engine_tunables_model_retry_attempts_null_m_c,
	} engine_tunables_model_retry_attempts_choice;
	union {
		uint32_t engine_tunables_context_budget_tokens_uint;
	};
	enum {
		engine_tunables_context_budget_tokens_uint_c,
		engine_tunables_context_budget_tokens_null_m_c,
	} engine_tunables_context_budget_tokens_choice;
	union {
		uint32_t engine_tunables_max_iterations_uint;
	};
	enum {
		engine_tunables_max_iterations_uint_c,
		engine_tunables_max_iterations_null_m_c,
	} engine_tunables_max_iterations_choice;
	union {
		uint32_t engine_tunables_tool_result_budget_uint;
	};
	enum {
		engine_tunables_tool_result_budget_uint_c,
		engine_tunables_tool_result_budget_null_m_c,
	} engine_tunables_tool_result_budget_choice;
};

struct context_engine_sel_r {
	enum {
		context_engine_sel_lcm_tstr_c,
		context_engine_sel_budgeted_tstr_c,
	} context_engine_sel_choice;
};

struct memory_provider_sel_r {
	enum {
		memory_provider_sel_mnemosyne_tstr_c,
		memory_provider_sel_file_tstr_c,
		memory_provider_sel_none_tstr_c,
	} memory_provider_sel_choice;
};

struct bound_account {
	struct zcbor_string bound_account_transport_instance;
	struct zcbor_string bound_account_credential_ref;
};

struct engine_foreign_agent {
	struct zcbor_string engine_foreign_agent_agent;
};

struct engine_foreign {
	struct engine_foreign_agent engine_foreign_Foreign;
};

struct engine_selector_r {
	union {
		struct engine_foreign engine_selector_engine_foreign_m;
	};
	enum {
		engine_selector_Core_tstr_c,
		engine_selector_engine_foreign_m_c,
	} engine_selector_choice;
};

struct profile_spec_engine {
	struct engine_selector_r profile_spec_engine;
};

struct foreign_backend_agent_native {
	union {
		struct zcbor_string AgentNative_model_tstr;
	};
	enum {
		AgentNative_model_tstr_c,
		AgentNative_model_null_m_c,
	} AgentNative_model_choice;
};

struct foreign_backend_node_provider {
	struct provider_selector_r NodeProvider_provider;
	struct zcbor_string NodeProvider_model;
	union {
		struct zcbor_string NodeProvider_credential_ref_tstr;
	};
	enum {
		NodeProvider_credential_ref_tstr_c,
		NodeProvider_credential_ref_null_m_c,
	} NodeProvider_credential_ref_choice;
};

struct foreign_backend_r {
	union {
		struct foreign_backend_agent_native foreign_backend_agent_native_m;
		struct foreign_backend_node_provider foreign_backend_node_provider_m;
	};
	enum {
		foreign_backend_agent_native_m_c,
		foreign_backend_node_provider_m_c,
	} foreign_backend_choice;
};

struct profile_spec_foreign_backend {
	struct foreign_backend_r profile_spec_foreign_backend;
};

struct author_agent {
	struct zcbor_string author_agent_agent;
};

struct author_r {
	union {
		struct author_agent author_agent_m;
	};
	enum {
		author_operator_tstr_c,
		author_agent_m_c,
	} author_choice;
};

struct profile_spec_created_by_r {
	union {
		struct author_r profile_spec_created_by_author_m;
	};
	enum {
		profile_spec_created_by_author_m_c,
		profile_spec_created_by_null_m_c,
	} profile_spec_created_by_choice;
};

struct profile_spec_owner_r {
	union {
		struct zcbor_string profile_spec_owner_tstr;
	};
	enum {
		profile_spec_owner_tstr_c,
		profile_spec_owner_null_m_c,
	} profile_spec_owner_choice;
};

struct profile_spec {
	struct zcbor_string profile_spec_id;
	struct provider_selector_r profile_spec_provider;
	struct zcbor_string profile_spec_model;
	union {
		struct zcbor_string profile_spec_base_url_tstr;
	};
	enum {
		profile_spec_base_url_tstr_c,
		profile_spec_base_url_null_m_c,
	} profile_spec_base_url_choice;
	union {
		struct {
			struct zcbor_string tool_allowlist_tstr_l_tstr[64];
			size_t tool_allowlist_tstr_l_tstr_count;
		};
	};
	enum {
		tool_allowlist_tstr_l_c,
		profile_spec_tool_allowlist_null_m_c,
	} profile_spec_tool_allowlist_choice;
	struct budget profile_spec_budget;
	struct engine_tunables profile_spec_tunables;
	struct context_engine_sel_r profile_spec_context_engine;
	struct memory_provider_sel_r profile_spec_memory_provider;
	union {
		struct zcbor_string profile_spec_credential_ref_tstr;
	};
	enum {
		profile_spec_credential_ref_tstr_c,
		profile_spec_credential_ref_null_m_c,
	} profile_spec_credential_ref_choice;
	union {
		struct zcbor_string profile_spec_fallback_credential_ref_tstr;
	};
	enum {
		profile_spec_fallback_credential_ref_tstr_c,
		profile_spec_fallback_credential_ref_null_m_c,
	} profile_spec_fallback_credential_ref_choice;
	struct bound_account profile_spec_bound_accounts_bound_account_m[64];
	size_t profile_spec_bound_accounts_bound_account_m_count;
	struct profile_spec_engine profile_spec_engine;
	bool profile_spec_engine_present;
	struct profile_spec_foreign_backend profile_spec_foreign_backend;
	bool profile_spec_foreign_backend_present;
	struct profile_spec_created_by_r profile_spec_created_by;
	bool profile_spec_created_by_present;
	struct profile_spec_owner_r profile_spec_owner;
	bool profile_spec_owner_present;
};

struct request_profile_create {
	struct profile_spec ProfileCreate_spec;
};

struct request_profile_update {
	struct profile_spec ProfileUpdate_spec;
};

struct request_profile_delete {
	struct zcbor_string ProfileDelete_id;
};

struct request_profile_select {
	struct zcbor_string ProfileSelect_id;
};

struct request_profile_clone {
	struct zcbor_string ProfileClone_source;
	struct zcbor_string ProfileClone_new_id;
};

struct request_profile_export {
	struct zcbor_string ProfileExport_id;
};

struct files_tstrtstr {
	struct zcbor_string skill_bundle_files_tstrtstr_key;
	struct zcbor_string files_tstrtstr;
};

struct skill_bundle_signature_r {
	union {
		struct zcbor_string skill_bundle_signature_tstr;
	};
	enum {
		skill_bundle_signature_tstr_c,
		skill_bundle_signature_null_m_c,
	} skill_bundle_signature_choice;
};

struct skill_bundle {
	struct zcbor_string skill_bundle_name;
	union {
		struct zcbor_string skill_bundle_category_tstr;
	};
	enum {
		skill_bundle_category_tstr_c,
		skill_bundle_category_null_m_c,
	} skill_bundle_category_choice;
	struct files_tstrtstr files_tstrtstr[64];
	size_t files_tstrtstr_count;
	struct skill_bundle_signature_r skill_bundle_signature;
	bool skill_bundle_signature_present;
};

struct distribution_head_seq_r {
	union {
		uint64_t distribution_head_seq_uint64_m;
	};
	enum {
		distribution_head_seq_uint64_m_c,
		distribution_head_seq_null_m_c,
	} distribution_head_seq_choice;
};

struct distribution_source_r {
	union {
		struct zcbor_string distribution_source_tstr;
	};
	enum {
		distribution_source_tstr_c,
		distribution_source_null_m_c,
	} distribution_source_choice;
};

struct distribution_soul_r {
	union {
		struct zcbor_string distribution_soul_tstr;
	};
	enum {
		distribution_soul_tstr_c,
		distribution_soul_null_m_c,
	} distribution_soul_choice;
};

struct distribution {
	uint32_t distribution_wire_version;
	struct profile_spec distribution_profile;
	struct skill_bundle distribution_skills_skill_bundle_m[64];
	size_t distribution_skills_skill_bundle_m_count;
	struct distribution_head_seq_r distribution_head_seq;
	bool distribution_head_seq_present;
	struct distribution_source_r distribution_source;
	bool distribution_source_present;
	struct distribution_soul_r distribution_soul;
	bool distribution_soul_present;
};

struct ProfileImport_new_id_r {
	union {
		struct zcbor_string ProfileImport_new_id_tstr;
	};
	enum {
		ProfileImport_new_id_tstr_c,
		ProfileImport_new_id_null_m_c,
	} ProfileImport_new_id_choice;
};

struct request_profile_import {
	struct distribution ProfileImport_dist;
	struct ProfileImport_new_id_r ProfileImport_new_id;
	bool ProfileImport_new_id_present;
};

struct ProfileHistory_after_r {
	union {
		struct zcbor_string ProfileHistory_after_tstr;
	};
	enum {
		ProfileHistory_after_tstr_c,
		ProfileHistory_after_null_m_c,
	} ProfileHistory_after_choice;
};

struct request_profile_history {
	struct zcbor_string ProfileHistory_id;
	struct ProfileHistory_after_r ProfileHistory_after;
	bool ProfileHistory_after_present;
};

struct request_profile_at {
	struct zcbor_string ProfileAt_id;
	uint64_t ProfileAt_seq;
};

struct request_profile_revert {
	struct zcbor_string ProfileRevert_id;
	uint64_t ProfileRevert_seq;
};

struct request_soul_get {
	struct zcbor_string SoulGet_id;
};

struct request_soul_set {
	struct zcbor_string SoulSet_id;
	struct zcbor_string SoulSet_text;
};

struct SkillHistory_after_r {
	union {
		struct zcbor_string SkillHistory_after_tstr;
	};
	enum {
		SkillHistory_after_tstr_c,
		SkillHistory_after_null_m_c,
	} SkillHistory_after_choice;
};

struct request_skill_history {
	struct zcbor_string SkillHistory_name;
	struct SkillHistory_after_r SkillHistory_after;
	bool SkillHistory_after_present;
};

struct request_skill_at {
	struct zcbor_string SkillAt_name;
	uint64_t SkillAt_seq;
};

struct request_skill_revert {
	struct zcbor_string SkillRevert_name;
	uint64_t SkillRevert_seq;
};

struct CuratorList_profile_r {
	union {
		struct zcbor_string CuratorList_profile_tstr;
	};
	enum {
		CuratorList_profile_tstr_c,
		CuratorList_profile_null_m_c,
	} CuratorList_profile_choice;
};

struct request_curator_list {
	struct CuratorList_profile_r CuratorList_profile;
	bool CuratorList_profile_present;
};

struct CuratorPin_profile_r {
	union {
		struct zcbor_string CuratorPin_profile_tstr;
	};
	enum {
		CuratorPin_profile_tstr_c,
		CuratorPin_profile_null_m_c,
	} CuratorPin_profile_choice;
};

struct request_curator_pin {
	struct CuratorPin_profile_r CuratorPin_profile;
	bool CuratorPin_profile_present;
	struct zcbor_string CuratorPin_name;
};

struct CuratorUnpin_profile_r {
	union {
		struct zcbor_string CuratorUnpin_profile_tstr;
	};
	enum {
		CuratorUnpin_profile_tstr_c,
		CuratorUnpin_profile_null_m_c,
	} CuratorUnpin_profile_choice;
};

struct request_curator_unpin {
	struct CuratorUnpin_profile_r CuratorUnpin_profile;
	bool CuratorUnpin_profile_present;
	struct zcbor_string CuratorUnpin_name;
};

struct CuratorArchive_profile_r {
	union {
		struct zcbor_string CuratorArchive_profile_tstr;
	};
	enum {
		CuratorArchive_profile_tstr_c,
		CuratorArchive_profile_null_m_c,
	} CuratorArchive_profile_choice;
};

struct request_curator_archive {
	struct CuratorArchive_profile_r CuratorArchive_profile;
	bool CuratorArchive_profile_present;
	struct zcbor_string CuratorArchive_name;
};

struct CuratorRestore_profile_r {
	union {
		struct zcbor_string CuratorRestore_profile_tstr;
	};
	enum {
		CuratorRestore_profile_tstr_c,
		CuratorRestore_profile_null_m_c,
	} CuratorRestore_profile_choice;
};

struct request_curator_restore {
	struct CuratorRestore_profile_r CuratorRestore_profile;
	bool CuratorRestore_profile_present;
	struct zcbor_string CuratorRestore_name;
};

struct CuratorRun_profile_r {
	union {
		struct zcbor_string CuratorRun_profile_tstr;
	};
	enum {
		CuratorRun_profile_tstr_c,
		CuratorRun_profile_null_m_c,
	} CuratorRun_profile_choice;
};

struct request_curator_run {
	struct CuratorRun_profile_r CuratorRun_profile;
	bool CuratorRun_profile_present;
};

struct request_credential_set {
	struct zcbor_string CredentialSet_profile;
	struct zcbor_string CredentialSet_secret;
};

struct request_credential_remove {
	struct zcbor_string CredentialRemove_profile;
};

struct CredentialSetLabel_label_r {
	union {
		struct zcbor_string CredentialSetLabel_label_tstr;
	};
	enum {
		CredentialSetLabel_label_tstr_c,
		CredentialSetLabel_label_null_m_c,
	} CredentialSetLabel_label_choice;
};

struct request_credential_set_label {
	struct zcbor_string CredentialSetLabel_profile;
	struct CredentialSetLabel_label_r CredentialSetLabel_label;
	bool CredentialSetLabel_label_present;
};

struct model_engine_r {
	enum {
		model_engine_Llama_tstr_c,
		model_engine_MistralRs_tstr_c,
	} model_engine_choice;
};

struct search_sort_r {
	enum {
		search_sort_Trending_tstr_c,
		search_sort_Downloads_tstr_c,
		search_sort_Likes_tstr_c,
		search_sort_Modified_tstr_c,
		search_sort_Created_tstr_c,
	} search_sort_choice;
};

struct search_query {
	struct zcbor_string search_query_text;
	struct model_engine_r search_query_engine;
	struct search_sort_r search_query_sort;
	uint32_t search_query_page;
	uint32_t search_query_limit;
};

struct request_model_search {
	struct search_query ModelSearch_query;
};

struct ModelFiles_after_r {
	union {
		struct zcbor_string ModelFiles_after_tstr;
	};
	enum {
		ModelFiles_after_tstr_c,
		ModelFiles_after_null_m_c,
	} ModelFiles_after_choice;
};

struct request_model_files {
	struct zcbor_string ModelFiles_repo;
	union {
		struct zcbor_string ModelFiles_revision_tstr;
	};
	enum {
		ModelFiles_revision_tstr_c,
		ModelFiles_revision_null_m_c,
	} ModelFiles_revision_choice;
	struct model_engine_r ModelFiles_engine;
	struct ModelFiles_after_r ModelFiles_after;
	bool ModelFiles_after_present;
};

struct model_source_hf {
	struct zcbor_string Hf_repo;
	union {
		struct zcbor_string Hf_file_tstr;
	};
	enum {
		Hf_file_tstr_c,
		Hf_file_null_m_c,
	} Hf_file_choice;
	struct zcbor_string Hf_revision;
};

struct model_source_local {
	struct zcbor_string Local_path;
};

struct model_source_r {
	union {
		struct model_source_hf model_source_hf_m;
		struct model_source_local model_source_local_m;
	};
	enum {
		model_source_hf_m_c,
		model_source_local_m_c,
	} model_source_choice;
};

struct model_ref {
	struct model_engine_r model_ref_engine;
	struct model_source_r model_ref_source;
};

struct request_model_download {
	struct model_ref ModelDownload_model;
};

struct request_model_cancel {
	uint64_t ModelCancel_id;
};

struct request_model_pause {
	uint64_t ModelPause_id;
};

struct request_model_resume {
	uint64_t ModelResume_id;
};

struct request_model_delete {
	struct zcbor_string ModelDelete_id;
};

struct request_model_activate {
	struct zcbor_string ModelActivate_id;
	union {
		struct zcbor_string ModelActivate_profile_tstr;
	};
	enum {
		ModelActivate_profile_tstr_c,
		ModelActivate_profile_null_m_c,
	} ModelActivate_profile_choice;
};

struct model_recommend_args {
	struct zcbor_string model_recommend_args_repo;
	union {
		struct zcbor_string model_recommend_args_revision_tstr;
	};
	enum {
		model_recommend_args_revision_tstr_c,
		model_recommend_args_revision_null_m_c,
	} model_recommend_args_revision_choice;
	struct model_engine_r model_recommend_args_engine;
	union {
		uint64_t model_recommend_args_budget_bytes_uint64_m;
	};
	enum {
		model_recommend_args_budget_bytes_uint64_m_c,
		model_recommend_args_budget_bytes_null_m_c,
	} model_recommend_args_budget_bytes_choice;
};

struct request_model_recommend {
	struct model_recommend_args request_model_recommend_ModelRecommend;
};

struct model_quantize_args {
	struct zcbor_string model_quantize_args_repo;
	union {
		struct zcbor_string model_quantize_args_revision_tstr;
	};
	enum {
		model_quantize_args_revision_tstr_c,
		model_quantize_args_revision_null_m_c,
	} model_quantize_args_revision_choice;
	struct zcbor_string model_quantize_args_target_quant;
	union {
		struct zcbor_string model_quantize_args_source_file_tstr;
	};
	enum {
		model_quantize_args_source_file_tstr_c,
		model_quantize_args_source_file_null_m_c,
	} model_quantize_args_source_file_choice;
};

struct request_model_quantize {
	struct model_quantize_args request_model_quantize_ModelQuantize;
};

struct request_model_inspect {
	struct zcbor_string ModelInspect_id;
};

struct request_swarm_run_detail {
	struct zcbor_string SwarmRunDetail_run_id;
};

struct swarm_policy_mode_t_r {
	enum {
		swarm_policy_mode_t_always_tstr_c,
		swarm_policy_mode_t_idle_tstr_c,
		swarm_policy_mode_t_scheduled_tstr_c,
		swarm_policy_mode_t_manual_tstr_c,
	} swarm_policy_mode_t_choice;
};

struct swarm_policy_schedule {
	struct zcbor_string swarm_policy_schedule;
};

struct swarm_policy {
	struct swarm_policy_mode_t_r swarm_policy_mode;
	uint32_t swarm_policy_vram_cap_mb;
	uint32_t swarm_policy_duty_cycle_pct;
	struct swarm_policy_schedule swarm_policy_schedule;
	bool swarm_policy_schedule_present;
};

struct request_swarm_join {
	struct zcbor_string SwarmJoin_run_id;
	struct swarm_policy SwarmJoin_policy;
	struct zcbor_string SwarmJoin_op_id;
};

struct swarm_leave_mode_r {
	enum {
		swarm_leave_mode_graceful_tstr_c,
		swarm_leave_mode_immediate_tstr_c,
	} swarm_leave_mode_choice;
};

struct request_swarm_leave {
	struct zcbor_string SwarmLeave_run_id;
	struct swarm_leave_mode_r SwarmLeave_mode;
	struct zcbor_string SwarmLeave_op_id;
};

struct request_swarm_set_policy {
	struct swarm_policy SwarmSetPolicy_policy;
};

struct Models_after_r {
	union {
		struct zcbor_string Models_after_tstr;
	};
	enum {
		Models_after_tstr_c,
		Models_after_null_m_c,
	} Models_after_choice;
};

struct request_models {
	struct Models_after_r Models_after;
	bool Models_after_present;
};

struct request_model_current {
	union {
		struct zcbor_string ModelCurrent_profile_tstr;
	};
	enum {
		ModelCurrent_profile_tstr_c,
		ModelCurrent_profile_null_m_c,
	} ModelCurrent_profile_choice;
};

struct ProviderModels_credential_ref_r {
	union {
		struct zcbor_string ProviderModels_credential_ref_tstr;
	};
	enum {
		ProviderModels_credential_ref_tstr_c,
		ProviderModels_credential_ref_null_m_c,
	} ProviderModels_credential_ref_choice;
};

struct ProviderModels_transient_key_r {
	union {
		struct zcbor_string ProviderModels_transient_key_tstr;
	};
	enum {
		ProviderModels_transient_key_tstr_c,
		ProviderModels_transient_key_null_m_c,
	} ProviderModels_transient_key_choice;
};

struct ProviderModels_after_r {
	union {
		struct zcbor_string ProviderModels_after_tstr;
	};
	enum {
		ProviderModels_after_tstr_c,
		ProviderModels_after_null_m_c,
	} ProviderModels_after_choice;
};

struct request_provider_models {
	struct zcbor_string ProviderModels_provider;
	struct ProviderModels_credential_ref_r ProviderModels_credential_ref;
	bool ProviderModels_credential_ref_present;
	struct ProviderModels_transient_key_r ProviderModels_transient_key;
	bool ProviderModels_transient_key_present;
	struct ProviderModels_after_r ProviderModels_after;
	bool ProviderModels_after_present;
};

struct custom_provider_credential_ref_r {
	union {
		struct zcbor_string custom_provider_credential_ref_tstr;
	};
	enum {
		custom_provider_credential_ref_tstr_c,
		custom_provider_credential_ref_null_m_c,
	} custom_provider_credential_ref_choice;
};

struct custom_provider_source_t_r {
	enum {
		custom_provider_source_t_config_tstr_c,
		custom_provider_source_t_user_tstr_c,
	} custom_provider_source_t_choice;
};

struct custom_provider {
	struct zcbor_string custom_provider_id;
	struct zcbor_string custom_provider_display_name;
	struct zcbor_string custom_provider_base_url;
	struct provider_selector_r custom_provider_wire_selector;
	bool custom_provider_requires_key;
	struct custom_provider_credential_ref_r custom_provider_credential_ref;
	bool custom_provider_credential_ref_present;
	struct custom_provider_source_t_r custom_provider_source;
};

struct request_custom_provider_set {
	struct custom_provider CustomProviderSet_provider;
};

struct request_custom_provider_remove {
	struct zcbor_string CustomProviderRemove_id;
};

struct params_tstrtstr {
	struct zcbor_string auth_begin_request_params_tstrtstr_key;
	struct zcbor_string params_tstrtstr;
};

struct auth_bind_request {
	struct zcbor_string auth_bind_request_profile;
	union {
		struct zcbor_string auth_bind_request_transport_instance_transport_id_m;
	};
	enum {
		auth_bind_request_transport_instance_transport_id_m_c,
		auth_bind_request_transport_instance_null_m_c,
	} auth_bind_request_transport_instance_choice;
	union {
		struct zcbor_string auth_bind_request_credential_ref_tstr;
	};
	enum {
		auth_bind_request_credential_ref_tstr_c,
		auth_bind_request_credential_ref_null_m_c,
	} auth_bind_request_credential_ref_choice;
};

struct auth_begin_request_bind_r {
	union {
		struct auth_bind_request auth_begin_request_bind_auth_bind_request_m;
	};
	enum {
		auth_begin_request_bind_auth_bind_request_m_c,
		auth_begin_request_bind_null_m_c,
	} auth_begin_request_bind_choice;
};

struct auth_begin_request {
	struct zcbor_string auth_begin_request_family;
	struct params_tstrtstr params_tstrtstr[64];
	size_t params_tstrtstr_count;
	struct zcbor_string auth_begin_request_redirect_uri;
	struct auth_begin_request_bind_r auth_begin_request_bind;
	bool auth_begin_request_bind_present;
};

struct request_auth_begin {
	struct auth_begin_request request_auth_begin_AuthBegin;
};

struct Fields_tstrtstr {
	struct zcbor_string auth_step_input_fields_Fields_tstrtstr_key;
	struct zcbor_string Fields_tstrtstr;
};

struct auth_step_input_fields {
	struct Fields_tstrtstr Fields_tstrtstr[64];
	size_t Fields_tstrtstr_count;
};

struct auth_step_input_callback {
	struct zcbor_string auth_step_input_callback_Callback;
};

struct auth_step_input_r {
	union {
		struct auth_step_input_fields auth_step_input_fields_m;
		struct auth_step_input_callback auth_step_input_callback_m;
	};
	enum {
		auth_step_input_fields_m_c,
		auth_step_input_callback_m_c,
		auth_step_input_Poll_tstr_c,
	} auth_step_input_choice;
};

struct auth_step_request {
	struct zcbor_string auth_step_request_flow_id;
	struct auth_step_input_r auth_step_request_input;
};

struct request_auth_step {
	struct auth_step_request request_auth_step_AuthStep;
};

struct auth_complete_request {
	struct zcbor_string auth_complete_request_flow_id;
	struct zcbor_string auth_complete_request_callback;
};

struct request_auth_complete {
	struct auth_complete_request request_auth_complete_AuthComplete;
};

struct request_auth_cancel {
	struct zcbor_string AuthCancel_flow_id;
};

struct CheckpointList_session_r {
	union {
		struct zcbor_string CheckpointList_session_session_id_m;
	};
	enum {
		CheckpointList_session_session_id_m_c,
		CheckpointList_session_null_m_c,
	} CheckpointList_session_choice;
};

struct CheckpointList_after_r {
	union {
		struct zcbor_string CheckpointList_after_tstr;
	};
	enum {
		CheckpointList_after_tstr_c,
		CheckpointList_after_null_m_c,
	} CheckpointList_after_choice;
};

struct request_checkpoint_list {
	struct CheckpointList_session_r CheckpointList_session;
	bool CheckpointList_session_present;
	struct CheckpointList_after_r CheckpointList_after;
	bool CheckpointList_after_present;
};

struct request_checkpoint_rewind {
	struct zcbor_string CheckpointRewind_session;
	struct zcbor_string CheckpointRewind_checkpoint_id;
};

struct session_scope_by_profile {
	struct zcbor_string session_scope_by_profile_ByProfile;
};

struct session_scope_by_transport {
	struct zcbor_string session_scope_by_transport_ByTransport;
};

struct session_scope_r {
	union {
		struct session_scope_by_profile session_scope_by_profile_m;
		struct session_scope_by_transport session_scope_by_transport_m;
	};
	enum {
		session_scope_TopLevel_tstr_c,
		session_scope_by_profile_m_c,
		session_scope_by_transport_m_c,
		session_scope_Archived_tstr_c,
		session_scope_All_tstr_c,
	} session_scope_choice;
};

struct session_query_scope {
	struct session_scope_r session_query_scope;
};

struct session_query_after_r {
	union {
		struct zcbor_string session_query_after_session_id_m;
	};
	enum {
		session_query_after_session_id_m_c,
		session_query_after_null_m_c,
	} session_query_after_choice;
};

struct session_query_limit {
	uint32_t session_query_limit;
};

struct session_query_since_rev_r {
	union {
		uint64_t session_query_since_rev_uint64_m;
	};
	enum {
		session_query_since_rev_uint64_m_c,
		session_query_since_rev_null_m_c,
	} session_query_since_rev_choice;
};

struct session_query {
	struct session_query_scope session_query_scope;
	bool session_query_scope_present;
	struct session_query_after_r session_query_after;
	bool session_query_after_present;
	struct session_query_limit session_query_limit;
	bool session_query_limit_present;
	struct session_query_since_rev_r session_query_since_rev;
	bool session_query_since_rev_present;
};

struct request_sessions_query {
	struct session_query SessionsQuery_query;
};

struct request_session_get {
	struct zcbor_string SessionGet_session;
};

struct request_session_search {
	struct zcbor_string SessionSearch_query;
	uint32_t SessionSearch_limit;
};

struct request_session_recap {
	struct zcbor_string SessionRecap_session;
};

struct session_meta_patch_title_r {
	union {
		struct zcbor_string session_meta_patch_title_tstr;
	};
	enum {
		session_meta_patch_title_tstr_c,
		session_meta_patch_title_null_m_c,
	} session_meta_patch_title_choice;
};

struct session_meta_patch_pinned_r {
	union {
		bool session_meta_patch_pinned_bool;
	};
	enum {
		session_meta_patch_pinned_bool_c,
		session_meta_patch_pinned_null_m_c,
	} session_meta_patch_pinned_choice;
};

struct session_meta_patch_archived_r {
	union {
		bool session_meta_patch_archived_bool;
	};
	enum {
		session_meta_patch_archived_bool_c,
		session_meta_patch_archived_null_m_c,
	} session_meta_patch_archived_choice;
};

struct session_meta_patch {
	struct session_meta_patch_title_r session_meta_patch_title;
	bool session_meta_patch_title_present;
	struct session_meta_patch_pinned_r session_meta_patch_pinned;
	bool session_meta_patch_pinned_present;
	struct session_meta_patch_archived_r session_meta_patch_archived;
	bool session_meta_patch_archived_present;
};

struct SessionUpdateMeta_op_id_r {
	union {
		struct zcbor_string SessionUpdateMeta_op_id_tstr;
	};
	enum {
		SessionUpdateMeta_op_id_tstr_c,
		SessionUpdateMeta_op_id_null_m_c,
	} SessionUpdateMeta_op_id_choice;
};

struct request_session_update_meta {
	struct zcbor_string SessionUpdateMeta_session;
	struct session_meta_patch SessionUpdateMeta_patch;
	struct SessionUpdateMeta_op_id_r SessionUpdateMeta_op_id;
	bool SessionUpdateMeta_op_id_present;
};

struct rewind_point_restore_workspace {
	bool rewind_point_restore_workspace;
};

struct rewind_point {
	struct rewind_anchor_r rewind_point_anchor;
	struct rewind_point_restore_workspace rewind_point_restore_workspace;
	bool rewind_point_restore_workspace_present;
};

struct request_rewind {
	struct zcbor_string Rewind_session;
	struct rewind_point Rewind_point;
};

struct agent_recipe_program_r {
	union {
		struct zcbor_string agent_recipe_program_tstr;
	};
	enum {
		agent_recipe_program_tstr_c,
		agent_recipe_program_null_m_c,
	} agent_recipe_program_choice;
};

struct agent_recipe_args_r {
	struct zcbor_string agent_recipe_args_tstr[64];
	size_t agent_recipe_args_tstr_count;
};

struct kv_pair {
	struct zcbor_string kv_pair_k;
	struct zcbor_string kv_pair_v;
};

struct agent_recipe_env_r {
	struct kv_pair agent_recipe_env_kv_pair_m[64];
	size_t agent_recipe_env_kv_pair_m_count;
};

struct agent_recipe_endpoint_r {
	union {
		struct zcbor_string agent_recipe_endpoint_tstr;
	};
	enum {
		agent_recipe_endpoint_tstr_c,
		agent_recipe_endpoint_null_m_c,
	} agent_recipe_endpoint_choice;
};

struct agent_recipe {
	struct agent_recipe_program_r agent_recipe_program;
	bool agent_recipe_program_present;
	struct agent_recipe_args_r agent_recipe_args;
	bool agent_recipe_args_present;
	struct agent_recipe_env_r agent_recipe_env;
	bool agent_recipe_env_present;
	struct agent_recipe_endpoint_r agent_recipe_endpoint;
	bool agent_recipe_endpoint_present;
};

struct agent_source_r {
	enum {
		agent_source_Builtin_tstr_c,
		agent_source_Manual_tstr_c,
		agent_source_Endpoint_tstr_c,
	} agent_source_choice;
};

struct agent_protocol_r {
	enum {
		agent_protocol_Acp_tstr_c,
		agent_protocol_StreamJson_tstr_c,
	} agent_protocol_choice;
};

struct agent_entry_protocol {
	struct agent_protocol_r agent_entry_protocol;
};

struct agent_entry_installed {
	bool agent_entry_installed;
};

struct agent_entry_version_r {
	union {
		struct zcbor_string agent_entry_version_tstr;
	};
	enum {
		agent_entry_version_tstr_c,
		agent_entry_version_null_m_c,
	} agent_entry_version_choice;
};

struct agent_entry_capabilities_r {
	struct kv_pair agent_entry_capabilities_kv_pair_m[64];
	size_t agent_entry_capabilities_kv_pair_m_count;
};

struct agent_verification_r {
	enum {
		agent_verification_Verified_tstr_c,
		agent_verification_Unverified_tstr_c,
		agent_verification_NotInstalled_tstr_c,
	} agent_verification_choice;
};

struct agent_entry_verification {
	struct agent_verification_r agent_entry_verification;
};

struct agent_entry {
	struct zcbor_string agent_entry_name;
	struct agent_recipe agent_entry_recipe;
	struct agent_source_r agent_entry_source;
	struct agent_entry_protocol agent_entry_protocol;
	bool agent_entry_protocol_present;
	struct agent_entry_installed agent_entry_installed;
	bool agent_entry_installed_present;
	struct agent_entry_version_r agent_entry_version;
	bool agent_entry_version_present;
	struct agent_entry_capabilities_r agent_entry_capabilities;
	bool agent_entry_capabilities_present;
	struct agent_entry_verification agent_entry_verification;
	bool agent_entry_verification_present;
};

struct request_agent_register {
	struct agent_entry AgentRegister_entry;
};

struct request_agent_remove {
	struct zcbor_string AgentRemove_name;
};

struct request_skill_get {
	struct zcbor_string SkillGet_name;
};

struct request_skill_put {
	struct skill_bundle SkillPut_bundle;
};

struct provider_info_base_url_r {
	union {
		struct zcbor_string provider_info_base_url_tstr;
	};
	enum {
		provider_info_base_url_tstr_c,
		provider_info_base_url_null_m_c,
	} provider_info_base_url_choice;
};

struct provider_info_available {
	bool provider_info_available;
};

struct provider_info {
	struct zcbor_string provider_info_name;
	struct provider_info_base_url_r provider_info_base_url;
	bool provider_info_base_url_present;
	struct provider_info_available provider_info_available;
	bool provider_info_available_present;
};

struct request_provider_register {
	struct provider_info ProviderRegister_provider;
};

struct tool_info_description_r {
	union {
		struct zcbor_string tool_info_description_tstr;
	};
	enum {
		tool_info_description_tstr_c,
		tool_info_description_null_m_c,
	} tool_info_description_choice;
};

struct tool_info_requires_r {
	union {
		struct zcbor_string tool_info_requires_tstr;
	};
	enum {
		tool_info_requires_tstr_c,
		tool_info_requires_null_m_c,
	} tool_info_requires_choice;
};

struct tool_info {
	struct zcbor_string tool_info_name;
	struct tool_info_description_r tool_info_description;
	bool tool_info_description_present;
	bool tool_info_enabled;
	struct tool_info_requires_r tool_info_requires;
	bool tool_info_requires_present;
};

struct request_tool_register {
	struct tool_info ToolRegister_tool;
};

struct request_tool_set_enabled {
	struct zcbor_string ToolSetEnabled_tool;
	bool ToolSetEnabled_enabled;
};

struct command_invocation {
	struct zcbor_string command_invocation_name;
	struct zcbor_string command_invocation_args;
	union {
		struct zcbor_string command_invocation_session_session_id_m;
	};
	enum {
		command_invocation_session_session_id_m_c,
		command_invocation_session_null_m_c,
	} command_invocation_session_choice;
	union {
		struct origin command_invocation_origin_origin_m;
	};
	enum {
		command_invocation_origin_origin_m_c,
		command_invocation_origin_null_m_c,
	} command_invocation_origin_choice;
};

struct request_command_invoke {
	struct command_invocation CommandInvoke_invocation;
};

struct node_config_view {
	struct zcbor_string node_config_view_format;
	struct zcbor_string node_config_view_body;
};

struct request_config_set {
	struct node_config_view ConfigSet_config;
};

struct GatewaySet_addr_r {
	union {
		struct zcbor_string GatewaySet_addr_tstr;
	};
	enum {
		GatewaySet_addr_tstr_c,
		GatewaySet_addr_null_m_c,
	} GatewaySet_addr_choice;
};

struct request_gateway_set {
	bool GatewaySet_enabled;
	struct GatewaySet_addr_r GatewaySet_addr;
	bool GatewaySet_addr_present;
};

struct cron_spec_target_r {
	union {
		struct zcbor_string cron_spec_target_tstr;
	};
	enum {
		cron_spec_target_tstr_c,
		cron_spec_target_null_m_c,
	} cron_spec_target_choice;
};

struct cron_spec_payload {
	struct zcbor_string cron_spec_payload;
};

struct cron_spec_enabled {
	bool cron_spec_enabled;
};

struct cron_spec_timezone_r {
	union {
		struct zcbor_string cron_spec_timezone_tstr;
	};
	enum {
		cron_spec_timezone_tstr_c,
		cron_spec_timezone_null_m_c,
	} cron_spec_timezone_choice;
};

struct cron_spec_repeat_r {
	union {
		uint32_t cron_spec_repeat_uint;
	};
	enum {
		cron_spec_repeat_uint_c,
		cron_spec_repeat_null_m_c,
	} cron_spec_repeat_choice;
};

struct cron_spec_jitter_secs_r {
	union {
		uint32_t cron_spec_jitter_secs_uint;
	};
	enum {
		cron_spec_jitter_secs_uint_c,
		cron_spec_jitter_secs_null_m_c,
	} cron_spec_jitter_secs_choice;
};

struct overlap_policy_r {
	enum {
		overlap_policy_Skip_tstr_c,
		overlap_policy_Allow_tstr_c,
		overlap_policy_Queue_tstr_c,
	} overlap_policy_choice;
};

struct cron_spec_overlap {
	struct overlap_policy_r cron_spec_overlap;
};

struct catch_up_policy_r {
	enum {
		catch_up_policy_Grace_tstr_c,
		catch_up_policy_Skip_tstr_c,
		catch_up_policy_Always_tstr_c,
	} catch_up_policy_choice;
};

struct cron_spec_catch_up {
	struct catch_up_policy_r cron_spec_catch_up;
};

struct cron_spec_script_r {
	union {
		struct zcbor_string cron_spec_script_tstr;
	};
	enum {
		cron_spec_script_tstr_c,
		cron_spec_script_null_m_c,
	} cron_spec_script_choice;
};

struct cron_spec_no_agent {
	bool cron_spec_no_agent;
};

struct cron_spec_context_from_r {
	struct zcbor_string cron_spec_context_from_tstr[64];
	size_t cron_spec_context_from_tstr_count;
};

struct cron_spec_deliver_r {
	union {
		struct zcbor_string cron_spec_deliver_tstr;
	};
	enum {
		cron_spec_deliver_tstr_c,
		cron_spec_deliver_null_m_c,
	} cron_spec_deliver_choice;
};

struct cron_spec_enabled_toolsets_r {
	union {
		struct {
			struct zcbor_string enabled_toolsets_tstr_l_tstr[64];
			size_t enabled_toolsets_tstr_l_tstr_count;
		};
	};
	enum {
		enabled_toolsets_tstr_l_c,
		cron_spec_enabled_toolsets_null_m_c,
	} cron_spec_enabled_toolsets_choice;
};

struct cron_spec_workdir_r {
	union {
		struct zcbor_string cron_spec_workdir_tstr;
	};
	enum {
		cron_spec_workdir_tstr_c,
		cron_spec_workdir_null_m_c,
	} cron_spec_workdir_choice;
};

struct cron_spec_model_r {
	union {
		struct zcbor_string cron_spec_model_tstr;
	};
	enum {
		cron_spec_model_tstr_c,
		cron_spec_model_null_m_c,
	} cron_spec_model_choice;
};

struct cron_spec_provider_r {
	union {
		struct zcbor_string cron_spec_provider_tstr;
	};
	enum {
		cron_spec_provider_tstr_c,
		cron_spec_provider_null_m_c,
	} cron_spec_provider_choice;
};

struct cron_spec_skills_r {
	struct zcbor_string cron_spec_skills_tstr[64];
	size_t cron_spec_skills_tstr_count;
};

struct cron_spec_origin_r {
	union {
		struct origin cron_spec_origin_origin_m;
	};
	enum {
		cron_spec_origin_origin_m_c,
		cron_spec_origin_null_m_c,
	} cron_spec_origin_choice;
};

struct cron_spec {
	struct zcbor_string cron_spec_name;
	struct zcbor_string cron_spec_schedule;
	struct cron_spec_target_r cron_spec_target;
	bool cron_spec_target_present;
	struct cron_spec_payload cron_spec_payload;
	bool cron_spec_payload_present;
	struct cron_spec_enabled cron_spec_enabled;
	bool cron_spec_enabled_present;
	struct cron_spec_timezone_r cron_spec_timezone;
	bool cron_spec_timezone_present;
	struct cron_spec_repeat_r cron_spec_repeat;
	bool cron_spec_repeat_present;
	struct cron_spec_jitter_secs_r cron_spec_jitter_secs;
	bool cron_spec_jitter_secs_present;
	struct cron_spec_overlap cron_spec_overlap;
	bool cron_spec_overlap_present;
	struct cron_spec_catch_up cron_spec_catch_up;
	bool cron_spec_catch_up_present;
	struct cron_spec_script_r cron_spec_script;
	bool cron_spec_script_present;
	struct cron_spec_no_agent cron_spec_no_agent;
	bool cron_spec_no_agent_present;
	struct cron_spec_context_from_r cron_spec_context_from;
	bool cron_spec_context_from_present;
	struct cron_spec_deliver_r cron_spec_deliver;
	bool cron_spec_deliver_present;
	struct cron_spec_enabled_toolsets_r cron_spec_enabled_toolsets;
	bool cron_spec_enabled_toolsets_present;
	struct cron_spec_workdir_r cron_spec_workdir;
	bool cron_spec_workdir_present;
	struct cron_spec_model_r cron_spec_model;
	bool cron_spec_model_present;
	struct cron_spec_provider_r cron_spec_provider;
	bool cron_spec_provider_present;
	struct cron_spec_skills_r cron_spec_skills;
	bool cron_spec_skills_present;
	struct cron_spec_origin_r cron_spec_origin;
	bool cron_spec_origin_present;
};

struct request_cron_create {
	struct cron_spec CronCreate_spec;
};

struct request_cron_update {
	struct zcbor_string CronUpdate_id;
	struct cron_spec CronUpdate_spec;
};

struct request_cron_delete {
	struct zcbor_string CronDelete_id;
};

struct request_cron_trigger {
	struct zcbor_string CronTrigger_id;
};

struct request_cron_runs {
	struct zcbor_string CronRuns_id;
};

struct request_cron_pause {
	struct zcbor_string CronPause_id;
	bool CronPause_paused;
};

struct request_cron_accept_suggestion {
	struct zcbor_string CronAcceptSuggestion_id;
};

struct request_cron_dismiss_suggestion {
	struct zcbor_string CronDismissSuggestion_id;
};

struct RoutingListChats_after_r {
	union {
		struct zcbor_string RoutingListChats_after_tstr;
	};
	enum {
		RoutingListChats_after_tstr_c,
		RoutingListChats_after_null_m_c,
	} RoutingListChats_after_choice;
};

struct request_routing_list_chats {
	struct RoutingListChats_after_r RoutingListChats_after;
	bool RoutingListChats_after_present;
};

struct request_routing_get {
	struct origin RoutingGet_origin;
};

struct chat_route_profile_r {
	union {
		struct zcbor_string chat_route_profile_profile_ref_m;
	};
	enum {
		chat_route_profile_profile_ref_m_c,
		chat_route_profile_null_m_c,
	} chat_route_profile_choice;
};

struct isolation_policy_r {
	enum {
		isolation_policy_PerUser_tstr_c,
		isolation_policy_PerChat_tstr_c,
		isolation_policy_PerThread_tstr_c,
		isolation_policy_Shared_tstr_c,
	} isolation_policy_choice;
};

struct chat_route_isolation {
	struct isolation_policy_r chat_route_isolation;
};

struct chat_route {
	struct origin chat_route_origin;
	struct zcbor_string chat_route_session;
	struct chat_route_profile_r chat_route_profile;
	bool chat_route_profile_present;
	struct chat_route_isolation chat_route_isolation;
	bool chat_route_isolation_present;
};

struct request_routing_set {
	struct chat_route RoutingSet_route;
};

struct RoutingBindChat_profile_r {
	union {
		struct zcbor_string RoutingBindChat_profile_profile_ref_m;
	};
	enum {
		RoutingBindChat_profile_profile_ref_m_c,
		RoutingBindChat_profile_null_m_c,
	} RoutingBindChat_profile_choice;
};

struct request_routing_bind_chat {
	struct origin RoutingBindChat_origin;
	struct zcbor_string RoutingBindChat_session;
	struct RoutingBindChat_profile_r RoutingBindChat_profile;
	bool RoutingBindChat_profile_present;
};

struct request_routing_unbind_chat {
	struct origin RoutingUnbindChat_origin;
};

struct TransportRooms_after_r {
	union {
		struct zcbor_string TransportRooms_after_tstr;
	};
	enum {
		TransportRooms_after_tstr_c,
		TransportRooms_after_null_m_c,
	} TransportRooms_after_choice;
};

struct request_transport_rooms {
	struct zcbor_string TransportRooms_transport;
	struct TransportRooms_after_r TransportRooms_after;
	bool TransportRooms_after_present;
};

struct request_transport_disconnect {
	struct zcbor_string TransportDisconnect_transport;
};

struct request_transport_remove {
	struct zcbor_string TransportRemove_transport;
};

struct request_transport_connect {
	struct zcbor_string TransportConnect_transport;
};

struct request_transport_set_enabled {
	struct zcbor_string TransportSetEnabled_transport;
	bool TransportSetEnabled_enabled;
};

struct TransportSetLabel_label_r {
	union {
		struct zcbor_string TransportSetLabel_label_tstr;
	};
	enum {
		TransportSetLabel_label_tstr_c,
		TransportSetLabel_label_null_m_c,
	} TransportSetLabel_label_choice;
};

struct request_transport_set_label {
	struct zcbor_string TransportSetLabel_transport;
	struct TransportSetLabel_label_r TransportSetLabel_label;
	bool TransportSetLabel_label_present;
};

struct ConvList_after_r {
	union {
		struct zcbor_string ConvList_after_tstr;
	};
	enum {
		ConvList_after_tstr_c,
		ConvList_after_null_m_c,
	} ConvList_after_choice;
};

struct ConvList_since_rev_r {
	union {
		uint64_t ConvList_since_rev_uint64_m;
	};
	enum {
		ConvList_since_rev_uint64_m_c,
		ConvList_since_rev_null_m_c,
	} ConvList_since_rev_choice;
};

struct request_conv_list {
	struct zcbor_string ConvList_transport;
	struct ConvList_after_r ConvList_after;
	bool ConvList_after_present;
	struct ConvList_since_rev_r ConvList_since_rev;
	bool ConvList_since_rev_present;
};

struct request_conv_get {
	struct zcbor_string ConvGet_transport;
	struct zcbor_string ConvGet_conv;
};

struct request_conv_create_details {
	struct zcbor_string ConvCreateDetails_transport;
};

struct create_conversation_details_max_participants {
	uint32_t create_conversation_details_max_participants;
};

struct contact_info_display_name_r {
	union {
		struct zcbor_string contact_info_display_name_tstr;
	};
	enum {
		contact_info_display_name_tstr_c,
		contact_info_display_name_null_m_c,
	} contact_info_display_name_choice;
};

struct presence_primitive_t_r {
	enum {
		presence_primitive_t_Offline_tstr_c,
		presence_primitive_t_Available_tstr_c,
		presence_primitive_t_Idle_tstr_c,
		presence_primitive_t_Invisible_tstr_c,
		presence_primitive_t_Away_tstr_c,
		presence_primitive_t_DoNotDisturb_tstr_c,
		presence_primitive_t_Streaming_tstr_c,
		presence_primitive_t_OutOfOffice_tstr_c,
	} presence_primitive_t_choice;
};

struct presence_message_r {
	union {
		struct zcbor_string presence_message_tstr;
	};
	enum {
		presence_message_tstr_c,
		presence_message_null_m_c,
	} presence_message_choice;
};

struct presence_emoji_r {
	union {
		struct zcbor_string presence_emoji_tstr;
	};
	enum {
		presence_emoji_tstr_c,
		presence_emoji_null_m_c,
	} presence_emoji_choice;
};

struct presence_mobile {
	bool presence_mobile;
};

struct presence_idle_since_r {
	union {
		uint64_t presence_idle_since_uint64_m;
	};
	enum {
		presence_idle_since_uint64_m_c,
		presence_idle_since_null_m_c,
	} presence_idle_since_choice;
};

struct presence {
	struct presence_primitive_t_r presence_primitive;
	struct presence_message_r presence_message;
	bool presence_message_present;
	struct presence_emoji_r presence_emoji;
	bool presence_emoji_present;
	struct presence_mobile presence_mobile;
	bool presence_mobile_present;
	struct presence_idle_since_r presence_idle_since;
	bool presence_idle_since_present;
};

struct contact_info_presence {
	struct presence contact_info_presence;
};

struct contact_permission_r {
	enum {
		contact_permission_Unset_tstr_c,
		contact_permission_Allow_tstr_c,
		contact_permission_Deny_tstr_c,
	} contact_permission_choice;
};

struct contact_info_permission {
	struct contact_permission_r contact_info_permission;
};

struct contact_info {
	struct zcbor_string contact_info_id;
	struct contact_info_display_name_r contact_info_display_name;
	bool contact_info_display_name_present;
	struct contact_info_presence contact_info_presence;
	bool contact_info_presence_present;
	struct contact_info_permission contact_info_permission;
	bool contact_info_permission_present;
};

struct create_conversation_details_participants_r {
	struct contact_info create_conversation_details_participants_contact_info_m[64];
	size_t create_conversation_details_participants_contact_info_m_count;
};

struct auth_field_kind_r {
	enum {
		auth_field_kind_Text_tstr_c,
		auth_field_kind_Password_tstr_c,
		auth_field_kind_Number_tstr_c,
		auth_field_kind_Choice_tstr_c,
	} auth_field_kind_choice;
};

struct auth_param_field_kind {
	struct auth_field_kind_r auth_param_field_kind;
};

struct auth_param_field_default_r {
	union {
		struct zcbor_string auth_param_field_default_tstr;
	};
	enum {
		auth_param_field_default_tstr_c,
		auth_param_field_default_null_m_c,
	} auth_param_field_default_choice;
};

struct auth_param_field_placeholder_r {
	union {
		struct zcbor_string auth_param_field_placeholder_tstr;
	};
	enum {
		auth_param_field_placeholder_tstr_c,
		auth_param_field_placeholder_null_m_c,
	} auth_param_field_placeholder_choice;
};

struct auth_param_field_choices_r {
	struct zcbor_string auth_param_field_choices_tstr[64];
	size_t auth_param_field_choices_tstr_count;
};

struct auth_param_field {
	struct zcbor_string auth_param_field_key;
	struct zcbor_string auth_param_field_label;
	bool auth_param_field_required;
	struct auth_param_field_kind auth_param_field_kind;
	bool auth_param_field_kind_present;
	struct auth_param_field_default_r auth_param_field_default;
	bool auth_param_field_default_present;
	struct auth_param_field_placeholder_r auth_param_field_placeholder;
	bool auth_param_field_placeholder_present;
	struct auth_param_field_choices_r auth_param_field_choices;
	bool auth_param_field_choices_present;
};

struct account_settings_schema_fields_r {
	struct auth_param_field account_settings_schema_fields_auth_param_field_m[64];
	size_t account_settings_schema_fields_auth_param_field_m_count;
};

struct account_settings_schema {
	struct account_settings_schema_fields_r account_settings_schema_fields;
	bool account_settings_schema_fields_present;
};

struct create_conversation_details_extras_schema {
	struct account_settings_schema create_conversation_details_extras_schema;
};

struct values_tstrtstr {
	struct zcbor_string account_settings_values_values_tstrtstr_key;
	struct zcbor_string values_tstrtstr;
};

struct account_settings_values_values_r {
	struct values_tstrtstr values_tstrtstr[64];
	size_t values_tstrtstr_count;
};

struct account_settings_values {
	struct account_settings_values_values_r account_settings_values_values;
	bool account_settings_values_values_present;
};

struct create_conversation_details_extras {
	struct account_settings_values create_conversation_details_extras;
};

struct create_conversation_details {
	struct create_conversation_details_max_participants create_conversation_details_max_participants;
	bool create_conversation_details_max_participants_present;
	struct create_conversation_details_participants_r create_conversation_details_participants;
	bool create_conversation_details_participants_present;
	struct create_conversation_details_extras_schema create_conversation_details_extras_schema;
	bool create_conversation_details_extras_schema_present;
	struct create_conversation_details_extras create_conversation_details_extras;
	bool create_conversation_details_extras_present;
};

struct ConvCreate_op_id_r {
	union {
		struct zcbor_string ConvCreate_op_id_tstr;
	};
	enum {
		ConvCreate_op_id_tstr_c,
		ConvCreate_op_id_null_m_c,
	} ConvCreate_op_id_choice;
};

struct request_conv_create {
	struct zcbor_string ConvCreate_transport;
	struct create_conversation_details ConvCreate_details;
	struct ConvCreate_op_id_r ConvCreate_op_id;
	bool ConvCreate_op_id_present;
};

struct request_conv_join_details {
	struct zcbor_string ConvJoinDetails_transport;
};

struct channel_join_details_name_r {
	union {
		struct zcbor_string channel_join_details_name_tstr;
	};
	enum {
		channel_join_details_name_tstr_c,
		channel_join_details_name_null_m_c,
	} channel_join_details_name_choice;
};

struct channel_join_details_name_max_length {
	uint32_t channel_join_details_name_max_length;
};

struct channel_join_details_nickname_r {
	union {
		struct zcbor_string channel_join_details_nickname_tstr;
	};
	enum {
		channel_join_details_nickname_tstr_c,
		channel_join_details_nickname_null_m_c,
	} channel_join_details_nickname_choice;
};

struct channel_join_details_nickname_supported {
	bool channel_join_details_nickname_supported;
};

struct channel_join_details_nickname_max_length {
	uint32_t channel_join_details_nickname_max_length;
};

struct channel_join_details_password_r {
	union {
		struct zcbor_string channel_join_details_password_tstr;
	};
	enum {
		channel_join_details_password_tstr_c,
		channel_join_details_password_null_m_c,
	} channel_join_details_password_choice;
};

struct channel_join_details_password_supported {
	bool channel_join_details_password_supported;
};

struct channel_join_details_password_max_length {
	uint32_t channel_join_details_password_max_length;
};

struct channel_join_details_extras_schema {
	struct account_settings_schema channel_join_details_extras_schema;
};

struct channel_join_details_extras {
	struct account_settings_values channel_join_details_extras;
};

struct channel_join_details {
	struct channel_join_details_name_r channel_join_details_name;
	bool channel_join_details_name_present;
	struct channel_join_details_name_max_length channel_join_details_name_max_length;
	bool channel_join_details_name_max_length_present;
	struct channel_join_details_nickname_r channel_join_details_nickname;
	bool channel_join_details_nickname_present;
	struct channel_join_details_nickname_supported channel_join_details_nickname_supported;
	bool channel_join_details_nickname_supported_present;
	struct channel_join_details_nickname_max_length channel_join_details_nickname_max_length;
	bool channel_join_details_nickname_max_length_present;
	struct channel_join_details_password_r channel_join_details_password;
	bool channel_join_details_password_present;
	struct channel_join_details_password_supported channel_join_details_password_supported;
	bool channel_join_details_password_supported_present;
	struct channel_join_details_password_max_length channel_join_details_password_max_length;
	bool channel_join_details_password_max_length_present;
	struct channel_join_details_extras_schema channel_join_details_extras_schema;
	bool channel_join_details_extras_schema_present;
	struct channel_join_details_extras channel_join_details_extras;
	bool channel_join_details_extras_present;
};

struct ConvJoin_op_id_r {
	union {
		struct zcbor_string ConvJoin_op_id_tstr;
	};
	enum {
		ConvJoin_op_id_tstr_c,
		ConvJoin_op_id_null_m_c,
	} ConvJoin_op_id_choice;
};

struct request_conv_join {
	struct zcbor_string ConvJoin_transport;
	struct channel_join_details ConvJoin_details;
	struct ConvJoin_op_id_r ConvJoin_op_id;
	bool ConvJoin_op_id_present;
};

struct request_conv_leave {
	struct zcbor_string ConvLeave_transport;
	struct zcbor_string ConvLeave_conv;
};

struct participant_contact {
	struct contact_info participant_contact_Contact;
};

struct participant_agent {
	struct zcbor_string Agent_profile;
	struct zcbor_string Agent_member;
};

struct participant_r {
	union {
		struct participant_contact participant_contact_m;
		struct participant_agent participant_agent_m;
	};
	enum {
		participant_contact_m_c,
		participant_agent_m_c,
	} participant_choice;
};

struct conv_send_args_from_r {
	union {
		struct participant_r conv_send_args_from_participant_m;
	};
	enum {
		conv_send_args_from_participant_m_c,
		conv_send_args_from_null_m_c,
	} conv_send_args_from_choice;
};

struct conv_send_args_op_id_r {
	union {
		struct zcbor_string conv_send_args_op_id_tstr;
	};
	enum {
		conv_send_args_op_id_tstr_c,
		conv_send_args_op_id_null_m_c,
	} conv_send_args_op_id_choice;
};

struct conv_send_args {
	struct zcbor_string conv_send_args_transport;
	struct zcbor_string conv_send_args_conv;
	struct conv_send_args_from_r conv_send_args_from;
	bool conv_send_args_from_present;
	struct user_msg conv_send_args_message;
	struct conv_send_args_op_id_r conv_send_args_op_id;
	bool conv_send_args_op_id_present;
};

struct request_conv_send {
	struct conv_send_args request_conv_send_ConvSend;
};

struct ConvSetTopic_topic_r {
	union {
		struct zcbor_string ConvSetTopic_topic_tstr;
	};
	enum {
		ConvSetTopic_topic_tstr_c,
		ConvSetTopic_topic_null_m_c,
	} ConvSetTopic_topic_choice;
};

struct ConvSetTopic_op_id_r {
	union {
		struct zcbor_string ConvSetTopic_op_id_tstr;
	};
	enum {
		ConvSetTopic_op_id_tstr_c,
		ConvSetTopic_op_id_null_m_c,
	} ConvSetTopic_op_id_choice;
};

struct request_conv_set_topic {
	struct zcbor_string ConvSetTopic_transport;
	struct zcbor_string ConvSetTopic_conv;
	struct ConvSetTopic_topic_r ConvSetTopic_topic;
	bool ConvSetTopic_topic_present;
	struct ConvSetTopic_op_id_r ConvSetTopic_op_id;
	bool ConvSetTopic_op_id_present;
};

struct ConvSetTitle_title_r {
	union {
		struct zcbor_string ConvSetTitle_title_tstr;
	};
	enum {
		ConvSetTitle_title_tstr_c,
		ConvSetTitle_title_null_m_c,
	} ConvSetTitle_title_choice;
};

struct ConvSetTitle_op_id_r {
	union {
		struct zcbor_string ConvSetTitle_op_id_tstr;
	};
	enum {
		ConvSetTitle_op_id_tstr_c,
		ConvSetTitle_op_id_null_m_c,
	} ConvSetTitle_op_id_choice;
};

struct request_conv_set_title {
	struct zcbor_string ConvSetTitle_transport;
	struct zcbor_string ConvSetTitle_conv;
	struct ConvSetTitle_title_r ConvSetTitle_title;
	bool ConvSetTitle_title_present;
	struct ConvSetTitle_op_id_r ConvSetTitle_op_id;
	bool ConvSetTitle_op_id_present;
};

struct ConvSetDescription_description_r {
	union {
		struct zcbor_string ConvSetDescription_description_tstr;
	};
	enum {
		ConvSetDescription_description_tstr_c,
		ConvSetDescription_description_null_m_c,
	} ConvSetDescription_description_choice;
};

struct ConvSetDescription_op_id_r {
	union {
		struct zcbor_string ConvSetDescription_op_id_tstr;
	};
	enum {
		ConvSetDescription_op_id_tstr_c,
		ConvSetDescription_op_id_null_m_c,
	} ConvSetDescription_op_id_choice;
};

struct request_conv_set_description {
	struct zcbor_string ConvSetDescription_transport;
	struct zcbor_string ConvSetDescription_conv;
	struct ConvSetDescription_description_r ConvSetDescription_description;
	bool ConvSetDescription_description_present;
	struct ConvSetDescription_op_id_r ConvSetDescription_op_id;
	bool ConvSetDescription_op_id_present;
};

struct request_conv_delete {
	struct zcbor_string ConvDelete_transport;
	struct zcbor_string ConvDelete_conv;
};

struct conv_history_args_after_cursor {
	uint64_t conv_history_args_after_cursor;
};

struct conv_history_args_before_cursor {
	uint64_t conv_history_args_before_cursor;
};

struct conv_history_args_max {
	uint32_t conv_history_args_max;
};

struct conv_history_args {
	struct zcbor_string conv_history_args_transport;
	struct zcbor_string conv_history_args_conv;
	struct conv_history_args_after_cursor conv_history_args_after_cursor;
	bool conv_history_args_after_cursor_present;
	struct conv_history_args_before_cursor conv_history_args_before_cursor;
	bool conv_history_args_before_cursor_present;
	struct conv_history_args_max conv_history_args_max;
	bool conv_history_args_max_present;
};

struct request_conv_history {
	struct conv_history_args request_conv_history_ConvHistory;
};

struct member_invite_args_message_r {
	union {
		struct zcbor_string member_invite_args_message_tstr;
	};
	enum {
		member_invite_args_message_tstr_c,
		member_invite_args_message_null_m_c,
	} member_invite_args_message_choice;
};

struct member_invite_args_op_id_r {
	union {
		struct zcbor_string member_invite_args_op_id_tstr;
	};
	enum {
		member_invite_args_op_id_tstr_c,
		member_invite_args_op_id_null_m_c,
	} member_invite_args_op_id_choice;
};

struct member_invite_args {
	struct zcbor_string member_invite_args_transport;
	struct zcbor_string member_invite_args_conv;
	struct participant_r member_invite_args_who;
	struct member_invite_args_message_r member_invite_args_message;
	bool member_invite_args_message_present;
	struct member_invite_args_op_id_r member_invite_args_op_id;
	bool member_invite_args_op_id_present;
};

struct request_member_invite {
	struct member_invite_args request_member_invite_MemberInvite;
};

struct member_remove_args_reason_r {
	union {
		struct zcbor_string member_remove_args_reason_tstr;
	};
	enum {
		member_remove_args_reason_tstr_c,
		member_remove_args_reason_null_m_c,
	} member_remove_args_reason_choice;
};

struct member_remove_args_op_id_r {
	union {
		struct zcbor_string member_remove_args_op_id_tstr;
	};
	enum {
		member_remove_args_op_id_tstr_c,
		member_remove_args_op_id_null_m_c,
	} member_remove_args_op_id_choice;
};

struct member_remove_args {
	struct zcbor_string member_remove_args_transport;
	struct zcbor_string member_remove_args_conv;
	struct participant_r member_remove_args_who;
	struct member_remove_args_reason_r member_remove_args_reason;
	bool member_remove_args_reason_present;
	struct member_remove_args_op_id_r member_remove_args_op_id;
	bool member_remove_args_op_id_present;
};

struct request_member_remove {
	struct member_remove_args request_member_remove_MemberRemove;
};

struct member_ban_args_reason_r {
	union {
		struct zcbor_string member_ban_args_reason_tstr;
	};
	enum {
		member_ban_args_reason_tstr_c,
		member_ban_args_reason_null_m_c,
	} member_ban_args_reason_choice;
};

struct member_ban_args_op_id_r {
	union {
		struct zcbor_string member_ban_args_op_id_tstr;
	};
	enum {
		member_ban_args_op_id_tstr_c,
		member_ban_args_op_id_null_m_c,
	} member_ban_args_op_id_choice;
};

struct member_ban_args {
	struct zcbor_string member_ban_args_transport;
	struct zcbor_string member_ban_args_conv;
	struct participant_r member_ban_args_who;
	struct member_ban_args_reason_r member_ban_args_reason;
	bool member_ban_args_reason_present;
	struct member_ban_args_op_id_r member_ban_args_op_id;
	bool member_ban_args_op_id_present;
};

struct request_member_ban {
	struct member_ban_args request_member_ban_MemberBan;
};

struct member_role_r {
	enum {
		member_role_None_tstr_c,
		member_role_Voice_tstr_c,
		member_role_HalfOp_tstr_c,
		member_role_Op_tstr_c,
		member_role_Founder_tstr_c,
	} member_role_choice;
};

struct member_set_role_args_op_id_r {
	union {
		struct zcbor_string member_set_role_args_op_id_tstr;
	};
	enum {
		member_set_role_args_op_id_tstr_c,
		member_set_role_args_op_id_null_m_c,
	} member_set_role_args_op_id_choice;
};

struct member_set_role_args {
	struct zcbor_string member_set_role_args_transport;
	struct zcbor_string member_set_role_args_conv;
	struct participant_r member_set_role_args_who;
	struct member_role_r member_set_role_args_role;
	struct member_set_role_args_op_id_r member_set_role_args_op_id;
	bool member_set_role_args_op_id_present;
};

struct request_member_set_role {
	struct member_set_role_args request_member_set_role_MemberSetRole;
};

struct request_contact_get_profile {
	struct zcbor_string ContactGetProfile_transport;
	struct contact_info ContactGetProfile_contact;
};

struct ContactSetAlias_alias_r {
	union {
		struct zcbor_string ContactSetAlias_alias_tstr;
	};
	enum {
		ContactSetAlias_alias_tstr_c,
		ContactSetAlias_alias_null_m_c,
	} ContactSetAlias_alias_choice;
};

struct ContactSetAlias_op_id_r {
	union {
		struct zcbor_string ContactSetAlias_op_id_tstr;
	};
	enum {
		ContactSetAlias_op_id_tstr_c,
		ContactSetAlias_op_id_null_m_c,
	} ContactSetAlias_op_id_choice;
};

struct request_contact_set_alias {
	struct zcbor_string ContactSetAlias_transport;
	struct contact_info ContactSetAlias_contact;
	struct ContactSetAlias_alias_r ContactSetAlias_alias;
	bool ContactSetAlias_alias_present;
	struct ContactSetAlias_op_id_r ContactSetAlias_op_id;
	bool ContactSetAlias_op_id_present;
};

struct request_contact_action_menu {
	struct zcbor_string ContactActionMenu_transport;
	struct contact_info ContactActionMenu_contact;
};

struct DirectorySearch_query_r {
	union {
		struct zcbor_string DirectorySearch_query_tstr;
	};
	enum {
		DirectorySearch_query_tstr_c,
		DirectorySearch_query_null_m_c,
	} DirectorySearch_query_choice;
};

struct request_directory_search {
	struct zcbor_string DirectorySearch_transport;
	struct DirectorySearch_query_r DirectorySearch_query;
	bool DirectorySearch_query_present;
};

struct RosterList_after_r {
	union {
		struct zcbor_string RosterList_after_tstr;
	};
	enum {
		RosterList_after_tstr_c,
		RosterList_after_null_m_c,
	} RosterList_after_choice;
};

struct RosterList_since_rev_r {
	union {
		uint64_t RosterList_since_rev_uint64_m;
	};
	enum {
		RosterList_since_rev_uint64_m_c,
		RosterList_since_rev_null_m_c,
	} RosterList_since_rev_choice;
};

struct request_roster_list {
	struct zcbor_string RosterList_transport;
	struct RosterList_after_r RosterList_after;
	bool RosterList_after_present;
	struct RosterList_since_rev_r RosterList_since_rev;
	bool RosterList_since_rev_present;
};

struct RosterAdd_op_id_r {
	union {
		struct zcbor_string RosterAdd_op_id_tstr;
	};
	enum {
		RosterAdd_op_id_tstr_c,
		RosterAdd_op_id_null_m_c,
	} RosterAdd_op_id_choice;
};

struct request_roster_add {
	struct zcbor_string RosterAdd_transport;
	struct contact_info RosterAdd_contact;
	struct RosterAdd_op_id_r RosterAdd_op_id;
	bool RosterAdd_op_id_present;
};

struct RosterUpdate_op_id_r {
	union {
		struct zcbor_string RosterUpdate_op_id_tstr;
	};
	enum {
		RosterUpdate_op_id_tstr_c,
		RosterUpdate_op_id_null_m_c,
	} RosterUpdate_op_id_choice;
};

struct request_roster_update {
	struct zcbor_string RosterUpdate_transport;
	struct contact_info RosterUpdate_contact;
	struct RosterUpdate_op_id_r RosterUpdate_op_id;
	bool RosterUpdate_op_id_present;
};

struct RosterRemove_op_id_r {
	union {
		struct zcbor_string RosterRemove_op_id_tstr;
	};
	enum {
		RosterRemove_op_id_tstr_c,
		RosterRemove_op_id_null_m_c,
	} RosterRemove_op_id_choice;
};

struct request_roster_remove {
	struct zcbor_string RosterRemove_transport;
	struct contact_info RosterRemove_contact;
	struct RosterRemove_op_id_r RosterRemove_op_id;
	bool RosterRemove_op_id_present;
};

struct fs_root_id_host {
	struct zcbor_string fs_root_id_host_host;
};

struct fs_root_id_session {
	struct zcbor_string fs_root_id_session_session;
};

struct fs_root_id_t_r {
	union {
		struct fs_root_id_host fs_root_id_t_fs_root_id_host_m;
		struct fs_root_id_session fs_root_id_t_fs_root_id_session_m;
	};
	enum {
		fs_root_id_t_fs_root_id_host_m_c,
		fs_root_id_t_workspace_tstr_c,
		fs_root_id_t_fs_root_id_session_m_c,
	} fs_root_id_t_choice;
};

struct FsList_show_ignored {
	bool FsList_show_ignored;
};

struct FsList_after_r {
	union {
		struct zcbor_string FsList_after_tstr;
	};
	enum {
		FsList_after_tstr_c,
		FsList_after_null_m_c,
	} FsList_after_choice;
};

struct request_fs_list {
	struct fs_root_id_t_r FsList_root;
	struct zcbor_string FsList_dir;
	struct FsList_show_ignored FsList_show_ignored;
	bool FsList_show_ignored_present;
	struct FsList_after_r FsList_after;
	bool FsList_after_present;
};

struct request_fs_stat {
	struct fs_root_id_t_r FsStat_root;
	struct zcbor_string FsStat_path;
};

struct FsRead_max_bytes {
	uint64_t FsRead_max_bytes;
};

struct request_fs_read {
	struct fs_root_id_t_r FsRead_root;
	struct zcbor_string FsRead_path;
	struct FsRead_max_bytes FsRead_max_bytes;
	bool FsRead_max_bytes_present;
};

struct fs_revision {
	uint64_t fs_revision_mtime_ms;
	uint64_t fs_revision_size;
};

struct fs_write_args_base_revision_r {
	union {
		struct fs_revision fs_write_args_base_revision_fs_revision_m;
	};
	enum {
		fs_write_args_base_revision_fs_revision_m_c,
		fs_write_args_base_revision_null_m_c,
	} fs_write_args_base_revision_choice;
};

struct fs_write_args_force {
	bool fs_write_args_force;
};

struct fs_write_args {
	struct fs_root_id_t_r fs_write_args_root;
	struct zcbor_string fs_write_args_path;
	struct zcbor_string fs_write_args_bytes;
	struct fs_write_args_base_revision_r fs_write_args_base_revision;
	bool fs_write_args_base_revision_present;
	struct fs_write_args_force fs_write_args_force;
	bool fs_write_args_force_present;
};

struct request_fs_write {
	struct fs_write_args request_fs_write_FsWrite;
};

struct fs_search_query_regex {
	bool fs_search_query_regex;
};

struct fs_search_query_case_sensitive {
	bool fs_search_query_case_sensitive;
};

struct fs_search_query_max_results {
	uint32_t fs_search_query_max_results;
};

struct fs_search_query_page {
	uint32_t fs_search_query_page;
};

struct fs_search_query {
	struct zcbor_string fs_search_query_query;
	struct fs_search_query_regex fs_search_query_regex;
	bool fs_search_query_regex_present;
	struct fs_search_query_case_sensitive fs_search_query_case_sensitive;
	bool fs_search_query_case_sensitive_present;
	struct fs_search_query_max_results fs_search_query_max_results;
	bool fs_search_query_max_results_present;
	struct fs_search_query_page fs_search_query_page;
	bool fs_search_query_page_present;
};

struct request_fs_search {
	struct fs_root_id_t_r FsSearch_root;
	struct fs_search_query FsSearch_query;
};

struct fs_watch_after_args {
	struct fs_root_id_t_r fs_watch_after_args_root;
	struct zcbor_string fs_watch_after_args_dir;
	uint64_t fs_watch_after_args_after_seq;
	uint32_t fs_watch_after_args_max;
};

struct request_fs_watch_poll {
	struct fs_watch_after_args request_fs_watch_poll_FsWatchPoll;
};

struct request_blob_put {
	struct zcbor_string BlobPut_bytes;
};

struct byte_range {
	uint64_t byte_range_offset;
	uint64_t byte_range_len;
};

struct BlobGet_range_r {
	union {
		struct byte_range BlobGet_range_byte_range_m;
	};
	enum {
		BlobGet_range_byte_range_m_c,
		BlobGet_range_null_m_c,
	} BlobGet_range_choice;
};

struct request_blob_get {
	struct content_hash BlobGet_hash;
	struct BlobGet_range_r BlobGet_range;
	bool BlobGet_range_present;
};

struct request_blob_stat {
	struct content_hash BlobStat_hash;
};

struct fs_write_from_blob_args_base_revision_r {
	union {
		struct fs_revision fs_write_from_blob_args_base_revision_fs_revision_m;
	};
	enum {
		fs_write_from_blob_args_base_revision_fs_revision_m_c,
		fs_write_from_blob_args_base_revision_null_m_c,
	} fs_write_from_blob_args_base_revision_choice;
};

struct fs_write_from_blob_args_force {
	bool fs_write_from_blob_args_force;
};

struct fs_write_from_blob_args {
	struct fs_root_id_t_r fs_write_from_blob_args_root;
	struct zcbor_string fs_write_from_blob_args_path;
	struct content_hash fs_write_from_blob_args_hash;
	struct fs_write_from_blob_args_base_revision_r fs_write_from_blob_args_base_revision;
	bool fs_write_from_blob_args_base_revision_present;
	struct fs_write_from_blob_args_force fs_write_from_blob_args_force;
	bool fs_write_from_blob_args_force_present;
};

struct request_fs_write_from_blob {
	struct fs_write_from_blob_args request_fs_write_from_blob_FsWriteFromBlob;
};

struct request_user_create {
	struct zcbor_string UserCreate_username;
	struct zcbor_string UserCreate_password;
	struct zcbor_string UserCreate_roles_tstr[64];
	size_t UserCreate_roles_tstr_count;
};

struct request_user_disable {
	struct zcbor_string UserDisable_user_id;
	bool UserDisable_disabled;
};

struct request_user_set_roles {
	struct zcbor_string UserSetRoles_user_id;
	struct zcbor_string UserSetRoles_roles_tstr[64];
	size_t UserSetRoles_roles_tstr_count;
};

struct request_user_set_password {
	struct zcbor_string UserSetPassword_user_id;
	struct zcbor_string UserSetPassword_password;
};

struct request_session_revoke {
	struct zcbor_string SessionRevoke_user_id;
};

struct request_resource_grant_create {
	struct zcbor_string ResourceGrantCreate_user_id;
	struct zcbor_string ResourceGrantCreate_resource_kind;
	struct zcbor_string ResourceGrantCreate_resource_id;
	struct zcbor_string ResourceGrantCreate_capability;
};

struct ResourceGrantList_user_id_r {
	union {
		struct zcbor_string ResourceGrantList_user_id_tstr;
	};
	enum {
		ResourceGrantList_user_id_tstr_c,
		ResourceGrantList_user_id_null_m_c,
	} ResourceGrantList_user_id_choice;
};

struct request_resource_grant_list {
	struct ResourceGrantList_user_id_r ResourceGrantList_user_id;
	bool ResourceGrantList_user_id_present;
};

struct request_resource_grant_revoke {
	struct zcbor_string ResourceGrantRevoke_id;
};

struct feedback_kind_r {
	enum {
		feedback_kind_response_tstr_c,
		feedback_kind_app_tstr_c,
	} feedback_kind_choice;
};

struct feedback_target_trace_r {
	union {
		uint64_t feedback_target_trace_trace_id_m;
	};
	enum {
		feedback_target_trace_trace_id_m_c,
		feedback_target_trace_null_m_c,
	} feedback_target_trace_choice;
};

struct feedback_target {
	struct zcbor_string feedback_target_session;
	uint64_t feedback_target_cursor;
	struct feedback_target_trace_r feedback_target_trace;
	bool feedback_target_trace_present;
};

struct FeedbackSubmit_target_r {
	union {
		struct feedback_target FeedbackSubmit_target_feedback_target_m;
	};
	enum {
		FeedbackSubmit_target_feedback_target_m_c,
		FeedbackSubmit_target_null_m_c,
	} FeedbackSubmit_target_choice;
};

struct feedback_rating_r {
	enum {
		feedback_rating_up_tstr_c,
		feedback_rating_down_tstr_c,
	} feedback_rating_choice;
};

struct FeedbackSubmit_rating_r {
	union {
		struct feedback_rating_r FeedbackSubmit_rating_feedback_rating_m;
	};
	enum {
		FeedbackSubmit_rating_feedback_rating_m_c,
		FeedbackSubmit_rating_null_m_c,
	} FeedbackSubmit_rating_choice;
};

struct FeedbackSubmit_comment_r {
	union {
		struct zcbor_string FeedbackSubmit_comment_tstr;
	};
	enum {
		FeedbackSubmit_comment_tstr_c,
		FeedbackSubmit_comment_null_m_c,
	} FeedbackSubmit_comment_choice;
};

struct feedback_diagnostics_app_version_r {
	union {
		struct zcbor_string feedback_diagnostics_app_version_tstr;
	};
	enum {
		feedback_diagnostics_app_version_tstr_c,
		feedback_diagnostics_app_version_null_m_c,
	} feedback_diagnostics_app_version_choice;
};

struct feedback_diagnostics_os_r {
	union {
		struct zcbor_string feedback_diagnostics_os_tstr;
	};
	enum {
		feedback_diagnostics_os_tstr_c,
		feedback_diagnostics_os_null_m_c,
	} feedback_diagnostics_os_choice;
};

struct feedback_diagnostics {
	struct feedback_diagnostics_app_version_r feedback_diagnostics_app_version;
	bool feedback_diagnostics_app_version_present;
	struct feedback_diagnostics_os_r feedback_diagnostics_os;
	bool feedback_diagnostics_os_present;
};

struct FeedbackSubmit_diagnostics_r {
	union {
		struct feedback_diagnostics FeedbackSubmit_diagnostics_feedback_diagnostics_m;
	};
	enum {
		FeedbackSubmit_diagnostics_feedback_diagnostics_m_c,
		FeedbackSubmit_diagnostics_null_m_c,
	} FeedbackSubmit_diagnostics_choice;
};

struct request_feedback_submit {
	struct feedback_kind_r FeedbackSubmit_kind;
	struct FeedbackSubmit_target_r FeedbackSubmit_target;
	bool FeedbackSubmit_target_present;
	struct FeedbackSubmit_rating_r FeedbackSubmit_rating;
	bool FeedbackSubmit_rating_present;
	struct FeedbackSubmit_comment_r FeedbackSubmit_comment;
	bool FeedbackSubmit_comment_present;
	bool FeedbackSubmit_include_content;
	struct FeedbackSubmit_diagnostics_r FeedbackSubmit_diagnostics;
	bool FeedbackSubmit_diagnostics_present;
	struct zcbor_string FeedbackSubmit_surface;
};

struct request_telemetry_consent_set {
	bool TelemetryConsentSet_enabled;
};

struct request_crash_consent_set {
	bool CrashConsentSet_enabled;
};

struct saved_presence_name_r {
	union {
		struct zcbor_string saved_presence_name_tstr;
	};
	enum {
		saved_presence_name_tstr_c,
		saved_presence_name_null_m_c,
	} saved_presence_name_choice;
};

struct saved_presence_message_r {
	union {
		struct zcbor_string saved_presence_message_tstr;
	};
	enum {
		saved_presence_message_tstr_c,
		saved_presence_message_null_m_c,
	} saved_presence_message_choice;
};

struct saved_presence_emoji_r {
	union {
		struct zcbor_string saved_presence_emoji_tstr;
	};
	enum {
		saved_presence_emoji_tstr_c,
		saved_presence_emoji_null_m_c,
	} saved_presence_emoji_choice;
};

struct saved_presence_last_used_r {
	union {
		uint64_t saved_presence_last_used_uint64_m;
	};
	enum {
		saved_presence_last_used_uint64_m_c,
		saved_presence_last_used_null_m_c,
	} saved_presence_last_used_choice;
};

struct saved_presence_use_count {
	uint64_t saved_presence_use_count;
};

struct saved_presence {
	struct zcbor_string saved_presence_id;
	struct saved_presence_name_r saved_presence_name;
	bool saved_presence_name_present;
	struct presence_primitive_t_r saved_presence_primitive;
	struct saved_presence_message_r saved_presence_message;
	bool saved_presence_message_present;
	struct saved_presence_emoji_r saved_presence_emoji;
	bool saved_presence_emoji_present;
	struct saved_presence_last_used_r saved_presence_last_used;
	bool saved_presence_last_used_present;
	struct saved_presence_use_count saved_presence_use_count;
	bool saved_presence_use_count_present;
};

struct request_presence_save {
	struct saved_presence PresenceSave_presence;
};

struct request_presence_delete {
	struct zcbor_string PresenceDelete_id;
};

struct request_presence_set_active {
	struct zcbor_string PresenceSetActive_id;
};

struct file_transfer_direction_t_r {
	enum {
		file_transfer_direction_t_Send_tstr_c,
		file_transfer_direction_t_Receive_tstr_c,
	} file_transfer_direction_t_choice;
};

struct file_transfer_direction {
	struct file_transfer_direction_t_r file_transfer_direction;
};

struct file_transfer_state_t_r {
	enum {
		file_transfer_state_t_Unknown_tstr_c,
		file_transfer_state_t_Negotiating_tstr_c,
		file_transfer_state_t_Started_tstr_c,
		file_transfer_state_t_Finished_tstr_c,
		file_transfer_state_t_Failed_tstr_c,
	} file_transfer_state_t_choice;
};

struct file_transfer_state {
	struct file_transfer_state_t_r file_transfer_state;
};

struct file_transfer_remote_r {
	union {
		struct contact_info file_transfer_remote_contact_info_m;
	};
	enum {
		file_transfer_remote_contact_info_m_c,
		file_transfer_remote_null_m_c,
	} file_transfer_remote_choice;
};

struct file_transfer_initiator_r {
	union {
		struct contact_info file_transfer_initiator_contact_info_m;
	};
	enum {
		file_transfer_initiator_contact_info_m_c,
		file_transfer_initiator_null_m_c,
	} file_transfer_initiator_choice;
};

struct file_transfer_file_size {
	uint64_t file_transfer_file_size;
};

struct file_transfer_transferred {
	uint64_t file_transfer_transferred;
};

struct file_transfer_content_type_r {
	union {
		struct zcbor_string file_transfer_content_type_tstr;
	};
	enum {
		file_transfer_content_type_tstr_c,
		file_transfer_content_type_null_m_c,
	} file_transfer_content_type_choice;
};

struct file_transfer_message_r {
	union {
		struct zcbor_string file_transfer_message_tstr;
	};
	enum {
		file_transfer_message_tstr_c,
		file_transfer_message_null_m_c,
	} file_transfer_message_choice;
};

struct file_transfer_error_r {
	union {
		struct zcbor_string file_transfer_error_tstr;
	};
	enum {
		file_transfer_error_tstr_c,
		file_transfer_error_null_m_c,
	} file_transfer_error_choice;
};

struct file_transfer_source_r {
	union {
		struct zcbor_string file_transfer_source_tstr;
	};
	enum {
		file_transfer_source_tstr_c,
		file_transfer_source_null_m_c,
	} file_transfer_source_choice;
};

struct file_transfer {
	struct zcbor_string file_transfer_name;
	struct blob_ref file_transfer_blob;
	struct file_transfer_direction file_transfer_direction;
	bool file_transfer_direction_present;
	struct file_transfer_state file_transfer_state;
	bool file_transfer_state_present;
	struct file_transfer_remote_r file_transfer_remote;
	bool file_transfer_remote_present;
	struct file_transfer_initiator_r file_transfer_initiator;
	bool file_transfer_initiator_present;
	struct file_transfer_file_size file_transfer_file_size;
	bool file_transfer_file_size_present;
	struct file_transfer_transferred file_transfer_transferred;
	bool file_transfer_transferred_present;
	struct file_transfer_content_type_r file_transfer_content_type;
	bool file_transfer_content_type_present;
	struct file_transfer_message_r file_transfer_message;
	bool file_transfer_message_present;
	struct file_transfer_error_r file_transfer_error;
	bool file_transfer_error_present;
	struct file_transfer_source_r file_transfer_source;
	bool file_transfer_source_present;
};

struct FtSend_op_id_r {
	union {
		struct zcbor_string FtSend_op_id_tstr;
	};
	enum {
		FtSend_op_id_tstr_c,
		FtSend_op_id_null_m_c,
	} FtSend_op_id_choice;
};

struct request_ft_send {
	struct zcbor_string FtSend_transport;
	struct file_transfer FtSend_transfer;
	struct FtSend_op_id_r FtSend_op_id;
	bool FtSend_op_id_present;
};

struct request_ft_receive {
	struct zcbor_string FtReceive_transport;
	struct file_transfer FtReceive_transfer;
};

struct PersonList_since_rev_r {
	union {
		uint64_t PersonList_since_rev_uint64_m;
	};
	enum {
		PersonList_since_rev_uint64_m_c,
		PersonList_since_rev_null_m_c,
	} PersonList_since_rev_choice;
};

struct request_person_list {
	struct PersonList_since_rev_r PersonList_since_rev;
	bool PersonList_since_rev_present;
};

struct request_transport_settings {
	struct zcbor_string TransportSettings_transport;
};

struct TransportConfigure_op_id_r {
	union {
		struct zcbor_string TransportConfigure_op_id_tstr;
	};
	enum {
		TransportConfigure_op_id_tstr_c,
		TransportConfigure_op_id_null_m_c,
	} TransportConfigure_op_id_choice;
};

struct request_transport_configure {
	struct zcbor_string TransportConfigure_transport;
	struct account_settings_values TransportConfigure_settings;
	struct TransportConfigure_op_id_r TransportConfigure_op_id;
	bool TransportConfigure_op_id_present;
};

struct api_request_r {
	union {
		struct request_submit api_request_request_submit_m;
		struct request_submit_routed api_request_request_submit_routed_m;
		struct request_poll api_request_request_poll_m;
		struct request_respond api_request_request_respond_m;
		struct request_assign api_request_request_assign_m;
		struct request_session_create api_request_request_session_create_m;
		struct request_cancel api_request_request_cancel_m;
		struct request_tree api_request_request_tree_m;
		struct request_unit api_request_request_unit_m;
		struct request_unit_events api_request_request_unit_events_m;
		struct request_unit_outbound api_request_request_unit_outbound_m;
		struct request_session_history api_request_request_session_history_m;
		struct request_subscribe api_request_request_subscribe_m;
		struct request_events_since api_request_request_events_since_m;
		struct request_delivery_targets api_request_request_delivery_targets_m;
		struct request_delivery_sessions api_request_request_delivery_sessions_m;
		struct request_handover api_request_request_handover_m;
		struct request_record_meta api_request_request_record_meta_m;
		struct request_unit_history api_request_request_unit_history_m;
		struct request_pause api_request_request_pause_m;
		struct request_resume api_request_request_resume_m;
		struct request_scale api_request_request_scale_m;
		struct request_set_session_model api_request_request_set_session_model_m;
		struct request_set_session_mode api_request_request_set_session_mode_m;
		struct request_set_session_overlay api_request_request_set_session_overlay_m;
		struct request_approvals_pending api_request_request_approvals_pending_m;
		struct request_approval_decide api_request_request_approval_decide_m;
		struct request_fingerprint_list api_request_request_fingerprint_list_m;
		struct request_fingerprint_revoke api_request_request_fingerprint_revoke_m;
		struct request_profile_get api_request_request_profile_get_m;
		struct request_profile_create api_request_request_profile_create_m;
		struct request_profile_update api_request_request_profile_update_m;
		struct request_profile_delete api_request_request_profile_delete_m;
		struct request_profile_select api_request_request_profile_select_m;
		struct request_profile_clone api_request_request_profile_clone_m;
		struct request_profile_export api_request_request_profile_export_m;
		struct request_profile_import api_request_request_profile_import_m;
		struct request_profile_history api_request_request_profile_history_m;
		struct request_profile_at api_request_request_profile_at_m;
		struct request_profile_revert api_request_request_profile_revert_m;
		struct request_soul_get api_request_request_soul_get_m;
		struct request_soul_set api_request_request_soul_set_m;
		struct request_skill_history api_request_request_skill_history_m;
		struct request_skill_at api_request_request_skill_at_m;
		struct request_skill_revert api_request_request_skill_revert_m;
		struct request_curator_list api_request_request_curator_list_m;
		struct request_curator_pin api_request_request_curator_pin_m;
		struct request_curator_unpin api_request_request_curator_unpin_m;
		struct request_curator_archive api_request_request_curator_archive_m;
		struct request_curator_restore api_request_request_curator_restore_m;
		struct request_curator_run api_request_request_curator_run_m;
		struct request_credential_set api_request_request_credential_set_m;
		struct request_credential_remove api_request_request_credential_remove_m;
		struct request_credential_set_label api_request_request_credential_set_label_m;
		struct request_model_search api_request_request_model_search_m;
		struct request_model_files api_request_request_model_files_m;
		struct request_model_download api_request_request_model_download_m;
		struct request_model_cancel api_request_request_model_cancel_m;
		struct request_model_pause api_request_request_model_pause_m;
		struct request_model_resume api_request_request_model_resume_m;
		struct request_model_delete api_request_request_model_delete_m;
		struct request_model_activate api_request_request_model_activate_m;
		struct request_model_recommend api_request_request_model_recommend_m;
		struct request_model_quantize api_request_request_model_quantize_m;
		struct request_model_inspect api_request_request_model_inspect_m;
		struct request_swarm_run_detail api_request_request_swarm_run_detail_m;
		struct request_swarm_join api_request_request_swarm_join_m;
		struct request_swarm_leave api_request_request_swarm_leave_m;
		struct request_swarm_set_policy api_request_request_swarm_set_policy_m;
		struct request_models api_request_request_models_m;
		struct request_model_current api_request_request_model_current_m;
		struct request_provider_models api_request_request_provider_models_m;
		struct request_custom_provider_set api_request_request_custom_provider_set_m;
		struct request_custom_provider_remove api_request_request_custom_provider_remove_m;
		struct request_auth_begin api_request_request_auth_begin_m;
		struct request_auth_step api_request_request_auth_step_m;
		struct request_auth_complete api_request_request_auth_complete_m;
		struct request_auth_cancel api_request_request_auth_cancel_m;
		struct request_checkpoint_list api_request_request_checkpoint_list_m;
		struct request_checkpoint_rewind api_request_request_checkpoint_rewind_m;
		struct request_sessions_query api_request_request_sessions_query_m;
		struct request_session_get api_request_request_session_get_m;
		struct request_session_search api_request_request_session_search_m;
		struct request_session_recap api_request_request_session_recap_m;
		struct request_session_update_meta api_request_request_session_update_meta_m;
		struct request_rewind api_request_request_rewind_m;
		struct request_agent_register api_request_request_agent_register_m;
		struct request_agent_remove api_request_request_agent_remove_m;
		struct request_skill_get api_request_request_skill_get_m;
		struct request_skill_put api_request_request_skill_put_m;
		struct request_provider_register api_request_request_provider_register_m;
		struct request_tool_register api_request_request_tool_register_m;
		struct request_tool_set_enabled api_request_request_tool_set_enabled_m;
		struct request_command_invoke api_request_request_command_invoke_m;
		struct request_config_set api_request_request_config_set_m;
		struct request_gateway_set api_request_request_gateway_set_m;
		struct request_cron_create api_request_request_cron_create_m;
		struct request_cron_update api_request_request_cron_update_m;
		struct request_cron_delete api_request_request_cron_delete_m;
		struct request_cron_trigger api_request_request_cron_trigger_m;
		struct request_cron_runs api_request_request_cron_runs_m;
		struct request_cron_pause api_request_request_cron_pause_m;
		struct request_cron_accept_suggestion api_request_request_cron_accept_suggestion_m;
		struct request_cron_dismiss_suggestion api_request_request_cron_dismiss_suggestion_m;
		struct request_routing_list_chats api_request_request_routing_list_chats_m;
		struct request_routing_get api_request_request_routing_get_m;
		struct request_routing_set api_request_request_routing_set_m;
		struct request_routing_bind_chat api_request_request_routing_bind_chat_m;
		struct request_routing_unbind_chat api_request_request_routing_unbind_chat_m;
		struct request_transport_rooms api_request_request_transport_rooms_m;
		struct request_transport_disconnect api_request_request_transport_disconnect_m;
		struct request_transport_remove api_request_request_transport_remove_m;
		struct request_transport_connect api_request_request_transport_connect_m;
		struct request_transport_set_enabled api_request_request_transport_set_enabled_m;
		struct request_transport_set_label api_request_request_transport_set_label_m;
		struct request_conv_list api_request_request_conv_list_m;
		struct request_conv_get api_request_request_conv_get_m;
		struct request_conv_create_details api_request_request_conv_create_details_m;
		struct request_conv_create api_request_request_conv_create_m;
		struct request_conv_join_details api_request_request_conv_join_details_m;
		struct request_conv_join api_request_request_conv_join_m;
		struct request_conv_leave api_request_request_conv_leave_m;
		struct request_conv_send api_request_request_conv_send_m;
		struct request_conv_set_topic api_request_request_conv_set_topic_m;
		struct request_conv_set_title api_request_request_conv_set_title_m;
		struct request_conv_set_description api_request_request_conv_set_description_m;
		struct request_conv_delete api_request_request_conv_delete_m;
		struct request_conv_history api_request_request_conv_history_m;
		struct request_member_invite api_request_request_member_invite_m;
		struct request_member_remove api_request_request_member_remove_m;
		struct request_member_ban api_request_request_member_ban_m;
		struct request_member_set_role api_request_request_member_set_role_m;
		struct request_contact_get_profile api_request_request_contact_get_profile_m;
		struct request_contact_set_alias api_request_request_contact_set_alias_m;
		struct request_contact_action_menu api_request_request_contact_action_menu_m;
		struct request_directory_search api_request_request_directory_search_m;
		struct request_roster_list api_request_request_roster_list_m;
		struct request_roster_add api_request_request_roster_add_m;
		struct request_roster_update api_request_request_roster_update_m;
		struct request_roster_remove api_request_request_roster_remove_m;
		struct request_fs_list api_request_request_fs_list_m;
		struct request_fs_stat api_request_request_fs_stat_m;
		struct request_fs_read api_request_request_fs_read_m;
		struct request_fs_write api_request_request_fs_write_m;
		struct request_fs_search api_request_request_fs_search_m;
		struct request_fs_watch_poll api_request_request_fs_watch_poll_m;
		struct request_blob_put api_request_request_blob_put_m;
		struct request_blob_get api_request_request_blob_get_m;
		struct request_blob_stat api_request_request_blob_stat_m;
		struct request_fs_write_from_blob api_request_request_fs_write_from_blob_m;
		struct request_user_create api_request_request_user_create_m;
		struct request_user_disable api_request_request_user_disable_m;
		struct request_user_set_roles api_request_request_user_set_roles_m;
		struct request_user_set_password api_request_request_user_set_password_m;
		struct request_session_revoke api_request_request_session_revoke_m;
		struct request_resource_grant_create api_request_request_resource_grant_create_m;
		struct request_resource_grant_list api_request_request_resource_grant_list_m;
		struct request_resource_grant_revoke api_request_request_resource_grant_revoke_m;
		struct request_feedback_submit api_request_request_feedback_submit_m;
		struct request_telemetry_consent_set api_request_request_telemetry_consent_set_m;
		struct request_crash_consent_set api_request_request_crash_consent_set_m;
		struct request_presence_save api_request_request_presence_save_m;
		struct request_presence_delete api_request_request_presence_delete_m;
		struct request_presence_set_active api_request_request_presence_set_active_m;
		struct request_ft_send api_request_request_ft_send_m;
		struct request_ft_receive api_request_request_ft_receive_m;
		struct request_person_list api_request_request_person_list_m;
		struct request_transport_settings api_request_request_transport_settings_m;
		struct request_transport_configure api_request_request_transport_configure_m;
	};
	enum {
		api_request_request_submit_m_c,
		api_request_request_submit_routed_m_c,
		api_request_request_poll_m_c,
		api_request_request_respond_m_c,
		api_request_request_health_m_c,
		api_request_request_stats_m_c,
		api_request_request_telemetry_m_c,
		api_request_request_sessions_m_c,
		api_request_request_assign_m_c,
		api_request_request_session_create_m_c,
		api_request_request_cancel_m_c,
		api_request_request_fleet_m_c,
		api_request_request_tree_m_c,
		api_request_request_unit_m_c,
		api_request_request_unit_events_m_c,
		api_request_request_unit_outbound_m_c,
		api_request_request_session_history_m_c,
		api_request_request_subscribe_m_c,
		api_request_request_events_since_m_c,
		api_request_request_delivery_targets_m_c,
		api_request_request_delivery_sessions_m_c,
		api_request_request_handover_m_c,
		api_request_request_record_meta_m_c,
		api_request_request_unit_history_m_c,
		api_request_request_pause_m_c,
		api_request_request_resume_m_c,
		api_request_request_scale_m_c,
		api_request_request_verifying_key_m_c,
		api_request_request_set_session_model_m_c,
		api_request_request_set_session_mode_m_c,
		api_request_request_set_session_overlay_m_c,
		api_request_request_approvals_pending_m_c,
		api_request_request_approval_decide_m_c,
		api_request_request_fingerprint_list_m_c,
		api_request_request_fingerprint_revoke_m_c,
		api_request_request_profile_list_m_c,
		api_request_request_profile_get_m_c,
		api_request_request_profile_create_m_c,
		api_request_request_profile_update_m_c,
		api_request_request_profile_delete_m_c,
		api_request_request_profile_select_m_c,
		api_request_request_profile_clone_m_c,
		api_request_request_profile_export_m_c,
		api_request_request_profile_import_m_c,
		api_request_request_profile_history_m_c,
		api_request_request_profile_at_m_c,
		api_request_request_profile_revert_m_c,
		api_request_request_soul_get_m_c,
		api_request_request_soul_set_m_c,
		api_request_request_skill_history_m_c,
		api_request_request_skill_at_m_c,
		api_request_request_skill_revert_m_c,
		api_request_request_curator_list_m_c,
		api_request_request_curator_pin_m_c,
		api_request_request_curator_unpin_m_c,
		api_request_request_curator_archive_m_c,
		api_request_request_curator_restore_m_c,
		api_request_request_curator_run_m_c,
		api_request_request_credential_set_m_c,
		api_request_request_credential_list_m_c,
		api_request_request_credential_remove_m_c,
		api_request_request_credential_set_label_m_c,
		api_request_request_model_search_m_c,
		api_request_request_model_files_m_c,
		api_request_request_model_download_m_c,
		api_request_request_model_downloads_m_c,
		api_request_request_model_cancel_m_c,
		api_request_request_model_pause_m_c,
		api_request_request_model_resume_m_c,
		api_request_request_model_catalog_m_c,
		api_request_request_model_delete_m_c,
		api_request_request_model_activate_m_c,
		api_request_request_model_recommend_m_c,
		api_request_request_model_quantize_m_c,
		api_request_request_model_quantizes_m_c,
		api_request_request_model_inspect_m_c,
		api_request_request_swarm_run_list_m_c,
		api_request_request_swarm_run_detail_m_c,
		api_request_request_swarm_join_m_c,
		api_request_request_swarm_leave_m_c,
		api_request_request_swarm_set_policy_m_c,
		api_request_request_swarm_hardware_report_m_c,
		api_request_request_models_m_c,
		api_request_request_model_current_m_c,
		api_request_request_provider_catalog_m_c,
		api_request_request_provider_models_m_c,
		api_request_request_custom_provider_list_m_c,
		api_request_request_custom_provider_set_m_c,
		api_request_request_custom_provider_remove_m_c,
		api_request_request_auth_begin_m_c,
		api_request_request_auth_step_m_c,
		api_request_request_auth_complete_m_c,
		api_request_request_auth_cancel_m_c,
		api_request_request_auth_providers_m_c,
		api_request_request_checkpoint_list_m_c,
		api_request_request_checkpoint_rewind_m_c,
		api_request_request_sessions_query_m_c,
		api_request_request_session_get_m_c,
		api_request_request_session_search_m_c,
		api_request_request_session_recap_m_c,
		api_request_request_session_update_meta_m_c,
		api_request_request_rewind_m_c,
		api_request_request_agent_discover_m_c,
		api_request_request_agent_catalog_m_c,
		api_request_request_agent_register_m_c,
		api_request_request_agent_remove_m_c,
		api_request_request_skill_get_m_c,
		api_request_request_skill_put_m_c,
		api_request_request_provider_list_m_c,
		api_request_request_provider_register_m_c,
		api_request_request_tool_list_m_c,
		api_request_request_tool_register_m_c,
		api_request_request_tool_set_enabled_m_c,
		api_request_request_command_list_m_c,
		api_request_request_command_invoke_m_c,
		api_request_request_caps_m_c,
		api_request_request_config_get_m_c,
		api_request_request_config_set_m_c,
		api_request_request_gateway_get_m_c,
		api_request_request_gateway_set_m_c,
		api_request_request_cron_list_m_c,
		api_request_request_cron_create_m_c,
		api_request_request_cron_update_m_c,
		api_request_request_cron_delete_m_c,
		api_request_request_cron_trigger_m_c,
		api_request_request_cron_runs_m_c,
		api_request_request_cron_pause_m_c,
		api_request_request_cron_suggestions_m_c,
		api_request_request_cron_accept_suggestion_m_c,
		api_request_request_cron_dismiss_suggestion_m_c,
		api_request_request_routing_list_chats_m_c,
		api_request_request_routing_get_m_c,
		api_request_request_routing_set_m_c,
		api_request_request_routing_bind_chat_m_c,
		api_request_request_routing_unbind_chat_m_c,
		api_request_request_transport_rooms_m_c,
		api_request_request_transport_adapters_m_c,
		api_request_request_transport_instances_m_c,
		api_request_request_transport_disconnect_m_c,
		api_request_request_transport_remove_m_c,
		api_request_request_transport_connect_m_c,
		api_request_request_transport_set_enabled_m_c,
		api_request_request_transport_set_label_m_c,
		api_request_request_conv_list_m_c,
		api_request_request_conv_get_m_c,
		api_request_request_conv_create_details_m_c,
		api_request_request_conv_create_m_c,
		api_request_request_conv_join_details_m_c,
		api_request_request_conv_join_m_c,
		api_request_request_conv_leave_m_c,
		api_request_request_conv_send_m_c,
		api_request_request_conv_set_topic_m_c,
		api_request_request_conv_set_title_m_c,
		api_request_request_conv_set_description_m_c,
		api_request_request_conv_delete_m_c,
		api_request_request_conv_history_m_c,
		api_request_request_member_invite_m_c,
		api_request_request_member_remove_m_c,
		api_request_request_member_ban_m_c,
		api_request_request_member_set_role_m_c,
		api_request_request_contact_get_profile_m_c,
		api_request_request_contact_set_alias_m_c,
		api_request_request_contact_action_menu_m_c,
		api_request_request_directory_search_m_c,
		api_request_request_roster_list_m_c,
		api_request_request_roster_add_m_c,
		api_request_request_roster_update_m_c,
		api_request_request_roster_remove_m_c,
		api_request_request_fs_roots_m_c,
		api_request_request_fs_list_m_c,
		api_request_request_fs_stat_m_c,
		api_request_request_fs_read_m_c,
		api_request_request_fs_write_m_c,
		api_request_request_fs_search_m_c,
		api_request_request_fs_watch_poll_m_c,
		api_request_request_blob_put_m_c,
		api_request_request_blob_get_m_c,
		api_request_request_blob_stat_m_c,
		api_request_request_fs_write_from_blob_m_c,
		api_request_request_user_create_m_c,
		api_request_request_user_list_m_c,
		api_request_request_user_disable_m_c,
		api_request_request_user_set_roles_m_c,
		api_request_request_user_set_password_m_c,
		api_request_request_role_list_m_c,
		api_request_request_who_am_i_m_c,
		api_request_request_session_revoke_m_c,
		api_request_request_resource_grant_create_m_c,
		api_request_request_resource_grant_list_m_c,
		api_request_request_resource_grant_revoke_m_c,
		api_request_request_feedback_submit_m_c,
		api_request_request_telemetry_consent_get_m_c,
		api_request_request_telemetry_consent_set_m_c,
		api_request_request_crash_consent_get_m_c,
		api_request_request_crash_consent_set_m_c,
		api_request_request_presence_list_m_c,
		api_request_request_presence_save_m_c,
		api_request_request_presence_delete_m_c,
		api_request_request_presence_set_active_m_c,
		api_request_request_notification_list_m_c,
		api_request_request_ft_send_m_c,
		api_request_request_ft_receive_m_c,
		api_request_request_person_list_m_c,
		api_request_request_transport_settings_m_c,
		api_request_request_transport_configure_m_c,
		api_request_request_bootstrap_m_c,
	} api_request_choice;
};

struct response_routed {
	struct zcbor_string Routed_session;
};

struct response_session_created {
	struct zcbor_string SessionCreated_session;
};

struct completion_source_process {
	struct zcbor_string completion_source_process_Process;
};

struct completion_source_delegation {
	struct zcbor_string completion_source_delegation_Delegation;
};

struct completion_source_r {
	union {
		struct completion_source_process completion_source_process_m;
		struct completion_source_delegation completion_source_delegation_m;
	};
	enum {
		completion_source_process_m_c,
		completion_source_delegation_m_c,
	} completion_source_choice;
};

struct turn_trigger_background {
	struct completion_source_r BackgroundCompletion_source;
};

struct turn_trigger_scheduled {
	struct zcbor_string Scheduled_job;
};

struct turn_trigger_r {
	union {
		struct turn_trigger_background turn_trigger_background_m;
		struct turn_trigger_scheduled turn_trigger_scheduled_m;
	};
	enum {
		turn_trigger_User_tstr_c,
		turn_trigger_Steer_tstr_c,
		turn_trigger_background_m_c,
		turn_trigger_scheduled_m_c,
	} turn_trigger_choice;
};

struct agent_event_turn_started {
	uint64_t TurnStarted_seq;
	struct turn_trigger_r TurnStarted_trigger;
};

struct agent_event_text_delta {
	uint64_t TextDelta_seq;
	struct zcbor_string TextDelta_text;
};

struct agent_event_reasoning_delta {
	uint64_t ReasoningDelta_seq;
	struct zcbor_string ReasoningDelta_text;
};

struct agent_event_content_delta {
	uint64_t ContentDelta_seq;
	struct zcbor_string ContentDelta_kind;
	struct zcbor_string ContentDelta_body;
};

struct tool_detail {
	struct zcbor_string tool_detail_kind;
	struct zcbor_string tool_detail_body;
};

struct tool_call_view_detail_r {
	union {
		struct tool_detail tool_call_view_detail_tool_detail_m;
	};
	enum {
		tool_call_view_detail_tool_detail_m_c,
		tool_call_view_detail_null_m_c,
	} tool_call_view_detail_choice;
};

struct tool_call_view {
	struct zcbor_string tool_call_view_call_id;
	struct zcbor_string tool_call_view_name;
	struct zcbor_string tool_call_view_args_summary;
	struct tool_call_view_detail_r tool_call_view_detail;
	bool tool_call_view_detail_present;
};

struct agent_event_tool_started {
	uint64_t ToolStarted_seq;
	struct tool_call_view ToolStarted_call;
};

struct tool_result_view_detail_r {
	union {
		struct tool_detail tool_result_view_detail_tool_detail_m;
	};
	enum {
		tool_result_view_detail_tool_detail_m_c,
		tool_result_view_detail_null_m_c,
	} tool_result_view_detail_choice;
};

struct tool_result_view {
	struct zcbor_string tool_result_view_call_id;
	bool tool_result_view_ok;
	struct zcbor_string tool_result_view_summary;
	struct tool_result_view_detail_r tool_result_view_detail;
	bool tool_result_view_detail_present;
};

struct agent_event_tool_finished {
	uint64_t ToolFinished_seq;
	struct tool_result_view ToolFinished_result;
};

struct usage_delta {
	uint64_t usage_delta_input_tokens;
	uint64_t usage_delta_output_tokens;
	uint32_t usage_delta_api_calls;
	uint64_t usage_delta_cache_read_tokens;
	uint64_t usage_delta_cache_write_tokens;
	uint64_t usage_delta_reasoning_tokens;
	uint64_t usage_delta_cost_micros;
};

struct agent_event_usage {
	uint64_t Usage_seq;
	struct usage_delta Usage_delta;
};

struct context_status {
	uint64_t context_status_used_tokens;
	union {
		uint64_t context_status_max_tokens_uint64_m;
	};
	enum {
		context_status_max_tokens_uint64_m_c,
		context_status_max_tokens_null_m_c,
	} context_status_max_tokens_choice;
	union {
		uint64_t context_status_budget_tokens_uint64_m;
	};
	enum {
		context_status_budget_tokens_uint64_m_c,
		context_status_budget_tokens_null_m_c,
	} context_status_budget_tokens_choice;
	bool context_status_compacted;
	uint32_t context_status_dropped_turns;
};

struct agent_event_context {
	uint64_t Context_seq;
	struct context_status Context_status;
};

struct rate_limit_snapshot {
	union {
		uint64_t rate_limit_snapshot_remaining_uint64_m;
	};
	enum {
		rate_limit_snapshot_remaining_uint64_m_c,
		rate_limit_snapshot_remaining_null_m_c,
	} rate_limit_snapshot_remaining_choice;
	union {
		uint64_t rate_limit_snapshot_limit_uint64_m;
	};
	enum {
		rate_limit_snapshot_limit_uint64_m_c,
		rate_limit_snapshot_limit_null_m_c,
	} rate_limit_snapshot_limit_choice;
	union {
		uint64_t rate_limit_snapshot_reset_ms_uint64_m;
	};
	enum {
		rate_limit_snapshot_reset_ms_uint64_m_c,
		rate_limit_snapshot_reset_ms_null_m_c,
	} rate_limit_snapshot_reset_ms_choice;
};

struct agent_event_rate_limit {
	uint64_t RateLimit_seq;
	struct rate_limit_snapshot RateLimit_snapshot;
};

struct end_reason_r {
	enum {
		end_reason_Completed_tstr_c,
		end_reason_Suspended_tstr_c,
		end_reason_Interrupted_tstr_c,
		end_reason_BudgetExhausted_tstr_c,
		end_reason_NoProgress_tstr_c,
		end_reason_Failed_tstr_c,
	} end_reason_choice;
};

struct foreign_stage_r {
	enum {
		foreign_stage_Spawn_tstr_c,
		foreign_stage_Handshake_tstr_c,
		foreign_stage_Turn_tstr_c,
		foreign_stage_Unknown_tstr_c,
	} foreign_stage_choice;
};

struct foreign_failure_agent_r {
	union {
		struct zcbor_string foreign_failure_agent_tstr;
	};
	enum {
		foreign_failure_agent_tstr_c,
		foreign_failure_agent_null_m_c,
	} foreign_failure_agent_choice;
};

struct foreign_failure {
	struct foreign_stage_r foreign_failure_stage;
	struct foreign_failure_agent_r foreign_failure_agent;
	bool foreign_failure_agent_present;
};

struct turn_summary_failure_r {
	union {
		struct foreign_failure turn_summary_failure_foreign_failure_m;
	};
	enum {
		turn_summary_failure_foreign_failure_m_c,
		turn_summary_failure_null_m_c,
	} turn_summary_failure_choice;
};

struct turn_summary {
	struct end_reason_r turn_summary_end_reason;
	union {
		struct zcbor_string turn_summary_final_text_tstr;
	};
	enum {
		turn_summary_final_text_tstr_c,
		turn_summary_final_text_null_m_c,
	} turn_summary_final_text_choice;
	struct usage_delta turn_summary_usage;
	struct turn_summary_failure_r turn_summary_failure;
	bool turn_summary_failure_present;
};

struct agent_event_turn_finished {
	uint64_t TurnFinished_seq;
	struct turn_summary TurnFinished_summary;
};

struct agent_event_error {
	uint64_t Error_seq;
	struct zcbor_string Error_failure;
};

struct agent_event_steered {
	uint64_t Steered_seq;
	uint64_t Steered_request_id;
	bool Steered_accepted;
};

struct conv_turn_view {
	struct zcbor_string conv_turn_view_role;
	struct zcbor_string conv_turn_view_text;
	struct zcbor_string conv_turn_view_tools_tstr[64];
	size_t conv_turn_view_tools_tstr_count;
};

struct conv_view {
	uint64_t conv_view_epoch;
	struct conv_turn_view conv_view_turns_conv_turn_view_m[64];
	size_t conv_view_turns_conv_turn_view_m_count;
	struct zcbor_string conv_view_waiting_for_tstr[64];
	size_t conv_view_waiting_for_tstr_count;
};

struct agent_event_snapshot {
	uint64_t Snapshot_seq;
	uint64_t Snapshot_request_id;
	struct conv_view Snapshot_view;
};

struct agent_event_rewound {
	uint64_t Rewound_seq;
	uint64_t Rewound_request_id;
	uint64_t Rewound_to_cursor;
	uint64_t Rewound_epoch;
};

struct agent_event_r {
	union {
		struct agent_event_turn_started agent_event_turn_started_m;
		struct agent_event_text_delta agent_event_text_delta_m;
		struct agent_event_reasoning_delta agent_event_reasoning_delta_m;
		struct agent_event_content_delta agent_event_content_delta_m;
		struct agent_event_tool_started agent_event_tool_started_m;
		struct agent_event_tool_finished agent_event_tool_finished_m;
		struct agent_event_usage agent_event_usage_m;
		struct agent_event_context agent_event_context_m;
		struct agent_event_rate_limit agent_event_rate_limit_m;
		struct agent_event_turn_finished agent_event_turn_finished_m;
		struct agent_event_error agent_event_error_m;
		struct agent_event_steered agent_event_steered_m;
		struct agent_event_snapshot agent_event_snapshot_m;
		struct agent_event_rewound agent_event_rewound_m;
	};
	enum {
		agent_event_turn_started_m_c,
		agent_event_text_delta_m_c,
		agent_event_reasoning_delta_m_c,
		agent_event_content_delta_m_c,
		agent_event_tool_started_m_c,
		agent_event_tool_finished_m_c,
		agent_event_usage_m_c,
		agent_event_context_m_c,
		agent_event_rate_limit_m_c,
		agent_event_turn_finished_m_c,
		agent_event_error_m_c,
		agent_event_steered_m_c,
		agent_event_snapshot_m_c,
		agent_event_rewound_m_c,
	} agent_event_choice;
};

struct outbound_event {
	struct agent_event_r outbound_event_Event;
};

struct Approval_allow_permanent_offered {
	bool Approval_allow_permanent_offered;
};

struct host_request_kind_approval {
	struct zcbor_string Approval_prompt;
	struct Approval_allow_permanent_offered Approval_allow_permanent_offered;
	bool Approval_allow_permanent_offered_present;
};

struct host_request_kind_input {
	struct zcbor_string Input_prompt;
};

struct host_request_kind_choice {
	struct zcbor_string Choice_prompt;
	struct zcbor_string Choice_options_tstr[64];
	size_t Choice_options_tstr_count;
};

struct host_request_kind_delegate {
	struct zcbor_string Delegate_label;
	struct budget Delegate_budget;
};

struct spawn_spec {
	struct zcbor_string spawn_spec_kind;
};

struct host_request_kind_spawn {
	struct spawn_spec Spawn_spec;
};

struct host_request_kind_t_r {
	union {
		struct host_request_kind_approval host_request_kind_t_host_request_kind_approval_m;
		struct host_request_kind_input host_request_kind_t_host_request_kind_input_m;
		struct host_request_kind_choice host_request_kind_t_host_request_kind_choice_m;
		struct host_request_kind_delegate host_request_kind_t_host_request_kind_delegate_m;
		struct host_request_kind_spawn host_request_kind_t_host_request_kind_spawn_m;
	};
	enum {
		host_request_kind_t_host_request_kind_approval_m_c,
		host_request_kind_t_host_request_kind_input_m_c,
		host_request_kind_t_host_request_kind_choice_m_c,
		host_request_kind_t_host_request_kind_delegate_m_c,
		host_request_kind_t_host_request_kind_spawn_m_c,
	} host_request_kind_t_choice;
};

struct host_request {
	uint64_t host_request_request_id;
	struct host_request_kind_t_r host_request_kind;
};

struct outbound_request {
	struct host_request outbound_request_Request;
};

struct outbound_r {
	union {
		struct outbound_event outbound_event_m;
		struct outbound_request outbound_request_m;
	};
	enum {
		outbound_event_m_c,
		outbound_request_m_c,
	} outbound_choice;
};

struct response_drained {
	struct outbound_r response_drained_Drained_outbound_m[64];
	size_t response_drained_Drained_outbound_m_count;
};

struct service_health {
	struct zcbor_string service_health_name;
	bool service_health_ok;
	uint32_t service_health_restarts;
	union {
		struct zcbor_string service_health_detail_tstr;
	};
	enum {
		service_health_detail_tstr_c,
		service_health_detail_null_m_c,
	} service_health_detail_choice;
};

struct health_report {
	bool health_report_all_ok;
	struct service_health health_report_services_service_health_m[64];
	size_t health_report_services_service_health_m_count;
};

struct response_health {
	struct health_report response_health_Health;
};

struct stats_report {
	uint64_t stats_report_pending_jobs;
	uint64_t stats_report_pending_wakes;
	uint64_t stats_report_sessions;
	uint64_t stats_report_active;
	struct usage_delta stats_report_usage;
};

struct response_stats {
	struct stats_report response_stats_Stats;
};

struct telemetry_dump {
	struct usage_delta telemetry_dump_usage;
	uint64_t telemetry_dump_events;
	bool telemetry_dump_healthy;
	uint64_t telemetry_dump_pending_jobs;
	uint64_t telemetry_dump_pending_wakes;
	uint64_t telemetry_dump_sessions;
	uint64_t telemetry_dump_active;
};

struct response_telemetry {
	struct telemetry_dump response_telemetry_Telemetry;
};

struct session_state_suspended {
	struct zcbor_string Suspended_job_id;
};

struct session_state_r {
	union {
		struct session_state_suspended session_state_suspended_m;
	};
	enum {
		session_state_Active_tstr_c,
		session_state_suspended_m_c,
		session_state_Ready_tstr_c,
		session_state_Completed_tstr_c,
		session_state_Unknown_tstr_c,
	} session_state_choice;
};

struct session_info_rewindable {
	bool session_info_rewindable;
};

struct session_info_bound_profile_r {
	union {
		struct zcbor_string session_info_bound_profile_profile_ref_m;
	};
	enum {
		session_info_bound_profile_profile_ref_m_c,
		session_info_bound_profile_null_m_c,
	} session_info_bound_profile_choice;
};

struct session_info_title_r {
	union {
		struct zcbor_string session_info_title_tstr;
	};
	enum {
		session_info_title_tstr_c,
		session_info_title_null_m_c,
	} session_info_title_choice;
};

struct session_info_last_activity_ms_r {
	union {
		uint64_t session_info_last_activity_ms_uint64_m;
	};
	enum {
		session_info_last_activity_ms_uint64_m_c,
		session_info_last_activity_ms_null_m_c,
	} session_info_last_activity_ms_choice;
};

struct lifecycle_r {
	enum {
		lifecycle_Durable_tstr_c,
		lifecycle_Live_tstr_c,
	} lifecycle_choice;
};

struct session_info_lifecycle {
	struct lifecycle_r session_info_lifecycle;
};

struct session_role_r {
	enum {
		session_role_Primary_tstr_c,
		session_role_ManagedChild_tstr_c,
		session_role_EphemeralSubagent_tstr_c,
	} session_role_choice;
};

struct session_info_role {
	struct session_role_r session_info_role;
};

struct session_info_parent_r {
	union {
		struct zcbor_string session_info_parent_session_id_m;
	};
	enum {
		session_info_parent_session_id_m_c,
		session_info_parent_null_m_c,
	} session_info_parent_choice;
};

struct session_info_pinned {
	bool session_info_pinned;
};

struct session_info_archived {
	bool session_info_archived;
};

struct session_info {
	struct zcbor_string session_info_session;
	struct session_state_r session_info_state;
	struct session_info_rewindable session_info_rewindable;
	bool session_info_rewindable_present;
	struct session_info_bound_profile_r session_info_bound_profile;
	bool session_info_bound_profile_present;
	struct session_info_title_r session_info_title;
	bool session_info_title_present;
	struct session_info_last_activity_ms_r session_info_last_activity_ms;
	bool session_info_last_activity_ms_present;
	struct session_info_lifecycle session_info_lifecycle;
	bool session_info_lifecycle_present;
	struct session_info_role session_info_role;
	bool session_info_role_present;
	struct session_info_parent_r session_info_parent;
	bool session_info_parent_present;
	struct session_info_pinned session_info_pinned;
	bool session_info_pinned_present;
	struct session_info_archived session_info_archived;
	bool session_info_archived_present;
};

struct response_sessions {
	struct session_info response_sessions_Sessions_session_info_m[64];
	size_t response_sessions_Sessions_session_info_m_count;
};

struct approval_info_path_r {
	union {
		struct zcbor_string approval_info_path_tstr;
	};
	enum {
		approval_info_path_tstr_c,
		approval_info_path_null_m_c,
	} approval_info_path_choice;
};

struct approval_info_fingerprint_r {
	union {
		struct zcbor_string approval_info_fingerprint_tstr;
	};
	enum {
		approval_info_fingerprint_tstr_c,
		approval_info_fingerprint_null_m_c,
	} approval_info_fingerprint_choice;
};

struct approval_info_detail_r {
	union {
		struct tool_detail approval_info_detail_tool_detail_m;
	};
	enum {
		approval_info_detail_tool_detail_m_c,
		approval_info_detail_null_m_c,
	} approval_info_detail_choice;
};

struct approval_info {
	struct zcbor_string approval_info_session;
	struct zcbor_string approval_info_request_id;
	struct zcbor_string approval_info_prompt;
	struct approval_info_path_r approval_info_path;
	bool approval_info_path_present;
	struct approval_info_fingerprint_r approval_info_fingerprint;
	bool approval_info_fingerprint_present;
	struct approval_info_detail_r approval_info_detail;
	bool approval_info_detail_present;
};

struct approval_page_next_r {
	union {
		struct zcbor_string approval_page_next_tstr;
	};
	enum {
		approval_page_next_tstr_c,
		approval_page_next_null_m_c,
	} approval_page_next_choice;
};

struct approval_page {
	struct approval_info approval_page_items_approval_info_m[64];
	size_t approval_page_items_approval_info_m_count;
	struct approval_page_next_r approval_page_next;
	bool approval_page_next_present;
};

struct response_approvals {
	struct approval_page response_approvals_Approvals;
};

struct remembered_fingerprint_label_r {
	union {
		struct zcbor_string remembered_fingerprint_label_tstr;
	};
	enum {
		remembered_fingerprint_label_tstr_c,
		remembered_fingerprint_label_null_m_c,
	} remembered_fingerprint_label_choice;
};

struct remembered_fingerprint_remembered_at_ms {
	uint64_t remembered_fingerprint_remembered_at_ms;
};

struct remembered_fingerprint {
	struct zcbor_string remembered_fingerprint_fingerprint;
	struct remembered_fingerprint_label_r remembered_fingerprint_label;
	bool remembered_fingerprint_label_present;
	struct remembered_fingerprint_remembered_at_ms remembered_fingerprint_remembered_at_ms;
	bool remembered_fingerprint_remembered_at_ms_present;
};

struct response_fingerprints {
	struct remembered_fingerprint response_fingerprints_Fingerprints_remembered_fingerprint_m[64];
	size_t response_fingerprints_Fingerprints_remembered_fingerprint_m_count;
};

struct fleet_report {
	struct zcbor_string fleet_report_children_unit_id_m[64];
	size_t fleet_report_children_unit_id_m_count;
	struct usage_delta fleet_report_usage;
};

struct response_fleet {
	struct fleet_report response_fleet_Fleet;
};

struct unit_kind_r {
	enum {
		unit_kind_Engine_tstr_c,
		unit_kind_Host_tstr_c,
		unit_kind_Orchestrator_tstr_c,
	} unit_kind_choice;
};

struct unit_state_finished {
	struct zcbor_string Finished_end_reason;
};

struct unit_state_r {
	union {
		struct unit_state_finished unit_state_finished_m;
	};
	enum {
		unit_state_Running_tstr_c,
		unit_state_finished_m_c,
		unit_state_Unknown_tstr_c,
	} unit_state_choice;
};

struct unit_node_profile_r {
	union {
		struct zcbor_string unit_node_profile_profile_ref_m;
	};
	enum {
		unit_node_profile_profile_ref_m_c,
		unit_node_profile_null_m_c,
	} unit_node_profile_choice;
};

struct unit_node_session_r {
	union {
		struct zcbor_string unit_node_session_session_id_m;
	};
	enum {
		unit_node_session_session_id_m_c,
		unit_node_session_null_m_c,
	} unit_node_session_choice;
};

struct unit_node_title_r {
	union {
		struct zcbor_string unit_node_title_tstr;
	};
	enum {
		unit_node_title_tstr_c,
		unit_node_title_null_m_c,
	} unit_node_title_choice;
};

struct unit_node_role_r {
	union {
		struct session_role_r unit_node_role_session_role_m;
	};
	enum {
		unit_node_role_session_role_m_c,
		unit_node_role_null_m_c,
	} unit_node_role_choice;
};

struct delegation_lifetime_r {
	enum {
		delegation_lifetime_Persistent_tstr_c,
		delegation_lifetime_Ephemeral_tstr_c,
	} delegation_lifetime_choice;
};

struct unit_node_lifetime_r {
	union {
		struct delegation_lifetime_r unit_node_lifetime_delegation_lifetime_m;
	};
	enum {
		unit_node_lifetime_delegation_lifetime_m_c,
		unit_node_lifetime_null_m_c,
	} unit_node_lifetime_choice;
};

struct unit_node_engine_r {
	union {
		struct engine_selector_r unit_node_engine_engine_selector_m;
	};
	enum {
		unit_node_engine_engine_selector_m_c,
		unit_node_engine_null_m_c,
	} unit_node_engine_choice;
};

struct unit_node {
	struct zcbor_string unit_node_id;
	struct unit_kind_r unit_node_kind;
	struct unit_state_r unit_node_state;
	union {
		struct zcbor_string unit_node_work_tstr;
	};
	enum {
		unit_node_work_tstr_c,
		unit_node_work_null_m_c,
	} unit_node_work_choice;
	struct usage_delta unit_node_usage;
	struct zcbor_string unit_node_children_unit_id_m[64];
	size_t unit_node_children_unit_id_m_count;
	struct unit_node_profile_r unit_node_profile;
	bool unit_node_profile_present;
	struct unit_node_session_r unit_node_session;
	bool unit_node_session_present;
	struct unit_node_title_r unit_node_title;
	bool unit_node_title_present;
	struct unit_node_role_r unit_node_role;
	bool unit_node_role_present;
	struct unit_node_lifetime_r unit_node_lifetime;
	bool unit_node_lifetime_present;
	struct unit_node_engine_r unit_node_engine;
	bool unit_node_engine_present;
};

struct tree_report_next_r {
	union {
		struct zcbor_string tree_report_next_tstr;
	};
	enum {
		tree_report_next_tstr_c,
		tree_report_next_null_m_c,
	} tree_report_next_choice;
};

struct tree_report {
	union {
		struct zcbor_string tree_report_root_unit_id_m;
	};
	enum {
		tree_report_root_unit_id_m_c,
		tree_report_root_null_m_c,
	} tree_report_root_choice;
	struct unit_node tree_report_nodes_unit_node_m[64];
	size_t tree_report_nodes_unit_node_m_count;
	struct tree_report_next_r tree_report_next;
	bool tree_report_next_present;
	uint64_t tree_report_rev;
};

struct response_tree {
	struct tree_report response_tree_Tree;
};

struct response_unit {
	union {
		struct unit_node response_unit_Unit_unit_node_m;
	};
	enum {
		response_unit_Unit_unit_node_m_c,
		response_unit_Unit_null_m_c,
	} response_unit_Unit_choice;
};

struct manage_event_view_started {
	uint64_t Started_seq;
};

struct manage_event_view_progress {
	uint64_t Progress_seq;
	union {
		struct zcbor_string Progress_text_tstr;
	};
	enum {
		Progress_text_tstr_c,
		Progress_text_null_m_c,
	} Progress_text_choice;
};

struct manage_event_view_usage {
	uint64_t Usage_seq;
	struct usage_delta Usage_delta;
};

struct manage_event_view_finished {
	uint64_t Finished_seq;
	struct zcbor_string Finished_end_reason;
	union {
		struct zcbor_string Finished_summary_tstr;
	};
	enum {
		Finished_summary_tstr_c,
		Finished_summary_null_m_c,
	} Finished_summary_choice;
};

struct manage_event_view_error {
	uint64_t Error_seq;
	struct zcbor_string Error_message;
};

struct subagent_phase_r {
	enum {
		subagent_phase_Spawned_tstr_c,
		subagent_phase_Finished_tstr_c,
	} subagent_phase_choice;
};

struct manage_event_view_subagent {
	uint64_t Subagent_seq;
	struct zcbor_string Subagent_child;
	struct session_role_r Subagent_role;
	struct subagent_phase_r Subagent_phase;
	uint32_t Subagent_active_children;
};

struct manage_event_view_r {
	union {
		struct manage_event_view_started manage_event_view_started_m;
		struct manage_event_view_progress manage_event_view_progress_m;
		struct manage_event_view_usage manage_event_view_usage_m;
		struct manage_event_view_finished manage_event_view_finished_m;
		struct manage_event_view_error manage_event_view_error_m;
		struct manage_event_view_subagent manage_event_view_subagent_m;
	};
	enum {
		manage_event_view_started_m_c,
		manage_event_view_progress_m_c,
		manage_event_view_usage_m_c,
		manage_event_view_finished_m_c,
		manage_event_view_error_m_c,
		manage_event_view_subagent_m_c,
	} manage_event_view_choice;
};

struct response_unit_events {
	struct manage_event_view_r response_unit_events_UnitEvents_manage_event_view_m[64];
	size_t response_unit_events_UnitEvents_manage_event_view_m_count;
};

struct journal_record_origin_op_r {
	union {
		struct zcbor_string journal_record_origin_op_origin_op_m;
	};
	enum {
		journal_record_origin_op_origin_op_m_c,
		journal_record_origin_op_null_m_c,
	} journal_record_origin_op_choice;
};

struct journal_record_payload_management {
	struct zcbor_string Management_detail;
};

struct transcript_role_r {
	enum {
		transcript_role_Assistant_tstr_c,
		transcript_role_User_tstr_c,
		transcript_role_System_tstr_c,
	} transcript_role_choice;
};

struct transcript_block_message {
	struct transcript_role_r Message_role;
	struct zcbor_string Message_text;
};

struct ToolCall_detail_r {
	union {
		struct tool_detail ToolCall_detail_tool_detail_m;
	};
	enum {
		ToolCall_detail_tool_detail_m_c,
		ToolCall_detail_null_m_c,
	} ToolCall_detail_choice;
};

struct transcript_block_tool_call {
	struct zcbor_string ToolCall_call_id;
	struct zcbor_string ToolCall_name;
	struct zcbor_string ToolCall_args_summary;
	struct ToolCall_detail_r ToolCall_detail;
	bool ToolCall_detail_present;
};

struct ToolResult_detail_r {
	union {
		struct tool_detail ToolResult_detail_tool_detail_m;
	};
	enum {
		ToolResult_detail_tool_detail_m_c,
		ToolResult_detail_null_m_c,
	} ToolResult_detail_choice;
};

struct transcript_block_tool_result {
	struct zcbor_string ToolResult_call_id;
	bool ToolResult_ok;
	struct zcbor_string ToolResult_summary;
	struct ToolResult_detail_r ToolResult_detail;
	bool ToolResult_detail_present;
};

struct transcript_block_request {
	uint64_t Request_request_id;
	struct host_request_kind_t_r Request_kind;
};

struct transcript_block_content {
	struct zcbor_string Content_kind;
	struct zcbor_string Content_body;
};

struct transcript_block_r {
	union {
		struct transcript_block_message transcript_block_message_m;
		struct transcript_block_tool_call transcript_block_tool_call_m;
		struct transcript_block_tool_result transcript_block_tool_result_m;
		struct transcript_block_request transcript_block_request_m;
		struct transcript_block_content transcript_block_content_m;
	};
	enum {
		transcript_block_message_m_c,
		transcript_block_tool_call_m_c,
		transcript_block_tool_result_m_c,
		transcript_block_request_m_c,
		transcript_block_content_m_c,
	} transcript_block_choice;
};

struct journal_record_payload_block {
	struct transcript_block_r Block_block;
};

struct chat_message_id_r {
	union {
		struct zcbor_string chat_message_id_tstr;
	};
	enum {
		chat_message_id_tstr_c,
		chat_message_id_null_m_c,
	} chat_message_id_choice;
};

struct chat_message_author_r {
	union {
		struct participant_r chat_message_author_participant_m;
	};
	enum {
		chat_message_author_participant_m_c,
		chat_message_author_null_m_c,
	} chat_message_author_choice;
};

struct chat_message_replying_to_r {
	union {
		struct zcbor_string chat_message_replying_to_tstr;
	};
	enum {
		chat_message_replying_to_tstr_c,
		chat_message_replying_to_null_m_c,
	} chat_message_replying_to_choice;
};

struct message_attachment_content_type_r {
	union {
		struct zcbor_string message_attachment_content_type_tstr;
	};
	enum {
		message_attachment_content_type_tstr_c,
		message_attachment_content_type_null_m_c,
	} message_attachment_content_type_choice;
};

struct message_attachment_is_inline {
	bool message_attachment_is_inline;
};

struct message_attachment_local_uri_r {
	union {
		struct zcbor_string message_attachment_local_uri_tstr;
	};
	enum {
		message_attachment_local_uri_tstr_c,
		message_attachment_local_uri_null_m_c,
	} message_attachment_local_uri_choice;
};

struct message_attachment_remote_uri_r {
	union {
		struct zcbor_string message_attachment_remote_uri_tstr;
	};
	enum {
		message_attachment_remote_uri_tstr_c,
		message_attachment_remote_uri_null_m_c,
	} message_attachment_remote_uri_choice;
};

struct message_attachment_size {
	uint64_t message_attachment_size;
};

struct message_attachment {
	struct zcbor_string message_attachment_id;
	struct message_attachment_content_type_r message_attachment_content_type;
	bool message_attachment_content_type_present;
	struct message_attachment_is_inline message_attachment_is_inline;
	bool message_attachment_is_inline_present;
	struct message_attachment_local_uri_r message_attachment_local_uri;
	bool message_attachment_local_uri_present;
	struct message_attachment_remote_uri_r message_attachment_remote_uri;
	bool message_attachment_remote_uri_present;
	struct message_attachment_size message_attachment_size;
	bool message_attachment_size_present;
};

struct chat_message_attachments_r {
	struct message_attachment chat_message_attachments_message_attachment_m[64];
	size_t chat_message_attachments_message_attachment_m_count;
};

struct chat_message_timestamp_r {
	union {
		uint64_t chat_message_timestamp_uint64_m;
	};
	enum {
		chat_message_timestamp_uint64_m_c,
		chat_message_timestamp_null_m_c,
	} chat_message_timestamp_choice;
};

struct chat_message_delivered_at_r {
	union {
		uint64_t chat_message_delivered_at_uint64_m;
	};
	enum {
		chat_message_delivered_at_uint64_m_c,
		chat_message_delivered_at_null_m_c,
	} chat_message_delivered_at_choice;
};

struct chat_message_edited_at_r {
	union {
		uint64_t chat_message_edited_at_uint64_m;
	};
	enum {
		chat_message_edited_at_uint64_m_c,
		chat_message_edited_at_null_m_c,
	} chat_message_edited_at_choice;
};

struct chat_message_error_r {
	union {
		struct zcbor_string chat_message_error_tstr;
	};
	enum {
		chat_message_error_tstr_c,
		chat_message_error_null_m_c,
	} chat_message_error_choice;
};

struct chat_message_title_r {
	union {
		struct zcbor_string chat_message_title_tstr;
	};
	enum {
		chat_message_title_tstr_c,
		chat_message_title_null_m_c,
	} chat_message_title_choice;
};

struct chat_message_highlight_color_r {
	union {
		struct zcbor_string chat_message_highlight_color_tstr;
	};
	enum {
		chat_message_highlight_color_tstr_c,
		chat_message_highlight_color_null_m_c,
	} chat_message_highlight_color_choice;
};

struct chat_message_action {
	bool chat_message_action;
};

struct chat_message_event {
	bool chat_message_event;
};

struct chat_message_notice {
	bool chat_message_notice;
};

struct chat_message_system {
	bool chat_message_system;
};

struct chat_message_highlighted {
	bool chat_message_highlighted;
};

struct chat_message {
	struct chat_message_id_r chat_message_id;
	bool chat_message_id_present;
	struct chat_message_author_r chat_message_author;
	bool chat_message_author_present;
	struct chat_message_replying_to_r chat_message_replying_to;
	bool chat_message_replying_to_present;
	struct zcbor_string chat_message_text;
	struct chat_message_attachments_r chat_message_attachments;
	bool chat_message_attachments_present;
	struct chat_message_timestamp_r chat_message_timestamp;
	bool chat_message_timestamp_present;
	struct chat_message_delivered_at_r chat_message_delivered_at;
	bool chat_message_delivered_at_present;
	struct chat_message_edited_at_r chat_message_edited_at;
	bool chat_message_edited_at_present;
	struct chat_message_error_r chat_message_error;
	bool chat_message_error_present;
	struct chat_message_title_r chat_message_title;
	bool chat_message_title_present;
	struct chat_message_highlight_color_r chat_message_highlight_color;
	bool chat_message_highlight_color_present;
	struct chat_message_action chat_message_action;
	bool chat_message_action_present;
	struct chat_message_event chat_message_event;
	bool chat_message_event_present;
	struct chat_message_notice chat_message_notice;
	bool chat_message_notice_present;
	struct chat_message_system chat_message_system;
	bool chat_message_system_present;
	struct chat_message_highlighted chat_message_highlighted;
	bool chat_message_highlighted_present;
};

struct journal_record_payload_chat {
	struct chat_message Chat_message;
};

struct journal_record_payload_t_r {
	union {
		struct journal_record_payload_management journal_record_payload_t_journal_record_payload_management_m;
		struct journal_record_payload_block journal_record_payload_t_journal_record_payload_block_m;
		struct journal_record_payload_chat journal_record_payload_t_journal_record_payload_chat_m;
	};
	enum {
		journal_record_payload_t_journal_record_payload_management_m_c,
		journal_record_payload_t_journal_record_payload_block_m_c,
		journal_record_payload_t_journal_record_payload_chat_m_c,
	} journal_record_payload_t_choice;
};

struct journal_record {
	uint64_t journal_record_cursor;
	uint64_t journal_record_segment;
	uint64_t journal_record_seq;
	uint64_t journal_record_epoch;
	uint64_t journal_record_trace;
	struct zcbor_string journal_record_kind;
	uint64_t journal_record_timestamp_ms;
	bool journal_record_verified;
	struct journal_record_origin_op_r journal_record_origin_op;
	bool journal_record_origin_op_present;
	struct journal_record_payload_t_r journal_record_payload;
};

struct journal_page_view {
	struct journal_record journal_page_view_entries_journal_record_m[64];
	size_t journal_page_view_entries_journal_record_m_count;
	uint64_t journal_page_view_next_cursor;
	uint64_t journal_page_view_head_cursor;
	union {
		uint64_t journal_page_view_sealed_after_uint64_m;
	};
	enum {
		journal_page_view_sealed_after_uint64_m_c,
		journal_page_view_sealed_after_null_m_c,
	} journal_page_view_sealed_after_choice;
};

struct response_journal {
	struct journal_page_view response_journal_Journal;
};

struct direction_r {
	enum {
		direction_Inbound_tstr_c,
		direction_Outbound_tstr_c,
	} direction_choice;
};

struct disposition_r {
	enum {
		disposition_Context_tstr_c,
		disposition_Transport_tstr_c,
	} disposition_choice;
};

struct session_payload_event {
	struct agent_event_r session_payload_event_Event;
};

struct session_payload_request {
	struct host_request session_payload_request_Request;
};

struct session_payload_command {
	struct agent_command_r session_payload_command_Command;
};

struct session_payload_response {
	struct host_response session_payload_response_Response;
};

struct session_payload_meta {
	struct zcbor_string Meta_kind;
	struct zcbor_string Meta_body;
};

struct session_payload_r {
	union {
		struct session_payload_event session_payload_event_m;
		struct session_payload_request session_payload_request_m;
		struct session_payload_command session_payload_command_m;
		struct session_payload_response session_payload_response_m;
		struct session_payload_meta session_payload_meta_m;
	};
	enum {
		session_payload_event_m_c,
		session_payload_request_m_c,
		session_payload_command_m_c,
		session_payload_response_m_c,
		session_payload_meta_m_c,
	} session_payload_choice;
};

struct session_log_entry {
	uint64_t session_log_entry_seq;
	struct direction_r session_log_entry_direction;
	struct origin session_log_entry_origin;
	struct disposition_r session_log_entry_disposition;
	struct session_payload_r session_log_entry_payload;
};

struct log_page_view {
	struct session_log_entry log_page_view_entries_session_log_entry_m[64];
	size_t log_page_view_entries_session_log_entry_m_count;
	uint64_t log_page_view_next_seq;
	uint64_t log_page_view_head_seq;
	uint64_t log_page_view_epoch;
};

struct response_log_page {
	struct log_page_view response_log_page_LogPage;
};

struct node_event_session_advanced {
	struct zcbor_string SessionAdvanced_session;
	uint64_t SessionAdvanced_epoch;
	uint64_t SessionAdvanced_head_seq;
};

struct SessionMetaChanged_origin_op_r {
	union {
		struct zcbor_string SessionMetaChanged_origin_op_origin_op_m;
	};
	enum {
		SessionMetaChanged_origin_op_origin_op_m_c,
		SessionMetaChanged_origin_op_null_m_c,
	} SessionMetaChanged_origin_op_choice;
};

struct node_event_session_meta_changed {
	struct zcbor_string SessionMetaChanged_session;
	uint64_t SessionMetaChanged_rev;
	struct SessionMetaChanged_origin_op_r SessionMetaChanged_origin_op;
	bool SessionMetaChanged_origin_op_present;
};

struct node_event_roster_changed {
	uint64_t RosterChanged_rev;
};

struct node_event_fleet_changed {
	uint64_t FleetChanged_rev;
};

struct node_event_profiles_changed {
	uint64_t ProfilesChanged_rev;
};

struct node_event_approval_pending {
	struct zcbor_string ApprovalPending_session;
	struct zcbor_string ApprovalPending_request_id;
};

struct node_event_download_progress {
	uint64_t DownloadProgress_id;
	uint32_t DownloadProgress_pct;
	struct zcbor_string DownloadProgress_state;
	uint64_t DownloadProgress_downloaded_bytes;
	uint64_t DownloadProgress_total_bytes;
};

struct node_event_catalog_changed {
	uint64_t CatalogChanged_rev;
};

struct connection_state_r {
	enum {
		connection_state_Offline_tstr_c,
		connection_state_Connecting_tstr_c,
		connection_state_Connected_tstr_c,
		connection_state_Disconnecting_tstr_c,
		connection_state_Error_tstr_c,
	} connection_state_choice;
};

struct presence_state_r {
	enum {
		presence_state_Unknown_tstr_c,
		presence_state_Offline_tstr_c,
		presence_state_Available_tstr_c,
		presence_state_Idle_tstr_c,
		presence_state_Away_tstr_c,
		presence_state_Busy_tstr_c,
	} presence_state_choice;
};

struct TransportChanged_presence {
	struct presence_state_r TransportChanged_presence;
};

struct disconnect_reason_r {
	enum {
		disconnect_reason_UserRequested_tstr_c,
		disconnect_reason_NetworkError_tstr_c,
		disconnect_reason_AuthenticationFailed_tstr_c,
		disconnect_reason_ReplacedByOtherClient_tstr_c,
		disconnect_reason_InvalidSettings_tstr_c,
		disconnect_reason_CertificateError_tstr_c,
		disconnect_reason_Other_tstr_c,
	} disconnect_reason_choice;
};

struct TransportChanged_reason_r {
	union {
		struct disconnect_reason_r TransportChanged_reason_disconnect_reason_m;
	};
	enum {
		TransportChanged_reason_disconnect_reason_m_c,
		TransportChanged_reason_null_m_c,
	} TransportChanged_reason_choice;
};

struct TransportChanged_message_r {
	union {
		struct zcbor_string TransportChanged_message_tstr;
	};
	enum {
		TransportChanged_message_tstr_c,
		TransportChanged_message_null_m_c,
	} TransportChanged_message_choice;
};

struct TransportChanged_fatal {
	bool TransportChanged_fatal;
};

struct TransportChanged_origin_op_r {
	union {
		struct zcbor_string TransportChanged_origin_op_origin_op_m;
	};
	enum {
		TransportChanged_origin_op_origin_op_m_c,
		TransportChanged_origin_op_null_m_c,
	} TransportChanged_origin_op_choice;
};

struct node_event_transport_changed {
	struct zcbor_string TransportChanged_transport;
	struct connection_state_r TransportChanged_connection;
	struct TransportChanged_presence TransportChanged_presence;
	bool TransportChanged_presence_present;
	struct TransportChanged_reason_r TransportChanged_reason;
	bool TransportChanged_reason_present;
	struct TransportChanged_message_r TransportChanged_message;
	bool TransportChanged_message_present;
	struct TransportChanged_fatal TransportChanged_fatal;
	bool TransportChanged_fatal_present;
	struct TransportChanged_origin_op_r TransportChanged_origin_op;
	bool TransportChanged_origin_op_present;
};

struct conv_change_r {
	enum {
		conv_change_Added_tstr_c,
		conv_change_Removed_tstr_c,
	} conv_change_choice;
};

struct ConversationsChanged_origin_op_r {
	union {
		struct zcbor_string ConversationsChanged_origin_op_origin_op_m;
	};
	enum {
		ConversationsChanged_origin_op_origin_op_m_c,
		ConversationsChanged_origin_op_null_m_c,
	} ConversationsChanged_origin_op_choice;
};

struct node_event_conversations_changed {
	struct zcbor_string ConversationsChanged_transport;
	struct zcbor_string ConversationsChanged_conv;
	struct conv_change_r ConversationsChanged_change;
	uint64_t ConversationsChanged_rev;
	struct ConversationsChanged_origin_op_r ConversationsChanged_origin_op;
	bool ConversationsChanged_origin_op_present;
};

struct membership_change_r {
	enum {
		membership_change_Joined_tstr_c,
		membership_change_Left_tstr_c,
		membership_change_Invited_tstr_c,
		membership_change_Kicked_tstr_c,
		membership_change_Banned_tstr_c,
	} membership_change_choice;
};

struct MembershipChanged_actor_r {
	union {
		struct zcbor_string MembershipChanged_actor_tstr;
	};
	enum {
		MembershipChanged_actor_tstr_c,
		MembershipChanged_actor_null_m_c,
	} MembershipChanged_actor_choice;
};

struct MembershipChanged_reason_r {
	union {
		struct zcbor_string MembershipChanged_reason_tstr;
	};
	enum {
		MembershipChanged_reason_tstr_c,
		MembershipChanged_reason_null_m_c,
	} MembershipChanged_reason_choice;
};

struct MembershipChanged_origin_op_r {
	union {
		struct zcbor_string MembershipChanged_origin_op_origin_op_m;
	};
	enum {
		MembershipChanged_origin_op_origin_op_m_c,
		MembershipChanged_origin_op_null_m_c,
	} MembershipChanged_origin_op_choice;
};

struct node_event_membership_changed {
	struct zcbor_string MembershipChanged_transport;
	struct zcbor_string MembershipChanged_conv;
	struct zcbor_string MembershipChanged_member;
	struct membership_change_r MembershipChanged_change;
	struct MembershipChanged_actor_r MembershipChanged_actor;
	bool MembershipChanged_actor_present;
	struct MembershipChanged_reason_r MembershipChanged_reason;
	bool MembershipChanged_reason_present;
	bool MembershipChanged_is_self;
	struct MembershipChanged_origin_op_r MembershipChanged_origin_op;
	bool MembershipChanged_origin_op_present;
};

struct node_event_contacts_changed {
	struct zcbor_string ContactsChanged_transport;
	uint64_t ContactsChanged_rev;
};

struct node_event_resync_needed {
	struct zcbor_string ResyncNeeded_scope;
};

struct node_event_notifications_changed {
	uint64_t NotificationsChanged_rev;
};

struct node_event_persons_changed {
	uint64_t PersonsChanged_rev;
};

struct MessagesChanged_origin_op_r {
	union {
		struct zcbor_string MessagesChanged_origin_op_origin_op_m;
	};
	enum {
		MessagesChanged_origin_op_origin_op_m_c,
		MessagesChanged_origin_op_null_m_c,
	} MessagesChanged_origin_op_choice;
};

struct node_event_messages_changed {
	struct zcbor_string MessagesChanged_transport;
	struct zcbor_string MessagesChanged_conv;
	struct MessagesChanged_origin_op_r MessagesChanged_origin_op;
	bool MessagesChanged_origin_op_present;
};

struct SwarmChanged_run_id {
	struct zcbor_string SwarmChanged_run_id;
};

struct node_event_swarm_changed {
	struct SwarmChanged_run_id SwarmChanged_run_id;
	bool SwarmChanged_run_id_present;
	uint64_t SwarmChanged_rev;
};

struct node_event_r {
	union {
		struct node_event_session_advanced node_event_session_advanced_m;
		struct node_event_session_meta_changed node_event_session_meta_changed_m;
		struct node_event_roster_changed node_event_roster_changed_m;
		struct node_event_fleet_changed node_event_fleet_changed_m;
		struct node_event_profiles_changed node_event_profiles_changed_m;
		struct node_event_approval_pending node_event_approval_pending_m;
		struct node_event_download_progress node_event_download_progress_m;
		struct node_event_catalog_changed node_event_catalog_changed_m;
		struct node_event_transport_changed node_event_transport_changed_m;
		struct node_event_conversations_changed node_event_conversations_changed_m;
		struct node_event_membership_changed node_event_membership_changed_m;
		struct node_event_contacts_changed node_event_contacts_changed_m;
		struct node_event_resync_needed node_event_resync_needed_m;
		struct node_event_notifications_changed node_event_notifications_changed_m;
		struct node_event_persons_changed node_event_persons_changed_m;
		struct node_event_messages_changed node_event_messages_changed_m;
		struct node_event_swarm_changed node_event_swarm_changed_m;
	};
	enum {
		node_event_session_advanced_m_c,
		node_event_session_meta_changed_m_c,
		node_event_roster_changed_m_c,
		node_event_fleet_changed_m_c,
		node_event_profiles_changed_m_c,
		node_event_approval_pending_m_c,
		node_event_download_progress_m_c,
		node_event_catalog_changed_m_c,
		node_event_transport_changed_m_c,
		node_event_conversations_changed_m_c,
		node_event_membership_changed_m_c,
		node_event_contacts_changed_m_c,
		node_event_resync_needed_m_c,
		node_event_notifications_changed_m_c,
		node_event_persons_changed_m_c,
		node_event_messages_changed_m_c,
		node_event_swarm_changed_m_c,
	} node_event_choice;
};

struct events_page_epoch {
	uint64_t events_page_epoch;
};

struct events_page {
	struct node_event_r events_page_events_node_event_m[64];
	size_t events_page_events_node_event_m_count;
	uint64_t events_page_next_cursor;
	uint64_t events_page_head_cursor;
	struct events_page_epoch events_page_epoch;
	bool events_page_epoch_present;
};

struct response_events_page {
	struct events_page response_events_page_EventsPage;
};

struct response_delivery_targets {
	struct delivery_target response_delivery_targets_DeliveryTargets_delivery_target_m[64];
	size_t response_delivery_targets_DeliveryTargets_delivery_target_m_count;
};

struct delivery_session_page_next_r {
	union {
		struct zcbor_string delivery_session_page_next_tstr;
	};
	enum {
		delivery_session_page_next_tstr_c,
		delivery_session_page_next_null_m_c,
	} delivery_session_page_next_choice;
};

struct delivery_session_page {
	struct zcbor_string delivery_session_page_items_session_id_m[64];
	size_t delivery_session_page_items_session_id_m_count;
	struct delivery_session_page_next_r delivery_session_page_next;
	bool delivery_session_page_next_present;
};

struct response_delivery_sessions {
	struct delivery_session_page response_delivery_sessions_DeliverySessions;
};

struct response_verifying_key {
	union {
		struct zcbor_string response_verifying_key_VerifyingKey_tstr;
	};
	enum {
		response_verifying_key_VerifyingKey_tstr_c,
		response_verifying_key_VerifyingKey_null_m_c,
	} response_verifying_key_VerifyingKey_choice;
};

struct search_hit {
	struct zcbor_string search_hit_repo;
	union {
		struct zcbor_string search_hit_author_tstr;
	};
	enum {
		search_hit_author_tstr_c,
		search_hit_author_null_m_c,
	} search_hit_author_choice;
	uint64_t search_hit_downloads;
	uint64_t search_hit_likes;
	union {
		uint64_t search_hit_num_parameters_uint64_m;
	};
	enum {
		search_hit_num_parameters_uint64_m_c,
		search_hit_num_parameters_null_m_c,
	} search_hit_num_parameters_choice;
	union {
		struct zcbor_string search_hit_pipeline_tag_tstr;
	};
	enum {
		search_hit_pipeline_tag_tstr_c,
		search_hit_pipeline_tag_null_m_c,
	} search_hit_pipeline_tag_choice;
	union {
		struct zcbor_string search_hit_last_modified_tstr;
	};
	enum {
		search_hit_last_modified_tstr_c,
		search_hit_last_modified_null_m_c,
	} search_hit_last_modified_choice;
	bool search_hit_gated;
	bool search_hit_private;
};

struct search_page {
	uint32_t search_page_page;
	struct search_hit search_page_results_search_hit_m[64];
	size_t search_page_results_search_hit_m_count;
	bool search_page_has_more;
};

struct response_model_search {
	struct search_page response_model_search_ModelSearch;
};

struct model_file {
	struct zcbor_string model_file_path;
	uint64_t model_file_size_bytes;
	union {
		struct zcbor_string model_file_quant_tstr;
	};
	enum {
		model_file_quant_tstr_c,
		model_file_quant_null_m_c,
	} model_file_quant_choice;
	bool model_file_is_split;
	bool model_file_is_first_shard;
	bool model_file_is_mmproj;
};

struct model_file_page_next_r {
	union {
		struct zcbor_string model_file_page_next_tstr;
	};
	enum {
		model_file_page_next_tstr_c,
		model_file_page_next_null_m_c,
	} model_file_page_next_choice;
};

struct model_file_page {
	struct model_file model_file_page_items_model_file_m[64];
	size_t model_file_page_items_model_file_m_count;
	struct model_file_page_next_r model_file_page_next;
	bool model_file_page_next_present;
};

struct response_model_files {
	struct model_file_page response_model_files_ModelFiles;
};

struct response_model_download_started {
	uint64_t response_model_download_started_ModelDownloadStarted;
};

struct download_state_r {
	enum {
		download_state_Queued_tstr_c,
		download_state_Downloading_tstr_c,
		download_state_Completed_tstr_c,
		download_state_Paused_tstr_c,
		download_state_Cancelled_tstr_c,
		download_state_Failed_tstr_c,
	} download_state_choice;
};

struct download_status {
	uint64_t download_status_id;
	struct model_ref download_status_model;
	struct download_state_r download_status_state;
	uint64_t download_status_downloaded_bytes;
	uint64_t download_status_total_bytes;
	uint32_t download_status_files_done;
	uint32_t download_status_files_total;
	union {
		struct zcbor_string download_status_error_tstr;
	};
	enum {
		download_status_error_tstr_c,
		download_status_error_null_m_c,
	} download_status_error_choice;
};

struct response_model_downloads {
	struct download_status response_model_downloads_ModelDownloads_download_status_m[64];
	size_t response_model_downloads_ModelDownloads_download_status_m_count;
};

struct installed_model_arch_r {
	union {
		struct zcbor_string installed_model_arch_tstr;
	};
	enum {
		installed_model_arch_tstr_c,
		installed_model_arch_null_m_c,
	} installed_model_arch_choice;
};

struct installed_model_context_length_r {
	union {
		uint32_t installed_model_context_length_uint;
	};
	enum {
		installed_model_context_length_uint_c,
		installed_model_context_length_null_m_c,
	} installed_model_context_length_choice;
};

struct installed_model_file_type_r {
	union {
		struct zcbor_string installed_model_file_type_tstr;
	};
	enum {
		installed_model_file_type_tstr_c,
		installed_model_file_type_null_m_c,
	} installed_model_file_type_choice;
};

struct installed_model_mmproj_path_r {
	union {
		struct zcbor_string installed_model_mmproj_path_tstr;
	};
	enum {
		installed_model_mmproj_path_tstr_c,
		installed_model_mmproj_path_null_m_c,
	} installed_model_mmproj_path_choice;
};

struct installed_model_sha256_r {
	union {
		struct zcbor_string installed_model_sha256_tstr;
	};
	enum {
		installed_model_sha256_tstr_c,
		installed_model_sha256_null_m_c,
	} installed_model_sha256_choice;
};

struct installed_model {
	struct zcbor_string installed_model_id;
	struct model_ref installed_model_model;
	struct zcbor_string installed_model_display_name;
	struct zcbor_string installed_model_local_path;
	uint64_t installed_model_size_bytes;
	union {
		struct zcbor_string installed_model_quant_tstr;
	};
	enum {
		installed_model_quant_tstr_c,
		installed_model_quant_null_m_c,
	} installed_model_quant_choice;
	uint64_t installed_model_installed_at_ms;
	struct installed_model_arch_r installed_model_arch;
	bool installed_model_arch_present;
	struct installed_model_context_length_r installed_model_context_length;
	bool installed_model_context_length_present;
	struct installed_model_file_type_r installed_model_file_type;
	bool installed_model_file_type_present;
	struct installed_model_mmproj_path_r installed_model_mmproj_path;
	bool installed_model_mmproj_path_present;
	struct installed_model_sha256_r installed_model_sha256;
	bool installed_model_sha256_present;
};

struct response_model_catalog {
	struct installed_model response_model_catalog_ModelCatalog_installed_model_m[64];
	size_t response_model_catalog_ModelCatalog_installed_model_m_count;
};

struct quant_candidate {
	struct zcbor_string quant_candidate_quant;
	union {
		struct zcbor_string quant_candidate_file_tstr;
	};
	enum {
		quant_candidate_file_tstr_c,
		quant_candidate_file_null_m_c,
	} quant_candidate_file_choice;
	union {
		uint64_t quant_candidate_size_bytes_uint64_m;
	};
	enum {
		quant_candidate_size_bytes_uint64_m_c,
		quant_candidate_size_bytes_null_m_c,
	} quant_candidate_size_bytes_choice;
	bool quant_candidate_fits;
};

struct quant_recommendation {
	struct model_engine_r quant_recommendation_engine;
	struct zcbor_string quant_recommendation_repo;
	union {
		struct zcbor_string quant_recommendation_file_tstr;
	};
	enum {
		quant_recommendation_file_tstr_c,
		quant_recommendation_file_null_m_c,
	} quant_recommendation_file_choice;
	struct zcbor_string quant_recommendation_quant;
	union {
		uint64_t quant_recommendation_size_bytes_uint64_m;
	};
	enum {
		quant_recommendation_size_bytes_uint64_m_c,
		quant_recommendation_size_bytes_null_m_c,
	} quant_recommendation_size_bytes_choice;
	uint64_t quant_recommendation_budget_bytes;
	bool quant_recommendation_fits;
	struct zcbor_string quant_recommendation_reason;
	struct quant_candidate quant_recommendation_candidates_quant_candidate_m[64];
	size_t quant_recommendation_candidates_quant_candidate_m_count;
};

struct response_model_recommend {
	struct quant_recommendation response_model_recommend_ModelRecommend;
};

struct response_model_quantize_started {
	uint64_t response_model_quantize_started_ModelQuantizeStarted;
};

struct quantize_state_r {
	enum {
		quantize_state_Queued_tstr_c,
		quantize_state_Preparing_tstr_c,
		quantize_state_Quantizing_tstr_c,
		quantize_state_Completed_tstr_c,
		quantize_state_Failed_tstr_c,
	} quantize_state_choice;
};

struct quantize_status {
	uint64_t quantize_status_id;
	struct zcbor_string quantize_status_repo;
	struct zcbor_string quantize_status_source_file;
	struct zcbor_string quantize_status_target_quant;
	struct quantize_state_r quantize_status_state;
	union {
		struct zcbor_string quantize_status_output_path_tstr;
	};
	enum {
		quantize_status_output_path_tstr_c,
		quantize_status_output_path_null_m_c,
	} quantize_status_output_path_choice;
	union {
		struct zcbor_string quantize_status_model_id_model_id_m;
	};
	enum {
		quantize_status_model_id_model_id_m_c,
		quantize_status_model_id_null_m_c,
	} quantize_status_model_id_choice;
	union {
		struct zcbor_string quantize_status_error_tstr;
	};
	enum {
		quantize_status_error_tstr_c,
		quantize_status_error_null_m_c,
	} quantize_status_error_choice;
};

struct response_model_quantizes {
	struct quantize_status response_model_quantizes_ModelQuantizes_quantize_status_m[64];
	size_t response_model_quantizes_ModelQuantizes_quantize_status_m_count;
};

struct gguf_info {
	union {
		struct zcbor_string gguf_info_architecture_tstr;
	};
	enum {
		gguf_info_architecture_tstr_c,
		gguf_info_architecture_null_m_c,
	} gguf_info_architecture_choice;
	union {
		struct zcbor_string gguf_info_name_tstr;
	};
	enum {
		gguf_info_name_tstr_c,
		gguf_info_name_null_m_c,
	} gguf_info_name_choice;
	union {
		struct zcbor_string gguf_info_file_type_tstr;
	};
	enum {
		gguf_info_file_type_tstr_c,
		gguf_info_file_type_null_m_c,
	} gguf_info_file_type_choice;
	union {
		uint32_t gguf_info_context_length_uint;
	};
	enum {
		gguf_info_context_length_uint_c,
		gguf_info_context_length_null_m_c,
	} gguf_info_context_length_choice;
	union {
		uint32_t gguf_info_block_count_uint;
	};
	enum {
		gguf_info_block_count_uint_c,
		gguf_info_block_count_null_m_c,
	} gguf_info_block_count_choice;
	union {
		uint32_t gguf_info_quantization_version_uint;
	};
	enum {
		gguf_info_quantization_version_uint_c,
		gguf_info_quantization_version_null_m_c,
	} gguf_info_quantization_version_choice;
	union {
		uint64_t gguf_info_parameter_count_uint64_m;
	};
	enum {
		gguf_info_parameter_count_uint64_m_c,
		gguf_info_parameter_count_null_m_c,
	} gguf_info_parameter_count_choice;
	uint64_t gguf_info_size_bytes;
};

struct response_model_inspect {
	struct gguf_info response_model_inspect_ModelInspect;
};

struct headroom_tstrint {
	struct zcbor_string swarm_eligibility_headroom_tstrint_key;
	int32_t headroom_tstrint;
};

struct swarm_eligibility {
	bool swarm_eligibility_eligible;
	struct zcbor_string swarm_eligibility_reasons_tstr[64];
	size_t swarm_eligibility_reasons_tstr_count;
	struct headroom_tstrint headroom_tstrint[64];
	size_t headroom_tstrint_count;
};

struct swarm_run_summary_policy {
	struct swarm_policy swarm_run_summary_policy;
};

struct swarm_run_summary {
	struct zcbor_string swarm_run_summary_run_id;
	struct zcbor_string swarm_run_summary_phase;
	bool swarm_run_summary_joined;
	struct swarm_eligibility swarm_run_summary_eligibility;
	struct swarm_run_summary_policy swarm_run_summary_policy;
	bool swarm_run_summary_policy_present;
	uint64_t swarm_run_summary_last_round;
};

struct response_swarm_runs {
	struct swarm_run_summary response_swarm_runs_SwarmRuns_swarm_run_summary_m[64];
	size_t response_swarm_runs_SwarmRuns_swarm_run_summary_m_count;
};

struct swarm_contribution {
	uint64_t swarm_contribution_rounds;
	uint64_t swarm_contribution_tokens;
	uint64_t swarm_contribution_bytes_up;
	uint64_t swarm_contribution_bytes_down;
	uint64_t swarm_contribution_witness_count;
	uint64_t swarm_contribution_checkpoint_credits;
};

struct swarm_event_phase {
	struct zcbor_string Phase_run_id;
	struct zcbor_string Phase_phase;
	uint64_t Phase_epoch;
	uint64_t Phase_round;
};

struct swarm_event_progress {
	struct zcbor_string Progress_run_id;
	uint32_t Progress_inner_step;
	uint64_t Progress_loss_micros;
	uint64_t Progress_tokens_per_s_milli;
	uint32_t Progress_peers;
};

struct swarm_event_round_outcome {
	struct zcbor_string RoundOutcome_run_id;
	uint64_t RoundOutcome_round;
	uint32_t RoundOutcome_committed;
	uint32_t RoundOutcome_ingested;
	bool RoundOutcome_stalled;
};

struct swarm_event_contribution {
	struct zcbor_string Contribution_run_id;
	struct swarm_contribution Contribution_contribution;
};

struct swarm_event_warning {
	struct zcbor_string Warning_run_id;
	struct zcbor_string Warning_class;
	struct zcbor_string Warning_detail;
};

struct swarm_event_error {
	struct zcbor_string Error_run_id;
	struct zcbor_string Error_class;
	struct zcbor_string Error_detail;
};

struct swarm_event_r {
	union {
		struct swarm_event_phase swarm_event_phase_m;
		struct swarm_event_progress swarm_event_progress_m;
		struct swarm_event_round_outcome swarm_event_round_outcome_m;
		struct swarm_event_contribution swarm_event_contribution_m;
		struct swarm_event_warning swarm_event_warning_m;
		struct swarm_event_error swarm_event_error_m;
	};
	enum {
		swarm_event_phase_m_c,
		swarm_event_progress_m_c,
		swarm_event_round_outcome_m_c,
		swarm_event_contribution_m_c,
		swarm_event_warning_m_c,
		swarm_event_error_m_c,
	} swarm_event_choice;
};

struct swarm_run_detail {
	struct swarm_run_summary swarm_run_detail_summary;
	struct zcbor_string swarm_run_detail_coordinator;
	struct swarm_contribution swarm_run_detail_contribution;
	struct swarm_event_r swarm_run_detail_recent_events_swarm_event_m[64];
	size_t swarm_run_detail_recent_events_swarm_event_m_count;
};

struct response_swarm_run_detail {
	union {
		struct swarm_run_detail response_swarm_run_detail_SwarmRunDetail_swarm_run_detail_m;
	};
	enum {
		response_swarm_run_detail_SwarmRunDetail_swarm_run_detail_m_c,
		response_swarm_run_detail_SwarmRunDetail_null_m_c,
	} response_swarm_run_detail_SwarmRunDetail_choice;
};

struct swarm_capabilities {
	uint32_t swarm_capabilities_abi_version;
	struct zcbor_string swarm_capabilities_ops_tstr[64];
	size_t swarm_capabilities_ops_tstr_count;
	struct zcbor_string swarm_capabilities_payload_stores_tstr[64];
	size_t swarm_capabilities_payload_stores_tstr_count;
};

struct swarm_hardware_report {
	uint32_t swarm_hardware_report_gpus;
	uint64_t swarm_hardware_report_vram_mb;
	uint64_t swarm_hardware_report_shared_mb;
	uint64_t swarm_hardware_report_ram_mb;
	struct zcbor_string swarm_hardware_report_backend_lanes_tstr[64];
	size_t swarm_hardware_report_backend_lanes_tstr_count;
	struct swarm_capabilities swarm_hardware_report_capabilities;
	uint64_t swarm_hardware_report_up_kbps;
	uint64_t swarm_hardware_report_down_kbps;
	uint64_t swarm_hardware_report_disk_free_mb;
	struct zcbor_string swarm_hardware_report_throughput_class;
};

struct response_swarm_hardware_report {
	struct swarm_hardware_report response_swarm_hardware_report_SwarmHardwareReport;
};

struct profile_info_created_by_r {
	union {
		struct author_r profile_info_created_by_author_m;
	};
	enum {
		profile_info_created_by_author_m_c,
		profile_info_created_by_null_m_c,
	} profile_info_created_by_choice;
};

struct profile_info_owner_r {
	union {
		struct zcbor_string profile_info_owner_tstr;
	};
	enum {
		profile_info_owner_tstr_c,
		profile_info_owner_null_m_c,
	} profile_info_owner_choice;
};

struct profile_info {
	struct zcbor_string profile_info_id;
	struct provider_selector_r profile_info_provider;
	struct zcbor_string profile_info_model;
	bool profile_info_is_active;
	struct bound_account profile_info_bound_accounts_bound_account_m[64];
	size_t profile_info_bound_accounts_bound_account_m_count;
	struct profile_info_created_by_r profile_info_created_by;
	bool profile_info_created_by_present;
	struct profile_info_owner_r profile_info_owner;
	bool profile_info_owner_present;
};

struct response_profiles {
	struct profile_info response_profiles_Profiles_profile_info_m[64];
	size_t response_profiles_Profiles_profile_info_m_count;
};

struct response_profile {
	union {
		struct profile_spec response_profile_Profile_profile_spec_m;
	};
	enum {
		response_profile_Profile_profile_spec_m_c,
		response_profile_Profile_null_m_c,
	} response_profile_Profile_choice;
};

struct response_soul_text {
	struct zcbor_string response_soul_text_SoulText;
};

struct credential_info_label_r {
	union {
		struct zcbor_string credential_info_label_tstr;
	};
	enum {
		credential_info_label_tstr_c,
		credential_info_label_null_m_c,
	} credential_info_label_choice;
};

struct credential_info {
	struct zcbor_string credential_info_profile;
	bool credential_info_present;
	struct zcbor_string credential_info_hint;
	struct credential_info_label_r credential_info_label;
	bool credential_info_label_present;
};

struct response_credentials {
	struct credential_info response_credentials_Credentials_credential_info_m[64];
	size_t response_credentials_Credentials_credential_info_m_count;
};

struct model_descriptor_display_name_r {
	union {
		struct zcbor_string model_descriptor_display_name_tstr;
	};
	enum {
		model_descriptor_display_name_tstr_c,
		model_descriptor_display_name_null_m_c,
	} model_descriptor_display_name_choice;
};

struct model_descriptor {
	struct zcbor_string model_descriptor_id;
	struct provider_selector_r model_descriptor_provider;
	struct model_descriptor_display_name_r model_descriptor_display_name;
	bool model_descriptor_display_name_present;
	union {
		uint32_t model_descriptor_context_length_uint;
	};
	enum {
		model_descriptor_context_length_uint_c,
		model_descriptor_context_length_null_m_c,
	} model_descriptor_context_length_choice;
	union {
		uint64_t model_descriptor_input_price_micros_per_mtok_uint64_m;
	};
	enum {
		model_descriptor_input_price_micros_per_mtok_uint64_m_c,
		model_descriptor_input_price_micros_per_mtok_null_m_c,
	} model_descriptor_input_price_micros_per_mtok_choice;
	union {
		uint64_t model_descriptor_output_price_micros_per_mtok_uint64_m;
	};
	enum {
		model_descriptor_output_price_micros_per_mtok_uint64_m_c,
		model_descriptor_output_price_micros_per_mtok_null_m_c,
	} model_descriptor_output_price_micros_per_mtok_choice;
	bool model_descriptor_local;
};

struct model_page_next_r {
	union {
		struct zcbor_string model_page_next_tstr;
	};
	enum {
		model_page_next_tstr_c,
		model_page_next_null_m_c,
	} model_page_next_choice;
};

struct model_page {
	struct model_descriptor model_page_items_model_descriptor_m[64];
	size_t model_page_items_model_descriptor_m_count;
	struct model_page_next_r model_page_next;
	bool model_page_next_present;
};

struct response_models {
	struct model_page response_models_Models;
};

struct response_model_current {
	union {
		struct model_descriptor response_model_current_ModelCurrent_model_descriptor_m;
	};
	enum {
		response_model_current_ModelCurrent_model_descriptor_m_c,
		response_model_current_ModelCurrent_null_m_c,
	} response_model_current_ModelCurrent_choice;
};

struct provider_kind_wire_r {
	enum {
		provider_kind_wire_local_tstr_c,
		provider_kind_wire_cloud_tstr_c,
		provider_kind_wire_daemon_cloud_tstr_c,
	} provider_kind_wire_choice;
};

struct provider_sign_in {
	struct zcbor_string provider_sign_in_family;
	struct zcbor_string provider_sign_in_label;
};

struct provider_descriptor_sign_in_r {
	union {
		struct provider_sign_in provider_descriptor_sign_in_provider_sign_in_m;
	};
	enum {
		provider_descriptor_sign_in_provider_sign_in_m_c,
		provider_descriptor_sign_in_null_m_c,
	} provider_descriptor_sign_in_choice;
};

struct provider_descriptor {
	struct zcbor_string provider_descriptor_id;
	struct zcbor_string provider_descriptor_display_name;
	struct provider_kind_wire_r provider_descriptor_kind;
	struct provider_selector_r provider_descriptor_wire_selector;
	bool provider_descriptor_requires_key;
	bool provider_descriptor_supports_model_discovery;
	union {
		struct zcbor_string provider_descriptor_default_base_url_tstr;
	};
	enum {
		provider_descriptor_default_base_url_tstr_c,
		provider_descriptor_default_base_url_null_m_c,
	} provider_descriptor_default_base_url_choice;
	struct provider_descriptor_sign_in_r provider_descriptor_sign_in;
	bool provider_descriptor_sign_in_present;
};

struct response_provider_catalog {
	struct provider_descriptor response_provider_catalog_ProviderCatalog_provider_descriptor_m[64];
	size_t response_provider_catalog_ProviderCatalog_provider_descriptor_m_count;
};

struct response_provider_models {
	struct model_page response_provider_models_ProviderModels;
};

struct response_custom_providers {
	struct custom_provider response_custom_providers_CustomProviders_custom_provider_m[64];
	size_t response_custom_providers_CustomProviders_custom_provider_m_count;
};

struct response_distribution {
	struct distribution response_distribution_Distribution;
};

struct response_profile_id {
	struct zcbor_string response_profile_id_ProfileId;
};

struct revision {
	uint64_t revision_seq;
	union {
		uint64_t revision_parent_uint64_m;
	};
	enum {
		revision_parent_uint64_m_c,
		revision_parent_null_m_c,
	} revision_parent_choice;
	struct content_hash revision_content_hash;
	struct author_r revision_author;
	struct zcbor_string revision_reason;
	uint64_t revision_ts_ms;
};

struct revision_page_next_r {
	union {
		struct zcbor_string revision_page_next_tstr;
	};
	enum {
		revision_page_next_tstr_c,
		revision_page_next_null_m_c,
	} revision_page_next_choice;
};

struct revision_page {
	struct revision revision_page_items_revision_m[64];
	size_t revision_page_items_revision_m_count;
	struct revision_page_next_r revision_page_next;
	bool revision_page_next_present;
};

struct response_revisions {
	struct revision_page response_revisions_Revisions;
};

struct response_skill_bundle {
	struct skill_bundle response_skill_bundle_SkillBundle;
};

struct skill_creator_r {
	enum {
		skill_creator_agent_tstr_c,
		skill_creator_user_tstr_c,
		skill_creator_bundled_tstr_c,
	} skill_creator_choice;
};

struct skill_state_r {
	enum {
		skill_state_active_tstr_c,
		skill_state_stale_tstr_c,
		skill_state_archived_tstr_c,
	} skill_state_choice;
};

struct skill_usage {
	struct skill_creator_r skill_usage_created_by;
	struct skill_state_r skill_usage_state;
	bool skill_usage_pinned;
	uint64_t skill_usage_use_count;
	uint64_t skill_usage_view_count;
	uint64_t skill_usage_patch_count;
	uint64_t skill_usage_created_at_ms;
	union {
		uint64_t skill_usage_last_used_ms_uint64_m;
	};
	enum {
		skill_usage_last_used_ms_uint64_m_c,
		skill_usage_last_used_ms_null_m_c,
	} skill_usage_last_used_ms_choice;
	union {
		uint64_t skill_usage_last_viewed_ms_uint64_m;
	};
	enum {
		skill_usage_last_viewed_ms_uint64_m_c,
		skill_usage_last_viewed_ms_null_m_c,
	} skill_usage_last_viewed_ms_choice;
	union {
		uint64_t skill_usage_last_patched_ms_uint64_m;
	};
	enum {
		skill_usage_last_patched_ms_uint64_m_c,
		skill_usage_last_patched_ms_null_m_c,
	} skill_usage_last_patched_ms_choice;
};

struct curator_entry {
	struct zcbor_string curator_entry_name;
	union {
		struct zcbor_string curator_entry_category_tstr;
	};
	enum {
		curator_entry_category_tstr_c,
		curator_entry_category_null_m_c,
	} curator_entry_category_choice;
	bool curator_entry_is_bundled;
	struct skill_usage curator_entry_usage;
};

struct response_curator_skills {
	struct curator_entry response_curator_skills_CuratorSkills_curator_entry_m[64];
	size_t response_curator_skills_CuratorSkills_curator_entry_m_count;
};

struct curator_change {
	struct zcbor_string curator_change_name;
	struct skill_state_r curator_change_from;
	struct skill_state_r curator_change_to;
};

struct response_curator_run {
	struct curator_change response_curator_run_CuratorRun_curator_change_m[64];
	size_t response_curator_run_CuratorRun_curator_change_m_count;
};

struct auth_challenge_redirect {
	struct zcbor_string Redirect_authorization_url;
};

struct auth_challenge_form {
	struct zcbor_string Form_title;
	struct auth_param_field Form_fields_auth_param_field_m[64];
	size_t Form_fields_auth_param_field_m_count;
};

struct auth_challenge_qr {
	struct zcbor_string Qr_payload;
	union {
		struct zcbor_string Qr_image_byte_array_m;
	};
	enum {
		Qr_image_byte_array_m_c,
		Qr_image_null_m_c,
	} Qr_image_choice;
	uint64_t Qr_poll_interval_ms;
};

struct auth_challenge_message {
	struct zcbor_string Message_text;
};

struct auth_challenge_r {
	union {
		struct auth_challenge_redirect auth_challenge_redirect_m;
		struct auth_challenge_form auth_challenge_form_m;
		struct auth_challenge_qr auth_challenge_qr_m;
		struct auth_challenge_message auth_challenge_message_m;
	};
	enum {
		auth_challenge_redirect_m_c,
		auth_challenge_form_m_c,
		auth_challenge_qr_m_c,
		auth_challenge_message_m_c,
	} auth_challenge_choice;
};

struct auth_begin_response {
	struct zcbor_string auth_begin_response_flow_id;
	struct auth_challenge_r auth_begin_response_challenge;
	uint64_t auth_begin_response_expires_at;
};

struct response_auth_begun {
	struct auth_begin_response response_auth_begun_AuthBegun;
};

struct auth_step_result_challenge {
	struct auth_challenge_r auth_step_result_challenge_Challenge;
};

struct auth_complete_response {
	struct zcbor_string auth_complete_response_credential_ref;
	struct zcbor_string auth_complete_response_account_label;
	struct zcbor_string auth_complete_response_transport_instance;
	union {
		struct zcbor_string auth_complete_response_bound_profile_tstr;
	};
	enum {
		auth_complete_response_bound_profile_tstr_c,
		auth_complete_response_bound_profile_null_m_c,
	} auth_complete_response_bound_profile_choice;
};

struct auth_step_result_completed {
	struct auth_complete_response auth_step_result_completed_Completed;
};

struct auth_step_result_r {
	union {
		struct auth_step_result_challenge auth_step_result_challenge_m;
		struct auth_step_result_completed auth_step_result_completed_m;
	};
	enum {
		auth_step_result_challenge_m_c,
		auth_step_result_completed_m_c,
	} auth_step_result_choice;
};

struct response_auth_stepped {
	struct auth_step_result_r response_auth_stepped_AuthStepped;
};

struct response_auth_completed {
	struct auth_complete_response response_auth_completed_AuthCompleted;
};

struct auth_flow_kind_r {
	enum {
		auth_flow_kind_MatrixSso_tstr_c,
		auth_flow_kind_OAuth2Pkce_tstr_c,
		auth_flow_kind_BotToken_tstr_c,
		auth_flow_kind_UserToken_tstr_c,
		auth_flow_kind_PhoneOtp_tstr_c,
		auth_flow_kind_QrPairing_tstr_c,
		auth_flow_kind_UserPassword_tstr_c,
	} auth_flow_kind_choice;
};

struct auth_provider_info {
	struct zcbor_string auth_provider_info_family;
	struct auth_flow_kind_r auth_provider_info_flow_kind;
	struct zcbor_string auth_provider_info_display_name;
	struct auth_param_field auth_provider_info_params_schema_auth_param_field_m[64];
	size_t auth_provider_info_params_schema_auth_param_field_m_count;
};

struct response_auth_providers {
	struct auth_provider_info response_auth_providers_AuthProviders_auth_provider_info_m[64];
	size_t response_auth_providers_AuthProviders_auth_provider_info_m_count;
};

struct checkpoint_info_turn_ordinal_r {
	union {
		uint64_t checkpoint_info_turn_ordinal_uint64_m;
	};
	enum {
		checkpoint_info_turn_ordinal_uint64_m_c,
		checkpoint_info_turn_ordinal_null_m_c,
	} checkpoint_info_turn_ordinal_choice;
};

struct checkpoint_info_cursor_r {
	union {
		uint64_t checkpoint_info_cursor_uint64_m;
	};
	enum {
		checkpoint_info_cursor_uint64_m_c,
		checkpoint_info_cursor_null_m_c,
	} checkpoint_info_cursor_choice;
};

struct checkpoint_info {
	struct zcbor_string checkpoint_info_id;
	struct zcbor_string checkpoint_info_session;
	struct zcbor_string checkpoint_info_tool;
	uint64_t checkpoint_info_created_unix;
	struct checkpoint_info_turn_ordinal_r checkpoint_info_turn_ordinal;
	bool checkpoint_info_turn_ordinal_present;
	struct checkpoint_info_cursor_r checkpoint_info_cursor;
	bool checkpoint_info_cursor_present;
};

struct checkpoint_page_next_r {
	union {
		struct zcbor_string checkpoint_page_next_tstr;
	};
	enum {
		checkpoint_page_next_tstr_c,
		checkpoint_page_next_null_m_c,
	} checkpoint_page_next_choice;
};

struct checkpoint_page {
	struct checkpoint_info checkpoint_page_items_checkpoint_info_m[64];
	size_t checkpoint_page_items_checkpoint_info_m_count;
	struct checkpoint_page_next_r checkpoint_page_next;
	bool checkpoint_page_next_present;
};

struct response_checkpoints {
	struct checkpoint_page response_checkpoints_Checkpoints;
};

struct session_page_next_cursor_r {
	union {
		struct zcbor_string session_page_next_cursor_session_id_m;
	};
	enum {
		session_page_next_cursor_session_id_m_c,
		session_page_next_cursor_null_m_c,
	} session_page_next_cursor_choice;
};

struct session_page_removed_r {
	struct zcbor_string session_page_removed_session_id_m[64];
	size_t session_page_removed_session_id_m_count;
};

struct origin_ops_origin_op_m {
	struct zcbor_string origin_ops_origin_op_m_key;
	struct zcbor_string origin_ops_origin_op_m;
};

struct origin_ops {
	struct origin_ops_origin_op_m origin_ops_origin_op_m[64];
	size_t origin_ops_origin_op_m_count;
};

struct session_page_origin_ops {
	struct origin_ops session_page_origin_ops;
};

struct session_page {
	struct session_info session_page_sessions_session_info_m[64];
	size_t session_page_sessions_session_info_m_count;
	struct session_page_next_cursor_r session_page_next_cursor;
	bool session_page_next_cursor_present;
	uint64_t session_page_rev;
	struct session_page_removed_r session_page_removed;
	bool session_page_removed_present;
	struct session_page_origin_ops session_page_origin_ops;
	bool session_page_origin_ops_present;
};

struct response_session_page {
	struct session_page response_session_page_SessionPage;
};

struct session_detail_overlay_r {
	union {
		struct session_overlay session_detail_overlay_session_overlay_m;
	};
	enum {
		session_detail_overlay_session_overlay_m_c,
		session_detail_overlay_null_m_c,
	} session_detail_overlay_choice;
};

struct session_detail_model_r {
	union {
		struct zcbor_string session_detail_model_tstr;
	};
	enum {
		session_detail_model_tstr_c,
		session_detail_model_null_m_c,
	} session_detail_model_choice;
};

struct session_detail_delivery_targets_r {
	struct delivery_target session_detail_delivery_targets_delivery_target_m[64];
	size_t session_detail_delivery_targets_delivery_target_m_count;
};

struct session_detail_children_r {
	struct zcbor_string session_detail_children_session_id_m[64];
	size_t session_detail_children_session_id_m_count;
};

struct session_detail_checkpoints {
	uint32_t session_detail_checkpoints;
};

struct session_detail_engine {
	struct engine_selector_r session_detail_engine;
};

struct session_detail_foreign_backend {
	struct foreign_backend_r session_detail_foreign_backend;
};

struct model_choice {
	struct zcbor_string model_choice_id;
	struct zcbor_string model_choice_label;
};

struct model_selector {
	struct zcbor_string model_selector_option_id;
	struct zcbor_string model_selector_current;
	struct model_choice model_selector_choices_model_choice_m[64];
	size_t model_selector_choices_model_choice_m_count;
};

struct session_detail_model_selector_r {
	union {
		struct model_selector session_detail_model_selector_model_selector_m;
	};
	enum {
		session_detail_model_selector_model_selector_m_c,
		session_detail_model_selector_null_m_c,
	} session_detail_model_selector_choice;
};

struct session_detail {
	struct session_info session_detail_info;
	struct session_detail_overlay_r session_detail_overlay;
	bool session_detail_overlay_present;
	struct session_detail_model_r session_detail_model;
	bool session_detail_model_present;
	struct session_detail_delivery_targets_r session_detail_delivery_targets;
	bool session_detail_delivery_targets_present;
	struct session_detail_children_r session_detail_children;
	bool session_detail_children_present;
	struct session_detail_checkpoints session_detail_checkpoints;
	bool session_detail_checkpoints_present;
	struct session_detail_engine session_detail_engine;
	bool session_detail_engine_present;
	struct session_detail_foreign_backend session_detail_foreign_backend;
	bool session_detail_foreign_backend_present;
	struct session_detail_model_selector_r session_detail_model_selector;
	bool session_detail_model_selector_present;
};

struct response_session_detail {
	union {
		struct session_detail response_session_detail_SessionDetail_session_detail_m;
	};
	enum {
		response_session_detail_SessionDetail_session_detail_m_c,
		response_session_detail_SessionDetail_null_m_c,
	} response_session_detail_SessionDetail_choice;
};

struct session_search_hit {
	struct zcbor_string session_search_hit_session;
	struct zcbor_string session_search_hit_title;
	struct zcbor_string session_search_hit_snippet;
};

struct response_session_search {
	struct session_search_hit response_session_search_SessionSearch_session_search_hit_m[64];
	size_t response_session_search_SessionSearch_session_search_hit_m_count;
};

struct top_tools_name_l {
	struct zcbor_string top_tools_name_l_name;
	uint32_t top_tools_name_l_count;
};

struct session_recap {
	union {
		struct zcbor_string session_recap_title_tstr;
	};
	enum {
		session_recap_title_tstr_c,
		session_recap_title_null_m_c,
	} session_recap_title_choice;
	uint32_t session_recap_user_turns;
	uint32_t session_recap_assistant_turns;
	uint32_t session_recap_tool_results;
	struct top_tools_name_l top_tools_name_l[64];
	size_t top_tools_name_l_count;
	struct zcbor_string session_recap_files_touched_tstr[64];
	size_t session_recap_files_touched_tstr_count;
	union {
		struct zcbor_string session_recap_last_ask_tstr;
	};
	enum {
		session_recap_last_ask_tstr_c,
		session_recap_last_ask_null_m_c,
	} session_recap_last_ask_choice;
	union {
		struct zcbor_string session_recap_last_reply_tstr;
	};
	enum {
		session_recap_last_reply_tstr_c,
		session_recap_last_reply_null_m_c,
	} session_recap_last_reply_choice;
};

struct response_session_recap {
	union {
		struct session_recap response_session_recap_SessionRecap_session_recap_m;
	};
	enum {
		response_session_recap_SessionRecap_session_recap_m_c,
		response_session_recap_SessionRecap_null_m_c,
	} response_session_recap_SessionRecap_choice;
};

struct response_agent_catalog {
	struct agent_entry response_agent_catalog_AgentCatalog_agent_entry_m[64];
	size_t response_agent_catalog_AgentCatalog_agent_entry_m_count;
};

struct response_providers {
	struct provider_info response_providers_Providers_provider_info_m[64];
	size_t response_providers_Providers_provider_info_m_count;
};

struct response_tools {
	struct tool_info response_tools_Tools_tool_info_m[64];
	size_t response_tools_Tools_tool_info_m_count;
};

struct command_scope_r {
	enum {
		command_scope_Session_tstr_c,
		command_scope_Node_tstr_c,
	} command_scope_choice;
};

struct command_surface_r {
	enum {
		command_surface_Cli_tstr_c,
		command_surface_Gui_tstr_c,
		command_surface_Chat_tstr_c,
	} command_surface_choice;
};

struct command_access_r {
	enum {
		command_access_User_tstr_c,
		command_access_Admin_tstr_c,
	} command_access_choice;
};

struct command_spec {
	struct zcbor_string command_spec_name;
	struct zcbor_string command_spec_aliases_tstr[64];
	size_t command_spec_aliases_tstr_count;
	struct zcbor_string command_spec_summary;
	struct zcbor_string command_spec_category;
	struct zcbor_string command_spec_args_hint;
	struct zcbor_string command_spec_subcommands_tstr[64];
	size_t command_spec_subcommands_tstr_count;
	struct command_scope_r command_spec_scope;
	struct command_surface_r command_spec_surfaces_command_surface_m[64];
	size_t command_spec_surfaces_command_surface_m_count;
	bool command_spec_side_effecting;
	bool command_spec_confirm;
	struct command_access_r command_spec_min_access;
	struct zcbor_string command_spec_source;
};

struct response_commands {
	struct command_spec response_commands_Commands_command_spec_m[64];
	size_t response_commands_Commands_command_spec_m_count;
};

struct command_output {
	struct zcbor_string command_output_text;
	bool command_output_ephemeral;
};

struct response_command_output {
	struct command_output response_command_output_CommandOutput;
};

struct response_config {
	struct node_config_view response_config_Config;
};

struct gateway_status {
	bool gateway_status_enabled;
	union {
		struct zcbor_string gateway_status_addr_tstr;
	};
	enum {
		gateway_status_addr_tstr_c,
		gateway_status_addr_null_m_c,
	} gateway_status_addr_choice;
	bool gateway_status_listening;
	union {
		struct zcbor_string gateway_status_last_error_tstr;
	};
	enum {
		gateway_status_last_error_tstr_c,
		gateway_status_last_error_null_m_c,
	} gateway_status_last_error_choice;
};

struct response_gateway_status {
	struct gateway_status response_gateway_status_GatewayStatus;
};

struct caps_report_max_composed_profiles {
	uint32_t caps_report_max_composed_profiles;
};

struct caps_report_max_ephemeral_per_session {
	uint32_t caps_report_max_ephemeral_per_session;
};

struct caps_report {
	uint32_t caps_report_orchestrate_max_depth;
	uint32_t caps_report_orchestrate_max_fanout;
	struct caps_report_max_composed_profiles caps_report_max_composed_profiles;
	bool caps_report_max_composed_profiles_present;
	struct caps_report_max_ephemeral_per_session caps_report_max_ephemeral_per_session;
	bool caps_report_max_ephemeral_per_session_present;
};

struct response_caps {
	struct caps_report response_caps_Caps;
};

struct cron_job_next_fire_unix_r {
	union {
		uint64_t cron_job_next_fire_unix_uint64_m;
	};
	enum {
		cron_job_next_fire_unix_uint64_m_c,
		cron_job_next_fire_unix_null_m_c,
	} cron_job_next_fire_unix_choice;
};

struct cron_job_paused {
	bool cron_job_paused;
};

struct cron_job_last_run_unix_r {
	union {
		uint64_t cron_job_last_run_unix_uint64_m;
	};
	enum {
		cron_job_last_run_unix_uint64_m_c,
		cron_job_last_run_unix_null_m_c,
	} cron_job_last_run_unix_choice;
};

struct cron_job_last_ok_r {
	union {
		bool cron_job_last_ok_bool;
	};
	enum {
		cron_job_last_ok_bool_c,
		cron_job_last_ok_null_m_c,
	} cron_job_last_ok_choice;
};

struct cron_job_last_detail_r {
	union {
		struct zcbor_string cron_job_last_detail_tstr;
	};
	enum {
		cron_job_last_detail_tstr_c,
		cron_job_last_detail_null_m_c,
	} cron_job_last_detail_choice;
};

struct cron_job_fire_count {
	uint32_t cron_job_fire_count;
};

struct cron_job {
	struct zcbor_string cron_job_id;
	struct cron_spec cron_job_spec;
	struct cron_job_next_fire_unix_r cron_job_next_fire_unix;
	bool cron_job_next_fire_unix_present;
	struct cron_job_paused cron_job_paused;
	bool cron_job_paused_present;
	struct cron_job_last_run_unix_r cron_job_last_run_unix;
	bool cron_job_last_run_unix_present;
	struct cron_job_last_ok_r cron_job_last_ok;
	bool cron_job_last_ok_present;
	struct cron_job_last_detail_r cron_job_last_detail;
	bool cron_job_last_detail_present;
	struct cron_job_fire_count cron_job_fire_count;
	bool cron_job_fire_count_present;
};

struct response_cron_jobs {
	struct cron_job response_cron_jobs_CronJobs_cron_job_m[64];
	size_t response_cron_jobs_CronJobs_cron_job_m_count;
};

struct response_cron_id {
	struct zcbor_string response_cron_id_CronId;
};

struct cron_run_detail_r {
	union {
		struct zcbor_string cron_run_detail_tstr;
	};
	enum {
		cron_run_detail_tstr_c,
		cron_run_detail_null_m_c,
	} cron_run_detail_choice;
};

struct cron_run_finished_unix_r {
	union {
		uint64_t cron_run_finished_unix_uint64_m;
	};
	enum {
		cron_run_finished_unix_uint64_m_c,
		cron_run_finished_unix_null_m_c,
	} cron_run_finished_unix_choice;
};

struct cron_run_session_r {
	union {
		struct zcbor_string cron_run_session_session_id_m;
	};
	enum {
		cron_run_session_session_id_m_c,
		cron_run_session_null_m_c,
	} cron_run_session_choice;
};

struct run_trigger_r {
	enum {
		run_trigger_Scheduled_tstr_c,
		run_trigger_Manual_tstr_c,
	} run_trigger_choice;
};

struct cron_run_trigger {
	struct run_trigger_r cron_run_trigger;
};

struct cron_run {
	uint64_t cron_run_started_unix;
	bool cron_run_ok;
	struct cron_run_detail_r cron_run_detail;
	bool cron_run_detail_present;
	struct cron_run_finished_unix_r cron_run_finished_unix;
	bool cron_run_finished_unix_present;
	struct cron_run_session_r cron_run_session;
	bool cron_run_session_present;
	struct cron_run_trigger cron_run_trigger;
	bool cron_run_trigger_present;
};

struct response_cron_runs {
	struct cron_run response_cron_runs_CronRuns_cron_run_m[64];
	size_t response_cron_runs_CronRuns_cron_run_m_count;
};

struct cron_suggestion_description {
	struct zcbor_string cron_suggestion_description;
};

struct cron_suggestion_source {
	struct zcbor_string cron_suggestion_source;
};

struct suggestion_status_r {
	enum {
		suggestion_status_Pending_tstr_c,
		suggestion_status_Accepted_tstr_c,
		suggestion_status_Dismissed_tstr_c,
	} suggestion_status_choice;
};

struct cron_suggestion_status {
	struct suggestion_status_r cron_suggestion_status;
};

struct cron_suggestion {
	struct zcbor_string cron_suggestion_id;
	struct zcbor_string cron_suggestion_title;
	struct cron_suggestion_description cron_suggestion_description;
	bool cron_suggestion_description_present;
	struct cron_suggestion_source cron_suggestion_source;
	bool cron_suggestion_source_present;
	struct cron_spec cron_suggestion_spec;
	struct zcbor_string cron_suggestion_dedup_key;
	struct cron_suggestion_status cron_suggestion_status;
	bool cron_suggestion_status_present;
};

struct response_cron_suggestions {
	struct cron_suggestion response_cron_suggestions_CronSuggestions_cron_suggestion_m[64];
	size_t response_cron_suggestions_CronSuggestions_cron_suggestion_m_count;
};

struct chat_route_page_next_r {
	union {
		struct zcbor_string chat_route_page_next_tstr;
	};
	enum {
		chat_route_page_next_tstr_c,
		chat_route_page_next_null_m_c,
	} chat_route_page_next_choice;
};

struct chat_route_page {
	struct chat_route chat_route_page_items_chat_route_m[64];
	size_t chat_route_page_items_chat_route_m_count;
	struct chat_route_page_next_r chat_route_page_next;
	bool chat_route_page_next_present;
};

struct response_chat_routes {
	struct chat_route_page response_chat_routes_ChatRoutes;
};

struct response_chat_route {
	union {
		struct chat_route response_chat_route_ChatRoute_chat_route_m;
	};
	enum {
		response_chat_route_ChatRoute_chat_route_m_c,
		response_chat_route_ChatRoute_null_m_c,
	} response_chat_route_ChatRoute_choice;
};

struct room_info_name_r {
	union {
		struct zcbor_string room_info_name_tstr;
	};
	enum {
		room_info_name_tstr_c,
		room_info_name_null_m_c,
	} room_info_name_choice;
};

struct room_info_session_r {
	union {
		struct zcbor_string room_info_session_session_id_m;
	};
	enum {
		room_info_session_session_id_m_c,
		room_info_session_null_m_c,
	} room_info_session_choice;
};

struct room_info {
	struct zcbor_string room_info_transport;
	struct zcbor_string room_info_room;
	struct room_info_name_r room_info_name;
	bool room_info_name_present;
	struct room_info_session_r room_info_session;
	bool room_info_session_present;
};

struct room_page_next_r {
	union {
		struct zcbor_string room_page_next_tstr;
	};
	enum {
		room_page_next_tstr_c,
		room_page_next_null_m_c,
	} room_page_next_choice;
};

struct room_page {
	struct room_info room_page_items_room_info_m[64];
	size_t room_page_items_room_info_m_count;
	struct room_page_next_r room_page_next;
	bool room_page_next_present;
};

struct response_rooms {
	struct room_page response_rooms_Rooms;
};

struct adapter_capabilities {
	bool adapter_capabilities_rooms;
	bool adapter_capabilities_direct_messages;
	bool adapter_capabilities_presence;
	bool adapter_capabilities_room_enumeration;
	bool adapter_capabilities_file_transfer;
	bool adapter_capabilities_interactive_auth;
};

struct adapter_info_account_schema {
	struct account_settings_schema adapter_info_account_schema;
};

struct policy_entry {
	struct zcbor_string policy_entry_key;
	struct zcbor_string policy_entry_label;
	struct zcbor_string policy_entry_value;
};

struct adapter_info_policies_r {
	struct policy_entry adapter_info_policies_policy_entry_m[64];
	size_t adapter_info_policies_policy_entry_m_count;
};

struct conversation_ops {
	bool conversation_ops_create;
	bool conversation_ops_join_channel;
	bool conversation_ops_leave;
	bool conversation_ops_delete;
	bool conversation_ops_send;
	bool conversation_ops_set_topic;
	bool conversation_ops_set_title;
	bool conversation_ops_set_description;
};

struct adapter_info_conversation_ops_r {
	union {
		struct conversation_ops adapter_info_conversation_ops_conversation_ops_m;
	};
	enum {
		adapter_info_conversation_ops_conversation_ops_m_c,
		adapter_info_conversation_ops_null_m_c,
	} adapter_info_conversation_ops_choice;
};

struct membership_ops {
	bool membership_ops_invite;
	bool membership_ops_remove;
	bool membership_ops_ban;
	bool membership_ops_set_role;
};

struct adapter_info_membership_ops_r {
	union {
		struct membership_ops adapter_info_membership_ops_membership_ops_m;
	};
	enum {
		adapter_info_membership_ops_membership_ops_m_c,
		adapter_info_membership_ops_null_m_c,
	} adapter_info_membership_ops_choice;
};

struct contacts_ops {
	bool contacts_ops_get_profile;
	bool contacts_ops_action_menu;
	bool contacts_ops_set_alias;
};

struct adapter_info_contacts_ops_r {
	union {
		struct contacts_ops adapter_info_contacts_ops_contacts_ops_m;
	};
	enum {
		adapter_info_contacts_ops_contacts_ops_m_c,
		adapter_info_contacts_ops_null_m_c,
	} adapter_info_contacts_ops_choice;
};

struct roster_ops {
	bool roster_ops_list;
	bool roster_ops_add;
	bool roster_ops_update;
	bool roster_ops_remove;
};

struct adapter_info_roster_ops_r {
	union {
		struct roster_ops adapter_info_roster_ops_roster_ops_m;
	};
	enum {
		adapter_info_roster_ops_roster_ops_m_c,
		adapter_info_roster_ops_null_m_c,
	} adapter_info_roster_ops_choice;
};

struct adapter_info_directory {
	bool adapter_info_directory;
};

struct adapter_info {
	struct zcbor_string adapter_info_family;
	struct zcbor_string adapter_info_display_name;
	struct adapter_capabilities adapter_info_capabilities;
	struct adapter_info_account_schema adapter_info_account_schema;
	bool adapter_info_account_schema_present;
	struct adapter_info_policies_r adapter_info_policies;
	bool adapter_info_policies_present;
	struct adapter_info_conversation_ops_r adapter_info_conversation_ops;
	bool adapter_info_conversation_ops_present;
	struct adapter_info_membership_ops_r adapter_info_membership_ops;
	bool adapter_info_membership_ops_present;
	struct adapter_info_contacts_ops_r adapter_info_contacts_ops;
	bool adapter_info_contacts_ops_present;
	struct adapter_info_roster_ops_r adapter_info_roster_ops;
	bool adapter_info_roster_ops_present;
	struct adapter_info_directory adapter_info_directory;
	bool adapter_info_directory_present;
};

struct response_adapters {
	struct adapter_info response_adapters_Adapters_adapter_info_m[64];
	size_t response_adapters_Adapters_adapter_info_m_count;
};

struct transport_instance_info_connection {
	struct connection_state_r transport_instance_info_connection;
};

struct transport_instance_info_presence {
	struct presence_state_r transport_instance_info_presence;
};

struct transport_instance_info_bound_profile_r {
	union {
		struct zcbor_string transport_instance_info_bound_profile_profile_ref_m;
	};
	enum {
		transport_instance_info_bound_profile_profile_ref_m_c,
		transport_instance_info_bound_profile_null_m_c,
	} transport_instance_info_bound_profile_choice;
};

struct transport_instance_info_reason_r {
	union {
		struct disconnect_reason_r transport_instance_info_reason_disconnect_reason_m;
	};
	enum {
		transport_instance_info_reason_disconnect_reason_m_c,
		transport_instance_info_reason_null_m_c,
	} transport_instance_info_reason_choice;
};

struct transport_instance_info_message_r {
	union {
		struct zcbor_string transport_instance_info_message_tstr;
	};
	enum {
		transport_instance_info_message_tstr_c,
		transport_instance_info_message_null_m_c,
	} transport_instance_info_message_choice;
};

struct transport_instance_info_fatal {
	bool transport_instance_info_fatal;
};

struct transport_instance_info_enabled {
	bool transport_instance_info_enabled;
};

struct transport_instance_info_label_r {
	union {
		struct zcbor_string transport_instance_info_label_tstr;
	};
	enum {
		transport_instance_info_label_tstr_c,
		transport_instance_info_label_null_m_c,
	} transport_instance_info_label_choice;
};

struct transport_instance_info {
	struct zcbor_string transport_instance_info_transport;
	struct zcbor_string transport_instance_info_family;
	struct zcbor_string transport_instance_info_display_name;
	struct transport_instance_info_connection transport_instance_info_connection;
	bool transport_instance_info_connection_present;
	struct transport_instance_info_presence transport_instance_info_presence;
	bool transport_instance_info_presence_present;
	struct transport_instance_info_bound_profile_r transport_instance_info_bound_profile;
	bool transport_instance_info_bound_profile_present;
	struct transport_instance_info_reason_r transport_instance_info_reason;
	bool transport_instance_info_reason_present;
	struct transport_instance_info_message_r transport_instance_info_message;
	bool transport_instance_info_message_present;
	struct transport_instance_info_fatal transport_instance_info_fatal;
	bool transport_instance_info_fatal_present;
	struct transport_instance_info_enabled transport_instance_info_enabled;
	bool transport_instance_info_enabled_present;
	struct transport_instance_info_label_r transport_instance_info_label;
	bool transport_instance_info_label_present;
};

struct response_transport_instances {
	struct transport_instance_info response_transport_instances_TransportInstances_transport_instance_info_m[64];
	size_t response_transport_instances_TransportInstances_transport_instance_info_m_count;
};

struct conversation_type_r {
	enum {
		conversation_type_Unset_tstr_c,
		conversation_type_Dm_tstr_c,
		conversation_type_GroupDm_tstr_c,
		conversation_type_Channel_tstr_c,
		conversation_type_Thread_tstr_c,
		conversation_type_Space_tstr_c,
	} conversation_type_choice;
};

struct conversation_info_title_r {
	union {
		struct zcbor_string conversation_info_title_tstr;
	};
	enum {
		conversation_info_title_tstr_c,
		conversation_info_title_null_m_c,
	} conversation_info_title_choice;
};

struct conversation_info_topic_r {
	union {
		struct zcbor_string conversation_info_topic_tstr;
	};
	enum {
		conversation_info_topic_tstr_c,
		conversation_info_topic_null_m_c,
	} conversation_info_topic_choice;
};

struct conversation_info_description_r {
	union {
		struct zcbor_string conversation_info_description_tstr;
	};
	enum {
		conversation_info_description_tstr_c,
		conversation_info_description_null_m_c,
	} conversation_info_description_choice;
};

struct conversation_member_alias_r {
	union {
		struct zcbor_string conversation_member_alias_tstr;
	};
	enum {
		conversation_member_alias_tstr_c,
		conversation_member_alias_null_m_c,
	} conversation_member_alias_choice;
};

struct conversation_member_nickname_r {
	union {
		struct zcbor_string conversation_member_nickname_tstr;
	};
	enum {
		conversation_member_nickname_tstr_c,
		conversation_member_nickname_null_m_c,
	} conversation_member_nickname_choice;
};

struct typing_state_r {
	enum {
		typing_state_None_tstr_c,
		typing_state_Typing_tstr_c,
		typing_state_Paused_tstr_c,
	} typing_state_choice;
};

struct conversation_member_typing {
	struct typing_state_r conversation_member_typing;
};

struct conversation_member_role {
	struct member_role_r conversation_member_role;
};

struct conversation_member_session_r {
	union {
		struct zcbor_string conversation_member_session_session_id_m;
	};
	enum {
		conversation_member_session_session_id_m_c,
		conversation_member_session_null_m_c,
	} conversation_member_session_choice;
};

struct conversation_member {
	struct contact_info conversation_member_contact;
	struct conversation_member_alias_r conversation_member_alias;
	bool conversation_member_alias_present;
	struct conversation_member_nickname_r conversation_member_nickname;
	bool conversation_member_nickname_present;
	struct conversation_member_typing conversation_member_typing;
	bool conversation_member_typing_present;
	struct conversation_member_role conversation_member_role;
	bool conversation_member_role_present;
	struct conversation_member_session_r conversation_member_session;
	bool conversation_member_session_present;
};

struct conversation_info_members_r {
	struct conversation_member conversation_info_members_conversation_member_m[64];
	size_t conversation_info_members_conversation_member_m_count;
};

struct conversation_info_parent_r {
	union {
		struct zcbor_string conversation_info_parent_tstr;
	};
	enum {
		conversation_info_parent_tstr_c,
		conversation_info_parent_null_m_c,
	} conversation_info_parent_choice;
};

struct conversation_info {
	struct zcbor_string conversation_info_transport;
	struct zcbor_string conversation_info_id;
	struct conversation_type_r conversation_info_kind;
	struct conversation_info_title_r conversation_info_title;
	bool conversation_info_title_present;
	struct conversation_info_topic_r conversation_info_topic;
	bool conversation_info_topic_present;
	struct conversation_info_description_r conversation_info_description;
	bool conversation_info_description_present;
	struct conversation_info_members_r conversation_info_members;
	bool conversation_info_members_present;
	struct conversation_info_parent_r conversation_info_parent;
	bool conversation_info_parent_present;
};

struct conv_page_next_r {
	union {
		struct zcbor_string conv_page_next_tstr;
	};
	enum {
		conv_page_next_tstr_c,
		conv_page_next_null_m_c,
	} conv_page_next_choice;
};

struct conv_page_removed_r {
	struct zcbor_string conv_page_removed_tstr[64];
	size_t conv_page_removed_tstr_count;
};

struct conv_page_origin_ops {
	struct origin_ops conv_page_origin_ops;
};

struct conv_page {
	struct conversation_info conv_page_items_conversation_info_m[64];
	size_t conv_page_items_conversation_info_m_count;
	struct conv_page_next_r conv_page_next;
	bool conv_page_next_present;
	uint64_t conv_page_rev;
	struct conv_page_removed_r conv_page_removed;
	bool conv_page_removed_present;
	struct conv_page_origin_ops conv_page_origin_ops;
	bool conv_page_origin_ops_present;
};

struct response_conversations {
	struct conv_page response_conversations_Conversations;
};

struct response_conversation {
	union {
		struct conversation_info response_conversation_Conversation_conversation_info_m;
	};
	enum {
		response_conversation_Conversation_conversation_info_m_c,
		response_conversation_Conversation_null_m_c,
	} response_conversation_Conversation_choice;
};

struct response_conv_create_details {
	struct create_conversation_details response_conv_create_details_ConvCreateDetails;
};

struct response_conv_join_details {
	struct channel_join_details response_conv_join_details_ConvJoinDetails;
};

struct response_contact_profile {
	struct zcbor_string response_contact_profile_ContactProfile;
};

struct response_contacts {
	struct contact_info response_contacts_Contacts_contact_info_m[64];
	size_t response_contacts_Contacts_contact_info_m_count;
};

struct contact_page_next_r {
	union {
		struct zcbor_string contact_page_next_tstr;
	};
	enum {
		contact_page_next_tstr_c,
		contact_page_next_null_m_c,
	} contact_page_next_choice;
};

struct contact_page_removed_r {
	struct zcbor_string contact_page_removed_tstr[64];
	size_t contact_page_removed_tstr_count;
};

struct contact_page_origin_ops {
	struct origin_ops contact_page_origin_ops;
};

struct contact_page {
	struct contact_info contact_page_items_contact_info_m[64];
	size_t contact_page_items_contact_info_m_count;
	struct contact_page_next_r contact_page_next;
	bool contact_page_next_present;
	uint64_t contact_page_rev;
	struct contact_page_removed_r contact_page_removed;
	bool contact_page_removed_present;
	struct contact_page_origin_ops contact_page_origin_ops;
	bool contact_page_origin_ops_present;
};

struct response_contact_page {
	struct contact_page response_contact_page_ContactPage;
};

struct action_menu_items_r {
	struct zcbor_string action_menu_items_tstr[64];
	size_t action_menu_items_tstr_count;
};

struct action_menu {
	struct action_menu_items_r action_menu_items;
	bool action_menu_items_present;
};

struct response_action_menu {
	union {
		struct action_menu response_action_menu_ActionMenu_action_menu_m;
	};
	enum {
		response_action_menu_ActionMenu_action_menu_m_c,
		response_action_menu_ActionMenu_null_m_c,
	} response_action_menu_ActionMenu_choice;
};

struct api_error_unknown_session {
	struct zcbor_string api_error_unknown_session_UnknownSession;
};

struct api_error_unsupported {
	struct zcbor_string api_error_unsupported_Unsupported;
};

struct api_error_conflict {
	struct zcbor_string api_error_conflict_Conflict;
};

struct api_error_unauthenticated {
	struct zcbor_string api_error_unauthenticated_Unauthenticated;
};

struct api_error_forbidden {
	struct zcbor_string api_error_forbidden_Forbidden;
};

struct api_error_other {
	struct zcbor_string api_error_other_Other;
};

struct api_error_r {
	union {
		struct api_error_unknown_session api_error_unknown_session_m;
		struct api_error_unsupported api_error_unsupported_m;
		struct api_error_conflict api_error_conflict_m;
		struct api_error_unauthenticated api_error_unauthenticated_m;
		struct api_error_forbidden api_error_forbidden_m;
		struct api_error_other api_error_other_m;
	};
	enum {
		api_error_unknown_session_m_c,
		api_error_unsupported_m_c,
		api_error_conflict_m_c,
		api_error_unauthenticated_m_c,
		api_error_forbidden_m_c,
		api_error_other_m_c,
	} api_error_choice;
};

struct response_error {
	struct api_error_r response_error_Error;
};

struct fs_root_kind_t_r {
	enum {
		fs_root_kind_t_host_tstr_c,
		fs_root_kind_t_workspace_tstr_c,
		fs_root_kind_t_session_tstr_c,
	} fs_root_kind_t_choice;
};

struct fs_root_session_r {
	union {
		struct zcbor_string fs_root_session_session_id_m;
	};
	enum {
		fs_root_session_session_id_m_c,
		fs_root_session_null_m_c,
	} fs_root_session_choice;
};

struct fs_root {
	struct fs_root_id_t_r fs_root_id;
	struct zcbor_string fs_root_label;
	struct fs_root_kind_t_r fs_root_kind;
	struct fs_root_session_r fs_root_session;
	bool fs_root_session_present;
};

struct response_fs_roots {
	struct fs_root response_fs_roots_FsRoots_fs_root_m[64];
	size_t response_fs_roots_FsRoots_fs_root_m_count;
};

struct fs_entry_kind_t_r {
	enum {
		fs_entry_kind_t_file_tstr_c,
		fs_entry_kind_t_dir_tstr_c,
		fs_entry_kind_t_symlink_tstr_c,
	} fs_entry_kind_t_choice;
};

struct fs_entry_ignored {
	bool fs_entry_ignored;
};

struct fs_entry {
	struct zcbor_string fs_entry_name;
	struct zcbor_string fs_entry_path;
	struct fs_entry_kind_t_r fs_entry_kind;
	uint64_t fs_entry_size;
	uint64_t fs_entry_mtime_ms;
	struct fs_entry_ignored fs_entry_ignored;
	bool fs_entry_ignored_present;
};

struct fs_list_page_next_r {
	union {
		struct zcbor_string fs_list_page_next_tstr;
	};
	enum {
		fs_list_page_next_tstr_c,
		fs_list_page_next_null_m_c,
	} fs_list_page_next_choice;
};

struct fs_list_page {
	struct fs_entry fs_list_page_items_fs_entry_m[64];
	size_t fs_list_page_items_fs_entry_m_count;
	struct fs_list_page_next_r fs_list_page_next;
	bool fs_list_page_next_present;
};

struct response_fs_list {
	struct fs_list_page response_fs_list_FsList;
};

struct response_fs_stat {
	struct fs_entry response_fs_stat_FsStat;
};

struct fs_content_truncated {
	bool fs_content_truncated;
};

struct fs_content_blob_ref_r {
	union {
		struct blob_ref fs_content_blob_ref_blob_ref_m;
	};
	enum {
		fs_content_blob_ref_blob_ref_m_c,
		fs_content_blob_ref_null_m_c,
	} fs_content_blob_ref_choice;
};

struct fs_content {
	struct zcbor_string fs_content_bytes;
	struct fs_revision fs_content_revision;
	struct fs_content_truncated fs_content_truncated;
	bool fs_content_truncated_present;
	struct fs_content_blob_ref_r fs_content_blob_ref;
	bool fs_content_blob_ref_present;
};

struct response_fs_read {
	struct fs_content response_fs_read_FsRead;
};

struct response_fs_write {
	struct fs_revision response_fs_write_FsWrite;
};

struct fs_search_hit {
	struct zcbor_string fs_search_hit_path;
	uint32_t fs_search_hit_line;
	uint32_t fs_search_hit_col;
	struct zcbor_string fs_search_hit_preview;
};

struct fs_search_page_has_more {
	bool fs_search_page_has_more;
};

struct fs_search_page {
	struct fs_search_hit fs_search_page_hits_fs_search_hit_m[64];
	size_t fs_search_page_hits_fs_search_hit_m_count;
	struct fs_search_page_has_more fs_search_page_has_more;
	bool fs_search_page_has_more_present;
};

struct response_fs_search {
	struct fs_search_page response_fs_search_FsSearch;
};

struct fs_change_kind_t_r {
	enum {
		fs_change_kind_t_created_tstr_c,
		fs_change_kind_t_modified_tstr_c,
		fs_change_kind_t_removed_tstr_c,
	} fs_change_kind_t_choice;
};

struct fs_change {
	struct zcbor_string fs_change_path;
	struct fs_change_kind_t_r fs_change_kind;
};

struct fs_watch_page_view {
	struct fs_change fs_watch_page_view_events_fs_change_m[64];
	size_t fs_watch_page_view_events_fs_change_m_count;
	uint64_t fs_watch_page_view_next_seq;
	uint64_t fs_watch_page_view_head_seq;
	bool fs_watch_page_view_reset;
};

struct response_fs_watch {
	struct fs_watch_page_view response_fs_watch_FsWatch;
};

struct response_blob_put {
	struct blob_ref response_blob_put_BlobPut;
};

struct response_blob_get {
	struct zcbor_string response_blob_get_BlobGet;
};

struct blob_stat {
	uint64_t blob_stat_size;
	bool blob_stat_present;
};

struct response_blob_stat {
	struct blob_stat response_blob_stat_BlobStat;
};

struct access_user {
	struct zcbor_string access_user_user_id;
	struct zcbor_string access_user_username;
	bool access_user_disabled;
	int32_t access_user_created_at;
	struct zcbor_string access_user_roles_tstr[64];
	size_t access_user_roles_tstr_count;
};

struct response_access_user {
	struct access_user response_access_user_AccessUser;
};

struct response_access_users {
	struct access_user response_access_users_AccessUsers_access_user_m[64];
	size_t response_access_users_AccessUsers_access_user_m_count;
};

struct role_info {
	struct zcbor_string role_info_role;
	struct zcbor_string role_info_capabilities_tstr[64];
	size_t role_info_capabilities_tstr_count;
};

struct response_access_roles {
	struct role_info response_access_roles_AccessRoles_role_info_m[64];
	size_t response_access_roles_AccessRoles_role_info_m_count;
};

struct principal_view {
	struct zcbor_string principal_view_user_id;
	struct zcbor_string principal_view_username;
	struct zcbor_string principal_view_roles_tstr[64];
	size_t principal_view_roles_tstr_count;
	struct zcbor_string principal_view_capabilities_tstr[64];
	size_t principal_view_capabilities_tstr_count;
};

struct response_who_am_i {
	struct principal_view response_who_am_i_WhoAmI;
};

struct feedback_ack {
	bool feedback_ack_accepted;
	bool feedback_ack_queued;
};

struct response_feedback_ack {
	struct feedback_ack response_feedback_ack_FeedbackAck;
};

struct response_telemetry_consent {
	bool TelemetryConsent_enabled;
};

struct response_crash_consent {
	bool CrashConsent_enabled;
};

struct response_saved_presences {
	struct saved_presence response_saved_presences_SavedPresences_saved_presence_m[64];
	size_t response_saved_presences_SavedPresences_saved_presence_m_count;
};

struct notification_info_account_r {
	union {
		struct zcbor_string notification_info_account_transport_id_m;
	};
	enum {
		notification_info_account_transport_id_m_c,
		notification_info_account_null_m_c,
	} notification_info_account_choice;
};

struct notification_info_created_ms {
	uint64_t notification_info_created_ms;
};

struct notification_info_read {
	bool notification_info_read;
};

struct notification_info_title_r {
	union {
		struct zcbor_string notification_info_title_tstr;
	};
	enum {
		notification_info_title_tstr_c,
		notification_info_title_null_m_c,
	} notification_info_title_choice;
};

struct notification_info_subtitle_r {
	union {
		struct zcbor_string notification_info_subtitle_tstr;
	};
	enum {
		notification_info_subtitle_tstr_c,
		notification_info_subtitle_null_m_c,
	} notification_info_subtitle_choice;
};

struct notification_info_icon_name_r {
	union {
		struct zcbor_string notification_info_icon_name_tstr;
	};
	enum {
		notification_info_icon_name_tstr_c,
		notification_info_icon_name_null_m_c,
	} notification_info_icon_name_choice;
};

struct notification_info_interactive {
	bool notification_info_interactive;
};

struct notification_info_persistent {
	bool notification_info_persistent;
};

struct add_contact_request_message_r {
	union {
		struct zcbor_string add_contact_request_message_tstr;
	};
	enum {
		add_contact_request_message_tstr_c,
		add_contact_request_message_null_m_c,
	} add_contact_request_message_choice;
};

struct add_contact_request_handled {
	bool add_contact_request_handled;
};

struct add_contact_request {
	struct contact_info add_contact_request_contact;
	struct add_contact_request_message_r add_contact_request_message;
	bool add_contact_request_message_present;
	struct add_contact_request_handled add_contact_request_handled;
	bool add_contact_request_handled_present;
};

struct notification_kind_add_contact {
	struct add_contact_request notification_kind_add_contact_AddContact;
};

struct authorization_request_message_r {
	union {
		struct zcbor_string authorization_request_message_tstr;
	};
	enum {
		authorization_request_message_tstr_c,
		authorization_request_message_null_m_c,
	} authorization_request_message_choice;
};

struct authorization_request_add {
	bool authorization_request_add;
};

struct authorization_request_handled {
	bool authorization_request_handled;
};

struct authorization_request {
	struct contact_info authorization_request_contact;
	struct authorization_request_message_r authorization_request_message;
	bool authorization_request_message_present;
	struct authorization_request_add authorization_request_add;
	bool authorization_request_add_present;
	struct authorization_request_handled authorization_request_handled;
	bool authorization_request_handled_present;
};

struct notification_kind_authorization {
	struct authorization_request notification_kind_authorization_Authorization;
};

struct Link_link_text_r {
	union {
		struct zcbor_string Link_link_text_tstr;
	};
	enum {
		Link_link_text_tstr_c,
		Link_link_text_null_m_c,
	} Link_link_text_choice;
};

struct notification_kind_link {
	struct Link_link_text_r Link_link_text;
	bool Link_link_text_present;
	struct zcbor_string Link_link_uri;
};

struct notification_kind_r {
	union {
		struct notification_kind_add_contact notification_kind_add_contact_m;
		struct notification_kind_authorization notification_kind_authorization_m;
		struct notification_kind_link notification_kind_link_m;
	};
	enum {
		notification_kind_generic_m_c,
		notification_kind_add_contact_m_c,
		notification_kind_authorization_m_c,
		notification_kind_link_m_c,
		notification_kind_connection_error_m_c,
	} notification_kind_choice;
};

struct notification_info_deleted {
	bool notification_info_deleted;
};

struct notification_info {
	struct zcbor_string notification_info_id;
	struct notification_info_account_r notification_info_account;
	bool notification_info_account_present;
	struct notification_info_created_ms notification_info_created_ms;
	bool notification_info_created_ms_present;
	struct notification_info_read notification_info_read;
	bool notification_info_read_present;
	struct notification_info_title_r notification_info_title;
	bool notification_info_title_present;
	struct notification_info_subtitle_r notification_info_subtitle;
	bool notification_info_subtitle_present;
	struct notification_info_icon_name_r notification_info_icon_name;
	bool notification_info_icon_name_present;
	struct notification_info_interactive notification_info_interactive;
	bool notification_info_interactive_present;
	struct notification_info_persistent notification_info_persistent;
	bool notification_info_persistent_present;
	struct notification_kind_r notification_info_kind;
	struct notification_info_deleted notification_info_deleted;
	bool notification_info_deleted_present;
};

struct response_notifications {
	uint64_t Notifications_rev;
	struct notification_info Notifications_items_notification_info_m[64];
	size_t Notifications_items_notification_info_m_count;
};

struct person_alias_r {
	union {
		struct zcbor_string person_alias_tstr;
	};
	enum {
		person_alias_tstr_c,
		person_alias_null_m_c,
	} person_alias_choice;
};

struct image {
	struct blob_ref image_blob;
};

struct person_avatar_r {
	union {
		struct image person_avatar_image_m;
	};
	enum {
		person_avatar_image_m_c,
		person_avatar_null_m_c,
	} person_avatar_choice;
};

struct person_endpoint {
	struct zcbor_string person_endpoint_transport;
	struct contact_info person_endpoint_contact;
};

struct person_endpoints_r {
	struct person_endpoint person_endpoints_person_endpoint_m[64];
	size_t person_endpoints_person_endpoint_m_count;
};

struct person {
	struct zcbor_string person_id;
	struct person_alias_r person_alias;
	bool person_alias_present;
	struct person_avatar_r person_avatar;
	bool person_avatar_present;
	struct person_endpoints_r person_endpoints;
	bool person_endpoints_present;
};

struct Persons_removed_r {
	struct zcbor_string Persons_removed_tstr[64];
	size_t Persons_removed_tstr_count;
};

struct Persons_origin_ops {
	struct origin_ops Persons_origin_ops;
};

struct response_persons {
	uint64_t Persons_rev;
	struct person Persons_items_person_m[64];
	size_t Persons_items_person_m_count;
	struct Persons_removed_r Persons_removed;
	bool Persons_removed_present;
	struct Persons_origin_ops Persons_origin_ops;
	bool Persons_origin_ops_present;
};

struct response_transport_settings {
	struct account_settings_values response_transport_settings_TransportSettings;
};

struct revs_uint64_m {
	struct zcbor_string response_bootstrap_revs_uint64_m_key;
	uint64_t revs_uint64_m;
};

struct response_bootstrap {
	uint64_t Bootstrap_cursor;
	uint64_t Bootstrap_epoch;
	struct revs_uint64_m revs_uint64_m[64];
	size_t revs_uint64_m_count;
};

struct api_response_r {
	union {
		struct response_routed api_response_response_routed_m;
		struct response_session_created api_response_response_session_created_m;
		struct response_drained api_response_response_drained_m;
		struct response_health api_response_response_health_m;
		struct response_stats api_response_response_stats_m;
		struct response_telemetry api_response_response_telemetry_m;
		struct response_sessions api_response_response_sessions_m;
		struct response_approvals api_response_response_approvals_m;
		struct response_fingerprints api_response_response_fingerprints_m;
		struct response_fleet api_response_response_fleet_m;
		struct response_tree api_response_response_tree_m;
		struct response_unit api_response_response_unit_m;
		struct response_unit_events api_response_response_unit_events_m;
		struct response_journal api_response_response_journal_m;
		struct response_log_page api_response_response_log_page_m;
		struct response_events_page api_response_response_events_page_m;
		struct response_delivery_targets api_response_response_delivery_targets_m;
		struct response_delivery_sessions api_response_response_delivery_sessions_m;
		struct response_verifying_key api_response_response_verifying_key_m;
		struct response_model_search api_response_response_model_search_m;
		struct response_model_files api_response_response_model_files_m;
		struct response_model_download_started api_response_response_model_download_started_m;
		struct response_model_downloads api_response_response_model_downloads_m;
		struct response_model_catalog api_response_response_model_catalog_m;
		struct response_model_recommend api_response_response_model_recommend_m;
		struct response_model_quantize_started api_response_response_model_quantize_started_m;
		struct response_model_quantizes api_response_response_model_quantizes_m;
		struct response_model_inspect api_response_response_model_inspect_m;
		struct response_swarm_runs api_response_response_swarm_runs_m;
		struct response_swarm_run_detail api_response_response_swarm_run_detail_m;
		struct response_swarm_hardware_report api_response_response_swarm_hardware_report_m;
		struct response_profiles api_response_response_profiles_m;
		struct response_profile api_response_response_profile_m;
		struct response_soul_text api_response_response_soul_text_m;
		struct response_credentials api_response_response_credentials_m;
		struct response_models api_response_response_models_m;
		struct response_model_current api_response_response_model_current_m;
		struct response_provider_catalog api_response_response_provider_catalog_m;
		struct response_provider_models api_response_response_provider_models_m;
		struct response_custom_providers api_response_response_custom_providers_m;
		struct response_distribution api_response_response_distribution_m;
		struct response_profile_id api_response_response_profile_id_m;
		struct response_revisions api_response_response_revisions_m;
		struct response_skill_bundle api_response_response_skill_bundle_m;
		struct response_curator_skills api_response_response_curator_skills_m;
		struct response_curator_run api_response_response_curator_run_m;
		struct response_auth_begun api_response_response_auth_begun_m;
		struct response_auth_stepped api_response_response_auth_stepped_m;
		struct response_auth_completed api_response_response_auth_completed_m;
		struct response_auth_providers api_response_response_auth_providers_m;
		struct response_checkpoints api_response_response_checkpoints_m;
		struct response_session_page api_response_response_session_page_m;
		struct response_session_detail api_response_response_session_detail_m;
		struct response_session_search api_response_response_session_search_m;
		struct response_session_recap api_response_response_session_recap_m;
		struct response_agent_catalog api_response_response_agent_catalog_m;
		struct response_providers api_response_response_providers_m;
		struct response_tools api_response_response_tools_m;
		struct response_commands api_response_response_commands_m;
		struct response_command_output api_response_response_command_output_m;
		struct response_config api_response_response_config_m;
		struct response_gateway_status api_response_response_gateway_status_m;
		struct response_caps api_response_response_caps_m;
		struct response_cron_jobs api_response_response_cron_jobs_m;
		struct response_cron_id api_response_response_cron_id_m;
		struct response_cron_runs api_response_response_cron_runs_m;
		struct response_cron_suggestions api_response_response_cron_suggestions_m;
		struct response_chat_routes api_response_response_chat_routes_m;
		struct response_chat_route api_response_response_chat_route_m;
		struct response_rooms api_response_response_rooms_m;
		struct response_adapters api_response_response_adapters_m;
		struct response_transport_instances api_response_response_transport_instances_m;
		struct response_conversations api_response_response_conversations_m;
		struct response_conversation api_response_response_conversation_m;
		struct response_conv_create_details api_response_response_conv_create_details_m;
		struct response_conv_join_details api_response_response_conv_join_details_m;
		struct response_contact_profile api_response_response_contact_profile_m;
		struct response_contacts api_response_response_contacts_m;
		struct response_contact_page api_response_response_contact_page_m;
		struct response_action_menu api_response_response_action_menu_m;
		struct response_error api_response_response_error_m;
		struct response_fs_roots api_response_response_fs_roots_m;
		struct response_fs_list api_response_response_fs_list_m;
		struct response_fs_stat api_response_response_fs_stat_m;
		struct response_fs_read api_response_response_fs_read_m;
		struct response_fs_write api_response_response_fs_write_m;
		struct response_fs_search api_response_response_fs_search_m;
		struct response_fs_watch api_response_response_fs_watch_m;
		struct response_blob_put api_response_response_blob_put_m;
		struct response_blob_get api_response_response_blob_get_m;
		struct response_blob_stat api_response_response_blob_stat_m;
		struct response_access_user api_response_response_access_user_m;
		struct response_access_users api_response_response_access_users_m;
		struct response_access_roles api_response_response_access_roles_m;
		struct response_who_am_i api_response_response_who_am_i_m;
		struct response_feedback_ack api_response_response_feedback_ack_m;
		struct response_telemetry_consent api_response_response_telemetry_consent_m;
		struct response_crash_consent api_response_response_crash_consent_m;
		struct response_saved_presences api_response_response_saved_presences_m;
		struct response_notifications api_response_response_notifications_m;
		struct response_persons api_response_response_persons_m;
		struct response_transport_settings api_response_response_transport_settings_m;
		struct response_bootstrap api_response_response_bootstrap_m;
	};
	enum {
		api_response_response_ok_m_c,
		api_response_response_routed_m_c,
		api_response_response_session_created_m_c,
		api_response_response_drained_m_c,
		api_response_response_health_m_c,
		api_response_response_stats_m_c,
		api_response_response_telemetry_m_c,
		api_response_response_sessions_m_c,
		api_response_response_approvals_m_c,
		api_response_response_fingerprints_m_c,
		api_response_response_fleet_m_c,
		api_response_response_tree_m_c,
		api_response_response_unit_m_c,
		api_response_response_unit_events_m_c,
		api_response_response_journal_m_c,
		api_response_response_log_page_m_c,
		api_response_response_events_page_m_c,
		api_response_response_delivery_targets_m_c,
		api_response_response_delivery_sessions_m_c,
		api_response_response_verifying_key_m_c,
		api_response_response_model_search_m_c,
		api_response_response_model_files_m_c,
		api_response_response_model_download_started_m_c,
		api_response_response_model_downloads_m_c,
		api_response_response_model_catalog_m_c,
		api_response_response_model_recommend_m_c,
		api_response_response_model_quantize_started_m_c,
		api_response_response_model_quantizes_m_c,
		api_response_response_model_inspect_m_c,
		api_response_response_swarm_runs_m_c,
		api_response_response_swarm_run_detail_m_c,
		api_response_response_swarm_hardware_report_m_c,
		api_response_response_profiles_m_c,
		api_response_response_profile_m_c,
		api_response_response_soul_text_m_c,
		api_response_response_credentials_m_c,
		api_response_response_models_m_c,
		api_response_response_model_current_m_c,
		api_response_response_provider_catalog_m_c,
		api_response_response_provider_models_m_c,
		api_response_response_custom_providers_m_c,
		api_response_response_distribution_m_c,
		api_response_response_profile_id_m_c,
		api_response_response_revisions_m_c,
		api_response_response_skill_bundle_m_c,
		api_response_response_curator_skills_m_c,
		api_response_response_curator_run_m_c,
		api_response_response_auth_begun_m_c,
		api_response_response_auth_stepped_m_c,
		api_response_response_auth_completed_m_c,
		api_response_response_auth_providers_m_c,
		api_response_response_checkpoints_m_c,
		api_response_response_session_page_m_c,
		api_response_response_session_detail_m_c,
		api_response_response_session_search_m_c,
		api_response_response_session_recap_m_c,
		api_response_response_agent_catalog_m_c,
		api_response_response_providers_m_c,
		api_response_response_tools_m_c,
		api_response_response_commands_m_c,
		api_response_response_command_output_m_c,
		api_response_response_config_m_c,
		api_response_response_gateway_status_m_c,
		api_response_response_caps_m_c,
		api_response_response_cron_jobs_m_c,
		api_response_response_cron_id_m_c,
		api_response_response_cron_runs_m_c,
		api_response_response_cron_suggestions_m_c,
		api_response_response_chat_routes_m_c,
		api_response_response_chat_route_m_c,
		api_response_response_rooms_m_c,
		api_response_response_adapters_m_c,
		api_response_response_transport_instances_m_c,
		api_response_response_conversations_m_c,
		api_response_response_conversation_m_c,
		api_response_response_conv_create_details_m_c,
		api_response_response_conv_join_details_m_c,
		api_response_response_contact_profile_m_c,
		api_response_response_contacts_m_c,
		api_response_response_contact_page_m_c,
		api_response_response_action_menu_m_c,
		api_response_response_error_m_c,
		api_response_response_fs_roots_m_c,
		api_response_response_fs_list_m_c,
		api_response_response_fs_stat_m_c,
		api_response_response_fs_read_m_c,
		api_response_response_fs_write_m_c,
		api_response_response_fs_search_m_c,
		api_response_response_fs_watch_m_c,
		api_response_response_blob_put_m_c,
		api_response_response_blob_get_m_c,
		api_response_response_blob_stat_m_c,
		api_response_response_access_user_m_c,
		api_response_response_access_users_m_c,
		api_response_response_access_roles_m_c,
		api_response_response_who_am_i_m_c,
		api_response_response_feedback_ack_m_c,
		api_response_response_telemetry_consent_m_c,
		api_response_response_crash_consent_m_c,
		api_response_response_saved_presences_m_c,
		api_response_response_notifications_m_c,
		api_response_response_persons_m_c,
		api_response_response_transport_settings_m_c,
		api_response_response_bootstrap_m_c,
	} api_response_choice;
};

#ifdef __cplusplus
}
#endif

#endif /* DAEMON_API_CLIENT_TYPES_H__ */
