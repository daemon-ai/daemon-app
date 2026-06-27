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

QString contextEngineName(int choice) {
    return choice == context_engine_sel_r::context_engine_sel_budgeted_tstr_c
               ? QStringLiteral("budgeted")
               : QStringLiteral("lcm");
}

QString memoryProviderName(int choice) {
    switch (choice) {
    case memory_provider_sel_r::memory_provider_sel_file_tstr_c:
        return QStringLiteral("file");
    case memory_provider_sel_r::memory_provider_sel_none_tstr_c:
        return QStringLiteral("none");
    default:
        return QStringLiteral("mnemosyne");
    }
}

// Decode the concrete generated profile_spec into the typed mirror (so a ProfileGet -> edit ->
// ProfileUpdate round-trip preserves untouched fields).
DecodedProfileSpec decodeProfileSpecStruct(const profile_spec& ps) {
    DecodedProfileSpec out;
    out.id = fromZcbor(ps.profile_spec_id);
    out.provider = providerName(ps.profile_spec_provider.provider_selector_choice);
    out.model = fromZcbor(ps.profile_spec_model);
    if (ps.profile_spec_base_url_choice == profile_spec::profile_spec_base_url_tstr_c) {
        out.hasBaseUrl = true;
        out.baseUrl = fromZcbor(ps.profile_spec_base_url_tstr);
    }
    out.systemPrompt = fromZcbor(ps.profile_spec_system_prompt);
    if (ps.profile_spec_tool_allowlist_choice == profile_spec::tool_allowlist_tstr_l_c) {
        out.hasToolAllowlist = true;
        for (size_t i = 0; i < ps.tool_allowlist_tstr_l_tstr_count; ++i) {
            out.toolAllowlist << fromZcbor(ps.tool_allowlist_tstr_l_tstr[i]);
        }
    }
    if (ps.profile_spec_budget.budget_tokens_choice == budget::budget_tokens_uint_c) {
        out.hasBudgetTokens = true;
        out.budgetTokens = ps.profile_spec_budget.budget_tokens_uint;
    }
    if (ps.profile_spec_budget.budget_wall_ms_choice == budget::budget_wall_ms_uint_c) {
        out.hasBudgetWallMs = true;
        out.budgetWallMs = ps.profile_spec_budget.budget_wall_ms_uint;
    }
    const engine_tunables& tn = ps.profile_spec_tunables;
    if (tn.engine_tunables_model_retry_attempts_choice ==
        engine_tunables::engine_tunables_model_retry_attempts_uint_c) {
        out.hasModelRetryAttempts = true;
        out.modelRetryAttempts = tn.engine_tunables_model_retry_attempts_uint;
    }
    if (tn.engine_tunables_context_budget_tokens_choice ==
        engine_tunables::engine_tunables_context_budget_tokens_uint_c) {
        out.hasContextBudgetTokens = true;
        out.contextBudgetTokens = tn.engine_tunables_context_budget_tokens_uint;
    }
    if (tn.engine_tunables_max_iterations_choice ==
        engine_tunables::engine_tunables_max_iterations_uint_c) {
        out.hasMaxIterations = true;
        out.maxIterations = tn.engine_tunables_max_iterations_uint;
    }
    if (tn.engine_tunables_tool_result_budget_choice ==
        engine_tunables::engine_tunables_tool_result_budget_uint_c) {
        out.hasToolResultBudget = true;
        out.toolResultBudget = tn.engine_tunables_tool_result_budget_uint;
    }
    out.contextEngine = contextEngineName(ps.profile_spec_context_engine.context_engine_sel_choice);
    out.memoryProvider =
        memoryProviderName(ps.profile_spec_memory_provider.memory_provider_sel_choice);
    if (ps.profile_spec_credential_ref_choice == profile_spec::profile_spec_credential_ref_tstr_c) {
        out.hasCredentialRef = true;
        out.credentialRef = fromZcbor(ps.profile_spec_credential_ref_tstr);
    }
    if (ps.profile_spec_fallback_credential_ref_choice ==
        profile_spec::profile_spec_fallback_credential_ref_tstr_c) {
        out.hasFallbackCredentialRef = true;
        out.fallbackCredentialRef = fromZcbor(ps.profile_spec_fallback_credential_ref_tstr);
    }
    for (size_t i = 0; i < ps.profile_spec_bound_accounts_bound_account_m_count; ++i) {
        const bound_account& ba = ps.profile_spec_bound_accounts_bound_account_m[i];
        out.boundAccounts.append(DecodedBoundAccount{fromZcbor(ba.bound_account_transport_instance),
                                                     fromZcbor(ba.bound_account_credential_ref)});
    }
    return out;
}

QString endReasonName(int choice) {
    switch (choice) {
    case end_reason_r::end_reason_Completed_tstr_c:
        return QStringLiteral("Completed");
    case end_reason_r::end_reason_Suspended_tstr_c:
        return QStringLiteral("Suspended");
    case end_reason_r::end_reason_Interrupted_tstr_c:
        return QStringLiteral("Interrupted");
    case end_reason_r::end_reason_BudgetExhausted_tstr_c:
        return QStringLiteral("BudgetExhausted");
    case end_reason_r::end_reason_NoProgress_tstr_c:
        return QStringLiteral("NoProgress");
    default:
        return QStringLiteral("Failed");
    }
}

