// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [integrations wire v38] The native-chat + persons + transport-settings codec wrappers: ConvSend /
// ConvHistory (a per-conversation JournalRecordPayload::Chat stream), PersonList, and the account
// settings read/configure ops. Kept in its own TU (the channels/messaging domain) so it merges
// cleanly beside fs_fleet_channels.cpp. Hand-written over the vendored zcbor codec — never CBOR by
// hand.

#include "daemon/node_api_codec/internal.h"

namespace daemonapp::daemon {

using namespace codec_detail;

namespace {

// Project the wire chat-message participant author into the flattened id/name/isAgent triple.
void decodeChatAuthor(const chat_message_author_r& a, DecodedChatMessage* out) {
    if (a.chat_message_author_choice !=
        chat_message_author_r::chat_message_author_participant_m_c) {
        return; // the null arm — no author (a system/event line)
    }
    const participant_r& who = a.chat_message_author_participant_m;
    if (who.participant_choice == participant_r::participant_contact_m_c) {
        const contact_info& c = who.participant_contact_m.participant_contact_Contact;
        out->authorId = fromZcbor(c.contact_info_id);
        if (c.contact_info_display_name_present &&
            c.contact_info_display_name.contact_info_display_name_choice ==
                contact_info_display_name_r::contact_info_display_name_tstr_c) {
            out->authorName = fromZcbor(c.contact_info_display_name.contact_info_display_name_tstr);
        }
    } else {
        // The Agent arm: the author is an agent (profile + member). Surface the member as the id.
        out->authorIsAgent = true;
        out->authorId = fromZcbor(who.participant_agent_m.Agent_member);
    }
}

// Project one wire message-attachment into the decoded struct.
DecodedMessageAttachment decodeAttachment(const message_attachment& a) {
    DecodedMessageAttachment d;
    d.id = fromZcbor(a.message_attachment_id);
    if (a.message_attachment_content_type_present &&
        a.message_attachment_content_type.message_attachment_content_type_choice ==
            message_attachment_content_type_r::message_attachment_content_type_tstr_c) {
        d.contentType =
            fromZcbor(a.message_attachment_content_type.message_attachment_content_type_tstr);
    }
    if (a.message_attachment_is_inline_present) {
        d.isInline = a.message_attachment_is_inline.message_attachment_is_inline;
    }
    if (a.message_attachment_local_uri_present &&
        a.message_attachment_local_uri.message_attachment_local_uri_choice ==
            message_attachment_local_uri_r::message_attachment_local_uri_tstr_c) {
        d.localUri = fromZcbor(a.message_attachment_local_uri.message_attachment_local_uri_tstr);
    }
    if (a.message_attachment_remote_uri_present &&
        a.message_attachment_remote_uri.message_attachment_remote_uri_choice ==
            message_attachment_remote_uri_r::message_attachment_remote_uri_tstr_c) {
        d.remoteUri = fromZcbor(a.message_attachment_remote_uri.message_attachment_remote_uri_tstr);
    }
    if (a.message_attachment_size_present) {
        d.size = a.message_attachment_size.message_attachment_size;
    }
    return d;
}

// Project one wire chat-message into the decoded struct (`cursor` is stamped by the caller).
DecodedChatMessage decodeChatMessageStruct(const chat_message& m) {
    DecodedChatMessage out;
    if (m.chat_message_id_present &&
        m.chat_message_id.chat_message_id_choice == chat_message_id_r::chat_message_id_tstr_c) {
        out.id = fromZcbor(m.chat_message_id.chat_message_id_tstr);
    }
    if (m.chat_message_author_present) {
        decodeChatAuthor(m.chat_message_author, &out);
    }
    if (m.chat_message_replying_to_present &&
        m.chat_message_replying_to.chat_message_replying_to_choice ==
            chat_message_replying_to_r::chat_message_replying_to_tstr_c) {
        out.replyingTo = fromZcbor(m.chat_message_replying_to.chat_message_replying_to_tstr);
    }
    out.text = fromZcbor(m.chat_message_text);
    if (m.chat_message_attachments_present) {
        for (size_t i = 0;
             i < m.chat_message_attachments.chat_message_attachments_message_attachment_m_count;
             ++i) {
            out.attachments.append(decodeAttachment(
                m.chat_message_attachments.chat_message_attachments_message_attachment_m[i]));
        }
    }
    if (m.chat_message_timestamp_present &&
        m.chat_message_timestamp.chat_message_timestamp_choice ==
            chat_message_timestamp_r::chat_message_timestamp_uint64_m_c) {
        out.hasTimestamp = true;
        out.timestamp = m.chat_message_timestamp.chat_message_timestamp_uint64_m;
    }
    if (m.chat_message_delivered_at_present &&
        m.chat_message_delivered_at.chat_message_delivered_at_choice ==
            chat_message_delivered_at_r::chat_message_delivered_at_uint64_m_c) {
        out.hasDeliveredAt = true;
        out.deliveredAt = m.chat_message_delivered_at.chat_message_delivered_at_uint64_m;
    }
    if (m.chat_message_edited_at_present &&
        m.chat_message_edited_at.chat_message_edited_at_choice ==
            chat_message_edited_at_r::chat_message_edited_at_uint64_m_c) {
        out.hasEditedAt = true;
        out.editedAt = m.chat_message_edited_at.chat_message_edited_at_uint64_m;
    }
    if (m.chat_message_error_present && m.chat_message_error.chat_message_error_choice ==
                                            chat_message_error_r::chat_message_error_tstr_c) {
        out.error = fromZcbor(m.chat_message_error.chat_message_error_tstr);
    }
    if (m.chat_message_title_present && m.chat_message_title.chat_message_title_choice ==
                                            chat_message_title_r::chat_message_title_tstr_c) {
        out.title = fromZcbor(m.chat_message_title.chat_message_title_tstr);
    }
    if (m.chat_message_highlight_color_present &&
        m.chat_message_highlight_color.chat_message_highlight_color_choice ==
            chat_message_highlight_color_r::chat_message_highlight_color_tstr_c) {
        out.highlightColor =
            fromZcbor(m.chat_message_highlight_color.chat_message_highlight_color_tstr);
    }
    if (m.chat_message_action_present) {
        out.action = m.chat_message_action.chat_message_action;
    }
    if (m.chat_message_event_present) {
        out.event = m.chat_message_event.chat_message_event;
    }
    if (m.chat_message_notice_present) {
        out.notice = m.chat_message_notice.chat_message_notice;
    }
    if (m.chat_message_system_present) {
        out.system = m.chat_message_system.chat_message_system;
    }
    if (m.chat_message_highlighted_present) {
        out.highlighted = m.chat_message_highlighted.chat_message_highlighted;
    }
    return out;
}

} // namespace

QByteArray NodeApiCodec::encodeConvSendRequest(const QString& transport, const QString& conv,
                                               const QString& text, const QString& opId) {
    const QByteArray t = transport.toUtf8();
    const QByteArray c = conv.toUtf8();
    const QByteArray tx = text.toUtf8();
    const QByteArray op = opId.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_conv_send_m_c, [&](api_request_r& request) {
            conv_send_args& a = request.api_request_request_conv_send_m.request_conv_send_ConvSend;
            setZcbor(a.conv_send_args_transport, t);
            setZcbor(a.conv_send_args_conv, c);
            // The node fills `from` (this account's identity); the client omits it.
            a.conv_send_args_from_present = false;
            setZcbor(a.conv_send_args_message.user_msg_text, tx);
            a.conv_send_args_message.user_msg_attachments_present = false;
            a.conv_send_args_message.user_msg_notice_present = false;
            // [api/39 §10.3] The optional op_id: the outbox mints it and reuses it across retries
            // so the node dedups on (principal, op_id) and stamps `origin_op` provenance. Empty ⇒
            // absent (byte-identical to the v38 shape for a send that carries no op-id).
            a.conv_send_args_op_id_present = !opId.isEmpty();
            if (!opId.isEmpty()) {
                a.conv_send_args_op_id.conv_send_args_op_id_choice =
                    conv_send_args_op_id_r::conv_send_args_op_id_tstr_c;
                setZcbor(a.conv_send_args_op_id.conv_send_args_op_id_tstr, op);
            }
        });
}

