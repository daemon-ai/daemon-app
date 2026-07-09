// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// NodeApiCodec response decoders: responseKind (the choice -> facade-kind lookup table) plus the
// chat/session, model, profile, and credential response projections. Each routes its preamble
// through codec_detail::decodeChecked; the per-field Qt projection stays explicit.

#include "daemon/node_api_codec/internal.h"

namespace daemonapp::daemon {

using namespace codec_detail;
ApiResponseKind NodeApiCodec::responseKind(const QByteArray& responseCbor) {
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get())) {
        return ApiResponseKind::Unknown;
    }
    // 1:1 generated-choice -> facade-kind map. A flat switch here is inherently cc~=N; a data table
    // keeps the projection trivial (cc ~3) and makes a missing arm a one-line add. Any arm not
    // listed falls through to Unknown, exactly as the old `default` did.
    struct KindEntry {
        decltype(api_response_r::api_response_choice) choice;
        ApiResponseKind kind;
    };
    static constexpr auto kKindMap = std::to_array<KindEntry>({
        {api_response_r::api_response_response_health_m_c, ApiResponseKind::Health},
        {api_response_r::api_response_response_session_page_m_c, ApiResponseKind::SessionPage},
        {api_response_r::api_response_response_session_detail_m_c, ApiResponseKind::SessionDetail},
        {api_response_r::api_response_response_log_page_m_c, ApiResponseKind::LogPage},
        {api_response_r::api_response_response_events_page_m_c, ApiResponseKind::EventsPage},
        {api_response_r::api_response_response_journal_m_c, ApiResponseKind::Journal},
        {api_response_r::api_response_response_fs_roots_m_c, ApiResponseKind::FsRoots},
        {api_response_r::api_response_response_fs_list_m_c, ApiResponseKind::FsList},
        {api_response_r::api_response_response_fs_stat_m_c, ApiResponseKind::FsStat},
        {api_response_r::api_response_response_fs_read_m_c, ApiResponseKind::FsRead},
        {api_response_r::api_response_response_fs_write_m_c, ApiResponseKind::FsWrite},
        {api_response_r::api_response_response_fs_watch_m_c, ApiResponseKind::FsWatch},
        {api_response_r::api_response_response_fs_search_m_c, ApiResponseKind::FsSearch},
        {api_response_r::api_response_response_fleet_m_c, ApiResponseKind::Fleet},
        {api_response_r::api_response_response_tree_m_c, ApiResponseKind::Tree},
        {api_response_r::api_response_response_unit_m_c, ApiResponseKind::Unit},
        {api_response_r::api_response_response_unit_events_m_c, ApiResponseKind::UnitEvents},
        {api_response_r::api_response_response_adapters_m_c, ApiResponseKind::Adapters},
        {api_response_r::api_response_response_transport_instances_m_c,
         ApiResponseKind::TransportInstances},
        {api_response_r::api_response_response_conversations_m_c, ApiResponseKind::Conversations},
        {api_response_r::api_response_response_ok_m_c, ApiResponseKind::Ok},
        {api_response_r::api_response_response_session_created_m_c,
         ApiResponseKind::SessionCreated},
        {api_response_r::api_response_response_credentials_m_c, ApiResponseKind::Credentials},
        {api_response_r::api_response_response_models_m_c, ApiResponseKind::Models},
        {api_response_r::api_response_response_model_current_m_c, ApiResponseKind::ModelCurrent},
        {api_response_r::api_response_response_provider_catalog_m_c,
         ApiResponseKind::ProviderCatalog},
        {api_response_r::api_response_response_provider_models_m_c,
         ApiResponseKind::ProviderModels},
        {api_response_r::api_response_response_profiles_m_c, ApiResponseKind::Profiles},
        {api_response_r::api_response_response_profile_m_c, ApiResponseKind::Profile},
        {api_response_r::api_response_response_soul_text_m_c, ApiResponseKind::SoulText},
        {api_response_r::api_response_response_distribution_m_c, ApiResponseKind::Distribution},
        {api_response_r::api_response_response_revisions_m_c, ApiResponseKind::Revisions},
        {api_response_r::api_response_response_drained_m_c, ApiResponseKind::Drained},
        {api_response_r::api_response_response_approvals_m_c, ApiResponseKind::Approvals},
        {api_response_r::api_response_response_commands_m_c, ApiResponseKind::Commands},
        {api_response_r::api_response_response_command_output_m_c, ApiResponseKind::CommandOutput},
        {api_response_r::api_response_response_session_search_m_c, ApiResponseKind::SessionSearch},
        {api_response_r::api_response_response_error_m_c, ApiResponseKind::Error},
        {api_response_r::api_response_response_model_search_m_c, ApiResponseKind::ModelSearch},
        {api_response_r::api_response_response_model_files_m_c, ApiResponseKind::ModelFiles},
        {api_response_r::api_response_response_model_download_started_m_c,
         ApiResponseKind::ModelDownloadStarted},
        {api_response_r::api_response_response_model_downloads_m_c,
         ApiResponseKind::ModelDownloads},
        {api_response_r::api_response_response_model_catalog_m_c, ApiResponseKind::ModelCatalog},
        {api_response_r::api_response_response_model_recommend_m_c,
         ApiResponseKind::ModelRecommend},
        {api_response_r::api_response_response_agent_catalog_m_c, ApiResponseKind::AgentCatalog},
        // app-wizard-auth stream additions (appended).
        {api_response_r::api_response_response_auth_providers_m_c, ApiResponseKind::AuthProviders},
        {api_response_r::api_response_response_auth_begun_m_c, ApiResponseKind::AuthBegun},
        {api_response_r::api_response_response_auth_stepped_m_c, ApiResponseKind::AuthStepped},
        {api_response_r::api_response_response_auth_completed_m_c, ApiResponseKind::AuthCompleted},
        {api_response_r::api_response_response_model_quantize_started_m_c,
         ApiResponseKind::ModelQuantizeStarted},
        {api_response_r::api_response_response_model_quantizes_m_c,
         ApiResponseKind::ModelQuantizes},
        {api_response_r::api_response_response_model_inspect_m_c, ApiResponseKind::ModelInspect},
        // app-client-verticals stream additions (appended).
        {api_response_r::api_response_response_checkpoints_m_c, ApiResponseKind::Checkpoints},
        {api_response_r::api_response_response_chat_routes_m_c, ApiResponseKind::ChatRoutes},
        {api_response_r::api_response_response_chat_route_m_c, ApiResponseKind::ChatRoute},
        {api_response_r::api_response_response_rooms_m_c, ApiResponseKind::Rooms},
        // user feedback over OpenTelemetry (wire v32).
        {api_response_r::api_response_response_feedback_ack_m_c, ApiResponseKind::FeedbackAck},
        {api_response_r::api_response_response_telemetry_consent_m_c,
         ApiResponseKind::TelemetryConsent},
        // node gateway status (wire v32).
        {api_response_r::api_response_response_gateway_status_m_c, ApiResponseKind::GatewayStatus},
        // custom OpenAI-compatible providers (generalized Daemon Cloud).
        {api_response_r::api_response_response_custom_providers_m_c,
         ApiResponseKind::CustomProviders},
    });
    for (const auto& entry : kKindMap) {
        if (response->api_response_choice == entry.choice) {
            return entry.kind;
        }
    }
    return ApiResponseKind::Unknown;
}

bool NodeApiCodec::decodeHealth(const QByteArray& responseCbor, DecodedHealth* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_health_m_c);
    if (!response) {
        return false;
    }
    const health_report& report = response->api_response_response_health_m.response_health_Health;
    out->allOk = report.health_report_all_ok;
    out->services.clear();
    for (size_t i = 0; i < report.health_report_services_service_health_m_count; ++i) {
        const service_health& svc = report.health_report_services_service_health_m[i];
        DecodedServiceHealth entry;
        entry.name = fromZcbor(svc.service_health_name);
        entry.ok = svc.service_health_ok;
        entry.restarts = svc.service_health_restarts;
        if (svc.service_health_detail_choice == service_health::service_health_detail_tstr_c) {
            entry.detail = fromZcbor(svc.service_health_detail_tstr);
        }
        out->services.append(entry);
    }
    return true;
}

bool NodeApiCodec::decodeSessionPage(const QByteArray& responseCbor, QList<CachedSessionRow>* out,
                                     QString* nextCursor, quint64* rev, QStringList* removed) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_session_page_m_c);
    if (!response) {
        return false;
    }
    const session_page& page =
        response->api_response_response_session_page_m.response_session_page_SessionPage;
    out->clear();
    for (size_t i = 0; i < page.session_page_sessions_session_info_m_count; ++i) {
        out->append(sessionRowFromInfo(page.session_page_sessions_session_info_m[i]));
    }
    if (nextCursor != nullptr) {
        nextCursor->clear();
        if (page.session_page_next_cursor_present &&
            page.session_page_next_cursor.session_page_next_cursor_choice ==
                session_page_next_cursor_r::session_page_next_cursor_session_id_m_c) {
            *nextCursor =
                fromZcbor(page.session_page_next_cursor.session_page_next_cursor_session_id_m);
        }
    }
    if (rev != nullptr) {
        *rev = page.session_page_rev;
    }
    if (removed != nullptr) {
        removed->clear();
        if (page.session_page_removed_present) {
            const session_page_removed_r& rm = page.session_page_removed;
            for (size_t i = 0; i < rm.session_page_removed_session_id_m_count; ++i) {
                removed->append(fromZcbor(rm.session_page_removed_session_id_m[i]));
            }
        }
    }
    return true;
}