DecodedAgentEvent decodeAgentEvent(const agent_event_r& ev) {
    DecodedAgentEvent out;
    switch (ev.agent_event_choice) {
    case agent_event_r::agent_event_turn_started_m_c:
        out.kind = AgentEventKind::TurnStarted;
        out.seq = ev.agent_event_turn_started_m.TurnStarted_seq;
        break;
    case agent_event_r::agent_event_text_delta_m_c:
        out.kind = AgentEventKind::TextDelta;
        out.seq = ev.agent_event_text_delta_m.TextDelta_seq;
        out.text = fromZcbor(ev.agent_event_text_delta_m.TextDelta_text);
        break;
    case agent_event_r::agent_event_reasoning_delta_m_c:
        out.kind = AgentEventKind::ReasoningDelta;
        out.seq = ev.agent_event_reasoning_delta_m.ReasoningDelta_seq;
        out.text = fromZcbor(ev.agent_event_reasoning_delta_m.ReasoningDelta_text);
        break;
    case agent_event_r::agent_event_tool_started_m_c: {
        out.kind = AgentEventKind::ToolStarted;
        out.seq = ev.agent_event_tool_started_m.ToolStarted_seq;
        const tool_call_view& call = ev.agent_event_tool_started_m.ToolStarted_call;
        out.callId = fromZcbor(call.tool_call_view_call_id);
        out.toolName = fromZcbor(call.tool_call_view_name);
        out.toolArgs = fromZcbor(call.tool_call_view_args_summary);
        break;
    }
    case agent_event_r::agent_event_tool_finished_m_c: {
        out.kind = AgentEventKind::ToolFinished;
        out.seq = ev.agent_event_tool_finished_m.ToolFinished_seq;
        const tool_result_view& result = ev.agent_event_tool_finished_m.ToolFinished_result;
        out.callId = fromZcbor(result.tool_result_view_call_id);
        out.toolOk = result.tool_result_view_ok;
        break;
    }
    case agent_event_r::agent_event_usage_m_c: {
        out.kind = AgentEventKind::Usage;
        out.seq = ev.agent_event_usage_m.Usage_seq;
        const usage_delta& usage = ev.agent_event_usage_m.Usage_delta;
        out.inputTokens = usage.usage_delta_input_tokens;
        out.outputTokens = usage.usage_delta_output_tokens;
        out.costMicros = usage.usage_delta_cost_micros;
        break;
    }
    case agent_event_r::agent_event_context_m_c: {
        out.kind = AgentEventKind::Context;
        out.seq = ev.agent_event_context_m.Context_seq;
        const context_status& ctx = ev.agent_event_context_m.Context_status;
        out.contextUsed = ctx.context_status_used_tokens;
        if (ctx.context_status_max_tokens_choice ==
            context_status::context_status_max_tokens_uint_c) {
            out.hasContextMax = true;
            out.contextMax = ctx.context_status_max_tokens_uint;
        }
        break;
    }
    case agent_event_r::agent_event_turn_finished_m_c: {
        out.kind = AgentEventKind::TurnFinished;
        out.seq = ev.agent_event_turn_finished_m.TurnFinished_seq;
        const turn_summary& summary = ev.agent_event_turn_finished_m.TurnFinished_summary;
        out.endReason = endReasonName(summary.turn_summary_end_reason.end_reason_choice);
        out.turnCompleted = summary.turn_summary_end_reason.end_reason_choice ==
                            end_reason_r::end_reason_Completed_tstr_c;
        if (summary.turn_summary_final_text_choice ==
            turn_summary::turn_summary_final_text_tstr_c) {
            out.finalText = fromZcbor(summary.turn_summary_final_text_tstr);
        }
        break;
    }
    case agent_event_r::agent_event_error_m_c:
        out.kind = AgentEventKind::Error;
        out.seq = ev.agent_event_error_m.Error_seq;
        out.text = fromZcbor(ev.agent_event_error_m.Error_failure);
        break;
    default:
        out.kind = AgentEventKind::Other;
        break;
    }
    return out;
}

// --- Local model track (Phase 2) helpers ---------------------------------------------------

// ModelEngine wire string <-> generated choice (variant names, no serde rename).
int modelEngineChoice(const QString& engine) {
    return engine == QStringLiteral("mistral_rs") || engine == QStringLiteral("mistralrs")
               ? model_engine_r::model_engine_MistralRs_tstr_c
               : model_engine_r::model_engine_Llama_tstr_c;
}

QString modelEngineName(int choice) {
    return choice == model_engine_r::model_engine_MistralRs_tstr_c ? QStringLiteral("mistral_rs")
                                                                   : QStringLiteral("llama");
}

int searchSortChoice(const QString& sort) {
    if (sort == QStringLiteral("downloads")) {
        return search_sort_r::search_sort_Downloads_tstr_c;
    }
    if (sort == QStringLiteral("likes")) {
        return search_sort_r::search_sort_Likes_tstr_c;
    }
    if (sort == QStringLiteral("modified")) {
        return search_sort_r::search_sort_Modified_tstr_c;
    }
    if (sort == QStringLiteral("created")) {
        return search_sort_r::search_sort_Created_tstr_c;
    }
    return search_sort_r::search_sort_Trending_tstr_c;
}

QString downloadStateName(int choice) {
    switch (choice) {
    case download_state_r::download_state_Queued_tstr_c:
        return QStringLiteral("Queued");
    case download_state_r::download_state_Downloading_tstr_c:
        return QStringLiteral("Downloading");
    case download_state_r::download_state_Completed_tstr_c:
        return QStringLiteral("Completed");
    case download_state_r::download_state_Paused_tstr_c:
        return QStringLiteral("Paused");
    case download_state_r::download_state_Cancelled_tstr_c:
        return QStringLiteral("Cancelled");
    default:
        return QStringLiteral("Failed");
    }
}

// Project a model_ref's engine + (Hf) repo/file into plain strings. A Local source leaves
// repo/file empty (the installed row still carries its local_path).
void fillModelRef(const model_ref& m, QString* repo, QString* file, QString* engine) {
    if (engine != nullptr) {
        *engine = modelEngineName(m.model_ref_engine.model_engine_choice);
    }
    if (m.model_ref_source.model_source_choice == model_source_r::model_source_hf_m_c) {
        const model_source_hf& hf = m.model_ref_source.model_source_hf_m;
        if (repo != nullptr) {
            *repo = fromZcbor(hf.Hf_repo);
        }
        if (file != nullptr && hf.Hf_file_choice == model_source_hf::Hf_file_tstr_c) {
            *file = fromZcbor(hf.Hf_file_tstr);
        }
    }
}

