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
                                 std::vector<mirror::Conversation>* out, QString* next) {
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
        *next = hasNext ? codec_detail::fromZcbor(page.conv_page_next.conv_page_next_tstr)
                        : QString();
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
    const journal_page_view& page = response->api_response_response_journal_m.response_journal_Journal;
    out->clear();
    for (size_t i = 0; i < page.journal_page_view_entries_journal_record_m_count; ++i) {
        const journal_record& rec = page.journal_page_view_entries_journal_record_m[i];
        // The conversation journal only appends Chat records; skip any Block/Management defensively.
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

bool decodePersonsToMirror(const QByteArray& responseCbor, std::vector<mirror::Person>* out) {
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
    out->reserve(rp.response_persons_Persons_person_m_count);
    for (size_t i = 0; i < rp.response_persons_Persons_person_m_count; ++i) {
        out->push_back(mirror::map_person(rp.response_persons_Persons_person_m[i]));
    }
    return true;
}

bool decodeContactsToMirror(const QByteArray& responseCbor, const QString& transport,
                            std::vector<mirror::Contact>* out, QString* next) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_contact_page_m_c);
    if (!response) {
        return false;
    }
    const contact_page& page = response->api_response_response_contact_page_m.response_contact_page_ContactPage;
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
            page.contact_page_next_present &&
            page.contact_page_next.contact_page_next_choice == contact_page_next_r::contact_page_next_tstr_c;
        *next = hasNext ? codec_detail::fromZcbor(page.contact_page_next.contact_page_next_tstr)
                        : QString();
    }
    return true;
}

} // namespace daemonapp::daemon
