// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/mirror_decode.h"

#include "daemon/node_api_codec/internal.h"

// The G1 generated mapper declarations (raw zcbor DTO → canonical entity). Bodies are human-owned
// in entities_map.cpp; the drift gate checks the signatures. This is the single wire→canonical
// point (§3.3) — nothing here re-derives a field.
#include "mirror/generated/entities_map_gen.h"

namespace daemonapp::daemon {

using codec_detail::decodeChecked;

bool decodeConversationsToMirror(const QByteArray& responseCbor,
                                 std::vector<mirror::Conversation>* out, QString* next,
                                 quint64* rev, QStringList* removed) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_conversations_m_c);
    if (!response) {
        return false;
    }
    const conv_page& page =
        response->api_response_response_conversations_m.response_conversations_Conversations;
    out->clear();
    out->reserve(page.conv_page_items_conversation_info_m_count);
    for (size_t i = 0; i < page.conv_page_items_conversation_info_m_count; ++i) {
        out->push_back(mirror::map_conversation(page.conv_page_items_conversation_info_m[i]));
    }
    if (next != nullptr) {
        const bool hasNext =
            page.conv_page_next_present &&
            page.conv_page_next.conv_page_next_choice == conv_page_next_r::conv_page_next_tstr_c;
        *next =
            hasNext ? codec_detail::fromZcbor(page.conv_page_next.conv_page_next_tstr) : QString();
    }
    // [api/39 §10.2] The collection rev + the delta-read `removed` tombstone id list.
    if (rev != nullptr) {
        *rev = page.conv_page_rev;
    }
    if (removed != nullptr) {
        removed->clear();
        if (page.conv_page_removed_present) {
            for (size_t i = 0; i < page.conv_page_removed.conv_page_removed_tstr_count; ++i) {
                removed->append(
                    codec_detail::fromZcbor(page.conv_page_removed.conv_page_removed_tstr[i]));
            }
        }
    }
    return true;
}

bool decodeChatHistoryToMirror(const QByteArray& responseCbor, const QString& transport,
                               const QString& conv, std::vector<mirror::ChatMessage>* out,
                               quint64* nextCursor, quint64* headCursor) {
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
        // The conversation journal only appends Chat records; skip any Block/Management
        // defensively.
        if (rec.journal_record_payload.journal_record_payload_t_choice !=
            journal_record_payload_t_r::journal_record_payload_t_journal_record_payload_chat_m_c) {
            continue;
        }
        mirror::ChatMessage msg = mirror::map_chat_message(
            rec.journal_record_payload.journal_record_payload_t_journal_record_payload_chat_m
                .Chat_message);
        // Scope + envelope fields the payload does not carry (§3.6 merging rule): the ConvHistory
        // request scope + the journal-record cursor. origin_op (rung-3 provenance) rides the
        // envelope from api/39 (BR); null against v38.
        msg.transport = transport;
        msg.conv = conv;
        msg.cursor = rec.journal_record_cursor;
        // [api/39 §10.3] Provenance rides the journal-record envelope (not the chat payload): the
        // op_id that caused this record, when the node stamped one. The ingestor threads it into
        // the journal so the outbox confirm keys on provenance (§6.6). Null against v38.
        if (rec.journal_record_origin_op_present &&
            rec.journal_record_origin_op.journal_record_origin_op_choice ==
                journal_record_origin_op_r::journal_record_origin_op_origin_op_m_c) {
            msg.origin_op = codec_detail::fromZcbor(
                rec.journal_record_origin_op.journal_record_origin_op_origin_op_m);
        }
        out->push_back(std::move(msg));
    }
    if (nextCursor != nullptr) {
        *nextCursor = page.journal_page_view_next_cursor;
    }
    if (headCursor != nullptr) {
        *headCursor = page.journal_page_view_head_cursor;
    }
    return true;
}

