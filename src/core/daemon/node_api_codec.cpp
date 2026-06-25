#include "daemon/node_api_codec.h"

#include <array>
#include <memory>

extern "C" {
#include "daemon_api_smoke_decode.h"
#include "daemon_api_smoke_encode.h"
}

namespace daemonapp::daemon {
namespace {

QString fromZcbor(const zcbor_string& s)
{
    if (s.value == nullptr || s.len == 0) {
        return {};
    }
    return QString::fromUtf8(reinterpret_cast<const char*>(s.value), static_cast<int>(s.len));
}

QString sessionStateName(int choice)
{
    switch (choice) {
    case session_state_r::session_state_Active_tstr_c:
        return QStringLiteral("Active");
    case session_state_r::session_state_Ready_tstr_c:
        return QStringLiteral("Ready");
    case session_state_r::session_state_Completed_tstr_c:
        return QStringLiteral("Completed");
    default:
        return QStringLiteral("Unknown");
    }
}

QString lifecycleName(int choice)
{
    return choice == lifecycle_r::lifecycle_Live_tstr_c ? QStringLiteral("Live")
                                                    : QStringLiteral("Durable");
}

QString roleName(int choice)
{
    switch (choice) {
    case session_role_r::session_role_ManagedChild_tstr_c:
        return QStringLiteral("ManagedChild");
    case session_role_r::session_role_EphemeralSubagent_tstr_c:
        return QStringLiteral("EphemeralSubagent");
    default:
        return QStringLiteral("Primary");
    }
}

bool encodeRequest(const api_request_r& request, QByteArray* out)
{
    // Requests in the first slice are small (Health is a bare string, SessionsQuery an empty
    // map); 256 bytes is comfortably above their canonical encoding size.
    std::array<uint8_t, 256> buffer{};
    size_t written = 0;
    if (cbor_encode_api_request(buffer.data(), buffer.size(), &request, &written) != ZCBOR_SUCCESS) {
        return false;
    }
    *out = QByteArray(reinterpret_cast<const char*>(buffer.data()), static_cast<int>(written));
    return true;
}

bool decodeResponse(const QByteArray& responseCbor, api_response_r* out)
{
    if (responseCbor.isEmpty()) {
        return false;
    }
    return cbor_decode_api_response(reinterpret_cast<const uint8_t*>(responseCbor.constData()),
                                    static_cast<size_t>(responseCbor.size()), out, nullptr)
        == ZCBOR_SUCCESS;
}

} // namespace

QByteArray NodeApiCodec::encodeHealthRequest()
{
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_health_m_c;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeSessionsQueryRequest()
{
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_sessions_query_m_c;
    // Leave every optional field absent: an empty session-query map asks the daemon for its
    // default (TopLevel) scope.
    request.api_request_request_sessions_query_m = request_sessions_query{};
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeSubscribeRequest(const QString& sessionId, quint64 afterSeq,
                                                quint32 max)
{
    const QByteArray session = sessionId.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_subscribe_m_c;
    request_subscribe& subscribe = request.api_request_request_subscribe_m;
    subscribe.Subscribe_session.value = reinterpret_cast<const uint8_t*>(session.constData());
    subscribe.Subscribe_session.len = static_cast<size_t>(session.size());
    subscribe.Subscribe_after_seq = static_cast<uint32_t>(afterSeq);
    subscribe.Subscribe_max = max;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

ApiResponseKind NodeApiCodec::responseKind(const QByteArray& responseCbor)
{
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get())) {
        return ApiResponseKind::Unknown;
    }
    switch (response->api_response_choice) {
    case api_response_r::api_response_response_health_m_c:
        return ApiResponseKind::Health;
    case api_response_r::api_response_response_session_page_m_c:
        return ApiResponseKind::SessionPage;
    case api_response_r::api_response_response_log_page_m_c:
        return ApiResponseKind::LogPage;
    case api_response_r::api_response_response_fs_roots_m_c:
        return ApiResponseKind::FsRoots;
    case api_response_r::api_response_response_ok_m_c:
        return ApiResponseKind::Ok;
    default:
        return ApiResponseKind::Unknown;
    }
}

bool NodeApiCodec::decodeHealth(const QByteArray& responseCbor, DecodedHealth* out)
{
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get())
        || response->api_response_choice != api_response_r::api_response_response_health_m_c) {
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
                                     QString* nextCursor)
{
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get())
        || response->api_response_choice != api_response_r::api_response_response_session_page_m_c) {
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
        if (info.session_info_bound_profile_present
            && info.session_info_bound_profile.session_info_bound_profile_choice
                == session_info_bound_profile_r::session_info_bound_profile_profile_ref_m_c) {
            row.profileRef =
                fromZcbor(info.session_info_bound_profile.session_info_bound_profile_profile_ref_m);
        }
        if (info.session_info_title_present
            && info.session_info_title.session_info_title_choice
                == session_info_title_r::session_info_title_tstr_c) {
            row.title = fromZcbor(info.session_info_title.session_info_title_tstr);
        }
        if (info.session_info_lifecycle_present) {
            row.lifecycle =
                lifecycleName(info.session_info_lifecycle.session_info_lifecycle.lifecycle_choice);
        }
        if (info.session_info_role_present) {
            row.role = roleName(info.session_info_role.session_info_role.session_role_choice);
        }
        if (info.session_info_parent_present
            && info.session_info_parent.session_info_parent_choice
                == session_info_parent_r::session_info_parent_session_id_m_c) {
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
        if (page.session_page_next_cursor_present
            && page.session_page_next_cursor.session_page_next_cursor_choice
                == session_page_next_cursor_r::session_page_next_cursor_session_id_m_c) {
            *nextCursor =
                fromZcbor(page.session_page_next_cursor.session_page_next_cursor_session_id_m);
        }
    }
    return true;
}

bool NodeApiCodec::decodeLogPage(const QByteArray& responseCbor, const QString& sessionId,
                                 QList<CachedLogRow>* out, quint64* nextSeq, quint64* headSeq)
{
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get())
        || response->api_response_choice != api_response_r::api_response_response_log_page_m_c) {
        return false;
    }
    const log_page_view& page = response->api_response_response_log_page_m.response_log_page_LogPage;
    out->clear();
    for (size_t i = 0; i < page.log_page_view_entries_session_log_entry_m_count; ++i) {
        const session_log_entry& entry = page.log_page_view_entries_session_log_entry_m[i];
        CachedLogRow row;
        row.sessionId = sessionId;
        row.seq = entry.session_log_entry_seq;
        row.direction = entry.session_log_entry_direction.direction_choice
                == direction_r::direction_Outbound_tstr_c
            ? QStringLiteral("Outbound")
            : QStringLiteral("Inbound");
        row.disposition = entry.session_log_entry_disposition.disposition_choice
                == disposition_r::disposition_Transport_tstr_c
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

} // namespace daemonapp::daemon