namespace {
QString approvalModeName(int choice) {
    switch (choice) {
    case approval_mode_r::approval_mode_ask_tstr_c:
        return QStringLiteral("ask");
    case approval_mode_r::approval_mode_accept_edits_tstr_c:
        return QStringLiteral("accept_edits");
    case approval_mode_r::approval_mode_auto_allow_tstr_c:
        return QStringLiteral("auto_allow");
    case approval_mode_r::approval_mode_deny_tstr_c:
        return QStringLiteral("deny");
    default:
        return {};
    }
}

QString sinkKindName(int choice) {
    return choice == sink_kind_r::sink_kind_Spectator_tstr_c ? QStringLiteral("Spectator")
                                                             : QStringLiteral("Primary");
}
} // namespace

bool NodeApiCodec::decodeSessionDetail(const QByteArray& responseCbor, DecodedSessionDetail* out,
                                       bool* found) {
    if (out == nullptr || found == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_session_detail_m_c);
    if (!response) {
        return false;
    }
    const response_session_detail& rsd = response->api_response_response_session_detail_m;
    if (rsd.response_session_detail_SessionDetail_choice ==
        response_session_detail::response_session_detail_SessionDetail_null_m_c) {
        *found = false; // unknown session (null arm)
        return true;
    }
    *found = true;
    const session_detail& detail = rsd.response_session_detail_SessionDetail_session_detail_m;
    *out = DecodedSessionDetail{};
    out->info = sessionRowFromInfo(detail.session_detail_info);
    // Resolved model (top-level SessionDetail.model, optional-null).
    if (detail.session_detail_model_present &&
        detail.session_detail_model.session_detail_model_choice ==
            session_detail_model_r::session_detail_model_tstr_c) {
        out->hasModel = true;
        out->model = fromZcbor(detail.session_detail_model.session_detail_model_tstr);
    }
    // Approval mode rides the optional overlay.
    if (detail.session_detail_overlay_present &&
        detail.session_detail_overlay.session_detail_overlay_choice ==
            session_detail_overlay_r::session_detail_overlay_session_overlay_m_c) {
        const session_overlay& overlay =
            detail.session_detail_overlay.session_detail_overlay_session_overlay_m;
        if (overlay.session_overlay_approval_mode_present &&
            overlay.session_overlay_approval_mode.session_overlay_approval_mode_choice ==
                session_overlay_approval_mode_r::session_overlay_approval_mode_approval_mode_m_c) {
            out->hasApprovalMode = true;
            out->approvalMode = approvalModeName(
                overlay.session_overlay_approval_mode.session_overlay_approval_mode_approval_mode_m
                    .approval_mode_choice);
        }
    }
    if (detail.session_detail_delivery_targets_present) {
        const session_detail_delivery_targets_r& dts = detail.session_detail_delivery_targets;
        for (size_t i = 0; i < dts.session_detail_delivery_targets_delivery_target_m_count; ++i) {
            const delivery_target& dt = dts.session_detail_delivery_targets_delivery_target_m[i];
            DecodedDeliveryTarget target;
            target.transport = fromZcbor(dt.delivery_target_transport);
            target.route = fromZcbor(dt.delivery_target_route);
            target.kind = sinkKindName(dt.delivery_target_kind.sink_kind_choice);
            out->deliveryTargets.append(target);
        }
    }
    if (detail.session_detail_children_present) {
        const session_detail_children_r& ch = detail.session_detail_children;
        for (size_t i = 0; i < ch.session_detail_children_session_id_m_count; ++i) {
            out->children.append(fromZcbor(ch.session_detail_children_session_id_m[i]));
        }
    }
    if (detail.session_detail_checkpoints_present) {
        out->hasCheckpointCount = true;
        out->checkpointCount = detail.session_detail_checkpoints.session_detail_checkpoints;
    }
    // The Foreign-engine model backend (wire v30, optional-null): the AgentNative-vs-NodeProvider
    // fork the composer reads to decide whether the model picker draws agent-advertised choices or
    // the node catalog. Reuses the shared decodeForeignBackend helper.
    if (detail.session_detail_foreign_backend_present) {
        out->hasForeignBackend = true;
        out->foreignBackend = decodeForeignBackend(
            detail.session_detail_foreign_backend.session_detail_foreign_backend);
    }
    // The foreign agent's advertised Model selector (wire v30, optional-null): only a resident
    // foreign session whose agent advertises a Model config option carries one.
    if (detail.session_detail_model_selector_present &&
        detail.session_detail_model_selector.session_detail_model_selector_choice ==
            session_detail_model_selector_r::session_detail_model_selector_model_selector_m_c) {
        const model_selector& sel =
            detail.session_detail_model_selector.session_detail_model_selector_model_selector_m;
        out->hasModelSelector = true;
        out->modelSelector.optionId = fromZcbor(sel.model_selector_option_id);
        out->modelSelector.current = fromZcbor(sel.model_selector_current);
        for (size_t i = 0; i < sel.model_selector_choices_model_choice_m_count; ++i) {
            const model_choice& c = sel.model_selector_choices_model_choice_m[i];
            out->modelSelector.choices.append(
                DecodedModelChoice{fromZcbor(c.model_choice_id), fromZcbor(c.model_choice_label)});
        }
    }
    return true;
}

bool NodeApiCodec::decodeLogPage(const QByteArray& responseCbor, const QString& sessionId,
                                 QList<CachedLogRow>* out, quint64* nextSeq, quint64* headSeq) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_log_page_m_c);
    if (!response) {
        return false;
    }
    const log_page_view& page =
        response->api_response_response_log_page_m.response_log_page_LogPage;
    out->clear();
    for (size_t i = 0; i < page.log_page_view_entries_session_log_entry_m_count; ++i) {
        const session_log_entry& entry = page.log_page_view_entries_session_log_entry_m[i];
        CachedLogRow row;
        row.sessionId = sessionId;
        row.seq = entry.session_log_entry_seq;
        row.direction = entry.session_log_entry_direction.direction_choice ==
                                direction_r::direction_Outbound_tstr_c
                            ? QStringLiteral("Outbound")
                            : QStringLiteral("Inbound");
        row.disposition = entry.session_log_entry_disposition.disposition_choice ==
                                  disposition_r::disposition_Transport_tstr_c
                              ? QStringLiteral("Transport")
                              : QStringLiteral("Context");
        out->append(row);
    }
    if (nextSeq != nullptr) {
        *nextSeq = page.log_page_view_next_seq;
    }
    if (headSeq != nullptr) {
        *headSeq = page.log_page_view_head_seq;
    }
    return true;
}

bool NodeApiCodec::decodeLogPageEntries(const QByteArray& responseCbor, QList<DecodedLogEntry>* out,
                                        quint64* nextSeq, quint64* headSeq, quint64* epoch) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_log_page_m_c);
    if (!response) {
        return false;
    }
    const log_page_view& page =
        response->api_response_response_log_page_m.response_log_page_LogPage;
    out->clear();
    for (size_t i = 0; i < page.log_page_view_entries_session_log_entry_m_count; ++i) {
        const session_log_entry& entry = page.log_page_view_entries_session_log_entry_m[i];
        DecodedLogEntry decoded;
        decoded.seq = entry.session_log_entry_seq;
        decoded.direction = entry.session_log_entry_direction.direction_choice ==
                                    direction_r::direction_Outbound_tstr_c
                                ? QStringLiteral("Outbound")
                                : QStringLiteral("Inbound");
        decoded.disposition = entry.session_log_entry_disposition.disposition_choice ==
                                      disposition_r::disposition_Transport_tstr_c
                                  ? QStringLiteral("Transport")
                                  : QStringLiteral("Context");
        decoded.originTransport = fromZcbor(entry.session_log_entry_origin.origin_transport);
        const session_payload_r& payload = entry.session_log_entry_payload;
        switch (payload.session_payload_choice) {
        case session_payload_r::session_payload_event_m_c:
            decoded.payloadKind = DecodedLogEntry::PayloadKind::Event;
            decoded.event =
                decodeAgentEvent(payload.session_payload_event_m.session_payload_event_Event);
            break;
        case session_payload_r::session_payload_request_m_c:
            decoded.payloadKind = DecodedLogEntry::PayloadKind::Request;
            decodeHostRequest(payload.session_payload_request_m.session_payload_request_Request,
                              &decoded);
            break;
        case session_payload_r::session_payload_command_m_c: {
            decoded.payloadKind = DecodedLogEntry::PayloadKind::Command;
            // Extract the user-visible text of the conversational command arms so the transcript
            // can render the node's echo of the user's message (StartTurn) / steer note (Steer).
            const agent_command_r& cmd =
                payload.session_payload_command_m.session_payload_command_Command;
            if (cmd.agent_command_choice == agent_command_r::agent_command_start_turn_m_c) {
                const user_msg& input = cmd.agent_command_start_turn_m.StartTurn_input;
                decoded.commandText = fromZcbor(input.user_msg_text);
                // [wave2:app-delegation] F1/F2: decode the delegation completion-notice provenance.
                // `notice` is always present (possibly null); only a real CompletionNoticeRef
                // counts.
                if (input.user_msg_notice_present &&
                    input.user_msg_notice.user_msg_notice_choice ==
                        user_msg_notice_r::user_msg_notice_completion_notice_ref_m_c) {
                    const completion_notice_ref& ref =
                        input.user_msg_notice.user_msg_notice_completion_notice_ref_m;
                    decoded.hasNotice = true;
                    decoded.noticeChild = fromZcbor(ref.completion_notice_ref_child);
                    if (ref.completion_notice_ref_call_id_present &&
                        ref.completion_notice_ref_call_id.completion_notice_ref_call_id_choice ==
                            completion_notice_ref_call_id_r::completion_notice_ref_call_id_tstr_c) {
                        decoded.noticeCallId = fromZcbor(
                            ref.completion_notice_ref_call_id.completion_notice_ref_call_id_tstr);
                    }
                }
            } else if (cmd.agent_command_choice == agent_command_r::agent_command_steer_m_c) {
                decoded.commandText = fromZcbor(cmd.agent_command_steer_m.Steer_text);
            }
            break;
        }
        case session_payload_r::session_payload_response_m_c:
            decoded.payloadKind = DecodedLogEntry::PayloadKind::Response;
            break;
        case session_payload_r::session_payload_meta_m_c:
            decoded.payloadKind = DecodedLogEntry::PayloadKind::Meta;
            break;
        default:
            decoded.payloadKind = DecodedLogEntry::PayloadKind::None;
            break;
        }
        out->append(decoded);
    }
    if (nextSeq != nullptr) {
        *nextSeq = page.log_page_view_next_seq;
    }
    if (headSeq != nullptr) {
        *headSeq = page.log_page_view_head_seq;
    }
    if (epoch != nullptr) {
        *epoch = page.log_page_view_epoch;
    }
    return true;
}

