/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 16
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "daemon_api_smoke_decode.h"
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

static bool decode_session_scope(zcbor_state_t *state, struct session_scope_r *result);
static bool decode_repeated_session_query_scope(zcbor_state_t *state, struct session_query_scope *result);
static bool decode_repeated_session_query_after(zcbor_state_t *state, struct session_query_after_r *result);
static bool decode_repeated_session_query_limit(zcbor_state_t *state, struct session_query_limit *result);
static bool decode_session_query(zcbor_state_t *state, struct session_query *result);
static bool decode_request_sessions_query(zcbor_state_t *state, struct request_sessions_query *result);
static bool decode_request_subscribe(zcbor_state_t *state, struct request_subscribe *result);
static bool decode_blob_ref(zcbor_state_t *state, struct blob_ref *result);
static bool decode_repeated_user_msg_attachments(zcbor_state_t *state, struct user_msg_attachments_r *result);
static bool decode_user_msg(zcbor_state_t *state, struct user_msg *result);
static bool decode_agent_command_start_turn(zcbor_state_t *state, struct agent_command_start_turn *result);
static bool decode_agent_command_steer(zcbor_state_t *state, struct agent_command_steer *result);
static bool decode_agent_command(zcbor_state_t *state, struct agent_command_r *result);
static bool decode_origin(zcbor_state_t *state, struct origin *result);
static bool decode_repeated_Submit_origin(zcbor_state_t *state, struct Submit_origin_r *result);
static bool decode_repeated_Submit_profile(zcbor_state_t *state, struct Submit_profile_r *result);
static bool decode_request_submit(zcbor_state_t *state, struct request_submit *result);
static bool decode_request_model_current(zcbor_state_t *state, struct request_model_current *result);
static bool decode_command_invocation(zcbor_state_t *state, struct command_invocation *result);
static bool decode_request_command_invoke(zcbor_state_t *state, struct request_command_invoke *result);
static bool decode_service_health(zcbor_state_t *state, struct service_health *result);
static bool decode_health_report(zcbor_state_t *state, struct health_report *result);
static bool decode_response_health(zcbor_state_t *state, struct response_health *result);
static bool decode_session_state(zcbor_state_t *state, struct session_state_r *result);
static bool decode_repeated_session_info_bound_profile(zcbor_state_t *state, struct session_info_bound_profile_r *result);
static bool decode_repeated_session_info_title(zcbor_state_t *state, struct session_info_title_r *result);
static bool decode_lifecycle(zcbor_state_t *state, struct lifecycle_r *result);
static bool decode_repeated_session_info_lifecycle(zcbor_state_t *state, struct session_info_lifecycle *result);
static bool decode_session_role(zcbor_state_t *state, struct session_role_r *result);
static bool decode_repeated_session_info_role(zcbor_state_t *state, struct session_info_role *result);
static bool decode_repeated_session_info_parent(zcbor_state_t *state, struct session_info_parent_r *result);
static bool decode_repeated_session_info_pinned(zcbor_state_t *state, struct session_info_pinned *result);
static bool decode_repeated_session_info_archived(zcbor_state_t *state, struct session_info_archived *result);
static bool decode_session_info(zcbor_state_t *state, struct session_info *result);
static bool decode_repeated_session_page_next_cursor(zcbor_state_t *state, struct session_page_next_cursor_r *result);
static bool decode_session_page(zcbor_state_t *state, struct session_page *result);
static bool decode_response_session_page(zcbor_state_t *state, struct response_session_page *result);
static bool decode_direction(zcbor_state_t *state, struct direction_r *result);
static bool decode_disposition(zcbor_state_t *state, struct disposition_r *result);
static bool decode_session_payload(zcbor_state_t *state, struct session_payload *result);
static bool decode_session_log_entry(zcbor_state_t *state, struct session_log_entry *result);
static bool decode_log_page_view(zcbor_state_t *state, struct log_page_view *result);
static bool decode_response_log_page(zcbor_state_t *state, struct response_log_page *result);
static bool decode_repeated_fs_root_session(zcbor_state_t *state, struct fs_root_session_r *result);
static bool decode_fs_root(zcbor_state_t *state, struct fs_root *result);
static bool decode_response_fs_roots(zcbor_state_t *state, struct response_fs_roots *result);
static bool decode_command_scope(zcbor_state_t *state, struct command_scope_r *result);
static bool decode_command_surface(zcbor_state_t *state, struct command_surface_r *result);
static bool decode_command_access(zcbor_state_t *state, struct command_access_r *result);
static bool decode_command_spec(zcbor_state_t *state, struct command_spec *result);
static bool decode_response_commands(zcbor_state_t *state, struct response_commands *result);
static bool decode_command_output(zcbor_state_t *state, struct command_output *result);
static bool decode_response_command_output(zcbor_state_t *state, struct response_command_output *result);
static bool decode_api_response(zcbor_state_t *state, struct api_response_r *result);
static bool decode_api_request(zcbor_state_t *state, struct api_request_r *result);


