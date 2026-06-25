/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 16
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_encode.h"
#include "daemon_api_client_encode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 16
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

static bool encode_session_scope(zcbor_state_t *state, const struct session_scope_r *input);
static bool encode_repeated_session_query_scope(zcbor_state_t *state, const struct session_query_scope *input);
static bool encode_repeated_session_query_after(zcbor_state_t *state, const struct session_query_after_r *input);
static bool encode_repeated_session_query_limit(zcbor_state_t *state, const struct session_query_limit *input);
static bool encode_session_query(zcbor_state_t *state, const struct session_query *input);
static bool encode_request_sessions_query(zcbor_state_t *state, const struct request_sessions_query *input);
static bool encode_request_subscribe(zcbor_state_t *state, const struct request_subscribe *input);
static bool encode_blob_ref(zcbor_state_t *state, const struct blob_ref *input);
static bool encode_repeated_user_msg_attachments(zcbor_state_t *state, const struct user_msg_attachments_r *input);
static bool encode_user_msg(zcbor_state_t *state, const struct user_msg *input);
static bool encode_agent_command_start_turn(zcbor_state_t *state, const struct agent_command_start_turn *input);
static bool encode_agent_command_steer(zcbor_state_t *state, const struct agent_command_steer *input);
static bool encode_agent_command(zcbor_state_t *state, const struct agent_command_r *input);
static bool encode_origin(zcbor_state_t *state, const struct origin *input);
static bool encode_repeated_Submit_origin(zcbor_state_t *state, const struct Submit_origin_r *input);
static bool encode_repeated_Submit_profile(zcbor_state_t *state, const struct Submit_profile_r *input);
static bool encode_request_submit(zcbor_state_t *state, const struct request_submit *input);
static bool encode_request_model_current(zcbor_state_t *state, const struct request_model_current *input);
static bool encode_command_invocation(zcbor_state_t *state, const struct command_invocation *input);
static bool encode_request_command_invoke(zcbor_state_t *state, const struct request_command_invoke *input);
static bool encode_service_health(zcbor_state_t *state, const struct service_health *input);
static bool encode_health_report(zcbor_state_t *state, const struct health_report *input);
static bool encode_response_health(zcbor_state_t *state, const struct response_health *input);
static bool encode_session_state(zcbor_state_t *state, const struct session_state_r *input);
static bool encode_repeated_session_info_bound_profile(zcbor_state_t *state, const struct session_info_bound_profile_r *input);
static bool encode_repeated_session_info_title(zcbor_state_t *state, const struct session_info_title_r *input);
static bool encode_lifecycle(zcbor_state_t *state, const struct lifecycle_r *input);
static bool encode_repeated_session_info_lifecycle(zcbor_state_t *state, const struct session_info_lifecycle *input);
static bool encode_session_role(zcbor_state_t *state, const struct session_role_r *input);
static bool encode_repeated_session_info_role(zcbor_state_t *state, const struct session_info_role *input);
static bool encode_repeated_session_info_parent(zcbor_state_t *state, const struct session_info_parent_r *input);
static bool encode_repeated_session_info_pinned(zcbor_state_t *state, const struct session_info_pinned *input);
static bool encode_repeated_session_info_archived(zcbor_state_t *state, const struct session_info_archived *input);
static bool encode_session_info(zcbor_state_t *state, const struct session_info *input);
static bool encode_repeated_session_page_next_cursor(zcbor_state_t *state, const struct session_page_next_cursor_r *input);
static bool encode_session_page(zcbor_state_t *state, const struct session_page *input);
static bool encode_response_session_page(zcbor_state_t *state, const struct response_session_page *input);
static bool encode_direction(zcbor_state_t *state, const struct direction_r *input);
static bool encode_disposition(zcbor_state_t *state, const struct disposition_r *input);
static bool encode_session_payload(zcbor_state_t *state, const struct session_payload *input);
static bool encode_session_log_entry(zcbor_state_t *state, const struct session_log_entry *input);
static bool encode_log_page_view(zcbor_state_t *state, const struct log_page_view *input);
static bool encode_response_log_page(zcbor_state_t *state, const struct response_log_page *input);
static bool encode_repeated_fs_root_session(zcbor_state_t *state, const struct fs_root_session_r *input);
static bool encode_fs_root(zcbor_state_t *state, const struct fs_root *input);
static bool encode_response_fs_roots(zcbor_state_t *state, const struct response_fs_roots *input);
static bool encode_command_scope(zcbor_state_t *state, const struct command_scope_r *input);
static bool encode_command_surface(zcbor_state_t *state, const struct command_surface_r *input);
static bool encode_command_access(zcbor_state_t *state, const struct command_access_r *input);
static bool encode_command_spec(zcbor_state_t *state, const struct command_spec *input);
static bool encode_response_commands(zcbor_state_t *state, const struct response_commands *input);
static bool encode_command_output(zcbor_state_t *state, const struct command_output *input);
static bool encode_response_command_output(zcbor_state_t *state, const struct response_command_output *input);
static bool encode_api_response(zcbor_state_t *state, const struct api_response_r *input);
static bool encode_api_request(zcbor_state_t *state, const struct api_request_r *input);