bool NodeApiCodec::decodeEventsPage(const QByteArray& responseCbor, DecodedEventsPage* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_events_page_m_c);
    if (!response) {
        return false;
    }
    const events_page& page =
        response->api_response_response_events_page_m.response_events_page_EventsPage;
    out->events.clear();
    out->nextCursor = page.events_page_next_cursor;
    out->headCursor = page.events_page_head_cursor;
    for (size_t i = 0; i < page.events_page_events_node_event_m_count; ++i) {
        const node_event_r& ev = page.events_page_events_node_event_m[i];
        DecodedNodeEvent decoded;
        switch (ev.node_event_choice) {
        case node_event_r::node_event_session_advanced_m_c: {
            const node_event_session_advanced& m = ev.node_event_session_advanced_m;
            decoded.kind = DecodedNodeEvent::Kind::SessionAdvanced;
            decoded.session = fromZcbor(m.SessionAdvanced_session);
            decoded.epoch = m.SessionAdvanced_epoch;
            decoded.headSeq = m.SessionAdvanced_head_seq;
            break;
        }
        case node_event_r::node_event_session_meta_changed_m_c: {
            const node_event_session_meta_changed& m = ev.node_event_session_meta_changed_m;
            decoded.kind = DecodedNodeEvent::Kind::SessionMetaChanged;
            decoded.session = fromZcbor(m.SessionMetaChanged_session);
            decoded.rev = m.SessionMetaChanged_rev;
            break;
        }
        case node_event_r::node_event_roster_changed_m_c:
            decoded.kind = DecodedNodeEvent::Kind::RosterChanged;
            decoded.rev = ev.node_event_roster_changed_m.RosterChanged_rev;
            break;
        case node_event_r::node_event_approval_pending_m_c: {
            const node_event_approval_pending& m = ev.node_event_approval_pending_m;
            decoded.kind = DecodedNodeEvent::Kind::ApprovalPending;
            decoded.session = fromZcbor(m.ApprovalPending_session);
            decoded.requestId = fromZcbor(m.ApprovalPending_request_id);
            break;
        }
        case node_event_r::node_event_download_progress_m_c: {
            const node_event_download_progress& m = ev.node_event_download_progress_m;
            decoded.kind = DecodedNodeEvent::Kind::DownloadProgress;
            decoded.downloadId = m.DownloadProgress_id;
            decoded.pct = m.DownloadProgress_pct;
            decoded.state = fromZcbor(m.DownloadProgress_state);
            decoded.downloadedBytes = m.DownloadProgress_downloaded_bytes;
            decoded.totalBytes = m.DownloadProgress_total_bytes;
            break;
        }
        case node_event_r::node_event_catalog_changed_m_c:
            decoded.kind = DecodedNodeEvent::Kind::CatalogChanged;
            break;
        case node_event_r::node_event_resync_needed_m_c:
            decoded.kind = DecodedNodeEvent::Kind::ResyncNeeded;
            decoded.scope = fromZcbor(ev.node_event_resync_needed_m.ResyncNeeded_scope);
            break;
        // [wave2:app-channels-liveness] F5: fleet supervision changed (rev only) — the
        // SubscriptionManager re-queries the Tree. Previously fell through to Unknown (ignored).
        case node_event_r::node_event_fleet_changed_m_c:
            decoded.kind = DecodedNodeEvent::Kind::FleetChanged;
            decoded.rev = ev.node_event_fleet_changed_m.FleetChanged_rev;
            break;
        // Profile roster mutation (wire v31, rev only): a create/update/delete/select bumped the
        // profile revision. Codec-only here — the SubscriptionManager routing lands in a later
        // phase.
        case node_event_r::node_event_profiles_changed_m_c:
            decoded.kind = DecodedNodeEvent::Kind::ProfilesChanged;
            decoded.rev = ev.node_event_profiles_changed_m.ProfilesChanged_rev;
            break;
        // [wave2:app-channels-liveness] B5: live per-account transport presence. Map the generated
        // connection/presence enums to the same lowercase names the TransportInstances decode uses.
        case node_event_r::node_event_transport_changed_m_c: {
            const node_event_transport_changed& m = ev.node_event_transport_changed_m;
            decoded.kind = DecodedNodeEvent::Kind::TransportChanged;
            decoded.transport = fromZcbor(m.TransportChanged_transport);
            decoded.connection = connectionStateName(m.TransportChanged_connection);
            decoded.hasPresence = m.TransportChanged_presence_present;
            if (decoded.hasPresence) {
                decoded.presence =
                    presenceStateName(m.TransportChanged_presence.TransportChanged_presence);
            }
            // [waveB:app-v30] D1: disconnect provenance (wire v30, all optional).
            if (m.TransportChanged_reason_present &&
                m.TransportChanged_reason.TransportChanged_reason_choice ==
                    TransportChanged_reason_r::TransportChanged_reason_disconnect_reason_m_c) {
                decoded.hasReason = true;
                decoded.reason = disconnectReasonName(
                    m.TransportChanged_reason.TransportChanged_reason_disconnect_reason_m);
            }
            if (m.TransportChanged_message_present &&
                m.TransportChanged_message.TransportChanged_message_choice ==
                    TransportChanged_message_r::TransportChanged_message_tstr_c) {
                decoded.hasMessage = true;
                decoded.message =
                    fromZcbor(m.TransportChanged_message.TransportChanged_message_tstr);
            }
            if (m.TransportChanged_fatal_present) {
                decoded.fatal = m.TransportChanged_fatal.TransportChanged_fatal;
            }
            break;
        }
        // [waveB:app-v30] D2: conversation-set + membership deltas. Invalidation pointers — the
        // SubscriptionManager refetches; no membership fact is derived here.
        case node_event_r::node_event_conversations_changed_m_c: {
            const node_event_conversations_changed& m = ev.node_event_conversations_changed_m;
            decoded.kind = DecodedNodeEvent::Kind::ConversationsChanged;
            decoded.transport = fromZcbor(m.ConversationsChanged_transport);
            decoded.conv = fromZcbor(m.ConversationsChanged_conv);
            decoded.convChange = m.ConversationsChanged_change.conv_change_choice ==
                                         conv_change_r::conv_change_Added_tstr_c
                                     ? QStringLiteral("added")
                                     : QStringLiteral("removed");
            break;
        }
        case node_event_r::node_event_membership_changed_m_c: {
            const node_event_membership_changed& m = ev.node_event_membership_changed_m;
            decoded.kind = DecodedNodeEvent::Kind::MembershipChanged;
            decoded.transport = fromZcbor(m.MembershipChanged_transport);
            decoded.conv = fromZcbor(m.MembershipChanged_conv);
            decoded.member = fromZcbor(m.MembershipChanged_member);
            switch (m.MembershipChanged_change.membership_change_choice) {
            case membership_change_r::membership_change_Joined_tstr_c:
                decoded.membershipChange = QStringLiteral("joined");
                break;
            case membership_change_r::membership_change_Left_tstr_c:
                decoded.membershipChange = QStringLiteral("left");
                break;
            case membership_change_r::membership_change_Invited_tstr_c:
                decoded.membershipChange = QStringLiteral("invited");
                break;
            case membership_change_r::membership_change_Kicked_tstr_c:
                decoded.membershipChange = QStringLiteral("kicked");
                break;
            case membership_change_r::membership_change_Banned_tstr_c:
            default:
                decoded.membershipChange = QStringLiteral("banned");
                break;
            }
            if (m.MembershipChanged_actor_present &&
                m.MembershipChanged_actor.MembershipChanged_actor_choice ==
                    MembershipChanged_actor_r::MembershipChanged_actor_tstr_c) {
                decoded.actor = fromZcbor(m.MembershipChanged_actor.MembershipChanged_actor_tstr);
            }
            if (m.MembershipChanged_reason_present &&
                m.MembershipChanged_reason.MembershipChanged_reason_choice ==
                    MembershipChanged_reason_r::MembershipChanged_reason_tstr_c) {
                decoded.memberReason =
                    fromZcbor(m.MembershipChanged_reason.MembershipChanged_reason_tstr);
            }
            decoded.isSelf = m.MembershipChanged_is_self;
            break;
        }
        // [acct-mgmt] A transport's contact roster changed (wire v34). Invalidation pointer only —
        // the SubscriptionManager refetches that transport's RosterList; no contact fact is derived
        // here. `transport` is the only payload (distinct from the session-inbox RosterChanged).
        case node_event_r::node_event_contacts_changed_m_c:
            decoded.kind = DecodedNodeEvent::Kind::ContactsChanged;
            decoded.transport =
                fromZcbor(ev.node_event_contacts_changed_m.ContactsChanged_transport);
            break;
        default:
            decoded.kind = DecodedNodeEvent::Kind::Unknown;
            break;
        }
        out->events.append(decoded);
    }
    return true;
}