bool decodePersonsToMirror(const QByteArray& responseCbor, std::vector<mirror::Person>* out,
                           quint64* rev, QStringList* removed,
                           std::vector<mirror::PersonEndpoint>* endpoints) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_persons_m_c);
    if (!response) {
        return false;
    }
    const response_persons& rp = response->api_response_response_persons_m;
    out->clear();
    out->reserve(rp.Persons_items_person_m_count);
    if (endpoints != nullptr) {
        endpoints->clear();
    }
    for (size_t i = 0; i < rp.Persons_items_person_m_count; ++i) {
        const ::person& p = rp.Persons_items_person_m[i];
        out->push_back(mirror::map_person(p));
        // AD (1a): the per-transport endpoints ride the same wire row; the mapper leaves the
        // owning `person` scope blank — stamp it here (the entity-map's "person.id" provenance).
        if (endpoints != nullptr && p.person_endpoints_present) {
            const QString owner = out->back().id;
            for (size_t j = 0; j < p.person_endpoints.person_endpoints_person_endpoint_m_count;
                 ++j) {
                mirror::PersonEndpoint e = mirror::map_person_endpoint(
                    p.person_endpoints.person_endpoints_person_endpoint_m[j]);
                e.person = owner;
                endpoints->push_back(std::move(e));
            }
        }
    }
    // [api/39 §10.2] The collection rev + the delta-read `removed` tombstone id list.
    if (rev != nullptr) {
        *rev = rp.Persons_rev;
    }
    if (removed != nullptr) {
        removed->clear();
        if (rp.Persons_removed_present) {
            for (size_t i = 0; i < rp.Persons_removed.Persons_removed_tstr_count; ++i) {
                removed->append(
                    codec_detail::fromZcbor(rp.Persons_removed.Persons_removed_tstr[i]));
            }
        }
    }
    return true;
}

bool decodeAdaptersToMirror(const QByteArray& responseCbor, std::vector<mirror::Adapter>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_adapters_m_c);
    if (!response) {
        return false;
    }
    const response_adapters& ra = response->api_response_response_adapters_m;
    out->clear();
    out->reserve(ra.response_adapters_Adapters_adapter_info_m_count);
    for (size_t i = 0; i < ra.response_adapters_Adapters_adapter_info_m_count; ++i) {
        out->push_back(mirror::map_adapter(ra.response_adapters_Adapters_adapter_info_m[i]));
    }
    return true;
}

bool decodeTransportInstancesToMirror(const QByteArray& responseCbor,
                                      std::vector<mirror::TransportAccount>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_transport_instances_m_c);
    if (!response) {
        return false;
    }
    const response_transport_instances& ri = response->api_response_response_transport_instances_m;
    out->clear();
    out->reserve(
        ri.response_transport_instances_TransportInstances_transport_instance_info_m_count);
    for (size_t i = 0;
         i < ri.response_transport_instances_TransportInstances_transport_instance_info_m_count;
         ++i) {
        out->push_back(mirror::map_transport_account(
            ri.response_transport_instances_TransportInstances_transport_instance_info_m[i]));
    }
    return true;
}

bool decodeContactsToMirror(const QByteArray& responseCbor, const QString& transport,
                            std::vector<mirror::Contact>* out, QString* next, quint64* rev,
                            QStringList* removed) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_contact_page_m_c);
    if (!response) {
        return false;
    }
    const contact_page& page =
        response->api_response_response_contact_page_m.response_contact_page_ContactPage;
    out->clear();
    out->reserve(page.contact_page_items_contact_info_m_count);
    for (size_t i = 0; i < page.contact_page_items_contact_info_m_count; ++i) {
        mirror::Contact c = mirror::map_contact(page.contact_page_items_contact_info_m[i]);
        // The roster payload carries only the id; transport is the RosterList request scope (§3.6).
        c.transport = transport;
        out->push_back(std::move(c));
    }
    if (next != nullptr) {
        const bool hasNext =
            page.contact_page_next_present && page.contact_page_next.contact_page_next_choice ==
                                                  contact_page_next_r::contact_page_next_tstr_c;
        *next = hasNext ? codec_detail::fromZcbor(page.contact_page_next.contact_page_next_tstr)
                        : QString();
    }
    // [api/39 §10.2] The collection rev + the delta-read `removed` tombstone id list.
    if (rev != nullptr) {
        *rev = page.contact_page_rev;
    }
    if (removed != nullptr) {
        removed->clear();
        if (page.contact_page_removed_present) {
            for (size_t i = 0; i < page.contact_page_removed.contact_page_removed_tstr_count; ++i) {
                removed->append(codec_detail::fromZcbor(
                    page.contact_page_removed.contact_page_removed_tstr[i]));
            }
        }
    }
    return true;
}