static bool encode_session_scope(
		zcbor_state_t *state, const struct session_scope_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).session_scope_choice == session_scope_TopLevel_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"TopLevel", tmp_str.len = sizeof("TopLevel") - 1, &tmp_str)))))
	: (((*input).session_scope_choice == session_scope_map_c) ? ((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ByProfile", tmp_str.len = sizeof("ByProfile") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).map_ByProfile))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1)))
	: (((*input).session_scope_choice == session_scope_Archived_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Archived", tmp_str.len = sizeof("Archived") - 1, &tmp_str)))))
	: (((*input).session_scope_choice == session_scope_All_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"All", tmp_str.len = sizeof("All") - 1, &tmp_str)))))
	: false))))));

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

static bool encode_session_query(
		zcbor_state_t *state, const struct session_query *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_encode(state, 3) && (((!(*input).session_query_scope_present || encode_repeated_session_query_scope(state, (&(*input).session_query_scope)))
	&& (!(*input).session_query_after_present || encode_repeated_session_query_after(state, (&(*input).session_query_after)))
	&& (!(*input).session_query_limit_present || encode_repeated_session_query_limit(state, (&(*input).session_query_limit)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

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

static bool encode_request_subscribe(
		zcbor_state_t *state, const struct request_subscribe *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Subscribe", tmp_str.len = sizeof("Subscribe") - 1, &tmp_str)))))
	&& (zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).Subscribe_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"after_seq", tmp_str.len = sizeof("after_seq") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).Subscribe_after_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).Subscribe_max))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_blob_ref(
		zcbor_state_t *state, const struct blob_ref *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"hash", tmp_str.len = sizeof("hash") - 1, &tmp_str)))))
	&& (zcbor_bstr_encode(state, (&(*input).blob_ref_hash))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"size", tmp_str.len = sizeof("size") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).blob_ref_size))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_user_msg_attachments(
		zcbor_state_t *state, const struct user_msg_attachments_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"attachments", tmp_str.len = sizeof("attachments") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 16) && ((zcbor_multi_encode_minmax(0, 16, &(*input).user_msg_attachments_blob_ref_m_count, (zcbor_encoder_t *)encode_blob_ref, state, (*&(*input).user_msg_attachments_blob_ref_m), sizeof(struct blob_ref))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 16))));

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
	&& (zcbor_uint32_encode(state, (&(*input).StartTurn_request_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

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
	&& (zcbor_uint32_encode(state, (&(*input).Steer_request_id))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

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
	: (((*input).agent_command_choice == agent_command_shutdown_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Shutdown", tmp_str.len = sizeof("Shutdown") - 1, &tmp_str)))))
	: false)))));

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
	&& (zcbor_tstr_encode(state, (&(*input).origin_scope))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

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
	&& (zcbor_list_start_encode(state, 16) && ((zcbor_multi_encode_minmax(0, 16, &(*input).health_report_services_service_health_m_count, (zcbor_encoder_t *)encode_service_health, state, (*&(*input).health_report_services_service_health_m), sizeof(struct service_health))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 16)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

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

static bool encode_session_state(
		zcbor_state_t *state, const struct session_state_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).session_state_choice == session_state_Active_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Active", tmp_str.len = sizeof("Active") - 1, &tmp_str)))))
	: (((*input).session_state_choice == session_state_Ready_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Ready", tmp_str.len = sizeof("Ready") - 1, &tmp_str)))))
	: (((*input).session_state_choice == session_state_Completed_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Completed", tmp_str.len = sizeof("Completed") - 1, &tmp_str)))))
	: (((*input).session_state_choice == session_state_Unknown_tstr_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Unknown", tmp_str.len = sizeof("Unknown") - 1, &tmp_str)))))
	: false))))));

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

	bool res = (((zcbor_map_start_encode(state, 9) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).session_info_session))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"state", tmp_str.len = sizeof("state") - 1, &tmp_str)))))
	&& (encode_session_state(state, (&(*input).session_info_state))))
	&& (!(*input).session_info_bound_profile_present || encode_repeated_session_info_bound_profile(state, (&(*input).session_info_bound_profile)))
	&& (!(*input).session_info_title_present || encode_repeated_session_info_title(state, (&(*input).session_info_title)))
	&& (!(*input).session_info_lifecycle_present || encode_repeated_session_info_lifecycle(state, (&(*input).session_info_lifecycle)))
	&& (!(*input).session_info_role_present || encode_repeated_session_info_role(state, (&(*input).session_info_role)))
	&& (!(*input).session_info_parent_present || encode_repeated_session_info_parent(state, (&(*input).session_info_parent)))
	&& (!(*input).session_info_pinned_present || encode_repeated_session_info_pinned(state, (&(*input).session_info_pinned)))
	&& (!(*input).session_info_archived_present || encode_repeated_session_info_archived(state, (&(*input).session_info_archived)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 9))));

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