bool NodeApiCodec::decodeJournal(const QByteArray& responseCbor, QList<DecodedJournalRecord>* out,
                                 quint64* nextCursor, quint64* headCursor, bool* hasSealedAfter,
                                 quint64* sealedAfter) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_journal_m_c);
    if (!response) {
        return false;
    }
    const journal_page_view& page =
        response->api_response_response_journal_m.response_journal_Journal;
    out->clear();
    for (size_t i = 0; i < page.journal_page_view_entries_journal_record_m_count; ++i) {
        const journal_record& rec = page.journal_page_view_entries_journal_record_m[i];
        DecodedJournalRecord decoded;
        decoded.cursor = rec.journal_record_cursor;
        decoded.seq = rec.journal_record_seq;
        decoded.epoch = rec.journal_record_epoch;
        decoded.kind = fromZcbor(rec.journal_record_kind);
        const journal_record_payload_t_r& payload = rec.journal_record_payload;
        if (payload.journal_record_payload_t_choice ==
            journal_record_payload_t_r::journal_record_payload_t_journal_record_payload_block_m_c) {
            decoded.isBlock = true;
            decoded.block = decodeTranscriptBlock(
                payload.journal_record_payload_t_journal_record_payload_block_m.Block_block);
        } else {
            decoded.isBlock = false;
            decoded.managementDetail =
                fromZcbor(payload.journal_record_payload_t_journal_record_payload_management_m
                              .Management_detail);
        }
        out->append(decoded);
    }
    if (nextCursor != nullptr) {
        *nextCursor = page.journal_page_view_next_cursor;
    }
    if (headCursor != nullptr) {
        *headCursor = page.journal_page_view_head_cursor;
    }
    const bool sealed = page.journal_page_view_sealed_after_choice ==
                        journal_page_view::journal_page_view_sealed_after_uint64_m_c;
    if (hasSealedAfter != nullptr) {
        *hasSealedAfter = sealed;
    }
    if (sealedAfter != nullptr) {
        *sealedAfter = sealed ? page.journal_page_view_sealed_after_uint64_m : 0;
    }
    return true;
}

bool NodeApiCodec::decodeDrained(const QByteArray& responseCbor, QList<DecodedAgentEvent>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_drained_m_c);
    if (!response) {
        return false;
    }
    const response_drained& drained = response->api_response_response_drained_m;
    out->clear();
    for (size_t i = 0; i < drained.response_drained_Drained_outbound_m_count; ++i) {
        const outbound_r& item = drained.response_drained_Drained_outbound_m[i];
        if (item.outbound_choice == outbound_r::outbound_event_m_c) {
            out->append(decodeAgentEvent(item.outbound_event_m.outbound_event_Event));
        }
    }
    return true;
}

bool NodeApiCodec::decodeCredentials(const QByteArray& responseCbor,
                                     QList<DecodedCredentialInfo>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_credentials_m_c);
    if (!response) {
        return false;
    }
    const response_credentials& creds = response->api_response_response_credentials_m;
    out->clear();
    for (size_t i = 0; i < creds.response_credentials_Credentials_credential_info_m_count; ++i) {
        const credential_info& info = creds.response_credentials_Credentials_credential_info_m[i];
        DecodedCredentialInfo entry;
        entry.profile = fromZcbor(info.credential_info_profile);
        entry.present = info.credential_info_present;
        entry.hint = fromZcbor(info.credential_info_hint);
        // [acct-mgmt] wire v35: node-persisted display label (explicit-null / absent = none).
        if (info.credential_info_label_present &&
            info.credential_info_label.credential_info_label_choice ==
                credential_info_label_r::credential_info_label_tstr_c) {
            entry.label = fromZcbor(info.credential_info_label.credential_info_label_tstr);
        }
        out->append(entry);
    }
    return true;
}

bool NodeApiCodec::decodeModels(const QByteArray& responseCbor, QList<DecodedModelDescriptor>* out,
                                QString* next) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_models_m_c);
    if (!response) {
        return false;
    }
    const model_page& page = response->api_response_response_models_m.response_models_Models;
    out->clear();
    for (size_t i = 0; i < page.model_page_items_model_descriptor_m_count; ++i) {
        DecodedModelDescriptor descriptor;
        fillDescriptor(page.model_page_items_model_descriptor_m[i], &descriptor);
        out->append(descriptor);
    }
    if (next != nullptr) {
        // The resume cursor (wire v25): present + non-null => more pages remain.
        const bool hasNext =
            page.model_page_next_present && page.model_page_next.model_page_next_choice ==
                                                model_page_next_r::model_page_next_tstr_c;
        *next = hasNext ? fromZcbor(page.model_page_next.model_page_next_tstr) : QString();
    }
    return true;
}

bool NodeApiCodec::decodeProviderCatalog(const QByteArray& responseCbor,
                                         QList<DecodedProviderDescriptor>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_provider_catalog_m_c);
    if (!response) {
        return false;
    }
    const response_provider_catalog& catalog = response->api_response_response_provider_catalog_m;
    out->clear();
    for (size_t i = 0;
         i < catalog.response_provider_catalog_ProviderCatalog_provider_descriptor_m_count; ++i) {
        DecodedProviderDescriptor descriptor;
        fillProviderDescriptor(
            catalog.response_provider_catalog_ProviderCatalog_provider_descriptor_m[i],
            &descriptor);
        out->append(descriptor);
    }
    return true;
}

bool NodeApiCodec::decodeCustomProviders(const QByteArray& responseCbor,
                                         QList<DecodedCustomProvider>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_custom_providers_m_c);
    if (!response) {
        return false;
    }
    const response_custom_providers& list = response->api_response_response_custom_providers_m;
    out->clear();
    for (size_t i = 0; i < list.response_custom_providers_CustomProviders_custom_provider_m_count;
         ++i) {
        DecodedCustomProvider cp;
        fillCustomProvider(list.response_custom_providers_CustomProviders_custom_provider_m[i],
                           &cp);
        out->append(cp);
    }
    return true;
}

bool NodeApiCodec::decodeProviderModels(const QByteArray& responseCbor,
                                        QList<DecodedModelDescriptor>* out, QString* next) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_provider_models_m_c);
    if (!response) {
        return false;
    }
    const model_page& page =
        response->api_response_response_provider_models_m.response_provider_models_ProviderModels;
    out->clear();
    for (size_t i = 0; i < page.model_page_items_model_descriptor_m_count; ++i) {
        DecodedModelDescriptor descriptor;
        fillDescriptor(page.model_page_items_model_descriptor_m[i], &descriptor);
        out->append(descriptor);
    }
    if (next != nullptr) {
        // The resume cursor (wire v25): present + non-null => more pages remain.
        const bool hasNext =
            page.model_page_next_present && page.model_page_next.model_page_next_choice ==
                                                model_page_next_r::model_page_next_tstr_c;
        *next = hasNext ? fromZcbor(page.model_page_next.model_page_next_tstr) : QString();
    }
    return true;
}

bool NodeApiCodec::decodeModelCurrent(const QByteArray& responseCbor, DecodedModelDescriptor* out,
                                      bool* hasModel) {
    if (out == nullptr || hasModel == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_model_current_m_c);
    if (!response) {
        return false;
    }
    const response_model_current& current = response->api_response_response_model_current_m;
    if (current.response_model_current_ModelCurrent_choice ==
        response_model_current::response_model_current_ModelCurrent_model_descriptor_m_c) {
        *hasModel = true;
        fillDescriptor(current.response_model_current_ModelCurrent_model_descriptor_m, out);
    } else {
        *hasModel = false;
    }
    return true;
}

bool NodeApiCodec::decodeError(const QByteArray& responseCbor, DecodedApiError* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_error_m_c);
    if (!response) {
        return false;
    }
    const api_error_r& err = response->api_response_response_error_m.response_error_Error;
    switch (err.api_error_choice) {
    case api_error_r::api_error_unknown_session_m_c:
        out->kind = QStringLiteral("UnknownSession");
        out->message =
            fromZcbor(err.api_error_unknown_session_m.api_error_unknown_session_UnknownSession);
        break;
    case api_error_r::api_error_unsupported_m_c:
        out->kind = QStringLiteral("Unsupported");
        out->message = fromZcbor(err.api_error_unsupported_m.api_error_unsupported_Unsupported);
        break;
    case api_error_r::api_error_conflict_m_c:
        out->kind = QStringLiteral("Conflict");
        out->message = fromZcbor(err.api_error_conflict_m.api_error_conflict_Conflict);
        break;
    case api_error_r::api_error_unauthenticated_m_c:
        out->kind = QStringLiteral("Unauthenticated");
        out->message =
            fromZcbor(err.api_error_unauthenticated_m.api_error_unauthenticated_Unauthenticated);
        break;
    case api_error_r::api_error_forbidden_m_c:
        out->kind = QStringLiteral("Forbidden");
        out->message = fromZcbor(err.api_error_forbidden_m.api_error_forbidden_Forbidden);
        break;
    default:
        out->kind = QStringLiteral("Other");
        out->message = fromZcbor(err.api_error_other_m.api_error_other_Other);
        break;
    }
    return true;
}