static bool decode_session_scope(
		zcbor_state_t *state, struct session_scope_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"TopLevel", tmp_str.len = sizeof("TopLevel") - 1, &tmp_str))))) && (((*result).session_scope_choice = session_scope_TopLevel_tstr_c), true))
	|| (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ByProfile", tmp_str.len = sizeof("ByProfile") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).map_ByProfile))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))) && (((*result).session_scope_choice = session_scope_map_c), true))
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

static bool decode_session_query(
		zcbor_state_t *state, struct session_query *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_decode(state) && ((zcbor_present_decode(&((*result).session_query_scope_present), (zcbor_decoder_t *)decode_repeated_session_query_scope, state, (&(*result).session_query_scope))
	&& zcbor_present_decode(&((*result).session_query_after_present), (zcbor_decoder_t *)decode_repeated_session_query_after, state, (&(*result).session_query_after))
	&& zcbor_present_decode(&((*result).session_query_limit_present), (zcbor_decoder_t *)decode_repeated_session_query_limit, state, (&(*result).session_query_limit))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_session_query_scope(state, (&(*result).session_query_scope));
		decode_repeated_session_query_after(state, (&(*result).session_query_after));
		decode_repeated_session_query_limit(state, (&(*result).session_query_limit));
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

static bool decode_request_subscribe(
		zcbor_state_t *state, struct request_subscribe *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Subscribe", tmp_str.len = sizeof("Subscribe") - 1, &tmp_str)))))
	&& (zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"session", tmp_str.len = sizeof("session") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).Subscribe_session))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"after_seq", tmp_str.len = sizeof("after_seq") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).Subscribe_after_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"max", tmp_str.len = sizeof("max") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).Subscribe_max))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_blob_ref(
		zcbor_state_t *state, struct blob_ref *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"hash", tmp_str.len = sizeof("hash") - 1, &tmp_str)))))
	&& (zcbor_bstr_decode(state, (&(*result).blob_ref_hash))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"size", tmp_str.len = sizeof("size") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).blob_ref_size))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_user_msg_attachments(
		zcbor_state_t *state, struct user_msg_attachments_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"attachments", tmp_str.len = sizeof("attachments") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 16, &(*result).user_msg_attachments_blob_ref_m_count, (zcbor_decoder_t *)decode_blob_ref, state, (*&(*result).user_msg_attachments_blob_ref_m), sizeof(struct blob_ref))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

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
	&& (zcbor_uint32_decode(state, (&(*result).StartTurn_request_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

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
	&& (zcbor_uint32_decode(state, (&(*result).Steer_request_id))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

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
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Shutdown", tmp_str.len = sizeof("Shutdown") - 1, &tmp_str))))) && (((*result).agent_command_choice = agent_command_shutdown_m_c), true)))), zcbor_union_end_code(state), int_res))));

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
	&& (zcbor_tstr_decode(state, (&(*result).origin_scope))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

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
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 16, &(*result).health_report_services_service_health_m_count, (zcbor_decoder_t *)decode_service_health, state, (*&(*result).health_report_services_service_health_m), sizeof(struct service_health))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

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

static bool decode_session_state(
		zcbor_state_t *state, struct session_state_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Active", tmp_str.len = sizeof("Active") - 1, &tmp_str))))) && (((*result).session_state_choice = session_state_Active_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Ready", tmp_str.len = sizeof("Ready") - 1, &tmp_str))))) && (((*result).session_state_choice = session_state_Ready_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Completed", tmp_str.len = sizeof("Completed") - 1, &tmp_str))))) && (((*result).session_state_choice = session_state_Completed_tstr_c), true))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Unknown", tmp_str.len = sizeof("Unknown") - 1, &tmp_str))))) && (((*result).session_state_choice = session_state_Unknown_tstr_c), true))), zcbor_union_end_code(state), int_res))));

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
	&& zcbor_present_decode(&((*result).session_info_bound_profile_present), (zcbor_decoder_t *)decode_repeated_session_info_bound_profile, state, (&(*result).session_info_bound_profile))
	&& zcbor_present_decode(&((*result).session_info_title_present), (zcbor_decoder_t *)decode_repeated_session_info_title, state, (&(*result).session_info_title))
	&& zcbor_present_decode(&((*result).session_info_lifecycle_present), (zcbor_decoder_t *)decode_repeated_session_info_lifecycle, state, (&(*result).session_info_lifecycle))
	&& zcbor_present_decode(&((*result).session_info_role_present), (zcbor_decoder_t *)decode_repeated_session_info_role, state, (&(*result).session_info_role))
	&& zcbor_present_decode(&((*result).session_info_parent_present), (zcbor_decoder_t *)decode_repeated_session_info_parent, state, (&(*result).session_info_parent))
	&& zcbor_present_decode(&((*result).session_info_pinned_present), (zcbor_decoder_t *)decode_repeated_session_info_pinned, state, (&(*result).session_info_pinned))
	&& zcbor_present_decode(&((*result).session_info_archived_present), (zcbor_decoder_t *)decode_repeated_session_info_archived, state, (&(*result).session_info_archived))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_session_info_bound_profile(state, (&(*result).session_info_bound_profile));
		decode_repeated_session_info_title(state, (&(*result).session_info_title));
		decode_repeated_session_info_lifecycle(state, (&(*result).session_info_lifecycle));
		decode_repeated_session_info_role(state, (&(*result).session_info_role));
		decode_repeated_session_info_parent(state, (&(*result).session_info_parent));
		decode_repeated_session_info_pinned(state, (&(*result).session_info_pinned));
		decode_repeated_session_info_archived(state, (&(*result).session_info_archived));
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

