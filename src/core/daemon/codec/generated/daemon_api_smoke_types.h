/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 16
 */

#ifndef DAEMON_API_SMOKE_TYPES_H__
#define DAEMON_API_SMOKE_TYPES_H__

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
#define DEFAULT_MAX_QTY 16

struct session_scope_r {
	union {
		struct {
			struct zcbor_string map_ByProfile;
		};
	};
	enum {
		session_scope_TopLevel_tstr_c,
		session_scope_map_c,
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

struct session_query {
	struct session_query_scope session_query_scope;
	bool session_query_scope_present;
	struct session_query_after_r session_query_after;
	bool session_query_after_present;
	struct session_query_limit session_query_limit;
	bool session_query_limit_present;
};

struct request_sessions_query {
	struct session_query SessionsQuery_query;
};

struct request_subscribe {
	struct zcbor_string Subscribe_session;
	uint32_t Subscribe_after_seq;
	uint32_t Subscribe_max;
};

struct blob_ref {
	struct zcbor_string blob_ref_hash;
	uint32_t blob_ref_size;
};

struct user_msg_attachments_r {
	struct blob_ref user_msg_attachments_blob_ref_m[16];
	size_t user_msg_attachments_blob_ref_m_count;
};

struct user_msg {
	struct zcbor_string user_msg_text;
	struct user_msg_attachments_r user_msg_attachments;
	bool user_msg_attachments_present;
};

struct agent_command_start_turn {
	struct user_msg StartTurn_input;
	uint32_t StartTurn_request_id;
};

struct agent_command_steer {
	struct zcbor_string Steer_text;
	uint32_t Steer_request_id;
};

struct agent_command_r {
	union {
		struct agent_command_start_turn agent_command_start_turn_m;
		struct agent_command_steer agent_command_steer_m;
	};
	enum {
		agent_command_start_turn_m_c,
		agent_command_steer_m_c,
		agent_command_shutdown_m_c,
	} agent_command_choice;
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
	struct Submit_profile_r Submit_profile;
	bool Submit_profile_present;
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

struct api_request_r {
	union {
		struct request_sessions_query api_request_request_sessions_query_m;
		struct request_subscribe api_request_request_subscribe_m;
		struct request_submit api_request_request_submit_m;
		struct request_model_current api_request_request_model_current_m;
	};
	enum {
		api_request_request_health_m_c,
		api_request_request_sessions_query_m_c,
		api_request_request_subscribe_m_c,
		api_request_request_submit_m_c,
		api_request_request_profile_list_m_c,
		api_request_request_model_current_m_c,
		api_request_request_fs_roots_m_c,
	} api_request_choice;
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
	struct service_health health_report_services_service_health_m[16];
	size_t health_report_services_service_health_m_count;
};

struct response_health {
	struct health_report response_health_Health;
};

struct session_state_r {
	enum {
		session_state_Active_tstr_c,
		session_state_Ready_tstr_c,
		session_state_Completed_tstr_c,
		session_state_Unknown_tstr_c,
	} session_state_choice;
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
	struct session_info_bound_profile_r session_info_bound_profile;
	bool session_info_bound_profile_present;
	struct session_info_title_r session_info_title;
	bool session_info_title_present;
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

struct session_page_next_cursor_r {
	union {
		struct zcbor_string session_page_next_cursor_session_id_m;
	};
	enum {
		session_page_next_cursor_session_id_m_c,
		session_page_next_cursor_null_m_c,
	} session_page_next_cursor_choice;
};

struct session_page {
	struct session_info session_page_sessions_session_info_m[16];
	size_t session_page_sessions_session_info_m_count;
	struct session_page_next_cursor_r session_page_next_cursor;
	bool session_page_next_cursor_present;
};

struct response_session_page {
	struct session_page response_session_page_SessionPage;
};

struct direction_r {
	enum {
		direction_Inbound_tstr_c,
		direction_Outbound_tstr_c,
	} direction_choice;
};

struct origin {
	struct zcbor_string origin_transport;
	struct zcbor_string origin_scope;
};

struct disposition_r {
	enum {
		disposition_Context_tstr_c,
		disposition_Transport_tstr_c,
	} disposition_choice;
};

struct session_payload {
	struct agent_command_r session_payload_Command;
};

struct session_log_entry {
	uint32_t session_log_entry_seq;
	struct direction_r session_log_entry_direction;
	struct origin session_log_entry_origin;
	struct disposition_r session_log_entry_disposition;
	struct session_payload session_log_entry_payload;
};

struct log_page_view {
	struct session_log_entry log_page_view_entries_session_log_entry_m[16];
	size_t log_page_view_entries_session_log_entry_m_count;
	uint32_t log_page_view_next_seq;
	uint32_t log_page_view_head_seq;
};

struct response_log_page {
	struct log_page_view response_log_page_LogPage;
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
	struct zcbor_string fs_root_id;
	struct zcbor_string fs_root_label;
	struct zcbor_string fs_root_kind;
	struct fs_root_session_r fs_root_session;
	bool fs_root_session_present;
};

struct response_fs_roots {
	struct fs_root response_fs_roots_FsRoots_fs_root_m[16];
	size_t response_fs_roots_FsRoots_fs_root_m_count;
};

struct api_response_r {
	union {
		struct response_health api_response_response_health_m;
		struct response_session_page api_response_response_session_page_m;
		struct response_log_page api_response_response_log_page_m;
		struct response_fs_roots api_response_response_fs_roots_m;
	};
	enum {
		api_response_response_health_m_c,
		api_response_response_session_page_m_c,
		api_response_response_log_page_m_c,
		api_response_response_fs_roots_m_c,
		api_response_response_ok_m_c,
	} api_response_choice;
};

#ifdef __cplusplus
}
#endif

#endif /* DAEMON_API_SMOKE_TYPES_H__ */