bool NodeApiCodec::decodeProfiles(const QByteArray& responseCbor, QList<DecodedProfileInfo>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_profiles_m_c);
    if (!response) {
        return false;
    }
    const response_profiles& profiles = response->api_response_response_profiles_m;
    out->clear();
    for (size_t i = 0; i < profiles.response_profiles_Profiles_profile_info_m_count; ++i) {
        const profile_info& info = profiles.response_profiles_Profiles_profile_info_m[i];
        DecodedProfileInfo entry;
        entry.id = fromZcbor(info.profile_info_id);
        entry.provider = providerName(info.profile_info_provider.provider_selector_choice);
        entry.model = fromZcbor(info.profile_info_model);
        entry.isActive = info.profile_info_is_active;
        // Provenance (wire v31, optional-null): created_by (author) + owner (session id).
        if (info.profile_info_created_by_present &&
            info.profile_info_created_by.profile_info_created_by_choice ==
                profile_info_created_by_r::profile_info_created_by_author_m_c) {
            entry.hasCreatedBy = true;
            entry.createdBy =
                authorToString(info.profile_info_created_by.profile_info_created_by_author_m,
                               &entry.createdByIsAgent);
        }
        if (info.profile_info_owner_present &&
            info.profile_info_owner.profile_info_owner_choice ==
                profile_info_owner_r::profile_info_owner_tstr_c) {
            entry.hasOwner = true;
            entry.owner = fromZcbor(info.profile_info_owner.profile_info_owner_tstr);
        }
        out->append(entry);
    }
    return true;
}

bool NodeApiCodec::decodeProfile(const QByteArray& responseCbor, DecodedProfileSpec* out,
                                 bool* found) {
    if (out == nullptr || found == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_profile_m_c);
    if (!response) {
        return false;
    }
    const response_profile& profile = response->api_response_response_profile_m;
    if (profile.response_profile_Profile_choice ==
        response_profile::response_profile_Profile_profile_spec_m_c) {
        *found = true;
        *out = decodeProfileSpecStruct(profile.response_profile_Profile_profile_spec_m);
    } else {
        *found = false;
    }
    return true;
}

bool NodeApiCodec::decodeSoulText(const QByteArray& responseCbor, QString* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_soul_text_m_c);
    if (!response) {
        return false;
    }
    *out = fromZcbor(response->api_response_response_soul_text_m.response_soul_text_SoulText);
    return true;
}

bool NodeApiCodec::decodeSessionCreated(const QByteArray& responseCbor, QString* outId) {
    if (outId == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_session_created_m_c);
    if (!response) {
        return false;
    }
    *outId = fromZcbor(response->api_response_response_session_created_m.SessionCreated_session);
    return true;
}

bool NodeApiCodec::decodeProfileId(const QByteArray& responseCbor, QString* outId) {
    if (outId == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_profile_id_m_c);
    if (!response) {
        return false;
    }
    *outId = fromZcbor(response->api_response_response_profile_id_m.response_profile_id_ProfileId);
    return true;
}

bool NodeApiCodec::decodeDistribution(const QByteArray& responseCbor, DecodedDistribution* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_distribution_m_c);
    if (!response) {
        return false;
    }
    const distribution& d =
        response->api_response_response_distribution_m.response_distribution_Distribution;
    out->wireVersion = d.distribution_wire_version;
    out->profile = decodeProfileSpecStruct(d.distribution_profile);
    out->source =
        (d.distribution_source_present && d.distribution_source.distribution_source_choice ==
                                              distribution_source_r::distribution_source_tstr_c)
            ? fromZcbor(d.distribution_source.distribution_source_tstr)
            : QString();
    // The persona (SOUL.md) text (wire v36, `? soul`): present + non-null carries the profile's
    // persona so an export/import round-trip preserves it. Absent/null for a Foreign profile.
    out->hasSoul = d.distribution_soul_present && d.distribution_soul.distribution_soul_choice ==
                                                      distribution_soul_r::distribution_soul_tstr_c;
    out->soul = out->hasSoul ? fromZcbor(d.distribution_soul.distribution_soul_tstr) : QString();
    out->skills.clear();
    for (size_t i = 0; i < d.distribution_skills_skill_bundle_m_count; ++i) {
        const skill_bundle& sb = d.distribution_skills_skill_bundle_m[i];
        DecodedSkillBundle b;
        b.name = fromZcbor(sb.skill_bundle_name);
        if (sb.skill_bundle_category_choice == skill_bundle::skill_bundle_category_tstr_c) {
            b.category = fromZcbor(sb.skill_bundle_category_tstr);
        }
        for (size_t f = 0; f < sb.files_tstrtstr_count; ++f) {
            b.files.insert(fromZcbor(sb.files_tstrtstr[f].skill_bundle_files_tstrtstr_key),
                           fromZcbor(sb.files_tstrtstr[f].files_tstrtstr));
        }
        out->skills.append(b);
    }
    return true;
}

bool NodeApiCodec::decodeRevisions(const QByteArray& responseCbor, QList<DecodedRevision>* out,
                                   QString* next) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_revisions_m_c);
    if (!response) {
        return false;
    }
    const revision_page& page =
        response->api_response_response_revisions_m.response_revisions_Revisions;
    out->clear();
    for (size_t i = 0; i < page.revision_page_items_revision_m_count; ++i) {
        const revision& r = page.revision_page_items_revision_m[i];
        DecodedRevision d;
        d.seq = r.revision_seq;
        d.hasParent = r.revision_parent_choice == revision::revision_parent_uint64_m_c;
        if (d.hasParent) {
            d.parent = r.revision_parent_uint64_m;
        }
        d.author = r.revision_author.author_choice == author_r::author_agent_m_c
                       ? fromZcbor(r.revision_author.author_agent_m.author_agent_agent)
                       : QStringLiteral("operator");
        d.reason = fromZcbor(r.revision_reason);
        d.tsMs = r.revision_ts_ms;
        out->append(d);
    }
    if (next != nullptr) {
        // The resume cursor (wire v25): present + non-null => more pages remain.
        const bool hasNext =
            page.revision_page_next_present && page.revision_page_next.revision_page_next_choice ==
                                                   revision_page_next_r::revision_page_next_tstr_c;
        *next = hasNext ? fromZcbor(page.revision_page_next.revision_page_next_tstr) : QString();
    }
    return true;
}

bool NodeApiCodec::decodeModelSearch(const QByteArray& responseCbor, QList<DecodedSearchHit>* out,
                                     bool* hasMore, quint32* page) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_model_search_m_c);
    if (!response) {
        return false;
    }
    const search_page& sp =
        response->api_response_response_model_search_m.response_model_search_ModelSearch;
    out->clear();
    for (size_t i = 0; i < sp.search_page_results_search_hit_m_count; ++i) {
        const search_hit& h = sp.search_page_results_search_hit_m[i];
        DecodedSearchHit hit;
        hit.repo = fromZcbor(h.search_hit_repo);
        if (h.search_hit_author_choice == search_hit::search_hit_author_tstr_c) {
            hit.author = fromZcbor(h.search_hit_author_tstr);
        }
        hit.downloads = h.search_hit_downloads;
        hit.likes = h.search_hit_likes;
        if (h.search_hit_num_parameters_choice ==
            search_hit::search_hit_num_parameters_uint64_m_c) {
            hit.hasNumParameters = true;
            hit.numParameters = h.search_hit_num_parameters_uint64_m;
        }
        if (h.search_hit_pipeline_tag_choice == search_hit::search_hit_pipeline_tag_tstr_c) {
            hit.pipelineTag = fromZcbor(h.search_hit_pipeline_tag_tstr);
        }
        if (h.search_hit_last_modified_choice == search_hit::search_hit_last_modified_tstr_c) {
            hit.lastModified = fromZcbor(h.search_hit_last_modified_tstr);
        }
        hit.gated = h.search_hit_gated;
        hit.isPrivate = h.search_hit_private;
        out->append(hit);
    }
    if (hasMore != nullptr) {
        *hasMore = sp.search_page_has_more;
    }
    if (page != nullptr) {
        *page = sp.search_page_page;
    }
    return true;
}

bool NodeApiCodec::decodeModelFiles(const QByteArray& responseCbor, QList<DecodedModelFile>* out,
                                    QString* next) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_model_files_m_c);
    if (!response) {
        return false;
    }
    const model_file_page& page =
        response->api_response_response_model_files_m.response_model_files_ModelFiles;
    out->clear();
    for (size_t i = 0; i < page.model_file_page_items_model_file_m_count; ++i) {
        const model_file& f = page.model_file_page_items_model_file_m[i];
        DecodedModelFile file;
        file.path = fromZcbor(f.model_file_path);
        file.sizeBytes = f.model_file_size_bytes;
        if (f.model_file_quant_choice == model_file::model_file_quant_tstr_c) {
            file.quant = fromZcbor(f.model_file_quant_tstr);
        }
        file.isSplit = f.model_file_is_split;
        file.isFirstShard = f.model_file_is_first_shard;
        file.isMmproj = f.model_file_is_mmproj;
        out->append(file);
    }
    if (next != nullptr) {
        // The resume cursor (wire v25): present + non-null => more pages remain.
        const bool hasNext = page.model_file_page_next_present &&
                             page.model_file_page_next.model_file_page_next_choice ==
                                 model_file_page_next_r::model_file_page_next_tstr_c;
        *next =
            hasNext ? fromZcbor(page.model_file_page_next.model_file_page_next_tstr) : QString();
    }
    return true;
}

bool NodeApiCodec::decodeModelDownloadStarted(const QByteArray& responseCbor, quint64* outId) {
    if (outId == nullptr) {
        return false;
    }
    const auto response = decodeChecked(
        responseCbor, api_response_r::api_response_response_model_download_started_m_c);
    if (!response) {
        return false;
    }
    *outId = response->api_response_response_model_download_started_m
                 .response_model_download_started_ModelDownloadStarted;
    return true;
}