static bool encode_session_page(
		zcbor_state_t *state, const struct session_page *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 2) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"sessions", tmp_str.len = sizeof("sessions") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 16) && ((zcbor_multi_encode_minmax(0, 16, &(*input).session_page_sessions_session_info_m_count, (zcbor_encoder_t *)encode_session_info, state, (*&(*input).session_page_sessions_session_info_m), sizeof(struct session_info))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 16)))
	&& (!(*input).session_page_next_cursor_present || encode_repeated_session_page_next_cursor(state, (&(*input).session_page_next_cursor)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 2))));

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

static bool encode_session_payload(
		zcbor_state_t *state, const struct session_payload *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 1) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Command", tmp_str.len = sizeof("Command") - 1, &tmp_str)))))
	&& (encode_agent_command(state, (&(*input).session_payload_Command))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_session_log_entry(
		zcbor_state_t *state, const struct session_log_entry *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).session_log_entry_seq))))
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

	bool res = (((zcbor_map_start_encode(state, 3) && (((((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"entries", tmp_str.len = sizeof("entries") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 16) && ((zcbor_multi_encode_minmax(0, 16, &(*input).log_page_view_entries_session_log_entry_m_count, (zcbor_encoder_t *)encode_session_log_entry, state, (*&(*input).log_page_view_entries_session_log_entry_m), sizeof(struct session_log_entry))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 16)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"next_seq", tmp_str.len = sizeof("next_seq") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).log_page_view_next_seq))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"head_seq", tmp_str.len = sizeof("head_seq") - 1, &tmp_str)))))
	&& (zcbor_uint32_encode(state, (&(*input).log_page_view_head_seq))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 3))));

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
	&& (zcbor_tstr_encode(state, (&(*input).fs_root_id))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"label", tmp_str.len = sizeof("label") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_root_label))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).fs_root_kind))))
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
	&& (zcbor_list_start_encode(state, 16) && ((zcbor_multi_encode_minmax(0, 16, &(*input).response_fs_roots_FsRoots_fs_root_m_count, (zcbor_encoder_t *)encode_fs_root, state, (*&(*input).response_fs_roots_FsRoots_fs_root_m), sizeof(struct fs_root))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 16)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

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
	&& (zcbor_list_start_encode(state, 16) && ((zcbor_multi_encode_minmax(0, 16, &(*input).command_spec_aliases_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).command_spec_aliases_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 16)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"summary", tmp_str.len = sizeof("summary") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).command_spec_summary))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"category", tmp_str.len = sizeof("category") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).command_spec_category))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"args_hint", tmp_str.len = sizeof("args_hint") - 1, &tmp_str)))))
	&& (zcbor_tstr_encode(state, (&(*input).command_spec_args_hint))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"subcommands", tmp_str.len = sizeof("subcommands") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 16) && ((zcbor_multi_encode_minmax(0, 16, &(*input).command_spec_subcommands_tstr_count, (zcbor_encoder_t *)zcbor_tstr_encode, state, (*&(*input).command_spec_subcommands_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 16)))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"scope", tmp_str.len = sizeof("scope") - 1, &tmp_str)))))
	&& (encode_command_scope(state, (&(*input).command_spec_scope))))
	&& (((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"surfaces", tmp_str.len = sizeof("surfaces") - 1, &tmp_str)))))
	&& (zcbor_list_start_encode(state, 16) && ((zcbor_multi_encode_minmax(0, 16, &(*input).command_spec_surfaces_command_surface_m_count, (zcbor_encoder_t *)encode_command_surface, state, (*&(*input).command_spec_surfaces_command_surface_m), sizeof(struct command_surface_r))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 16)))
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
	&& (zcbor_list_start_encode(state, 16) && ((zcbor_multi_encode_minmax(0, 16, &(*input).response_commands_Commands_command_spec_m_count, (zcbor_encoder_t *)encode_command_spec, state, (*&(*input).response_commands_Commands_command_spec_m), sizeof(struct command_spec))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 16)))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

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