bool decodeRoutePinsToMirror(const QByteArray& responseCbor, std::vector<mirror::RoutePin>* out,
                             QString* next) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_chat_routes_m_c);
    if (!response) {
        return false;
    }
    const chat_route_page& page =
        response->api_response_response_chat_routes_m.response_chat_routes_ChatRoutes;
    out->clear();
    out->reserve(page.chat_route_page_items_chat_route_m_count);
    for (size_t i = 0; i < page.chat_route_page_items_chat_route_m_count; ++i) {
        out->push_back(mirror::map_route_pin(page.chat_route_page_items_chat_route_m[i]));
    }
    if (next != nullptr) {
        const bool hasNext = page.chat_route_page_next_present &&
                             page.chat_route_page_next.chat_route_page_next_choice ==
                                 chat_route_page_next_r::chat_route_page_next_tstr_c;
        *next = hasNext
                    ? codec_detail::fromZcbor(page.chat_route_page_next.chat_route_page_next_tstr)
                    : QString();
    }
    return true;
}

bool decodeRoomsToMirror(const QByteArray& responseCbor, const QString& transport,
                         std::vector<mirror::Room>* out, QString* next) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_rooms_m_c);
    if (!response) {
        return false;
    }
    const room_page& page = response->api_response_response_rooms_m.response_rooms_Rooms;
    out->clear();
    out->reserve(page.room_page_items_room_info_m_count);
    for (size_t i = 0; i < page.room_page_items_room_info_m_count; ++i) {
        mirror::Room room = mirror::map_room(page.room_page_items_room_info_m[i]);
        // Stamp the request-scope transport so the (transport, room) key matches the fetch scope
        // even if a payload omitted it (§3.6 merging rule; the ingestor prunes per transport).
        if (room.transport.isEmpty()) {
            room.transport = transport;
        }
        out->push_back(std::move(room));
    }
    if (next != nullptr) {
        const bool hasNext =
            page.room_page_next_present &&
            page.room_page_next.room_page_next_choice == room_page_next_r::room_page_next_tstr_c;
        *next =
            hasNext ? codec_detail::fromZcbor(page.room_page_next.room_page_next_tstr) : QString();
    }
    return true;
}