bool NodeApiCodec::decodeModelDownloads(const QByteArray& responseCbor,
                                        QList<DecodedDownloadStatus>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_model_downloads_m_c);
    if (!response) {
        return false;
    }
    const response_model_downloads& dl = response->api_response_response_model_downloads_m;
    out->clear();
    for (size_t i = 0; i < dl.response_model_downloads_ModelDownloads_download_status_m_count;
         ++i) {
        const download_status& s = dl.response_model_downloads_ModelDownloads_download_status_m[i];
        DecodedDownloadStatus status;
        status.id = s.download_status_id;
        fillModelRef(s.download_status_model, &status.modelRepo, &status.modelFile, nullptr);
        status.state = downloadStateName(s.download_status_state.download_state_choice);
        status.downloadedBytes = s.download_status_downloaded_bytes;
        status.totalBytes = s.download_status_total_bytes;
        status.filesDone = s.download_status_files_done;
        status.filesTotal = s.download_status_files_total;
        if (s.download_status_error_choice == download_status::download_status_error_tstr_c) {
            status.error = fromZcbor(s.download_status_error_tstr);
        }
        out->append(status);
    }
    return true;
}

bool NodeApiCodec::decodeModelCatalog(const QByteArray& responseCbor,
                                      QList<DecodedInstalledModel>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_model_catalog_m_c);
    if (!response) {
        return false;
    }
    const response_model_catalog& cat = response->api_response_response_model_catalog_m;
    out->clear();
    for (size_t i = 0; i < cat.response_model_catalog_ModelCatalog_installed_model_m_count; ++i) {
        const installed_model& m = cat.response_model_catalog_ModelCatalog_installed_model_m[i];
        DecodedInstalledModel model;
        model.id = fromZcbor(m.installed_model_id);
        model.displayName = fromZcbor(m.installed_model_display_name);
        fillModelRef(m.installed_model_model, &model.repo, &model.file, &model.engine);
        model.localPath = fromZcbor(m.installed_model_local_path);
        model.sizeBytes = m.installed_model_size_bytes;
        if (m.installed_model_quant_choice == installed_model::installed_model_quant_tstr_c) {
            model.quant = fromZcbor(m.installed_model_quant_tstr);
        }
        model.installedAtMs = m.installed_model_installed_at_ms;
        if (m.installed_model_arch_present &&
            m.installed_model_arch.installed_model_arch_choice ==
                installed_model_arch_r::installed_model_arch_tstr_c) {
            model.arch = fromZcbor(m.installed_model_arch.installed_model_arch_tstr);
        }
        if (m.installed_model_context_length_present &&
            m.installed_model_context_length.installed_model_context_length_choice ==
                installed_model_context_length_r::installed_model_context_length_uint_c) {
            model.hasContextLength = true;
            model.contextLength =
                m.installed_model_context_length.installed_model_context_length_uint;
        }
        if (m.installed_model_file_type_present &&
            m.installed_model_file_type.installed_model_file_type_choice ==
                installed_model_file_type_r::installed_model_file_type_tstr_c) {
            model.fileType = fromZcbor(m.installed_model_file_type.installed_model_file_type_tstr);
        }
        if (m.installed_model_mmproj_path_present &&
            m.installed_model_mmproj_path.installed_model_mmproj_path_choice ==
                installed_model_mmproj_path_r::installed_model_mmproj_path_tstr_c) {
            model.mmprojPath =
                fromZcbor(m.installed_model_mmproj_path.installed_model_mmproj_path_tstr);
        }
        out->append(model);
    }
    return true;
}

bool NodeApiCodec::decodeModelRecommend(const QByteArray& responseCbor,
                                        DecodedQuantRecommendation* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_model_recommend_m_c);
    if (!response) {
        return false;
    }
    const quant_recommendation& r =
        response->api_response_response_model_recommend_m.response_model_recommend_ModelRecommend;
    *out = DecodedQuantRecommendation{};
    out->engine = modelEngineName(r.quant_recommendation_engine.model_engine_choice);
    out->repo = fromZcbor(r.quant_recommendation_repo);
    if (r.quant_recommendation_file_choice ==
        quant_recommendation::quant_recommendation_file_tstr_c) {
        out->file = fromZcbor(r.quant_recommendation_file_tstr);
    }
    out->quant = fromZcbor(r.quant_recommendation_quant);
    if (r.quant_recommendation_size_bytes_choice ==
        quant_recommendation::quant_recommendation_size_bytes_uint64_m_c) {
        out->hasSizeBytes = true;
        out->sizeBytes = r.quant_recommendation_size_bytes_uint64_m;
    }
    out->budgetBytes = r.quant_recommendation_budget_bytes;
    out->fits = r.quant_recommendation_fits;
    out->reason = fromZcbor(r.quant_recommendation_reason);
    for (size_t i = 0; i < r.quant_recommendation_candidates_quant_candidate_m_count; ++i) {
        const quant_candidate& c = r.quant_recommendation_candidates_quant_candidate_m[i];
        DecodedQuantCandidate cand;
        cand.quant = fromZcbor(c.quant_candidate_quant);
        if (c.quant_candidate_file_choice == quant_candidate::quant_candidate_file_tstr_c) {
            cand.file = fromZcbor(c.quant_candidate_file_tstr);
        }
        if (c.quant_candidate_size_bytes_choice ==
            quant_candidate::quant_candidate_size_bytes_uint64_m_c) {
            cand.hasSizeBytes = true;
            cand.sizeBytes = c.quant_candidate_size_bytes_uint64_m;
        }
        cand.fits = c.quant_candidate_fits;
        out->candidates.append(cand);
    }
    return true;
}

bool NodeApiCodec::decodeApprovals(const QByteArray& responseCbor, QList<DecodedApprovalInfo>* out,
                                   QString* next) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_approvals_m_c);
    if (!response) {
        return false;
    }
    const approval_page& page =
        response->api_response_response_approvals_m.response_approvals_Approvals;
    out->clear();
    for (size_t i = 0; i < page.approval_page_items_approval_info_m_count; ++i) {
        const approval_info& info = page.approval_page_items_approval_info_m[i];
        DecodedApprovalInfo entry;
        entry.session = fromZcbor(info.approval_info_session);
        entry.requestId = fromZcbor(info.approval_info_request_id);
        entry.prompt = fromZcbor(info.approval_info_prompt);
        if (info.approval_info_path_present &&
            info.approval_info_path.approval_info_path_choice ==
                approval_info_path_r::approval_info_path_tstr_c) {
            entry.hasPath = true;
            entry.path = fromZcbor(info.approval_info_path.approval_info_path_tstr);
        }
        // Wire v28: a present fingerprint means the node can remember a permanent decision for this
        // approval; it is the durable "allow permanently" offer signal.
        entry.hasFingerprint = info.approval_info_fingerprint_present &&
                               info.approval_info_fingerprint.approval_info_fingerprint_choice ==
                                   approval_info_fingerprint_r::approval_info_fingerprint_tstr_c;
        // [wave2:app-approvals-safety] D3: carry the fingerprint hex string for the structured
        // prompt's fingerprint chip (display/correlation only).
        if (entry.hasFingerprint) {
            entry.fingerprint =
                fromZcbor(info.approval_info_fingerprint.approval_info_fingerprint_tstr);
        }
        // [waveB:app-v30] D5: optional structured detail (ToolDetail{kind, body}, wire v30).
        if (info.approval_info_detail_present) {
            const tool_detail& td = info.approval_info_detail.approval_info_detail_tool_detail_m;
            entry.detailKind = fromZcbor(td.tool_detail_kind);
            entry.detailBody = bytesFromZcbor(td.tool_detail_body);
        }
        out->append(entry);
    }
    if (next != nullptr) {
        // The resume cursor (wire v25): present + non-null => more pages remain.
        const bool hasNext =
            page.approval_page_next_present && page.approval_page_next.approval_page_next_choice ==
                                                   approval_page_next_r::approval_page_next_tstr_c;
        *next = hasNext ? fromZcbor(page.approval_page_next.approval_page_next_tstr) : QString();
    }
    return true;
}

// [wave2:app-approvals-safety] D2: decode a Tools response (wire v29) into the node-wide inventory.
bool NodeApiCodec::decodeTools(const QByteArray& responseCbor, QList<DecodedToolInfo>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_tools_m_c);
    if (!response) {
        return false;
    }
    const response_tools& tools = response->api_response_response_tools_m;
    out->clear();
    for (size_t i = 0; i < tools.response_tools_Tools_tool_info_m_count; ++i) {
        const tool_info& info = tools.response_tools_Tools_tool_info_m[i];
        DecodedToolInfo entry;
        entry.name = fromZcbor(info.tool_info_name);
        if (info.tool_info_description_present &&
            info.tool_info_description.tool_info_description_choice ==
                tool_info_description_r::tool_info_description_tstr_c) {
            entry.description = fromZcbor(info.tool_info_description.tool_info_description_tstr);
        }
        entry.enabled = info.tool_info_enabled;
        if (info.tool_info_requires_present &&
            info.tool_info_requires.tool_info_requires_choice ==
                tool_info_requires_r::tool_info_requires_tstr_c) {
            entry.requires_ = fromZcbor(info.tool_info_requires.tool_info_requires_tstr);
        }
        out->append(entry);
    }
    return true;
}

// [wave2:app-approvals-safety] D4: decode a Fingerprints response (wire v29) into a session's
// remembered allow-list. `label` is always absent today (the node stores only the hash).
bool NodeApiCodec::decodeFingerprints(const QByteArray& responseCbor,
                                      QList<DecodedRememberedFingerprint>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_fingerprints_m_c);
    if (!response) {
        return false;
    }
    const response_fingerprints& fps = response->api_response_response_fingerprints_m;
    out->clear();
    for (size_t i = 0; i < fps.response_fingerprints_Fingerprints_remembered_fingerprint_m_count;
         ++i) {
        const remembered_fingerprint& fp =
            fps.response_fingerprints_Fingerprints_remembered_fingerprint_m[i];
        DecodedRememberedFingerprint entry;
        entry.fingerprint = fromZcbor(fp.remembered_fingerprint_fingerprint);
        if (fp.remembered_fingerprint_label_present &&
            fp.remembered_fingerprint_label.remembered_fingerprint_label_choice ==
                remembered_fingerprint_label_r::remembered_fingerprint_label_tstr_c) {
            entry.label =
                fromZcbor(fp.remembered_fingerprint_label.remembered_fingerprint_label_tstr);
        }
        // [waveB:app-v30] D6: when the node remembered it (wire v30, optional).
        if (fp.remembered_fingerprint_remembered_at_ms_present) {
            entry.rememberedAtMs =
                fp.remembered_fingerprint_remembered_at_ms.remembered_fingerprint_remembered_at_ms;
        }
        out->append(entry);
    }
    return true;
}

