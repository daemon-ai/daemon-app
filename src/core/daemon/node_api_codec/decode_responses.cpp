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
        const session_info& info = page.session_page_sessions_session_info_m[i];
        CachedSessionRow row;
        row.sessionId = fromZcbor(info.session_info_session);
        row.state = sessionStateName(info.session_info_state.session_state_choice);
        if (info.session_info_bound_profile_present &&
            info.session_info_bound_profile.session_info_bound_profile_choice ==
                session_info_bound_profile_r::session_info_bound_profile_profile_ref_m_c) {
            row.profileRef =
                fromZcbor(info.session_info_bound_profile.session_info_bound_profile_profile_ref_m);
        }
        if (info.session_info_title_present &&
            info.session_info_title.session_info_title_choice ==
                session_info_title_r::session_info_title_tstr_c) {
            row.title = fromZcbor(info.session_info_title.session_info_title_tstr);
        }
        if (info.session_info_lifecycle_present) {
            row.lifecycle =
                lifecycleName(info.session_info_lifecycle.session_info_lifecycle.lifecycle_choice);
        }
        if (info.session_info_role_present) {
            row.role = roleName(info.session_info_role.session_info_role.session_role_choice);
        }
        if (info.session_info_parent_present &&
            info.session_info_parent.session_info_parent_choice ==
                session_info_parent_r::session_info_parent_session_id_m_c) {
            row.parentSessionId =
                fromZcbor(info.session_info_parent.session_info_parent_session_id_m);
        }
        if (info.session_info_pinned_present) {
            row.pinned = info.session_info_pinned.session_info_pinned;
        }
        if (info.session_info_archived_present) {
            row.archived = info.session_info_archived.session_info_archived;
        }
        out->append(row);
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
                decoded.commandText =
                    fromZcbor(cmd.agent_command_start_turn_m.StartTurn_input.user_msg_text);
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
        // approval; it is the durable "allow permanently" offer signal (value not needed app-side).
        entry.hasFingerprint = info.approval_info_fingerprint_present;
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
    out->authorizationUrl = fromZcbor(begun.auth_begin_response_authorization_url);
    out->redirectUri = fromZcbor(begun.auth_begin_response_redirect_uri);
    out->expiresAt = begun.auth_begin_response_expires_at;
    out->flowKind = authFlowKindName(begun.auth_begin_response_flow_kind);
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
    const auth_complete_response& completed =
        response->api_response_response_auth_completed_m.response_auth_completed_AuthCompleted;
    out->credentialRef = fromZcbor(completed.auth_complete_response_credential_ref);
    out->accountLabel = fromZcbor(completed.auth_complete_response_account_label);
    out->transportInstance = fromZcbor(completed.auth_complete_response_transport_instance);
    out->hasBoundProfile = completed.auth_complete_response_bound_profile_choice ==
                           auth_complete_response::auth_complete_response_bound_profile_tstr_c;
    if (out->hasBoundProfile) {
        out->boundProfile = fromZcbor(completed.auth_complete_response_bound_profile_tstr);
    } else {
        out->boundProfile.clear();
    }
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

} // namespace daemonapp::daemon