static bool encode_api_response(
		zcbor_state_t *state, const struct api_response_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).api_response_choice == api_response_response_health_m_c) ? ((encode_response_health(state, (&(*input).api_response_response_health_m))))
	: (((*input).api_response_choice == api_response_response_session_page_m_c) ? ((encode_response_session_page(state, (&(*input).api_response_response_session_page_m))))
	: (((*input).api_response_choice == api_response_response_log_page_m_c) ? ((encode_response_log_page(state, (&(*input).api_response_response_log_page_m))))
	: (((*input).api_response_choice == api_response_response_fs_roots_m_c) ? ((encode_response_fs_roots(state, (&(*input).api_response_response_fs_roots_m))))
	: (((*input).api_response_choice == api_response_response_commands_m_c) ? ((encode_response_commands(state, (&(*input).api_response_response_commands_m))))
	: (((*input).api_response_choice == api_response_response_command_output_m_c) ? ((encode_response_command_output(state, (&(*input).api_response_response_command_output_m))))
	: (((*input).api_response_choice == api_response_response_ok_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Ok", tmp_str.len = sizeof("Ok") - 1, &tmp_str)))))
	: false)))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_api_request(
		zcbor_state_t *state, const struct api_request_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((((*input).api_request_choice == api_request_request_health_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"Health", tmp_str.len = sizeof("Health") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_sessions_query_m_c) ? ((encode_request_sessions_query(state, (&(*input).api_request_request_sessions_query_m))))
	: (((*input).api_request_choice == api_request_request_subscribe_m_c) ? ((encode_request_subscribe(state, (&(*input).api_request_request_subscribe_m))))
	: (((*input).api_request_choice == api_request_request_submit_m_c) ? ((encode_request_submit(state, (&(*input).api_request_request_submit_m))))
	: (((*input).api_request_choice == api_request_request_profile_list_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"ProfileList", tmp_str.len = sizeof("ProfileList") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_model_current_m_c) ? ((encode_request_model_current(state, (&(*input).api_request_request_model_current_m))))
	: (((*input).api_request_choice == api_request_request_fs_roots_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"FsRoots", tmp_str.len = sizeof("FsRoots") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_command_list_m_c) ? ((zcbor_tstr_encode(state, ((tmp_str.value = (uint8_t *)"CommandList", tmp_str.len = sizeof("CommandList") - 1, &tmp_str)))))
	: (((*input).api_request_choice == api_request_request_command_invoke_m_c) ? ((encode_request_command_invoke(state, (&(*input).api_request_request_command_invoke_m))))
	: false)))))))))));

	log_result(state, res, __func__);
	return res;
}



int cbor_encode_api_request(
		uint8_t *payload, size_t payload_len,
		const struct api_request_r *input,
		size_t *payload_len_out)
{
	zcbor_state_t states[11];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
		(zcbor_decoder_t *)encode_api_request, sizeof(states) / sizeof(zcbor_state_t), 1);
}


int cbor_encode_api_response(
		uint8_t *payload, size_t payload_len,
		const struct api_response_r *input,
		size_t *payload_len_out)
{
	zcbor_state_t states[14];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
		(zcbor_decoder_t *)encode_api_response, sizeof(states) / sizeof(zcbor_state_t), 1);
}