bool NodeApiCodec::decodeCommands(const QByteArray& responseCbor, QList<DecodedCommandSpec>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_commands_m_c);
    if (!response) {
        return false;
    }
    const response_commands& commands = response->api_response_response_commands_m;
    out->clear();
    for (size_t i = 0; i < commands.response_commands_Commands_command_spec_m_count; ++i) {
        const command_spec& spec = commands.response_commands_Commands_command_spec_m[i];
        DecodedCommandSpec entry;
        entry.name = fromZcbor(spec.command_spec_name);
        entry.summary = fromZcbor(spec.command_spec_summary);
        entry.category = fromZcbor(spec.command_spec_category);
        entry.argsHint = fromZcbor(spec.command_spec_args_hint);
        entry.sideEffecting = spec.command_spec_side_effecting;
        out->append(entry);
    }
    return true;
}

bool NodeApiCodec::decodeCommandOutput(const QByteArray& responseCbor, QString* outText) {
    if (outText == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_command_output_m_c);
    if (!response) {
        return false;
    }
    *outText = fromZcbor(response->api_response_response_command_output_m
                             .response_command_output_CommandOutput.command_output_text);
    return true;
}

bool NodeApiCodec::decodeSessionSearch(const QByteArray& responseCbor,
                                       QList<DecodedSessionSearchHit>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_session_search_m_c);
    if (!response) {
        return false;
    }
    const response_session_search& search = response->api_response_response_session_search_m;
    out->clear();
    for (size_t i = 0; i < search.response_session_search_SessionSearch_session_search_hit_m_count;
         ++i) {
        const session_search_hit& hit =
            search.response_session_search_SessionSearch_session_search_hit_m[i];
        DecodedSessionSearchHit entry;
        entry.session = fromZcbor(hit.session_search_hit_session);
        entry.title = fromZcbor(hit.session_search_hit_title);
        entry.snippet = fromZcbor(hit.session_search_hit_snippet);
        out->append(entry);
    }
    return true;
}

// --- Foreign-agent catalog (foreign engines; wire v29) -------------------------------------------

bool NodeApiCodec::decodeAgentCatalog(const QByteArray& responseCbor,
                                      QList<DecodedAgentEntry>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_agent_catalog_m_c);
    if (!response) {
        return false;
    }
    const response_agent_catalog& catalog = response->api_response_response_agent_catalog_m;
    out->clear();
    for (size_t i = 0; i < catalog.response_agent_catalog_AgentCatalog_agent_entry_m_count; ++i) {
        const agent_entry& row = catalog.response_agent_catalog_AgentCatalog_agent_entry_m[i];
        DecodedAgentEntry entry;
        entry.name = fromZcbor(row.agent_entry_name);
        switch (row.agent_entry_source.agent_source_choice) {
        case agent_source_r::agent_source_Manual_tstr_c:
            entry.source = QStringLiteral("Manual");
            break;
        case agent_source_r::agent_source_Endpoint_tstr_c:
            entry.source = QStringLiteral("Endpoint");
            break;
        default:
            entry.source = QStringLiteral("Builtin");
            break;
        }
        // Protocol (wire v29): ACP vs the Claude-Code stream-json bridge. Absent => Acp (the
        // #[serde(default)] on the node side), so a pre-v29 row decodes as ACP.
        if (row.agent_entry_protocol_present &&
            row.agent_entry_protocol.agent_entry_protocol.agent_protocol_choice ==
                agent_protocol_r::agent_protocol_StreamJson_tstr_c) {
            entry.protocol = QStringLiteral("StreamJson");
        } else {
            entry.protocol = QStringLiteral("Acp");
        }
        if (row.agent_entry_installed_present) {
            entry.installed = row.agent_entry_installed.agent_entry_installed;
        }
        if (row.agent_entry_version_present &&
            row.agent_entry_version.agent_entry_version_choice ==
                agent_entry_version_r::agent_entry_version_tstr_c) {
            entry.version = fromZcbor(row.agent_entry_version.agent_entry_version_tstr);
        }
        // The node-derived trust verdict (wire v32): absent => NotInstalled (the serde default), so
        // a pre-v32 row decodes as NotInstalled without a client re-derive.
        if (row.agent_entry_verification_present) {
            entry.verification = agentVerificationFromChoice(
                row.agent_entry_verification.agent_entry_verification.agent_verification_choice);
        }
        // The launch recipe is deliberately NOT decoded: recipes are node-side, operator-managed
        // state; no client surface renders or re-sends them (profiles bind BY NAME only).
        out->append(entry);
    }
    return true;
}

// --- app-wizard-auth stream decoders (appended as one block; sibling streams append after) ------

namespace {
// The wire auth-flow-kind as its Rust variant name (the client treats it as an opaque label).
QString authFlowKindName(const auth_flow_kind_r& kind) {
    switch (kind.auth_flow_kind_choice) {
    case auth_flow_kind_r::auth_flow_kind_OAuth2Pkce_tstr_c:
        return QStringLiteral("OAuth2Pkce");
    case auth_flow_kind_r::auth_flow_kind_MatrixSso_tstr_c:
    default:
        return QStringLiteral("MatrixSso");
    }
}

// [waveB:app-v31] Project the wire `auth_challenge_r` union into the decoded per-kind view.
DecodedAuthChallenge decodeAuthChallenge(const auth_challenge_r& wire) {
    DecodedAuthChallenge out;
    switch (wire.auth_challenge_choice) {
    case auth_challenge_r::auth_challenge_redirect_m_c:
        out.kind = AuthChallengeKind::Redirect;
        out.authorizationUrl = fromZcbor(wire.auth_challenge_redirect_m.Redirect_authorization_url);
        break;
    case auth_challenge_r::auth_challenge_form_m_c: {
        out.kind = AuthChallengeKind::Form;
        const auth_challenge_form& form = wire.auth_challenge_form_m;
        out.formTitle = fromZcbor(form.Form_title);
        for (size_t i = 0; i < form.Form_fields_auth_param_field_m_count; ++i) {
            const auth_param_field& field = form.Form_fields_auth_param_field_m[i];
            out.formFields.append(DecodedAuthParamField{fromZcbor(field.auth_param_field_key),
                                                        fromZcbor(field.auth_param_field_label),
                                                        field.auth_param_field_required});
        }
        break;
    }
    case auth_challenge_r::auth_challenge_qr_m_c: {
        out.kind = AuthChallengeKind::Qr;
        const auth_challenge_qr& qr = wire.auth_challenge_qr_m;
        out.qrPayload = fromZcbor(qr.Qr_payload);
        if (qr.Qr_image_choice == auth_challenge_qr::Qr_image_byte_array_m_c) {
            out.qrImage = bytesFromZcbor(qr.Qr_image_byte_array_m);
        }
        out.qrPollIntervalMs = qr.Qr_poll_interval_ms;
        break;
    }
    case auth_challenge_r::auth_challenge_message_m_c:
    default:
        out.kind = AuthChallengeKind::Message;
        out.messageText = fromZcbor(wire.auth_challenge_message_m.Message_text);
        break;
    }
    return out;
}

// [waveB:app-v31] Project the wire `auth_complete_response` into the decoded completion view.
DecodedAuthCompleteResponse decodeAuthComplete(const auth_complete_response& completed) {
    DecodedAuthCompleteResponse out;
    out.credentialRef = fromZcbor(completed.auth_complete_response_credential_ref);
    out.accountLabel = fromZcbor(completed.auth_complete_response_account_label);
    out.transportInstance = fromZcbor(completed.auth_complete_response_transport_instance);
    out.hasBoundProfile = completed.auth_complete_response_bound_profile_choice ==
                          auth_complete_response::auth_complete_response_bound_profile_tstr_c;
    if (out.hasBoundProfile) {
        out.boundProfile = fromZcbor(completed.auth_complete_response_bound_profile_tstr);
    }
    return out;
}
} // namespace

bool NodeApiCodec::decodeAuthProviders(const QByteArray& responseCbor,
                                       QList<DecodedAuthProviderInfo>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_auth_providers_m_c);
    if (!response) {
        return false;
    }
    const response_auth_providers& providers = response->api_response_response_auth_providers_m;
    out->clear();
    for (size_t i = 0;
         i < providers.response_auth_providers_AuthProviders_auth_provider_info_m_count; ++i) {
        const auth_provider_info& row =
            providers.response_auth_providers_AuthProviders_auth_provider_info_m[i];
        DecodedAuthProviderInfo info;
        info.family = fromZcbor(row.auth_provider_info_family);
        info.flowKind = authFlowKindName(row.auth_provider_info_flow_kind);
        info.displayName = fromZcbor(row.auth_provider_info_display_name);
        for (size_t j = 0; j < row.auth_provider_info_params_schema_auth_param_field_m_count; ++j) {
            const auth_param_field& field =
                row.auth_provider_info_params_schema_auth_param_field_m[j];
            info.paramsSchema.append(DecodedAuthParamField{fromZcbor(field.auth_param_field_key),
                                                           fromZcbor(field.auth_param_field_label),
                                                           field.auth_param_field_required});
        }
        out->append(info);
    }
    return true;
}