bool decodeSessionsToMirror(const QByteArray& responseCbor, std::vector<mirror::Session>* out,
                            QString* next, quint64* rev, QStringList* removed,
                            QHash<QString, QString>* originOps) {
    if (out == nullptr) {
        return false;
    }
    // SessionsQuery answers with SessionPage (dispatch: ApiRequest::SessionsQuery ->
    // ApiResponse::SessionPage) — the paged roster read the legacy SessionRepository consumed. The
    // bare `Sessions` arm (response-sessions) answers the old ApiRequest::Sessions verb, which the
    // mirror path does not issue.
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_session_page_m_c);
    if (!response) {
        return false;
    }
    const session_page& page =
        response->api_response_response_session_page_m.response_session_page_SessionPage;
    out->clear();
    out->reserve(page.session_page_sessions_session_info_m_count);
    for (size_t i = 0; i < page.session_page_sessions_session_info_m_count; ++i) {
        out->push_back(mirror::map_session(page.session_page_sessions_session_info_m[i]));
    }
    if (next != nullptr) {
        next->clear();
        if (page.session_page_next_cursor_present &&
            page.session_page_next_cursor.session_page_next_cursor_choice ==
                session_page_next_cursor_r::session_page_next_cursor_session_id_m_c) {
            *next = codec_detail::fromZcbor(
                page.session_page_next_cursor.session_page_next_cursor_session_id_m);
        }
    }
    // [api/39 §10.2] The roster rev + the delta-read `removed` tombstone id list.
    if (rev != nullptr) {
        *rev = page.session_page_rev;
    }
    if (removed != nullptr) {
        removed->clear();
        if (page.session_page_removed_present) {
            for (size_t i = 0;
                 i < page.session_page_removed.session_page_removed_session_id_m_count; ++i) {
                removed->append(codec_detail::fromZcbor(
                    page.session_page_removed.session_page_removed_session_id_m[i]));
            }
        }
    }
    // [api/39 §10.3 carrier 2] the page-side provenance map: session id → causing op's op_id,
    // present only for items whose latest reflected mutation carried an op_id.
    if (originOps != nullptr) {
        originOps->clear();
        if (page.session_page_origin_ops_present) {
            const ::origin_ops& ops = page.session_page_origin_ops.session_page_origin_ops;
            for (size_t i = 0; i < ops.origin_ops_origin_op_m_count; ++i) {
                originOps->insert(
                    codec_detail::fromZcbor(
                        ops.origin_ops_origin_op_m[i].origin_ops_origin_op_m_key),
                    codec_detail::fromZcbor(ops.origin_ops_origin_op_m[i].origin_ops_origin_op_m));
            }
        }
    }
    return true;
}

bool decodeFleetUnitsToMirror(const QByteArray& responseCbor, std::vector<mirror::FleetUnit>* out,
                              QString* next, quint64* rev) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_tree_m_c);
    if (!response) {
        return false;
    }
    const tree_report& tree = response->api_response_response_tree_m.response_tree_Tree;
    out->clear();
    out->reserve(tree.tree_report_nodes_unit_node_m_count);
    for (size_t i = 0; i < tree.tree_report_nodes_unit_node_m_count; ++i) {
        out->push_back(mirror::map_fleet_unit(tree.tree_report_nodes_unit_node_m[i]));
    }
    if (next != nullptr) {
        const bool hasNext =
            tree.tree_report_next_present && tree.tree_report_next.tree_report_next_choice ==
                                                 tree_report_next_r::tree_report_next_tstr_c;
        *next = hasNext ? codec_detail::fromZcbor(tree.tree_report_next.tree_report_next_tstr)
                        : QString();
    }
    if (rev != nullptr) {
        *rev = tree.tree_report_rev;
    }
    return true;
}

bool decodeSessionDetailToMirror(const QByteArray& responseCbor, mirror::Session* out,
                                 bool* found) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_session_detail_m_c);
    if (!response) {
        return false;
    }
    const response_session_detail& rsd = response->api_response_response_session_detail_m;
    if (rsd.response_session_detail_SessionDetail_choice !=
        response_session_detail::response_session_detail_SessionDetail_session_detail_m_c) {
        if (found != nullptr) {
            *found = false; // the null arm: the node does not know this session
        }
        return true;
    }
    const session_detail& detail = rsd.response_session_detail_SessionDetail_session_detail_m;
    // Base fields from the embedded session-info, then the detail-only hydration (model,
    // checkpoints) per entity-map provenance (session-detail.model / session-detail.checkpoints).
    *out = mirror::map_session(detail.session_detail_info);
    if (detail.session_detail_model_present &&
        detail.session_detail_model.session_detail_model_choice ==
            session_detail_model_r::session_detail_model_tstr_c) {
        out->model = codec_detail::fromZcbor(detail.session_detail_model.session_detail_model_tstr);
    }
    if (detail.session_detail_checkpoints_present) {
        out->checkpoints =
            static_cast<int>(detail.session_detail_checkpoints.session_detail_checkpoints);
    }
    if (found != nullptr) {
        *found = true;
    }
    return true;
}

} // namespace daemonapp::daemon