// Decode a parked HostRequest into the DecodedLogEntry HITL fields (CHA-4 / CHA-5).
void decodeHostRequest(const host_request& req, DecodedLogEntry* out) {
    out->hostRequestId = req.host_request_request_id;
    const host_request_kind_t_r& kind = req.host_request_kind;
    switch (kind.host_request_kind_t_choice) {
    case host_request_kind_t_r::host_request_kind_t_host_request_kind_approval_m_c:
        out->hostKind = QStringLiteral("Approval");
        out->hostPrompt =
            fromZcbor(kind.host_request_kind_t_host_request_kind_approval_m.Approval_prompt);
        break;
    case host_request_kind_t_r::host_request_kind_t_host_request_kind_input_m_c:
        out->hostKind = QStringLiteral("Input");
        out->hostPrompt =
            fromZcbor(kind.host_request_kind_t_host_request_kind_input_m.Input_prompt);
        break;
    case host_request_kind_t_r::host_request_kind_t_host_request_kind_choice_m_c: {
        out->hostKind = QStringLiteral("Choice");
        const host_request_kind_choice& choice =
            kind.host_request_kind_t_host_request_kind_choice_m;
        out->hostPrompt = fromZcbor(choice.Choice_prompt);
        for (size_t i = 0; i < choice.Choice_options_tstr_count; ++i) {
            out->hostOptions << fromZcbor(choice.Choice_options_tstr[i]);
        }
        break;
    }
    case host_request_kind_t_r::host_request_kind_t_host_request_kind_delegate_m_c:
        out->hostKind = QStringLiteral("Delegate");
        out->hostPrompt =
            fromZcbor(kind.host_request_kind_t_host_request_kind_delegate_m.Delegate_label);
        break;
    case host_request_kind_t_r::host_request_kind_t_host_request_kind_spawn_m_c:
        out->hostKind = QStringLiteral("Spawn");
        break;
    default:
        break;
    }
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

// Encode a ProfileCreate (update=false) or ProfileUpdate (update=true) carrying a full
// profile_spec. All string buffers are held locally for the duration of the encode (zcbor
// references them).
QByteArray encodeProfileMutation(bool update, const DecodedProfileSpec& s) {
    const auto setZ = [](zcbor_string& z, const QByteArray& b) {
        z.value = reinterpret_cast<const uint8_t*>(b.constData());
        z.len = static_cast<size_t>(b.size());
    };
    const QByteArray id = s.id.toUtf8();
    const QByteArray model = s.model.toUtf8();
    const QByteArray prompt = s.systemPrompt.toUtf8();
    const QByteArray baseUrl = s.baseUrl.toUtf8();
    const QByteArray credRef = s.credentialRef.toUtf8();
    const QByteArray fbCredRef = s.fallbackCredentialRef.toUtf8();
    QList<QByteArray> toolBufs;
    for (const QString& t : s.toolAllowlist) {
        toolBufs.append(t.toUtf8());
    }
    QList<QByteArray> baTi;
    QList<QByteArray> baCr;
    for (const DecodedBoundAccount& b : s.boundAccounts) {
        baTi.append(b.transportInstance.toUtf8());
        baCr.append(b.credentialRef.toUtf8());
    }

    api_request_r request{};
    request.api_request_choice = update ? api_request_r::api_request_request_profile_update_m_c
                                        : api_request_r::api_request_request_profile_create_m_c;
    profile_spec& ps = update ? request.api_request_request_profile_update_m.ProfileUpdate_spec
                              : request.api_request_request_profile_create_m.ProfileCreate_spec;

    setZ(ps.profile_spec_id, id);
    ps.profile_spec_provider.provider_selector_choice =
        s.provider == QStringLiteral("genai") ? provider_selector_r::provider_selector_genai_tstr_c
        : s.provider == QStringLiteral("llama_cpp")
            ? provider_selector_r::provider_selector_llama_cpp_tstr_c
        : s.provider == QStringLiteral("mistral_rs")
            ? provider_selector_r::provider_selector_mistral_rs_tstr_c
            : provider_selector_r::provider_selector_mock_tstr_c;
    setZ(ps.profile_spec_model, model);
    if (s.hasBaseUrl) {
        ps.profile_spec_base_url_choice = profile_spec::profile_spec_base_url_tstr_c;
        setZ(ps.profile_spec_base_url_tstr, baseUrl);
    } else {
        ps.profile_spec_base_url_choice = profile_spec::profile_spec_base_url_null_m_c;
    }
    setZ(ps.profile_spec_system_prompt, prompt);
    if (s.hasToolAllowlist) {
        ps.profile_spec_tool_allowlist_choice = profile_spec::tool_allowlist_tstr_l_c;
        const size_t n = qMin<size_t>(static_cast<size_t>(toolBufs.size()), 16);
        ps.tool_allowlist_tstr_l_tstr_count = n;
        for (size_t i = 0; i < n; ++i) {
            setZ(ps.tool_allowlist_tstr_l_tstr[i], toolBufs[static_cast<int>(i)]);
        }
    } else {
        ps.profile_spec_tool_allowlist_choice = profile_spec::profile_spec_tool_allowlist_null_m_c;
    }
    if (s.hasBudgetTokens) {
        ps.profile_spec_budget.budget_tokens_choice = budget::budget_tokens_uint_c;
        ps.profile_spec_budget.budget_tokens_uint = s.budgetTokens;
    } else {
        ps.profile_spec_budget.budget_tokens_choice = budget::budget_tokens_null_m_c;
    }
    if (s.hasBudgetWallMs) {
        ps.profile_spec_budget.budget_wall_ms_choice = budget::budget_wall_ms_uint_c;
        ps.profile_spec_budget.budget_wall_ms_uint = s.budgetWallMs;
    } else {
        ps.profile_spec_budget.budget_wall_ms_choice = budget::budget_wall_ms_null_m_c;
    }
    engine_tunables& tn = ps.profile_spec_tunables;
    if (s.hasModelRetryAttempts) {
        tn.engine_tunables_model_retry_attempts_choice =
            engine_tunables::engine_tunables_model_retry_attempts_uint_c;
        tn.engine_tunables_model_retry_attempts_uint = s.modelRetryAttempts;
    } else {
        tn.engine_tunables_model_retry_attempts_choice =
            engine_tunables::engine_tunables_model_retry_attempts_null_m_c;
    }
    if (s.hasContextBudgetTokens) {
        tn.engine_tunables_context_budget_tokens_choice =
            engine_tunables::engine_tunables_context_budget_tokens_uint_c;
        tn.engine_tunables_context_budget_tokens_uint = s.contextBudgetTokens;
    } else {
        tn.engine_tunables_context_budget_tokens_choice =
            engine_tunables::engine_tunables_context_budget_tokens_null_m_c;
    }
    if (s.hasMaxIterations) {
        tn.engine_tunables_max_iterations_choice =
            engine_tunables::engine_tunables_max_iterations_uint_c;
        tn.engine_tunables_max_iterations_uint = s.maxIterations;
    } else {
        tn.engine_tunables_max_iterations_choice =
            engine_tunables::engine_tunables_max_iterations_null_m_c;
    }
    if (s.hasToolResultBudget) {
        tn.engine_tunables_tool_result_budget_choice =
            engine_tunables::engine_tunables_tool_result_budget_uint_c;
        tn.engine_tunables_tool_result_budget_uint = s.toolResultBudget;
    } else {
        tn.engine_tunables_tool_result_budget_choice =
            engine_tunables::engine_tunables_tool_result_budget_null_m_c;
    }
    ps.profile_spec_context_engine.context_engine_sel_choice =
        s.contextEngine == QStringLiteral("budgeted")
            ? context_engine_sel_r::context_engine_sel_budgeted_tstr_c
            : context_engine_sel_r::context_engine_sel_lcm_tstr_c;
    ps.profile_spec_memory_provider.memory_provider_sel_choice =
        s.memoryProvider == QStringLiteral("file")
            ? memory_provider_sel_r::memory_provider_sel_file_tstr_c
        : s.memoryProvider == QStringLiteral("none")
            ? memory_provider_sel_r::memory_provider_sel_none_tstr_c
            : memory_provider_sel_r::memory_provider_sel_mnemosyne_tstr_c;
    if (s.hasCredentialRef) {
        ps.profile_spec_credential_ref_choice = profile_spec::profile_spec_credential_ref_tstr_c;
        setZ(ps.profile_spec_credential_ref_tstr, credRef);
    } else {
        ps.profile_spec_credential_ref_choice = profile_spec::profile_spec_credential_ref_null_m_c;
    }
    if (s.hasFallbackCredentialRef) {
        ps.profile_spec_fallback_credential_ref_choice =
            profile_spec::profile_spec_fallback_credential_ref_tstr_c;
        setZ(ps.profile_spec_fallback_credential_ref_tstr, fbCredRef);
    } else {
        ps.profile_spec_fallback_credential_ref_choice =
            profile_spec::profile_spec_fallback_credential_ref_null_m_c;
    }
    const size_t bn = qMin<size_t>(static_cast<size_t>(baTi.size()), 16);
    ps.profile_spec_bound_accounts_bound_account_m_count = bn;
    for (size_t i = 0; i < bn; ++i) {
        setZ(ps.profile_spec_bound_accounts_bound_account_m[i].bound_account_transport_instance,
             baTi[static_cast<int>(i)]);
        setZ(ps.profile_spec_bound_accounts_bound_account_m[i].bound_account_credential_ref,
             baCr[static_cast<int>(i)]);
    }

    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
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

QByteArray NodeApiCodec::encodeSubmitStartTurnRequest(const QString& sessionId, const QString& text,
                                                      const QString& profile, quint32 requestId) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    const QByteArray textUtf8 = text.toUtf8();
    const QByteArray profileUtf8 = profile.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_submit_m_c;
    request_submit& submit = request.api_request_request_submit_m;
    submit.Submit_session.value = reinterpret_cast<const uint8_t*>(sessionUtf8.constData());
    submit.Submit_session.len = static_cast<size_t>(sessionUtf8.size());
    submit.Submit_command.agent_command_choice = agent_command_r::agent_command_start_turn_m_c;
    agent_command_start_turn& start = submit.Submit_command.agent_command_start_turn_m;
    start.StartTurn_input.user_msg_text.value =
        reinterpret_cast<const uint8_t*>(textUtf8.constData());
    start.StartTurn_input.user_msg_text.len = static_cast<size_t>(textUtf8.size());
    start.StartTurn_input.user_msg_attachments_present = false;
    start.StartTurn_request_id = requestId;
    submit.Submit_origin_present = false;
    if (profile.isEmpty()) {
        submit.Submit_profile_present = false;
    } else {
        submit.Submit_profile_present = true;
        submit.Submit_profile.Submit_profile_choice =
            Submit_profile_r::Submit_profile_profile_ref_m_c;
        submit.Submit_profile.Submit_profile_profile_ref_m.value =
            reinterpret_cast<const uint8_t*>(profileUtf8.constData());
        submit.Submit_profile.Submit_profile_profile_ref_m.len =
            static_cast<size_t>(profileUtf8.size());
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodePollRequest(const QString& sessionId, quint32 max) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_poll_m_c;
    request_poll& poll = request.api_request_request_poll_m;
    poll.Poll_session.value = reinterpret_cast<const uint8_t*>(sessionUtf8.constData());
    poll.Poll_session.len = static_cast<size_t>(sessionUtf8.size());
    poll.Poll_max = max;
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

QByteArray NodeApiCodec::encodeProfileSelectRequest(const QString& id) {
    const QByteArray idUtf8 = id.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_profile_select_m_c;
    request_profile_select& select = request.api_request_request_profile_select_m;
    select.ProfileSelect_id.value = reinterpret_cast<const uint8_t*>(idUtf8.constData());
    select.ProfileSelect_id.len = static_cast<size_t>(idUtf8.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeProfileDeleteRequest(const QString& id) {
    const QByteArray idUtf8 = id.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_profile_delete_m_c;
    request_profile_delete& del = request.api_request_request_profile_delete_m;
    del.ProfileDelete_id.value = reinterpret_cast<const uint8_t*>(idUtf8.constData());
    del.ProfileDelete_id.len = static_cast<size_t>(idUtf8.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeProfileCreateRequest(const DecodedProfileSpec& spec) {
    return encodeProfileMutation(/*update=*/false, spec);
}

QByteArray NodeApiCodec::encodeProfileUpdateRequest(const DecodedProfileSpec& spec) {
    return encodeProfileMutation(/*update=*/true, spec);
}

QByteArray NodeApiCodec::encodeProfileCloneRequest(const QString& source, const QString& newId) {
    const QByteArray sourceUtf8 = source.toUtf8();
    const QByteArray newIdUtf8 = newId.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_profile_clone_m_c;
    request_profile_clone& clone = request.api_request_request_profile_clone_m;
    clone.ProfileClone_source.value = reinterpret_cast<const uint8_t*>(sourceUtf8.constData());
    clone.ProfileClone_source.len = static_cast<size_t>(sourceUtf8.size());
    clone.ProfileClone_new_id.value = reinterpret_cast<const uint8_t*>(newIdUtf8.constData());
    clone.ProfileClone_new_id.len = static_cast<size_t>(newIdUtf8.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeProfileGetRequest(const QString& id) {
    const QByteArray idUtf8 = id.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_profile_get_m_c;
    request_profile_get& get = request.api_request_request_profile_get_m;
    get.ProfileGet_id.value = reinterpret_cast<const uint8_t*>(idUtf8.constData());
    get.ProfileGet_id.len = static_cast<size_t>(idUtf8.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelSearchRequest(const QString& text, const QString& engine,
                                                  const QString& sort, quint32 page,
                                                  quint32 limit) {
    const QByteArray textUtf8 = text.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_search_m_c;
    search_query& q = request.api_request_request_model_search_m.ModelSearch_query;
    q.search_query_text.value = reinterpret_cast<const uint8_t*>(textUtf8.constData());
    q.search_query_text.len = static_cast<size_t>(textUtf8.size());
    q.search_query_engine.model_engine_choice =
        static_cast<decltype(q.search_query_engine.model_engine_choice)>(modelEngineChoice(engine));
    q.search_query_sort.search_sort_choice =
        static_cast<decltype(q.search_query_sort.search_sort_choice)>(searchSortChoice(sort));
    q.search_query_page = page;
    q.search_query_limit = limit;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

namespace {
// Fill a request_respond (HostResponse) shell for `sessionId`/`requestId`; the caller sets the
// body arm. Returns by writing into `request`; the session bytes are owned by the caller.
void fillRespondShell(api_request_r* request, const QByteArray& sessionUtf8, quint32 requestId) {
    request->api_request_choice = api_request_r::api_request_request_respond_m_c;
    request_respond& respond = request->api_request_request_respond_m;
    respond.Respond_session.value = reinterpret_cast<const uint8_t*>(sessionUtf8.constData());
    respond.Respond_session.len = static_cast<size_t>(sessionUtf8.size());
    respond.Respond_response.host_response_request_id = requestId;
}
} // namespace

QByteArray NodeApiCodec::encodeRespondApprovalRequest(const QString& sessionId, quint32 requestId,
                                                      bool allow) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    api_request_r request{};
    fillRespondShell(&request, sessionUtf8, requestId);
    host_response_body_t_r& body =
        request.api_request_request_respond_m.Respond_response.host_response_body;
    body.host_response_body_t_choice =
        host_response_body_t_r::host_response_body_t_host_response_body_approved_m_c;
    body.host_response_body_t_host_response_body_approved_m.host_response_body_approved_Approved =
        allow;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeRespondInputRequest(const QString& sessionId, quint32 requestId,
                                                   const QString& text) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    const QByteArray textUtf8 = text.toUtf8();
    api_request_r request{};
    fillRespondShell(&request, sessionUtf8, requestId);
    host_response_body_t_r& body =
        request.api_request_request_respond_m.Respond_response.host_response_body;
    body.host_response_body_t_choice =
        host_response_body_t_r::host_response_body_t_host_response_body_input_m_c;
    auto& input = body.host_response_body_t_host_response_body_input_m;
    input.host_response_body_input_Input.value =
        reinterpret_cast<const uint8_t*>(textUtf8.constData());
    input.host_response_body_input_Input.len = static_cast<size_t>(textUtf8.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelFilesRequest(const QString& repo, const QString& engine,
                                                 const QString& revision) {
    const QByteArray repoUtf8 = repo.toUtf8();
    const QByteArray revUtf8 = revision.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_files_m_c;
    request_model_files& f = request.api_request_request_model_files_m;
    f.ModelFiles_repo.value = reinterpret_cast<const uint8_t*>(repoUtf8.constData());
    f.ModelFiles_repo.len = static_cast<size_t>(repoUtf8.size());
    if (revision.isEmpty()) {
        f.ModelFiles_revision_choice = request_model_files::ModelFiles_revision_null_m_c;
    } else {
        f.ModelFiles_revision_choice = request_model_files::ModelFiles_revision_tstr_c;
        f.ModelFiles_revision_tstr.value = reinterpret_cast<const uint8_t*>(revUtf8.constData());
        f.ModelFiles_revision_tstr.len = static_cast<size_t>(revUtf8.size());
    }
    f.ModelFiles_engine.model_engine_choice =
        static_cast<decltype(f.ModelFiles_engine.model_engine_choice)>(modelEngineChoice(engine));
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeRespondChoiceRequest(const QString& sessionId, quint32 requestId,
                                                    quint32 chosen) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    api_request_r request{};
    fillRespondShell(&request, sessionUtf8, requestId);
    host_response_body_t_r& body =
        request.api_request_request_respond_m.Respond_response.host_response_body;
    body.host_response_body_t_choice =
        host_response_body_t_r::host_response_body_t_host_response_body_chosen_m_c;
    body.host_response_body_t_host_response_body_chosen_m.host_response_body_chosen_Chosen = chosen;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelRecommendRequest(const QString& repo, const QString& engine,
                                                     bool hasBudget, quint64 budgetBytes,
                                                     const QString& revision) {
    const QByteArray repoUtf8 = repo.toUtf8();
    const QByteArray revUtf8 = revision.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_recommend_m_c;
    request_model_recommend& r = request.api_request_request_model_recommend_m;
    r.ModelRecommend_repo.value = reinterpret_cast<const uint8_t*>(repoUtf8.constData());
    r.ModelRecommend_repo.len = static_cast<size_t>(repoUtf8.size());
    if (revision.isEmpty()) {
        r.ModelRecommend_revision_choice =
            request_model_recommend::ModelRecommend_revision_null_m_c;
    } else {
        r.ModelRecommend_revision_choice = request_model_recommend::ModelRecommend_revision_tstr_c;
        r.ModelRecommend_revision_tstr.value =
            reinterpret_cast<const uint8_t*>(revUtf8.constData());
        r.ModelRecommend_revision_tstr.len = static_cast<size_t>(revUtf8.size());
    }
    r.ModelRecommend_engine.model_engine_choice =
        static_cast<decltype(r.ModelRecommend_engine.model_engine_choice)>(
            modelEngineChoice(engine));
    if (hasBudget) {
        r.ModelRecommend_budget_bytes_choice =
            request_model_recommend::ModelRecommend_budget_bytes_uint_c;
        r.ModelRecommend_budget_bytes_uint = static_cast<uint32_t>(budgetBytes);
    } else {
        r.ModelRecommend_budget_bytes_choice =
            request_model_recommend::ModelRecommend_budget_bytes_null_m_c;
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeApprovalDecideRequest(const QString& sessionId,
                                                     const QString& requestId, bool allow) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    const QByteArray requestIdUtf8 = requestId.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_approval_decide_m_c;
    request_approval_decide& decide = request.api_request_request_approval_decide_m;
    decide.ApprovalDecide_session.value = reinterpret_cast<const uint8_t*>(sessionUtf8.constData());
    decide.ApprovalDecide_session.len = static_cast<size_t>(sessionUtf8.size());
    decide.ApprovalDecide_request_id.value =
        reinterpret_cast<const uint8_t*>(requestIdUtf8.constData());
    decide.ApprovalDecide_request_id.len = static_cast<size_t>(requestIdUtf8.size());
    decide.ApprovalDecide_allow = allow;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeSetSessionModeRequest(const QString& sessionId,
                                                     const QString& mode) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_set_session_mode_m_c;
    request_set_session_mode& set = request.api_request_request_set_session_mode_m;
    set.SetSessionMode_session.value = reinterpret_cast<const uint8_t*>(sessionUtf8.constData());
    set.SetSessionMode_session.len = static_cast<size_t>(sessionUtf8.size());
    auto choice = approval_mode_r::approval_mode_ask_tstr_c;
    if (mode == QStringLiteral("accept_edits")) {
        choice = approval_mode_r::approval_mode_accept_edits_tstr_c;
    } else if (mode == QStringLiteral("auto_allow")) {
        choice = approval_mode_r::approval_mode_auto_allow_tstr_c;
    } else if (mode == QStringLiteral("deny")) {
        choice = approval_mode_r::approval_mode_deny_tstr_c;
    }
    set.SetSessionMode_mode.approval_mode_choice = choice;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeApprovalsPendingRequest(const QString& sessionId) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_approvals_pending_m_c;
    request_approvals_pending& pending = request.api_request_request_approvals_pending_m;
    if (sessionId.isEmpty()) {
        pending.ApprovalsPending_session_present = false;
    } else {
        pending.ApprovalsPending_session_present = true;
        pending.ApprovalsPending_session.ApprovalsPending_session_choice =
            ApprovalsPending_session_r::ApprovalsPending_session_session_id_m_c;
        pending.ApprovalsPending_session.ApprovalsPending_session_session_id_m.value =
            reinterpret_cast<const uint8_t*>(sessionUtf8.constData());
        pending.ApprovalsPending_session.ApprovalsPending_session_session_id_m.len =
            static_cast<size_t>(sessionUtf8.size());
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelDownloadRequest(const QString& repo, const QString& file,
                                                    const QString& engine,
                                                    const QString& revision) {
    const QByteArray repoUtf8 = repo.toUtf8();
    const QByteArray fileUtf8 = file.toUtf8();
    const QByteArray revUtf8 = revision.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_download_m_c;
    model_ref& m = request.api_request_request_model_download_m.ModelDownload_model;
    m.model_ref_engine.model_engine_choice =
        static_cast<decltype(m.model_ref_engine.model_engine_choice)>(modelEngineChoice(engine));
    m.model_ref_source.model_source_choice = model_source_r::model_source_hf_m_c;
    model_source_hf& hf = m.model_ref_source.model_source_hf_m;
    hf.Hf_repo.value = reinterpret_cast<const uint8_t*>(repoUtf8.constData());
    hf.Hf_repo.len = static_cast<size_t>(repoUtf8.size());
    if (file.isEmpty()) {
        hf.Hf_file_choice = model_source_hf::Hf_file_null_m_c;
    } else {
        hf.Hf_file_choice = model_source_hf::Hf_file_tstr_c;
        hf.Hf_file_tstr.value = reinterpret_cast<const uint8_t*>(fileUtf8.constData());
        hf.Hf_file_tstr.len = static_cast<size_t>(fileUtf8.size());
    }
    hf.Hf_revision.value = reinterpret_cast<const uint8_t*>(revUtf8.constData());
    hf.Hf_revision.len = static_cast<size_t>(revUtf8.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeSubmitInterruptRequest(const QString& sessionId,
                                                      const QString& reason) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    const QByteArray reasonUtf8 = reason.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_submit_m_c;
    request_submit& submit = request.api_request_request_submit_m;
    submit.Submit_session.value = reinterpret_cast<const uint8_t*>(sessionUtf8.constData());
    submit.Submit_session.len = static_cast<size_t>(sessionUtf8.size());
    submit.Submit_command.agent_command_choice = agent_command_r::agent_command_interrupt_m_c;
    agent_command_interrupt& interrupt = submit.Submit_command.agent_command_interrupt_m;
    if (reason.isEmpty()) {
        interrupt.Interrupt_reason_choice = agent_command_interrupt::Interrupt_reason_null_m_c;
    } else {
        interrupt.Interrupt_reason_choice = agent_command_interrupt::Interrupt_reason_tstr_c;
        interrupt.Interrupt_reason_tstr.value =
            reinterpret_cast<const uint8_t*>(reasonUtf8.constData());
        interrupt.Interrupt_reason_tstr.len = static_cast<size_t>(reasonUtf8.size());
    }
    submit.Submit_origin_present = false;
    submit.Submit_profile_present = false;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelDownloadsRequest() {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_downloads_m_c;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeSubmitSteerRequest(const QString& sessionId, const QString& text,
                                                  quint32 requestId) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    const QByteArray textUtf8 = text.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_submit_m_c;
    request_submit& submit = request.api_request_request_submit_m;
    submit.Submit_session.value = reinterpret_cast<const uint8_t*>(sessionUtf8.constData());
    submit.Submit_session.len = static_cast<size_t>(sessionUtf8.size());
    submit.Submit_command.agent_command_choice = agent_command_r::agent_command_steer_m_c;
    agent_command_steer& steer = submit.Submit_command.agent_command_steer_m;
    steer.Steer_text.value = reinterpret_cast<const uint8_t*>(textUtf8.constData());
    steer.Steer_text.len = static_cast<size_t>(textUtf8.size());
    steer.Steer_request_id = requestId;
    submit.Submit_origin_present = false;
    submit.Submit_profile_present = false;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelCatalogRequest() {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_catalog_m_c;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeCommandListRequest() {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_command_list_m_c;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelDeleteRequest(const QString& id) {
    const QByteArray idUtf8 = id.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_delete_m_c;
    request_model_delete& del = request.api_request_request_model_delete_m;
    del.ModelDelete_id.value = reinterpret_cast<const uint8_t*>(idUtf8.constData());
    del.ModelDelete_id.len = static_cast<size_t>(idUtf8.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelActivateRequest(const QString& id, const QString& profile) {
    const QByteArray idUtf8 = id.toUtf8();
    const QByteArray profileUtf8 = profile.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_activate_m_c;
    request_model_activate& act = request.api_request_request_model_activate_m;
    act.ModelActivate_id.value = reinterpret_cast<const uint8_t*>(idUtf8.constData());
    act.ModelActivate_id.len = static_cast<size_t>(idUtf8.size());
    if (profile.isEmpty()) {
        act.ModelActivate_profile_choice = request_model_activate::ModelActivate_profile_null_m_c;
    } else {
        act.ModelActivate_profile_choice = request_model_activate::ModelActivate_profile_tstr_c;
        act.ModelActivate_profile_tstr.value =
            reinterpret_cast<const uint8_t*>(profileUtf8.constData());
        act.ModelActivate_profile_tstr.len = static_cast<size_t>(profileUtf8.size());
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeCommandInvokeRequest(const QString& name, const QString& args,
                                                    const QString& sessionId) {
    const QByteArray nameUtf8 = name.toUtf8();
    const QByteArray argsUtf8 = args.toUtf8();
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_command_invoke_m_c;
    command_invocation& inv = request.api_request_request_command_invoke_m.CommandInvoke_invocation;
    inv.command_invocation_name.value = reinterpret_cast<const uint8_t*>(nameUtf8.constData());
    inv.command_invocation_name.len = static_cast<size_t>(nameUtf8.size());
    inv.command_invocation_args.value = reinterpret_cast<const uint8_t*>(argsUtf8.constData());
    inv.command_invocation_args.len = static_cast<size_t>(argsUtf8.size());
    if (sessionId.isEmpty()) {
        inv.command_invocation_session_choice =
            command_invocation::command_invocation_session_null_m_c;
    } else {
        inv.command_invocation_session_choice =
            command_invocation::command_invocation_session_session_id_m_c;
        inv.command_invocation_session_session_id_m.value =
            reinterpret_cast<const uint8_t*>(sessionUtf8.constData());
        inv.command_invocation_session_session_id_m.len = static_cast<size_t>(sessionUtf8.size());
    }
    inv.command_invocation_origin_choice = command_invocation::command_invocation_origin_null_m_c;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelCancelRequest(quint64 id) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_cancel_m_c;
    request.api_request_request_model_cancel_m.ModelCancel_id = static_cast<uint32_t>(id);
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelPauseRequest(quint64 id) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_pause_m_c;
    request.api_request_request_model_pause_m.ModelPause_id = static_cast<uint32_t>(id);
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelResumeRequest(quint64 id) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_resume_m_c;
    request.api_request_request_model_resume_m.ModelResume_id = static_cast<uint32_t>(id);
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeSessionSearchRequest(const QString& query, quint32 limit) {
    const QByteArray queryUtf8 = query.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_session_search_m_c;
    request_session_search& search = request.api_request_request_session_search_m;
    search.SessionSearch_query.value = reinterpret_cast<const uint8_t*>(queryUtf8.constData());
    search.SessionSearch_query.len = static_cast<size_t>(queryUtf8.size());
    search.SessionSearch_limit = limit;
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
    case api_response_r::api_response_response_drained_m_c:
        return ApiResponseKind::Drained;
    case api_response_r::api_response_response_approvals_m_c:
        return ApiResponseKind::Approvals;
    case api_response_r::api_response_response_commands_m_c:
        return ApiResponseKind::Commands;
    case api_response_r::api_response_response_command_output_m_c:
        return ApiResponseKind::CommandOutput;
    case api_response_r::api_response_response_session_search_m_c:
        return ApiResponseKind::SessionSearch;
    case api_response_r::api_response_response_error_m_c:
        return ApiResponseKind::Error;
    case api_response_r::api_response_response_model_search_m_c:
        return ApiResponseKind::ModelSearch;
    case api_response_r::api_response_response_model_files_m_c:
        return ApiResponseKind::ModelFiles;
    case api_response_r::api_response_response_model_download_started_m_c:
        return ApiResponseKind::ModelDownloadStarted;
    case api_response_r::api_response_response_model_downloads_m_c:
        return ApiResponseKind::ModelDownloads;
    case api_response_r::api_response_response_model_catalog_m_c:
        return ApiResponseKind::ModelCatalog;
    case api_response_r::api_response_response_model_recommend_m_c:
        return ApiResponseKind::ModelRecommend;
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

bool NodeApiCodec::decodeLogPageEntries(const QByteArray& responseCbor, QList<DecodedLogEntry>* out,
                                        quint64* nextSeq, quint64* headSeq) {
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
        case session_payload_r::session_payload_command_m_c:
            decoded.payloadKind = DecodedLogEntry::PayloadKind::Command;
            break;
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
    return true;
}

bool NodeApiCodec::decodeDrained(const QByteArray& responseCbor, QList<DecodedAgentEvent>* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_drained_m_c) {
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

bool NodeApiCodec::decodeProfile(const QByteArray& responseCbor, DecodedProfileSpec* out,
                                 bool* found) {
    if (out == nullptr || found == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_profile_m_c) {
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

bool NodeApiCodec::decodeProfileId(const QByteArray& responseCbor, QString* outId) {
    if (outId == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_profile_id_m_c) {
        return false;
    }
    *outId = fromZcbor(response->api_response_response_profile_id_m.response_profile_id_ProfileId);
    return true;
}

bool NodeApiCodec::decodeModelSearch(const QByteArray& responseCbor, QList<DecodedSearchHit>* out,
                                     bool* hasMore, quint32* page) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_model_search_m_c) {
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
        if (h.search_hit_num_parameters_choice == search_hit::search_hit_num_parameters_uint_c) {
            hit.hasNumParameters = true;
            hit.numParameters = h.search_hit_num_parameters_uint;
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

bool NodeApiCodec::decodeModelFiles(const QByteArray& responseCbor, QList<DecodedModelFile>* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_model_files_m_c) {
        return false;
    }
    const response_model_files& files = response->api_response_response_model_files_m;
    out->clear();
    for (size_t i = 0; i < files.response_model_files_ModelFiles_model_file_m_count; ++i) {
        const model_file& f = files.response_model_files_ModelFiles_model_file_m[i];
        DecodedModelFile file;
        file.path = fromZcbor(f.model_file_path);
        file.sizeBytes = f.model_file_size_bytes;
        if (f.model_file_quant_choice == model_file::model_file_quant_tstr_c) {
            file.quant = fromZcbor(f.model_file_quant_tstr);
        }
        file.isSplit = f.model_file_is_split;
        file.isFirstShard = f.model_file_is_first_shard;
        out->append(file);
    }
    return true;
}

bool NodeApiCodec::decodeModelDownloadStarted(const QByteArray& responseCbor, quint64* outId) {
    if (outId == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice !=
            api_response_r::api_response_response_model_download_started_m_c) {
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
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice !=
            api_response_r::api_response_response_model_downloads_m_c) {
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
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_model_catalog_m_c) {
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
        out->append(model);
    }
    return true;
}

bool NodeApiCodec::decodeModelRecommend(const QByteArray& responseCbor,
                                        DecodedQuantRecommendation* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice !=
            api_response_r::api_response_response_model_recommend_m_c) {
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
        quant_recommendation::quant_recommendation_size_bytes_uint_c) {
        out->hasSizeBytes = true;
        out->sizeBytes = r.quant_recommendation_size_bytes_uint;
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
            quant_candidate::quant_candidate_size_bytes_uint_c) {
            cand.hasSizeBytes = true;
            cand.sizeBytes = c.quant_candidate_size_bytes_uint;
        }
        cand.fits = c.quant_candidate_fits;
        out->candidates.append(cand);
    }
    return true;
}

bool NodeApiCodec::decodeApprovals(const QByteArray& responseCbor,
                                   QList<DecodedApprovalInfo>* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_approvals_m_c) {
        return false;
    }
    const response_approvals& approvals = response->api_response_response_approvals_m;
    out->clear();
    for (size_t i = 0; i < approvals.response_approvals_Approvals_approval_info_m_count; ++i) {
        const approval_info& info = approvals.response_approvals_Approvals_approval_info_m[i];
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
        out->append(entry);
    }
    return true;
}

bool NodeApiCodec::decodeCommands(const QByteArray& responseCbor, QList<DecodedCommandSpec>* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_commands_m_c) {
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
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_command_output_m_c) {
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
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_session_search_m_c) {
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

} // namespace daemonapp::daemon