bool NodeApiCodec::decodeAuthBegun(const QByteArray& responseCbor, DecodedAuthBeginResponse* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_auth_begun_m_c);
    if (!response) {
        return false;
    }
    const auth_begin_response& begun =
        response->api_response_response_auth_begun_m.response_auth_begun_AuthBegun;
    out->flowId = fromZcbor(begun.auth_begin_response_flow_id);
    out->challenge = decodeAuthChallenge(begun.auth_begin_response_challenge);
    out->expiresAt = begun.auth_begin_response_expires_at;
    return true;
}

bool NodeApiCodec::decodeAuthStepped(const QByteArray& responseCbor, DecodedAuthStepResult* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_auth_stepped_m_c);
    if (!response) {
        return false;
    }
    const auth_step_result_r& result =
        response->api_response_response_auth_stepped_m.response_auth_stepped_AuthStepped;
    if (result.auth_step_result_choice == auth_step_result_r::auth_step_result_completed_m_c) {
        out->completed = true;
        out->completion = decodeAuthComplete(
            result.auth_step_result_completed_m.auth_step_result_completed_Completed);
    } else {
        out->completed = false;
        out->challenge = decodeAuthChallenge(
            result.auth_step_result_challenge_m.auth_step_result_challenge_Challenge);
    }
    return true;
}

bool NodeApiCodec::decodeAuthCompleted(const QByteArray& responseCbor,
                                       DecodedAuthCompleteResponse* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_auth_completed_m_c);
    if (!response) {
        return false;
    }
    *out = decodeAuthComplete(
        response->api_response_response_auth_completed_m.response_auth_completed_AuthCompleted);
    return true;
}

bool NodeApiCodec::decodeModelQuantizeStarted(const QByteArray& responseCbor, quint64* outId) {
    if (outId == nullptr) {
        return false;
    }
    const auto response = decodeChecked(
        responseCbor, api_response_r::api_response_response_model_quantize_started_m_c);
    if (!response) {
        return false;
    }
    *outId = response->api_response_response_model_quantize_started_m
                 .response_model_quantize_started_ModelQuantizeStarted;
    return true;
}

bool NodeApiCodec::decodeModelQuantizes(const QByteArray& responseCbor,
                                        QList<DecodedQuantizeStatus>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_model_quantizes_m_c);
    if (!response) {
        return false;
    }
    const response_model_quantizes& jobs = response->api_response_response_model_quantizes_m;
    out->clear();
    for (size_t i = 0; i < jobs.response_model_quantizes_ModelQuantizes_quantize_status_m_count;
         ++i) {
        const quantize_status& row =
            jobs.response_model_quantizes_ModelQuantizes_quantize_status_m[i];
        DecodedQuantizeStatus job;
        job.id = row.quantize_status_id;
        job.repo = fromZcbor(row.quantize_status_repo);
        job.sourceFile = fromZcbor(row.quantize_status_source_file);
        job.targetQuant = fromZcbor(row.quantize_status_target_quant);
        switch (row.quantize_status_state.quantize_state_choice) {
        case quantize_state_r::quantize_state_Preparing_tstr_c:
            job.state = QStringLiteral("Preparing");
            break;
        case quantize_state_r::quantize_state_Quantizing_tstr_c:
            job.state = QStringLiteral("Quantizing");
            break;
        case quantize_state_r::quantize_state_Completed_tstr_c:
            job.state = QStringLiteral("Completed");
            break;
        case quantize_state_r::quantize_state_Failed_tstr_c:
            job.state = QStringLiteral("Failed");
            break;
        case quantize_state_r::quantize_state_Queued_tstr_c:
        default:
            job.state = QStringLiteral("Queued");
            break;
        }
        if (row.quantize_status_output_path_choice ==
            quantize_status::quantize_status_output_path_tstr_c) {
            job.outputPath = fromZcbor(row.quantize_status_output_path_tstr);
        }
        if (row.quantize_status_model_id_choice ==
            quantize_status::quantize_status_model_id_model_id_m_c) {
            job.modelId = fromZcbor(row.quantize_status_model_id_model_id_m);
        }
        if (row.quantize_status_error_choice == quantize_status::quantize_status_error_tstr_c) {
            job.error = fromZcbor(row.quantize_status_error_tstr);
        }
        out->append(job);
    }
    return true;
}

bool NodeApiCodec::decodeModelInspect(const QByteArray& responseCbor, DecodedGgufInfo* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_model_inspect_m_c);
    if (!response) {
        return false;
    }
    const gguf_info& info =
        response->api_response_response_model_inspect_m.response_model_inspect_ModelInspect;
    *out = DecodedGgufInfo{};
    if (info.gguf_info_architecture_choice == gguf_info::gguf_info_architecture_tstr_c) {
        out->architecture = fromZcbor(info.gguf_info_architecture_tstr);
    }
    if (info.gguf_info_name_choice == gguf_info::gguf_info_name_tstr_c) {
        out->name = fromZcbor(info.gguf_info_name_tstr);
    }
    if (info.gguf_info_file_type_choice == gguf_info::gguf_info_file_type_tstr_c) {
        out->fileType = fromZcbor(info.gguf_info_file_type_tstr);
    }
    out->hasContextLength =
        info.gguf_info_context_length_choice == gguf_info::gguf_info_context_length_uint_c;
    if (out->hasContextLength) {
        out->contextLength = info.gguf_info_context_length_uint;
    }
    out->sizeBytes = info.gguf_info_size_bytes;
    return true;
}

// --- Checkpoints (E4/TOOL-9) ---------------------------------------------------------------------

bool NodeApiCodec::decodeCheckpoints(const QByteArray& responseCbor,
                                     QList<DecodedCheckpointInfo>* out, QString* next) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_checkpoints_m_c);
    if (!response) {
        return false;
    }
    const checkpoint_page& page =
        response->api_response_response_checkpoints_m.response_checkpoints_Checkpoints;
    out->clear();
    for (size_t i = 0; i < page.checkpoint_page_items_checkpoint_info_m_count; ++i) {
        const checkpoint_info& info = page.checkpoint_page_items_checkpoint_info_m[i];
        DecodedCheckpointInfo entry;
        entry.id = fromZcbor(info.checkpoint_info_id);
        entry.session = fromZcbor(info.checkpoint_info_session);
        entry.tool = fromZcbor(info.checkpoint_info_tool);
        entry.createdUnix = info.checkpoint_info_created_unix;
        if (info.checkpoint_info_turn_ordinal_present &&
            info.checkpoint_info_turn_ordinal.checkpoint_info_turn_ordinal_choice ==
                checkpoint_info_turn_ordinal_r::checkpoint_info_turn_ordinal_uint64_m_c) {
            entry.hasTurnOrdinal = true;
            entry.turnOrdinal =
                info.checkpoint_info_turn_ordinal.checkpoint_info_turn_ordinal_uint64_m;
        }
        if (info.checkpoint_info_cursor_present &&
            info.checkpoint_info_cursor.checkpoint_info_cursor_choice ==
                checkpoint_info_cursor_r::checkpoint_info_cursor_uint64_m_c) {
            entry.hasCursor = true;
            entry.cursorSeq = info.checkpoint_info_cursor.checkpoint_info_cursor_uint64_m;
        }
        out->append(entry);
    }
    if (next != nullptr) {
        // The resume cursor (wire v25): present + non-null => more pages remain.
        const bool hasNext = page.checkpoint_page_next_present &&
                             page.checkpoint_page_next.checkpoint_page_next_choice ==
                                 checkpoint_page_next_r::checkpoint_page_next_tstr_c;
        *next =
            hasNext ? fromZcbor(page.checkpoint_page_next.checkpoint_page_next_tstr) : QString();
    }
    return true;
}

// --- User feedback over OpenTelemetry (wire v32) ------------------------------------------------
bool NodeApiCodec::decodeFeedbackAck(const QByteArray& responseCbor, bool* accepted, bool* queued) {
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_feedback_ack_m_c);
    if (!response) {
        return false;
    }
    const feedback_ack& ack =
        response->api_response_response_feedback_ack_m.response_feedback_ack_FeedbackAck;
    if (accepted != nullptr) {
        *accepted = ack.feedback_ack_accepted;
    }
    if (queued != nullptr) {
        *queued = ack.feedback_ack_queued;
    }
    return true;
}

bool NodeApiCodec::decodeTelemetryConsent(const QByteArray& responseCbor, bool* enabled) {
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_telemetry_consent_m_c);
    if (!response) {
        return false;
    }
    if (enabled != nullptr) {
        *enabled = response->api_response_response_telemetry_consent_m.TelemetryConsent_enabled;
    }
    return true;
}

// --- Node gateway (wire v32) --------------------------------------------------------------------
bool NodeApiCodec::decodeGatewayStatus(const QByteArray& responseCbor, DecodedGatewayStatus* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_gateway_status_m_c);
    if (!response) {
        return false;
    }
    const gateway_status& s =
        response->api_response_response_gateway_status_m.response_gateway_status_GatewayStatus;
    *out = DecodedGatewayStatus{};
    out->enabled = s.gateway_status_enabled;
    out->listening = s.gateway_status_listening;
    if (s.gateway_status_addr_choice == gateway_status::gateway_status_addr_tstr_c) {
        out->hasAddr = true;
        out->addr = fromZcbor(s.gateway_status_addr_tstr);
    }
    if (s.gateway_status_last_error_choice == gateway_status::gateway_status_last_error_tstr_c) {
        out->hasLastError = true;
        out->lastError = fromZcbor(s.gateway_status_last_error_tstr);
    }
    return true;
}

} // namespace daemonapp::daemon