static bool decode_session_page(
		zcbor_state_t *state, struct session_page *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"sessions", tmp_str.len = sizeof("sessions") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 16, &(*result).session_page_sessions_session_info_m_count, (zcbor_decoder_t *)decode_session_info, state, (*&(*result).session_page_sessions_session_info_m), sizeof(struct session_info))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& zcbor_present_decode(&((*result).session_page_next_cursor_present), (zcbor_decoder_t *)decode_repeated_session_page_next_cursor, state, (&(*result).session_page_next_cursor))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_session_info(state, (*&(*result).session_page_sessions_session_info_m));
		decode_repeated_session_page_next_cursor(state, (&(*result).session_page_next_cursor));
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

static bool decode_session_payload(
		zcbor_state_t *state, struct session_payload *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Command", tmp_str.len = sizeof("Command") - 1, &tmp_str)))))
	&& (decode_agent_command(state, (&(*result).session_payload_Command))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_session_log_entry(
		zcbor_state_t *state, struct session_log_entry *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((zcbor_map_start_decode(state) && (((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"seq", tmp_str.len = sizeof("seq") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).session_log_entry_seq))))
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
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 16, &(*result).log_page_view_entries_session_log_entry_m_count, (zcbor_decoder_t *)decode_session_log_entry, state, (*&(*result).log_page_view_entries_session_log_entry_m), sizeof(struct session_log_entry))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"next_seq", tmp_str.len = sizeof("next_seq") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).log_page_view_next_seq))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"head_seq", tmp_str.len = sizeof("head_seq") - 1, &tmp_str)))))
	&& (zcbor_uint32_decode(state, (&(*result).log_page_view_head_seq))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

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
	&& (zcbor_tstr_decode(state, (&(*result).fs_root_id))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"label", tmp_str.len = sizeof("label") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_root_label))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"kind", tmp_str.len = sizeof("kind") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).fs_root_kind))))
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
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 16, &(*result).response_fs_roots_FsRoots_fs_root_m_count, (zcbor_decoder_t *)decode_fs_root, state, (*&(*result).response_fs_roots_FsRoots_fs_root_m), sizeof(struct fs_root))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_fs_root(state, (*&(*result).response_fs_roots_FsRoots_fs_root_m));
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
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 16, &(*result).command_spec_aliases_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).command_spec_aliases_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"summary", tmp_str.len = sizeof("summary") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).command_spec_summary))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"category", tmp_str.len = sizeof("category") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).command_spec_category))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"args_hint", tmp_str.len = sizeof("args_hint") - 1, &tmp_str)))))
	&& (zcbor_tstr_decode(state, (&(*result).command_spec_args_hint))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"subcommands", tmp_str.len = sizeof("subcommands") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 16, &(*result).command_spec_subcommands_tstr_count, (zcbor_decoder_t *)zcbor_tstr_decode, state, (*&(*result).command_spec_subcommands_tstr), sizeof(struct zcbor_string))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"scope", tmp_str.len = sizeof("scope") - 1, &tmp_str)))))
	&& (decode_command_scope(state, (&(*result).command_spec_scope))))
	&& (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"surfaces", tmp_str.len = sizeof("surfaces") - 1, &tmp_str)))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 16, &(*result).command_spec_surfaces_command_surface_m_count, (zcbor_decoder_t *)decode_command_surface, state, (*&(*result).command_spec_surfaces_command_surface_m), sizeof(struct command_surface_r))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))
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
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(0, 16, &(*result).response_commands_Commands_command_spec_m_count, (zcbor_decoder_t *)decode_command_spec, state, (*&(*result).response_commands_Commands_command_spec_m), sizeof(struct command_spec))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

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