QByteArray NodeApiCodec::encodeConvHistoryRequest(const QString& transport, const QString& conv,
                                                  bool hasAfter, quint64 afterCursor, bool hasMax,
                                                  quint32 max, bool hasBefore,
                                                  quint64 beforeCursor) {
    const QByteArray t = transport.toUtf8();
    const QByteArray c = conv.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_conv_history_m_c, [&](api_request_r& request) {
            conv_history_args& a =
                request.api_request_request_conv_history_m.request_conv_history_ConvHistory;
            setZcbor(a.conv_history_args_transport, t);
            setZcbor(a.conv_history_args_conv, c);
            a.conv_history_args_after_cursor_present = hasAfter;
            if (hasAfter) {
                a.conv_history_args_after_cursor.conv_history_args_after_cursor = afterCursor;
            }
            // [api/39 §10.2] The optional before_cursor: a BACKWARD window — the `max` records with
            // cursor < beforeCursor, newest-anchored. Forward (after_cursor) and backward
            // (before_cursor) are mutually exclusive; the caller passes at most one.
            a.conv_history_args_before_cursor_present = hasBefore;
            if (hasBefore) {
                a.conv_history_args_before_cursor.conv_history_args_before_cursor = beforeCursor;
            }
            a.conv_history_args_max_present = hasMax;
            if (hasMax) {
                a.conv_history_args_max.conv_history_args_max = max;
            }
        });
}

