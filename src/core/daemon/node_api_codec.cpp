#include "daemon/node_api_codec.h"

#include <memory>

extern "C" {
#include "daemon_api_client_decode.h"
#include "daemon_api_client_encode.h"
}

namespace daemonapp::daemon {
namespace {

QString fromZcbor(const zcbor_string& s) {
    if (s.value == nullptr || s.len == 0) {
        return {};
    }
    return QString::fromUtf8(reinterpret_cast<const char*>(s.value), static_cast<int>(s.len));
}

QString sessionStateName(int choice) {
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

QString lifecycleName(int choice) {
    return choice == lifecycle_r::lifecycle_Live_tstr_c ? QStringLiteral("Live")
                                                        : QStringLiteral("Durable");
}

QString roleName(int choice) {
    switch (choice) {
    case session_role_r::session_role_ManagedChild_tstr_c:
        return QStringLiteral("ManagedChild");
    case session_role_r::session_role_EphemeralSubagent_tstr_c:
        return QStringLiteral("EphemeralSubagent");
    default:
        return QStringLiteral("Primary");
    }
}

// ProviderSelector wire string <-> generated choice (serde rename_all = snake_case).
QString providerName(int choice) {
    switch (choice) {
    case provider_selector_r::provider_selector_genai_tstr_c:
        return QStringLiteral("genai");
    case provider_selector_r::provider_selector_llama_cpp_tstr_c:
        return QStringLiteral("llama_cpp");
    case provider_selector_r::provider_selector_mistral_rs_tstr_c:
        return QStringLiteral("mistral_rs");
    default:
        return QStringLiteral("mock");
    }
}

int providerChoice(const QString& provider) {
    if (provider == QStringLiteral("genai")) {
        return provider_selector_r::provider_selector_genai_tstr_c;
    }
    if (provider == QStringLiteral("llama_cpp")) {
        return provider_selector_r::provider_selector_llama_cpp_tstr_c;
    }
    if (provider == QStringLiteral("mistral_rs")) {
        return provider_selector_r::provider_selector_mistral_rs_tstr_c;
    }
    return provider_selector_r::provider_selector_mock_tstr_c;
}

void fillDescriptor(const model_descriptor& m, DecodedModelDescriptor* out) {
    out->id = fromZcbor(m.model_descriptor_id);
    out->provider = providerName(m.model_descriptor_provider.provider_selector_choice);
    if (m.model_descriptor_context_length_choice ==
        model_descriptor::model_descriptor_context_length_uint_c) {
        out->hasContextLength = true;
        out->contextLength = m.model_descriptor_context_length_uint;
    }
    if (m.model_descriptor_input_price_micros_per_mtok_choice ==
        model_descriptor::model_descriptor_input_price_micros_per_mtok_uint_c) {
        out->hasInputPrice = true;
        out->inputPriceMicrosPerMtok = m.model_descriptor_input_price_micros_per_mtok_uint;
    }
    if (m.model_descriptor_output_price_micros_per_mtok_choice ==
        model_descriptor::model_descriptor_output_price_micros_per_mtok_uint_c) {
        out->hasOutputPrice = true;
        out->outputPriceMicrosPerMtok = m.model_descriptor_output_price_micros_per_mtok_uint;
    }
    out->local = m.model_descriptor_local;
}

bool encodeRequest(const api_request_r& request, QByteArray* out) {
    // Grow the output buffer until the request fits rather than truncating: most requests are tiny,
    // but Submit/CommandInvoke carry user text and can exceed any small fixed size. Retry on encode
    // failure (zcbor signals a too-small buffer that way) up to a sane cap.
    constexpr size_t kMaxBuffer = 1u << 20; // 1 MiB
    for (size_t capacity = 256;; capacity *= 2) {
        QByteArray buffer(static_cast<qsizetype>(capacity), Qt::Uninitialized);
        size_t written = 0;
        const int rc = cbor_encode_api_request(reinterpret_cast<uint8_t*>(buffer.data()), capacity,
                                               &request, &written);
        if (rc == ZCBOR_SUCCESS) {
            buffer.resize(static_cast<qsizetype>(written));
            *out = buffer;
            return true;
        }
        if (capacity >= kMaxBuffer) {
            return false;
        }
    }
}

bool decodeResponse(const QByteArray& responseCbor, api_response_r* out) {
    if (responseCbor.isEmpty()) {
        return false;
    }
    const auto size = static_cast<size_t>(responseCbor.size());
    size_t consumed = 0;
    // Require the whole frame to decode: a short read that leaves trailing bytes is a malformed or
    // truncated response, not a success.
    return cbor_decode_api_response(reinterpret_cast<const uint8_t*>(responseCbor.constData()),
                                    size, out, &consumed) == ZCBOR_SUCCESS &&
           consumed == size;
}

} // namespace

QByteArray NodeApiCodec::encodeHealthRequest() {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_health_m_c;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeSessionsQueryRequest() {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_sessions_query_m_c;
    // Leave every optional field absent: an empty session-query map asks the daemon for its
    // default (TopLevel) scope.
    request.api_request_request_sessions_query_m = request_sessions_query{};
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeSubscribeRequest(const QString& sessionId, quint64 afterSeq,
                                                quint32 max) {
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

QByteArray NodeApiCodec::encodeCredentialSetRequest(const QString& profile, const QString& secret) {
    const QByteArray profileUtf8 = profile.toUtf8();
    const QByteArray secretUtf8 = secret.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_credential_set_m_c;
    request_credential_set& set = request.api_request_request_credential_set_m;
    set.CredentialSet_profile.value = reinterpret_cast<const uint8_t*>(profileUtf8.constData());
    set.CredentialSet_profile.len = static_cast<size_t>(profileUtf8.size());
    set.CredentialSet_secret.value = reinterpret_cast<const uint8_t*>(secretUtf8.constData());
    set.CredentialSet_secret.len = static_cast<size_t>(secretUtf8.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeCredentialListRequest() {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_credential_list_m_c;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeCredentialRemoveRequest(const QString& profile) {
    const QByteArray profileUtf8 = profile.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_credential_remove_m_c;
    request_credential_remove& remove = request.api_request_request_credential_remove_m;
    remove.CredentialRemove_profile.value =
        reinterpret_cast<const uint8_t*>(profileUtf8.constData());
    remove.CredentialRemove_profile.len = static_cast<size_t>(profileUtf8.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelsRequest() {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_models_m_c;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelCurrentRequest(const QString& profile) {
    const QByteArray profileUtf8 = profile.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_current_m_c;
    request_model_current& current = request.api_request_request_model_current_m;
    if (profile.isEmpty()) {
        current.ModelCurrent_profile_choice = request_model_current::ModelCurrent_profile_null_m_c;
    } else {
        current.ModelCurrent_profile_choice = request_model_current::ModelCurrent_profile_tstr_c;
        current.ModelCurrent_profile_tstr.value =
            reinterpret_cast<const uint8_t*>(profileUtf8.constData());
        current.ModelCurrent_profile_tstr.len = static_cast<size_t>(profileUtf8.size());
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeSetSessionModelRequest(const QString& sessionId,
                                                      const QString& model,
                                                      const QString& provider) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    const QByteArray modelUtf8 = model.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_set_session_model_m_c;
    request_set_session_model& set = request.api_request_request_set_session_model_m;
    set.SetSessionModel_session.value = reinterpret_cast<const uint8_t*>(sessionUtf8.constData());
    set.SetSessionModel_session.len = static_cast<size_t>(sessionUtf8.size());
    set.SetSessionModel_model.value = reinterpret_cast<const uint8_t*>(modelUtf8.constData());
    set.SetSessionModel_model.len = static_cast<size_t>(modelUtf8.size());
    if (provider.isEmpty()) {
        // Omit the optional `provider` key: keep the session's current provider binding.
        set.SetSessionModel_provider_present = false;
    } else {
        set.SetSessionModel_provider_present = true;
        set.SetSessionModel_provider.SetSessionModel_provider_choice =
            SetSessionModel_provider_r::SetSessionModel_provider_provider_selector_m_c;
        auto& selector = set.SetSessionModel_provider.SetSessionModel_provider_provider_selector_m;
        selector.provider_selector_choice =
            static_cast<decltype(selector.provider_selector_choice)>(providerChoice(provider));
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeProfileListRequest() {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_profile_list_m_c;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

ApiResponseKind NodeApiCodec::responseKind(const QByteArray& responseCbor) {
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
    case api_response_r::api_response_response_credentials_m_c:
        return ApiResponseKind::Credentials;
    case api_response_r::api_response_response_models_m_c:
        return ApiResponseKind::Models;
    case api_response_r::api_response_response_model_current_m_c:
        return ApiResponseKind::ModelCurrent;
    case api_response_r::api_response_response_profiles_m_c:
        return ApiResponseKind::Profiles;
    case api_response_r::api_response_response_error_m_c:
        return ApiResponseKind::Error;
    default:
        return ApiResponseKind::Unknown;
    }
}

bool NodeApiCodec::decodeHealth(const QByteArray& responseCbor, DecodedHealth* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_health_m_c) {
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
                                     QString* nextCursor) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_session_page_m_c) {
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
    return true;
}

bool NodeApiCodec::decodeLogPage(const QByteArray& responseCbor, const QString& sessionId,
                                 QList<CachedLogRow>* out, quint64* nextSeq, quint64* headSeq) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_log_page_m_c) {
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

bool NodeApiCodec::decodeCredentials(const QByteArray& responseCbor,
                                     QList<DecodedCredentialInfo>* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_credentials_m_c) {
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

bool NodeApiCodec::decodeModels(const QByteArray& responseCbor,
                                QList<DecodedModelDescriptor>* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_models_m_c) {
        return false;
    }
    const response_models& models = response->api_response_response_models_m;
    out->clear();
    for (size_t i = 0; i < models.response_models_Models_model_descriptor_m_count; ++i) {
        DecodedModelDescriptor descriptor;
        fillDescriptor(models.response_models_Models_model_descriptor_m[i], &descriptor);
        out->append(descriptor);
    }
    return true;
}

bool NodeApiCodec::decodeModelCurrent(const QByteArray& responseCbor, DecodedModelDescriptor* out,
                                      bool* hasModel) {
    if (out == nullptr || hasModel == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_model_current_m_c) {
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
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_error_m_c) {
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
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_profiles_m_c) {
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

} // namespace daemonapp::daemon