static bool decode_api_response(
		zcbor_state_t *state, struct api_response_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((decode_response_health(state, (&(*result).api_response_response_health_m)))) && (((*result).api_response_choice = api_response_response_health_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_response_session_page(state, (&(*result).api_response_response_session_page_m)))) && (((*result).api_response_choice = api_response_response_session_page_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_log_page(state, (&(*result).api_response_response_log_page_m)))) && (((*result).api_response_choice = api_response_response_log_page_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_fs_roots(state, (&(*result).api_response_response_fs_roots_m)))) && (((*result).api_response_choice = api_response_response_fs_roots_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_commands(state, (&(*result).api_response_response_commands_m)))) && (((*result).api_response_choice = api_response_response_commands_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_response_command_output(state, (&(*result).api_response_response_command_output_m)))) && (((*result).api_response_choice = api_response_response_command_output_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Ok", tmp_str.len = sizeof("Ok") - 1, &tmp_str))))) && (((*result).api_response_choice = api_response_response_ok_m_c), true)))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_api_request(
		zcbor_state_t *state, struct api_request_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) && (int_res = ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"Health", tmp_str.len = sizeof("Health") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_health_m_c), true))
	|| (((decode_request_sessions_query(state, (&(*result).api_request_request_sessions_query_m)))) && (((*result).api_request_choice = api_request_request_sessions_query_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_request_subscribe(state, (&(*result).api_request_request_subscribe_m)))) && (((*result).api_request_choice = api_request_request_subscribe_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((decode_request_submit(state, (&(*result).api_request_request_submit_m)))) && (((*result).api_request_choice = api_request_request_submit_m_c), true)))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"ProfileList", tmp_str.len = sizeof("ProfileList") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_profile_list_m_c), true)))
	|| (((decode_request_model_current(state, (&(*result).api_request_request_model_current_m)))) && (((*result).api_request_choice = api_request_request_model_current_m_c), true))
	|| (zcbor_union_elem_code(state) && (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"FsRoots", tmp_str.len = sizeof("FsRoots") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_fs_roots_m_c), true)))
	|| (((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"CommandList", tmp_str.len = sizeof("CommandList") - 1, &tmp_str))))) && (((*result).api_request_choice = api_request_request_command_list_m_c), true))
	|| (((decode_request_command_invoke(state, (&(*result).api_request_request_command_invoke_m)))) && (((*result).api_request_choice = api_request_request_command_invoke_m_c), true))), zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}



int cbor_decode_api_request(
		const uint8_t *payload, size_t payload_len,
		struct api_request_r *result,
		size_t *payload_len_out)
{
	zcbor_state_t states[11];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
		(zcbor_decoder_t *)decode_api_request, sizeof(states) / sizeof(zcbor_state_t), 1);
}


int cbor_decode_api_response(
		const uint8_t *payload, size_t payload_len,
		struct api_response_r *result,
		size_t *payload_len_out)
{
	zcbor_state_t states[14];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
		(zcbor_decoder_t *)decode_api_response, sizeof(states) / sizeof(zcbor_state_t), 1);
}