bool NodeApiCodec::decodeConvHistory(const QByteArray& responseCbor, QList<DecodedChatMessage>* out,
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
        // ConvHistory records carry a Chat payload; skip any Block/Management record defensively
        // (the conversation journal only ever appends Chat).
        if (rec.journal_record_payload.journal_record_payload_t_choice !=
            journal_record_payload_t_r::journal_record_payload_t_journal_record_payload_chat_m_c) {
            continue;
        }
        DecodedChatMessage msg = decodeChatMessageStruct(
            rec.journal_record_payload.journal_record_payload_t_journal_record_payload_chat_m
                .Chat_message);
        msg.cursor = rec.journal_record_cursor;
        out->append(msg);
    }
    if (nextCursor != nullptr) {
        *nextCursor = page.journal_page_view_next_cursor;
    }
    if (headCursor != nullptr) {
        *headCursor = page.journal_page_view_head_cursor;
    }
    return true;
}

QByteArray NodeApiCodec::encodePersonListRequest(bool hasSinceRev, quint64 sinceRev) {
    if (!hasSinceRev) {
        return encodeSimple(api_request_r::api_request_request_person_list_m_c);
    }
    // [api/39 §10.2] since_rev ⇒ delta read (changed persons + `removed` + `rev`).
    return encodeWithFill(
        api_request_r::api_request_request_person_list_m_c, [&](api_request_r& request) {
            request_person_list& l = request.api_request_request_person_list_m;
            l.PersonList_since_rev_present = true;
            l.PersonList_since_rev.PersonList_since_rev_choice =
                PersonList_since_rev_r::PersonList_since_rev_uint64_m_c;
            l.PersonList_since_rev.PersonList_since_rev_uint64_m = static_cast<uint64_t>(sinceRev);
        });
}

QByteArray NodeApiCodec::encodeTransportSettingsRequest(const QString& transport) {
    const QByteArray t = transport.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_transport_settings_m_c, [&](api_request_r& request) {
            setZcbor(request.api_request_request_transport_settings_m.TransportSettings_transport,
                     t);
        });
}

bool NodeApiCodec::decodeTransportSettings(const QByteArray& responseCbor,
                                           QMap<QString, QString>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_transport_settings_m_c);
    if (!response) {
        return false;
    }
    const account_settings_values& vals = response->api_response_response_transport_settings_m
                                              .response_transport_settings_TransportSettings;
    out->clear();
    if (vals.account_settings_values_values_present) {
        const account_settings_values_values_r& vr = vals.account_settings_values_values;
        for (size_t i = 0; i < vr.values_tstrtstr_count; ++i) {
            out->insert(
                fromZcbor(vr.values_tstrtstr[i].account_settings_values_values_tstrtstr_key),
                fromZcbor(vr.values_tstrtstr[i].values_tstrtstr));
        }
    }
    return true;
}

QByteArray NodeApiCodec::encodeTransportConfigureRequest(const QString& transport,
                                                         const QMap<QString, QString>& settings) {
    const QByteArray t = transport.toUtf8();
    // fillSettingsValues borrows into this scratch across the synchronous encode.
    QList<QByteArray> scratch;
    return encodeWithFill(
        api_request_r::api_request_request_transport_configure_m_c, [&](api_request_r& request) {
            request_transport_configure& c = request.api_request_request_transport_configure_m;
            setZcbor(c.TransportConfigure_transport, t);
            fillSettingsValues(c.TransportConfigure_settings, settings, scratch);
        });
}

} // namespace daemonapp::daemon
