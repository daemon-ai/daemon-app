#include "daemon/node_api_codec.h"

#include <memory>
#include <QHash>
#include <QPair>
#include <QSet>

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

QByteArray bytesFromZcbor(const zcbor_string& s) {
    if (s.value == nullptr || s.len == 0) {
        return {};
    }
    return {reinterpret_cast<const char*>(s.value), static_cast<qsizetype>(s.len)};
}

// Set a generated `fs_root_id_t_r` from the opaque client root string
// ("workspace" | "host:<id>" | "session:<id>"); `scratch` must outlive the encode call (the
// zcbor_string borrows it).
void setFsRootId(const QString& rootId, fs_root_id_t_r& out, QByteArray& scratch) {
    if (rootId.startsWith(QStringLiteral("host:"))) {
        scratch = rootId.mid(5).toUtf8();
        out.fs_root_id_t_choice = fs_root_id_t_r::fs_root_id_t_fs_root_id_host_m_c;
        out.fs_root_id_t_fs_root_id_host_m.fs_root_id_host_host.value =
            reinterpret_cast<const uint8_t*>(scratch.constData());
        out.fs_root_id_t_fs_root_id_host_m.fs_root_id_host_host.len =
            static_cast<size_t>(scratch.size());
    } else if (rootId.startsWith(QStringLiteral("session:"))) {
        scratch = rootId.mid(8).toUtf8();
        out.fs_root_id_t_choice = fs_root_id_t_r::fs_root_id_t_fs_root_id_session_m_c;
        out.fs_root_id_t_fs_root_id_session_m.fs_root_id_session_session.value =
            reinterpret_cast<const uint8_t*>(scratch.constData());
        out.fs_root_id_t_fs_root_id_session_m.fs_root_id_session_session.len =
            static_cast<size_t>(scratch.size());
    } else {
        // "workspace" (and any unrecognized id) -> the Workspace root.
        out.fs_root_id_t_choice = fs_root_id_t_r::fs_root_id_t_workspace_tstr_c;
    }
}

QString fsRootIdToString(const fs_root_id_t_r& r) {
    switch (r.fs_root_id_t_choice) {
    case fs_root_id_t_r::fs_root_id_t_fs_root_id_host_m_c:
        return QStringLiteral("host:") +
               fromZcbor(r.fs_root_id_t_fs_root_id_host_m.fs_root_id_host_host);
    case fs_root_id_t_r::fs_root_id_t_fs_root_id_session_m_c:
        return QStringLiteral("session:") +
               fromZcbor(r.fs_root_id_t_fs_root_id_session_m.fs_root_id_session_session);
    case fs_root_id_t_r::fs_root_id_t_workspace_tstr_c:
    default:
        return QStringLiteral("workspace");
    }
}

QString fsEntryKindName(int choice) {
    switch (choice) {
    case fs_entry_kind_t_r::fs_entry_kind_t_dir_tstr_c:
        return QStringLiteral("dir");
    case fs_entry_kind_t_r::fs_entry_kind_t_symlink_tstr_c:
        return QStringLiteral("symlink");
    case fs_entry_kind_t_r::fs_entry_kind_t_file_tstr_c:
    default:
        return QStringLiteral("file");
    }
}

QString fsChangeKindName(int choice) {
    switch (choice) {
    case fs_change_kind_t_r::fs_change_kind_t_created_tstr_c:
        return QStringLiteral("created");
    case fs_change_kind_t_r::fs_change_kind_t_removed_tstr_c:
        return QStringLiteral("removed");
    case fs_change_kind_t_r::fs_change_kind_t_modified_tstr_c:
    default:
        return QStringLiteral("modified");
    }
}

QString unitKindName(int choice) {
    switch (choice) {
    case unit_kind_r::unit_kind_Host_tstr_c:
        return QStringLiteral("Host");
    case unit_kind_r::unit_kind_Orchestrator_tstr_c:
        return QStringLiteral("Orchestrator");
    case unit_kind_r::unit_kind_Engine_tstr_c:
    default:
        return QStringLiteral("Engine");
    }
}

QString unitStateName(int choice) {
    switch (choice) {
    case unit_state_r::unit_state_finished_m_c:
        return QStringLiteral("Finished");
    case unit_state_r::unit_state_Unknown_tstr_c:
        return QStringLiteral("Unknown");
    case unit_state_r::unit_state_Running_tstr_c:
    default:
        return QStringLiteral("Running");
    }
}

QString sessionRoleName(int choice) {
    switch (choice) {
    case session_role_r::session_role_ManagedChild_tstr_c:
        return QStringLiteral("ManagedChild");
    case session_role_r::session_role_EphemeralSubagent_tstr_c:
        return QStringLiteral("EphemeralSubagent");
    case session_role_r::session_role_Primary_tstr_c:
    default:
        return QStringLiteral("Primary");
    }
}

// Project a generated unit_node onto the facade struct (children kept as the raw id list; depth is
// assigned later by the tree flatten).
DecodedUnitNode decodeUnitNodeStruct(const unit_node& n) {
    DecodedUnitNode out;
    out.id = fromZcbor(n.unit_node_id);
    out.kind = unitKindName(n.unit_node_kind.unit_kind_choice);
    out.state = unitStateName(n.unit_node_state.unit_state_choice);
    if (n.unit_node_state.unit_state_choice == unit_state_r::unit_state_finished_m_c) {
        out.endReason = fromZcbor(n.unit_node_state.unit_state_finished_m.Finished_end_reason);
    }
    if (n.unit_node_work_choice == unit_node::unit_node_work_tstr_c) {
        out.work = fromZcbor(n.unit_node_work_tstr);
    }
    for (size_t i = 0; i < n.unit_node_children_unit_id_m_count; ++i) {
        out.children << fromZcbor(n.unit_node_children_unit_id_m[i]);
    }
    if (n.unit_node_profile_present && n.unit_node_profile.unit_node_profile_choice ==
                                           unit_node_profile_r::unit_node_profile_profile_ref_m_c) {
        out.profileRef = fromZcbor(n.unit_node_profile.unit_node_profile_profile_ref_m);
    }
    if (n.unit_node_session_present && n.unit_node_session.unit_node_session_choice ==
                                           unit_node_session_r::unit_node_session_session_id_m_c) {
        out.sessionId = fromZcbor(n.unit_node_session.unit_node_session_session_id_m);
    }
    if (n.unit_node_title_present &&
        n.unit_node_title.unit_node_title_choice == unit_node_title_r::unit_node_title_tstr_c) {
        out.title = fromZcbor(n.unit_node_title.unit_node_title_tstr);
    }
    if (n.unit_node_role_present && n.unit_node_role.unit_node_role_choice ==
                                        unit_node_role_r::unit_node_role_session_role_m_c) {
        out.role =
            sessionRoleName(n.unit_node_role.unit_node_role_session_role_m.session_role_choice);
    }
    return out;
}

QString fsRootKindName(int choice) {
    switch (choice) {
    case fs_root_kind_t_r::fs_root_kind_t_host_tstr_c:
        return QStringLiteral("host");
    case fs_root_kind_t_r::fs_root_kind_t_session_tstr_c:
        return QStringLiteral("session");
    case fs_root_kind_t_r::fs_root_kind_t_workspace_tstr_c:
    default:
        return QStringLiteral("workspace");
    }
}

// --- Minimal canonical-CBOR writer/reader for the L0 wire envelope -----------------------------
// The envelope is a fixed, tiny shape (1-key outer map -> small inner map), so it is hand-coded
// rather than routed through the generated codec. Definite-length, big-endian args, matching the
// daemon's ciborium output (which the reader slices).

void cborAppendUint(QByteArray& b, quint64 v) {
    if (v < 24) {
        b.append(static_cast<char>(v));
    } else if (v <= 0xFF) {
        b.append(static_cast<char>(0x18));
        b.append(static_cast<char>(v));
    } else if (v <= 0xFFFF) {
        b.append(static_cast<char>(0x19));
        b.append(static_cast<char>((v >> 8) & 0xFF));
        b.append(static_cast<char>(v & 0xFF));
    } else if (v <= 0xFFFFFFFF) {
        b.append(static_cast<char>(0x1A));
        for (int s = 24; s >= 0; s -= 8) {
            b.append(static_cast<char>((v >> s) & 0xFF));
        }
    } else {
        b.append(static_cast<char>(0x1B));
        for (int s = 56; s >= 0; s -= 8) {
            b.append(static_cast<char>((v >> s) & 0xFF));
        }
    }
}

// Append a short (<24 byte) CBOR text string. All envelope keys/feature values qualify.
void cborAppendText(QByteArray& b, const char* s) {
    const auto len = static_cast<quint64>(qstrlen(s));
    b.append(static_cast<char>(0x60 | static_cast<char>(len)));
    b.append(s, static_cast<qsizetype>(len));
}

// Read a CBOR (major, argument) head at `i`, advancing `i`. Returns false on truncation / an
// indefinite or reserved length (the envelope is always definite).
bool cborReadHead(const uchar* p, qsizetype n, qsizetype& i, quint8& major, quint64& arg) {
    if (i >= n) {
        return false;
    }
    const quint8 ib = p[i++];
    major = ib >> 5;
    const quint8 lo = ib & 0x1F;
    if (lo < 24) {
        arg = lo;
        return true;
    }
    int nbytes = 0;
    switch (lo) {
    case 24:
        nbytes = 1;
        break;
    case 25:
        nbytes = 2;
        break;
    case 26:
        nbytes = 4;
        break;
    case 27:
        nbytes = 8;
        break;
    default:
        return false;
    }
    if (i + nbytes > n) {
        return false;
    }
    quint64 v = 0;
    for (int k = 0; k < nbytes; ++k) {
        v = (v << 8) | p[i++];
    }
    arg = v;
    return true;
}

// Read a CBOR text string at `i` into `out`, advancing `i`.
bool cborReadText(const uchar* p, qsizetype n, qsizetype& i, QByteArray* out) {
    quint8 major = 0;
    quint64 arg = 0;
    if (!cborReadHead(p, n, i, major, arg) || major != 3) {
        return false;
    }
    if (i + static_cast<qsizetype>(arg) > n) {
        return false;
    }
    *out = QByteArray(reinterpret_cast<const char*>(p + i), static_cast<qsizetype>(arg));
    i += static_cast<qsizetype>(arg);
    return true;
}

QString sessionStateName(int choice) {
    switch (choice) {
    case session_state_r::session_state_Active_tstr_c:
        return QStringLiteral("Active");
    case session_state_r::session_state_suspended_m_c:
        return QStringLiteral("Suspended");
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

// The host-request kind name for a journal `Request` block (the block carries the id + kind, not
// the live prompt - the live merged log carries that).
QString hostRequestKindName(const host_request_kind_t_r& kind) {
    switch (kind.host_request_kind_t_choice) {
    case host_request_kind_t_r::host_request_kind_t_host_request_kind_approval_m_c:
        return QStringLiteral("Approval");
    case host_request_kind_t_r::host_request_kind_t_host_request_kind_input_m_c:
        return QStringLiteral("Input");
    case host_request_kind_t_r::host_request_kind_t_host_request_kind_choice_m_c:
        return QStringLiteral("Choice");
    case host_request_kind_t_r::host_request_kind_t_host_request_kind_delegate_m_c:
        return QStringLiteral("Delegate");
    case host_request_kind_t_r::host_request_kind_t_host_request_kind_spawn_m_c:
        return QStringLiteral("Spawn");
    default:
        return {};
    }
}

// Map a decoded transcript-block (a coalesced durable journal payload) to the typed facade struct.
DecodedTranscriptBlock decodeTranscriptBlock(const transcript_block_r& block) {
    DecodedTranscriptBlock out;
    switch (block.transcript_block_choice) {
    case transcript_block_r::transcript_block_message_m_c: {
        const transcript_block_message& m = block.transcript_block_message_m;
        out.kind = DecodedTranscriptBlock::Kind::Message;
        switch (m.Message_role.transcript_role_choice) {
        case transcript_role_r::transcript_role_User_tstr_c:
            out.role = QStringLiteral("User");
            break;
        case transcript_role_r::transcript_role_System_tstr_c:
            out.role = QStringLiteral("System");
            break;
        default:
            out.role = QStringLiteral("Assistant");
            break;
        }
        out.text = fromZcbor(m.Message_text);
        break;
    }
    case transcript_block_r::transcript_block_tool_call_m_c: {
        const transcript_block_tool_call& t = block.transcript_block_tool_call_m;
        out.kind = DecodedTranscriptBlock::Kind::ToolCall;
        out.callId = fromZcbor(t.ToolCall_call_id);
        out.toolName = fromZcbor(t.ToolCall_name);
        out.argsSummary = fromZcbor(t.ToolCall_args_summary);
        break;
    }
    case transcript_block_r::transcript_block_tool_result_m_c: {
        const transcript_block_tool_result& t = block.transcript_block_tool_result_m;
        out.kind = DecodedTranscriptBlock::Kind::ToolResult;
        out.callId = fromZcbor(t.ToolResult_call_id);
        out.ok = t.ToolResult_ok;
        out.summary = fromZcbor(t.ToolResult_summary);
        break;
    }
    case transcript_block_r::transcript_block_request_m_c: {
        const transcript_block_request& r = block.transcript_block_request_m;
        out.kind = DecodedTranscriptBlock::Kind::Request;
        out.requestId = r.Request_request_id;
        out.hostKind = hostRequestKindName(r.Request_kind);
        break;
    }
    case transcript_block_r::transcript_block_content_m_c: {
        const transcript_block_content& c = block.transcript_block_content_m;
        out.kind = DecodedTranscriptBlock::Kind::Content;
        out.contentKind = fromZcbor(c.Content_kind);
        break;
    }
    default:
        break;
    }
    return out;
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
// Scratch buffers a populated profile_spec borrows (the zcbor_string fields point into these), so
// the caller must keep this alive until encodeRequest runs.
struct ProfileSpecScratch {
    QByteArray id, model, prompt, baseUrl, credRef, fbCredRef;
    QList<QByteArray> toolBufs, baTi, baCr;
};

// Populate a generated profile_spec from a DecodedProfileSpec (shared by ProfileCreate/Update and
// by ProfileImport's embedded Distribution.profile).
void fillProfileSpec(profile_spec& ps, const DecodedProfileSpec& s, ProfileSpecScratch& sc) {
    const auto setZ = [](zcbor_string& z, const QByteArray& b) {
        z.value = reinterpret_cast<const uint8_t*>(b.constData());
        z.len = static_cast<size_t>(b.size());
    };
    sc.id = s.id.toUtf8();
    sc.model = s.model.toUtf8();
    sc.prompt = s.systemPrompt.toUtf8();
    sc.baseUrl = s.baseUrl.toUtf8();
    sc.credRef = s.credentialRef.toUtf8();
    sc.fbCredRef = s.fallbackCredentialRef.toUtf8();
    sc.toolBufs.clear();
    for (const QString& t : s.toolAllowlist) {
        sc.toolBufs.append(t.toUtf8());
    }
    sc.baTi.clear();
    sc.baCr.clear();
    for (const DecodedBoundAccount& b : s.boundAccounts) {
        sc.baTi.append(b.transportInstance.toUtf8());
        sc.baCr.append(b.credentialRef.toUtf8());
    }
    const QByteArray& id = sc.id;
    const QByteArray& model = sc.model;
    const QByteArray& prompt = sc.prompt;
    const QByteArray& baseUrl = sc.baseUrl;
    const QByteArray& credRef = sc.credRef;
    const QByteArray& fbCredRef = sc.fbCredRef;
    const QList<QByteArray>& toolBufs = sc.toolBufs;
    const QList<QByteArray>& baTi = sc.baTi;
    const QList<QByteArray>& baCr = sc.baCr;

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
}

QByteArray encodeProfileMutation(bool update, const DecodedProfileSpec& s) {
    api_request_r request{};
    request.api_request_choice = update ? api_request_r::api_request_request_profile_update_m_c
                                        : api_request_r::api_request_request_profile_create_m_c;
    profile_spec& ps = update ? request.api_request_request_profile_update_m.ProfileUpdate_spec
                              : request.api_request_request_profile_create_m.ProfileCreate_spec;
    ProfileSpecScratch sc;
    fillProfileSpec(ps, s, sc);
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

QByteArray NodeApiCodec::encodeSessionsQueryRequest(bool hasSinceRev, quint64 sinceRev) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_sessions_query_m_c;
    // Leave scope/after/limit absent: an empty session-query map asks the daemon for its default
    // (TopLevel) scope, first page. `since_rev` (when set) makes it an L4 delta read.
    request.api_request_request_sessions_query_m = request_sessions_query{};
    session_query& q = request.api_request_request_sessions_query_m.SessionsQuery_query;
    q.session_query_since_rev_present = hasSinceRev;
    if (hasSinceRev) {
        q.session_query_since_rev.session_query_since_rev_choice =
            session_query_since_rev_r::session_query_since_rev_uint_c;
        q.session_query_since_rev.session_query_since_rev_uint = static_cast<uint32_t>(sinceRev);
    }
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

QByteArray NodeApiCodec::encodeSessionHistoryRequest(const QString& sessionId, quint64 afterCursor,
                                                     quint32 max) {
    const QByteArray session = sessionId.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_session_history_m_c;
    request_session_history& hist = request.api_request_request_session_history_m;
    hist.SessionHistory_session.value = reinterpret_cast<const uint8_t*>(session.constData());
    hist.SessionHistory_session.len = static_cast<size_t>(session.size());
    hist.SessionHistory_after_cursor = static_cast<uint32_t>(afterCursor);
    hist.SessionHistory_max = max;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeEventsSinceRequest(quint64 cursor, bool hasWaitMs, quint32 waitMs) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_events_since_m_c;
    request_events_since& feed = request.api_request_request_events_since_m;
    feed.EventsSince_cursor = static_cast<uint32_t>(cursor);
    feed.EventsSince_wait_ms_present = hasWaitMs;
    if (hasWaitMs) {
        feed.EventsSince_wait_ms.EventsSince_wait_ms_choice =
            EventsSince_wait_ms_r::EventsSince_wait_ms_uint_c;
        feed.EventsSince_wait_ms.EventsSince_wait_ms_uint = waitMs;
    }
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

QByteArray NodeApiCodec::encodeProfileExportRequest(const QString& id) {
    const QByteArray idUtf8 = id.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_profile_export_m_c;
    request.api_request_request_profile_export_m.ProfileExport_id.value =
        reinterpret_cast<const uint8_t*>(idUtf8.constData());
    request.api_request_request_profile_export_m.ProfileExport_id.len =
        static_cast<size_t>(idUtf8.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeProfileHistoryRequest(const QString& id) {
    const QByteArray idUtf8 = id.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_profile_history_m_c;
    request.api_request_request_profile_history_m.ProfileHistory_id.value =
        reinterpret_cast<const uint8_t*>(idUtf8.constData());
    request.api_request_request_profile_history_m.ProfileHistory_id.len =
        static_cast<size_t>(idUtf8.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeProfileAtRequest(const QString& id, quint64 seq) {
    const QByteArray idUtf8 = id.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_profile_at_m_c;
    request.api_request_request_profile_at_m.ProfileAt_id.value =
        reinterpret_cast<const uint8_t*>(idUtf8.constData());
    request.api_request_request_profile_at_m.ProfileAt_id.len = static_cast<size_t>(idUtf8.size());
    request.api_request_request_profile_at_m.ProfileAt_seq = static_cast<uint32_t>(seq);
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeProfileRevertRequest(const QString& id, quint64 seq) {
    const QByteArray idUtf8 = id.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_profile_revert_m_c;
    request.api_request_request_profile_revert_m.ProfileRevert_id.value =
        reinterpret_cast<const uint8_t*>(idUtf8.constData());
    request.api_request_request_profile_revert_m.ProfileRevert_id.len =
        static_cast<size_t>(idUtf8.size());
    request.api_request_request_profile_revert_m.ProfileRevert_seq = static_cast<uint32_t>(seq);
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeProfileImportRequest(const DecodedDistribution& dist,
                                                    const QString& newId) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_profile_import_m_c;
    request_profile_import& imp = request.api_request_request_profile_import_m;
    distribution& d = imp.ProfileImport_dist;
    d.distribution_wire_version = dist.wireVersion;
    // The embedded profile spec rides the shared filler (scratch must outlive encodeRequest).
    ProfileSpecScratch specScratch;
    fillProfileSpec(d.distribution_profile, dist.profile, specScratch);
    // head_seq + source are optional; omit them (a fresh import does not pin a parent).
    d.distribution_head_seq_present = false;
    d.distribution_source_present = false;
    // Skills: name + optional category + the path->content (tstr) files map.
    const auto setZ = [](zcbor_string& z, const QByteArray& b) {
        z.value = reinterpret_cast<const uint8_t*>(b.constData());
        z.len = static_cast<size_t>(b.size());
    };
    // Hold every borrowed buffer alive until encodeRequest below (reserve so the per-skill inner
    // lists are never moved while we hold references into them).
    QList<QByteArray> skillNames;
    QList<QByteArray> skillCats;
    QList<QList<QByteArray>> fileKeys; // per-skill
    QList<QList<QByteArray>> fileVals;
    const size_t sn = qMin<size_t>(static_cast<size_t>(dist.skills.size()), 64);
    skillNames.reserve(static_cast<qsizetype>(sn));
    skillCats.reserve(static_cast<qsizetype>(sn));
    fileKeys.reserve(static_cast<qsizetype>(sn));
    fileVals.reserve(static_cast<qsizetype>(sn));
    d.distribution_skills_skill_bundle_m_count = sn;
    for (size_t i = 0; i < sn; ++i) {
        const DecodedSkillBundle& src = dist.skills.at(static_cast<int>(i));
        skill_bundle& sb = d.distribution_skills_skill_bundle_m[i];
        skillNames.append(src.name.toUtf8());
        setZ(sb.skill_bundle_name, skillNames.last());
        if (!src.category.isEmpty()) {
            sb.skill_bundle_category_choice = skill_bundle::skill_bundle_category_tstr_c;
            skillCats.append(src.category.toUtf8());
            setZ(sb.skill_bundle_category_tstr, skillCats.last());
        } else {
            skillCats.append(QByteArray());
            sb.skill_bundle_category_choice = skill_bundle::skill_bundle_category_null_m_c;
        }
        fileKeys.append(QList<QByteArray>{});
        fileVals.append(QList<QByteArray>{});
        QList<QByteArray>& keys = fileKeys[static_cast<qsizetype>(i)];
        QList<QByteArray>& vals = fileVals[static_cast<qsizetype>(i)];
        const size_t fn = qMin<size_t>(static_cast<size_t>(src.files.size()), 64);
        keys.reserve(static_cast<qsizetype>(fn));
        vals.reserve(static_cast<qsizetype>(fn));
        sb.files_tstrtstr_count = fn;
        size_t fi = 0;
        for (auto it = src.files.constBegin(); it != src.files.constEnd() && fi < fn; ++it, ++fi) {
            keys.append(it.key().toUtf8());
            vals.append(it.value().toUtf8());
            setZ(sb.files_tstrtstr[fi].skill_bundle_files_tstrtstr_key, keys.last());
            setZ(sb.files_tstrtstr[fi].files_tstrtstr, vals.last());
        }
    }
    const QByteArray newIdUtf8 = newId.toUtf8();
    imp.ProfileImport_new_id_present = !newId.isEmpty();
    if (!newId.isEmpty()) {
        imp.ProfileImport_new_id.ProfileImport_new_id_choice =
            ProfileImport_new_id_r::ProfileImport_new_id_tstr_c;
        setZ(imp.ProfileImport_new_id.ProfileImport_new_id_tstr, newIdUtf8);
    }
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

QByteArray NodeApiCodec::encodeHelloFrame() {
    QByteArray b;
    b.append(static_cast<char>(0xA1)); // map(1)
    cborAppendText(b, "Hello");
    b.append(static_cast<char>(0xA2)); // map(2): wire_version, features
    cborAppendText(b, "wire_version");
    cborAppendUint(b, kWireVersion);
    cborAppendText(b, "features");
    b.append(static_cast<char>(0x82)); // array(2)
    cborAppendText(b, "mux");
    cborAppendText(b, "stream");
    return b;
}

QByteArray NodeApiCodec::encodeCallFrame(quint64 id, const QByteArray& requestCbor) {
    QByteArray b;
    b.append(static_cast<char>(0xA1)); // map(1)
    cborAppendText(b, "Call");
    b.append(static_cast<char>(0xA2)); // map(2): id, req
    cborAppendText(b, "id");
    cborAppendUint(b, id);
    cborAppendText(b, "req");
    b.append(requestCbor); // req is the final key, so the daemon decodes it as the trailing item
    return b;
}

QByteArray NodeApiCodec::encodeOpenFrame(quint64 id, const QByteArray& requestCbor) {
    QByteArray b;
    b.append(static_cast<char>(0xA1)); // map(1)
    cborAppendText(b, "Open");
    b.append(static_cast<char>(0xA2)); // map(2): id, req
    cborAppendText(b, "id");
    cborAppendUint(b, id);
    cborAppendText(b, "req");
    b.append(requestCbor);
    return b;
}

QByteArray NodeApiCodec::encodeCancelFrame(quint64 id) {
    QByteArray b;
    b.append(static_cast<char>(0xA1)); // map(1)
    cborAppendText(b, "Cancel");
    b.append(static_cast<char>(0xA1)); // map(1): id
    cborAppendText(b, "id");
    cborAppendUint(b, id);
    return b;
}

bool NodeApiCodec::decodeWireFrame(const QByteArray& frameCbor, DecodedWireFrame* out) {
    if (out == nullptr) {
        return false;
    }
    *out = DecodedWireFrame{};
    const auto* p = reinterpret_cast<const uchar*>(frameCbor.constData());
    const qsizetype n = frameCbor.size();
    qsizetype i = 0;
    quint8 major = 0;
    quint64 arg = 0;
    if (!cborReadHead(p, n, i, major, arg) || major != 5 || arg != 1) {
        return false; // outer must be map(1) { <tag>: ... }
    }
    QByteArray tag;
    if (!cborReadText(p, n, i, &tag)) {
        return false;
    }
    if (!cborReadHead(p, n, i, major, arg) || major != 5) {
        return false; // inner map
    }
    const quint64 innerFields = arg;
    if (tag == "Hello") {
        out->kind = WireFrameKind::Hello;
        // Inner map is {wire_version, features}. Surface the capability list so the client can gate
        // optional surfaces (e.g. versioning). Parse by key; the only non-array value
        // (wire_version) is a uint whose head cborReadHead consumes whole.
        for (quint64 field = 0; field < innerFields; ++field) {
            QByteArray hkey;
            if (!cborReadText(p, n, i, &hkey)) {
                break;
            }
            if (hkey == "features") {
                quint8 fmaj = 0;
                quint64 farg = 0;
                if (!cborReadHead(p, n, i, fmaj, farg) || fmaj != 4) {
                    break; // expected an array
                }
                for (quint64 k = 0; k < farg; ++k) {
                    QByteArray feat;
                    if (!cborReadText(p, n, i, &feat)) {
                        break;
                    }
                    out->features.append(QString::fromUtf8(feat));
                }
            } else if (!cborReadHead(p, n, i, major, arg)) {
                break; // wire_version (uint) or another scalar
            }
        }
        return true;
    }
    // Every other frame leads with "id" (declaration order). Read it.
    QByteArray key;
    if (!cborReadText(p, n, i, &key) || key != "id") {
        return false;
    }
    if (!cborReadHead(p, n, i, major, arg) || major != 0) {
        return false;
    }
    out->id = arg;
    if (tag == "Reply" || tag == "Item") {
        if (!cborReadText(p, n, i, &key) || key != "res") {
            return false;
        }
        // "res" is the final field, so the remainder of the frame is the inner ApiResponse item.
        out->payload = frameCbor.mid(i);
        out->kind = (tag == "Item") ? WireFrameKind::Item : WireFrameKind::Reply;
        return true;
    }
    if (tag == "End") {
        out->kind = WireFrameKind::End;
        if (innerFields >= 2 && cborReadText(p, n, i, &key) && key == "error") {
            // null (0xF6) means a clean close; anything else (or a missing value) is an error.
            out->hasError = (i >= n) || (p[i] != 0xF6);
        }
        return true;
    }
    if (tag == "Reset") {
        out->kind = WireFrameKind::Reset;
        if (cborReadText(p, n, i, &key) && key == "epoch" && cborReadHead(p, n, i, major, arg)) {
            out->epoch = arg;
        }
        if (cborReadText(p, n, i, &key) && key == "head_seq" && cborReadHead(p, n, i, major, arg)) {
            out->headSeq = arg;
        }
        return true;
    }
    return false;
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
    case api_response_r::api_response_response_events_page_m_c:
        return ApiResponseKind::EventsPage;
    case api_response_r::api_response_response_journal_m_c:
        return ApiResponseKind::Journal;
    case api_response_r::api_response_response_fs_roots_m_c:
        return ApiResponseKind::FsRoots;
    case api_response_r::api_response_response_fs_list_m_c:
        return ApiResponseKind::FsList;
    case api_response_r::api_response_response_fs_stat_m_c:
        return ApiResponseKind::FsStat;
    case api_response_r::api_response_response_fs_read_m_c:
        return ApiResponseKind::FsRead;
    case api_response_r::api_response_response_fs_write_m_c:
        return ApiResponseKind::FsWrite;
    case api_response_r::api_response_response_fs_watch_m_c:
        return ApiResponseKind::FsWatch;
    case api_response_r::api_response_response_fs_search_m_c:
        return ApiResponseKind::FsSearch;
    case api_response_r::api_response_response_fleet_m_c:
        return ApiResponseKind::Fleet;
    case api_response_r::api_response_response_tree_m_c:
        return ApiResponseKind::Tree;
    case api_response_r::api_response_response_unit_m_c:
        return ApiResponseKind::Unit;
    case api_response_r::api_response_response_unit_events_m_c:
        return ApiResponseKind::UnitEvents;
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
    case api_response_r::api_response_response_profile_m_c:
        return ApiResponseKind::Profile;
    case api_response_r::api_response_response_distribution_m_c:
        return ApiResponseKind::Distribution;
    case api_response_r::api_response_response_revisions_m_c:
        return ApiResponseKind::Revisions;
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
                                     QString* nextCursor, quint64* rev, QStringList* removed) {
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
                                        quint64* nextSeq, quint64* headSeq, quint64* epoch) {
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
    if (epoch != nullptr) {
        *epoch = page.log_page_view_epoch;
    }
    return true;
}

bool NodeApiCodec::decodeEventsPage(const QByteArray& responseCbor, DecodedEventsPage* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_events_page_m_c) {
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
        case node_event_r::node_event_fleet_changed_m_c:
            decoded.kind = DecodedNodeEvent::Kind::FleetChanged;
            decoded.rev = ev.node_event_fleet_changed_m.FleetChanged_rev;
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
            break;
        }
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
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_journal_m_c) {
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
                        journal_page_view::journal_page_view_sealed_after_uint_c;
    if (hasSealedAfter != nullptr) {
        *hasSealedAfter = sealed;
    }
    if (sealedAfter != nullptr) {
        *sealedAfter = sealed ? page.journal_page_view_sealed_after_uint : 0;
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

bool NodeApiCodec::decodeDistribution(const QByteArray& responseCbor, DecodedDistribution* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_distribution_m_c) {
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

bool NodeApiCodec::decodeRevisions(const QByteArray& responseCbor, QList<DecodedRevision>* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_revisions_m_c) {
        return false;
    }
    const response_revisions& rr = response->api_response_response_revisions_m;
    out->clear();
    for (size_t i = 0; i < rr.response_revisions_Revisions_revision_m_count; ++i) {
        const revision& r = rr.response_revisions_Revisions_revision_m[i];
        DecodedRevision d;
        d.seq = r.revision_seq;
        d.hasParent = r.revision_parent_choice == revision::revision_parent_uint_c;
        if (d.hasParent) {
            d.parent = r.revision_parent_uint;
        }
        d.author = r.revision_author.author_choice == author_r::author_agent_m_c
                       ? fromZcbor(r.revision_author.author_agent_m.author_agent_agent)
                       : QStringLiteral("operator");
        d.reason = fromZcbor(r.revision_reason);
        d.tsMs = r.revision_ts_ms;
        out->append(d);
    }
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

// --- Filesystem surface (Phase 4) ----------------------------------------------------------------

QByteArray NodeApiCodec::encodeFsRootsRequest() {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_fs_roots_m_c;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeFsListRequest(const QString& rootId, const QString& dir,
                                             bool showIgnored) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_fs_list_m_c;
    request_fs_list& l = request.api_request_request_fs_list_m;
    QByteArray rootScratch;
    setFsRootId(rootId, l.FsList_root, rootScratch);
    const QByteArray dirU = dir.toUtf8();
    l.FsList_dir.value = reinterpret_cast<const uint8_t*>(dirU.constData());
    l.FsList_dir.len = static_cast<size_t>(dirU.size());
    l.FsList_show_ignored_present = showIgnored;
    if (showIgnored) {
        l.FsList_show_ignored.FsList_show_ignored = true;
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeFsReadRequest(const QString& rootId, const QString& path,
                                             quint64 maxBytes) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_fs_read_m_c;
    request_fs_read& r = request.api_request_request_fs_read_m;
    QByteArray rootScratch;
    setFsRootId(rootId, r.FsRead_root, rootScratch);
    const QByteArray pathU = path.toUtf8();
    r.FsRead_path.value = reinterpret_cast<const uint8_t*>(pathU.constData());
    r.FsRead_path.len = static_cast<size_t>(pathU.size());
    r.FsRead_max_bytes_present = maxBytes > 0;
    if (maxBytes > 0) {
        r.FsRead_max_bytes.FsRead_max_bytes = static_cast<uint32_t>(maxBytes);
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeFsWriteRequest(const QString& rootId, const QString& path,
                                              const QByteArray& bytes, const QString& baseRevision,
                                              bool force) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_fs_write_m_c;
    request_fs_write& w = request.api_request_request_fs_write_m;
    QByteArray rootScratch;
    setFsRootId(rootId, w.FsWrite_root, rootScratch);
    const QByteArray pathU = path.toUtf8();
    w.FsWrite_path.value = reinterpret_cast<const uint8_t*>(pathU.constData());
    w.FsWrite_path.len = static_cast<size_t>(pathU.size());
    // Content is a CBOR bstr now (no array cap), so arbitrary file sizes round-trip.
    w.FsWrite_bytes.value = reinterpret_cast<const uint8_t*>(bytes.constData());
    w.FsWrite_bytes.len = static_cast<size_t>(bytes.size());
    // Optional optimistic-concurrency precondition: parse the opaque "mtime:size" etag.
    const QStringList rev = baseRevision.split(QLatin1Char(':'));
    w.FsWrite_base_revision_present = rev.size() == 2;
    if (rev.size() == 2) {
        w.FsWrite_base_revision.FsWrite_base_revision_choice =
            FsWrite_base_revision_r::FsWrite_base_revision_fs_revision_m_c;
        // mtime_ms/size are u64 (ms epoch / file length); parse as 64-bit so a real etag is not
        // truncated.
        w.FsWrite_base_revision.FsWrite_base_revision_fs_revision_m.fs_revision_mtime_ms =
            rev[0].toULongLong();
        w.FsWrite_base_revision.FsWrite_base_revision_fs_revision_m.fs_revision_size =
            rev[1].toULongLong();
    }
    w.FsWrite_force_present = force;
    if (force) {
        w.FsWrite_force.FsWrite_force = true;
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeFsWatchPollRequest(const QString& rootId, const QString& dir,
                                                  quint64 afterSeq, quint32 max) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_fs_watch_poll_m_c;
    request_fs_watch_poll& w = request.api_request_request_fs_watch_poll_m;
    QByteArray rootScratch;
    setFsRootId(rootId, w.FsWatchPoll_root, rootScratch);
    const QByteArray dirU = dir.toUtf8();
    w.FsWatchPoll_dir.value = reinterpret_cast<const uint8_t*>(dirU.constData());
    w.FsWatchPoll_dir.len = static_cast<size_t>(dirU.size());
    w.FsWatchPoll_after_seq = static_cast<uint32_t>(afterSeq);
    w.FsWatchPoll_max = max;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeFsSearchRequest(const QString& rootId, const QString& query,
                                               bool regex, bool caseSensitive, quint32 maxResults,
                                               quint32 page) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_fs_search_m_c;
    request_fs_search& s = request.api_request_request_fs_search_m;
    QByteArray rootScratch;
    setFsRootId(rootId, s.FsSearch_root, rootScratch);
    const QByteArray queryU = query.toUtf8();
    s.FsSearch_query.fs_search_query_query.value =
        reinterpret_cast<const uint8_t*>(queryU.constData());
    s.FsSearch_query.fs_search_query_query.len = static_cast<size_t>(queryU.size());
    s.FsSearch_query.fs_search_query_regex_present = regex;
    if (regex) {
        s.FsSearch_query.fs_search_query_regex.fs_search_query_regex = true;
    }
    s.FsSearch_query.fs_search_query_case_sensitive_present = caseSensitive;
    if (caseSensitive) {
        s.FsSearch_query.fs_search_query_case_sensitive.fs_search_query_case_sensitive = true;
    }
    s.FsSearch_query.fs_search_query_max_results_present = maxResults > 0;
    if (maxResults > 0) {
        s.FsSearch_query.fs_search_query_max_results.fs_search_query_max_results = maxResults;
    }
    s.FsSearch_query.fs_search_query_page_present = page > 0;
    if (page > 0) {
        s.FsSearch_query.fs_search_query_page.fs_search_query_page = page;
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

bool NodeApiCodec::decodeFsRoots(const QByteArray& responseCbor, QList<DecodedFsRoot>* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_fs_roots_m_c) {
        return false;
    }
    const response_fs_roots& page = response->api_response_response_fs_roots_m;
    out->clear();
    for (size_t i = 0; i < page.response_fs_roots_FsRoots_fs_root_m_count; ++i) {
        const fs_root& fr = page.response_fs_roots_FsRoots_fs_root_m[i];
        DecodedFsRoot row;
        row.id = fsRootIdToString(fr.fs_root_id);
        row.label = fromZcbor(fr.fs_root_label);
        row.kind = fsRootKindName(fr.fs_root_kind.fs_root_kind_t_choice);
        out->append(row);
    }
    return true;
}

bool NodeApiCodec::decodeFsList(const QByteArray& responseCbor, QList<DecodedFsEntry>* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_fs_list_m_c) {
        return false;
    }
    const response_fs_list& page = response->api_response_response_fs_list_m;
    out->clear();
    for (size_t i = 0; i < page.response_fs_list_FsList_fs_entry_m_count; ++i) {
        const fs_entry& e = page.response_fs_list_FsList_fs_entry_m[i];
        DecodedFsEntry row;
        row.name = fromZcbor(e.fs_entry_name);
        row.path = fromZcbor(e.fs_entry_path);
        row.kind = fsEntryKindName(e.fs_entry_kind.fs_entry_kind_t_choice);
        row.size = e.fs_entry_size;
        row.mtimeMs = e.fs_entry_mtime_ms;
        row.ignored = e.fs_entry_ignored_present && e.fs_entry_ignored.fs_entry_ignored;
        out->append(row);
    }
    return true;
}

bool NodeApiCodec::decodeFsRead(const QByteArray& responseCbor, DecodedFsContent* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_fs_read_m_c) {
        return false;
    }
    const fs_content& c = response->api_response_response_fs_read_m.response_fs_read_FsRead;
    out->bytes = bytesFromZcbor(c.fs_content_bytes);
    out->revision = QStringLiteral("%1:%2")
                        .arg(c.fs_content_revision.fs_revision_mtime_ms)
                        .arg(c.fs_content_revision.fs_revision_size);
    out->truncated = c.fs_content_truncated_present && c.fs_content_truncated.fs_content_truncated;
    return true;
}

bool NodeApiCodec::decodeFsWrite(const QByteArray& responseCbor, QString* revision) {
    if (revision == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_fs_write_m_c) {
        return false;
    }
    const fs_revision& r = response->api_response_response_fs_write_m.response_fs_write_FsWrite;
    *revision = QStringLiteral("%1:%2").arg(r.fs_revision_mtime_ms).arg(r.fs_revision_size);
    return true;
}

bool NodeApiCodec::decodeFsWatch(const QByteArray& responseCbor, DecodedFsWatchPage* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_fs_watch_m_c) {
        return false;
    }
    const fs_watch_page_view& p =
        response->api_response_response_fs_watch_m.response_fs_watch_FsWatch;
    out->events.clear();
    for (size_t i = 0; i < p.fs_watch_page_view_events_fs_change_m_count; ++i) {
        const fs_change& ch = p.fs_watch_page_view_events_fs_change_m[i];
        DecodedFsChange c;
        c.path = fromZcbor(ch.fs_change_path);
        c.kind = fsChangeKindName(ch.fs_change_kind.fs_change_kind_t_choice);
        out->events.append(c);
    }
    out->nextSeq = p.fs_watch_page_view_next_seq;
    out->headSeq = p.fs_watch_page_view_head_seq;
    out->reset = p.fs_watch_page_view_reset;
    return true;
}

bool NodeApiCodec::decodeFsSearch(const QByteArray& responseCbor, DecodedFsSearchPage* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_fs_search_m_c) {
        return false;
    }
    const fs_search_page& page =
        response->api_response_response_fs_search_m.response_fs_search_FsSearch;
    out->hits.clear();
    for (size_t i = 0; i < page.fs_search_page_hits_fs_search_hit_m_count; ++i) {
        const fs_search_hit& h = page.fs_search_page_hits_fs_search_hit_m[i];
        DecodedFsSearchHit hit;
        hit.path = fromZcbor(h.fs_search_hit_path);
        hit.line = h.fs_search_hit_line;
        hit.col = h.fs_search_hit_col;
        hit.preview = fromZcbor(h.fs_search_hit_preview);
        out->hits.append(hit);
    }
    out->hasMore = page.fs_search_page_has_more_present &&
                   page.fs_search_page_has_more.fs_search_page_has_more;
    return true;
}

// --- Fleet / subagent tree (Phase 5b) ------------------------------------------------------------

QByteArray NodeApiCodec::encodeTreeRequest() {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_tree_m_c;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeFleetRequest() {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_fleet_m_c;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeUnitRequest(const QString& unitId) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_unit_m_c;
    const QByteArray u = unitId.toUtf8();
    request.api_request_request_unit_m.Unit_unit.value =
        reinterpret_cast<const uint8_t*>(u.constData());
    request.api_request_request_unit_m.Unit_unit.len = static_cast<size_t>(u.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeUnitEventsRequest(const QString& unitId, quint32 max) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_unit_events_m_c;
    const QByteArray u = unitId.toUtf8();
    request.api_request_request_unit_events_m.UnitEvents_unit.value =
        reinterpret_cast<const uint8_t*>(u.constData());
    request.api_request_request_unit_events_m.UnitEvents_unit.len = static_cast<size_t>(u.size());
    request.api_request_request_unit_events_m.UnitEvents_max = max;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodePauseRequest(const QString& unitId) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_pause_m_c;
    const QByteArray u = unitId.toUtf8();
    request.api_request_request_pause_m.Pause_unit.value =
        reinterpret_cast<const uint8_t*>(u.constData());
    request.api_request_request_pause_m.Pause_unit.len = static_cast<size_t>(u.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeResumeRequest(const QString& unitId) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_resume_m_c;
    const QByteArray u = unitId.toUtf8();
    request.api_request_request_resume_m.Resume_unit.value =
        reinterpret_cast<const uint8_t*>(u.constData());
    request.api_request_request_resume_m.Resume_unit.len = static_cast<size_t>(u.size());
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeScaleRequest(const QString& unitId, quint32 n) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_scale_m_c;
    const QByteArray u = unitId.toUtf8();
    request.api_request_request_scale_m.Scale_unit.value =
        reinterpret_cast<const uint8_t*>(u.constData());
    request.api_request_request_scale_m.Scale_unit.len = static_cast<size_t>(u.size());
    request.api_request_request_scale_m.Scale_n = n;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

bool NodeApiCodec::decodeTreeReport(const QByteArray& responseCbor, QList<DecodedUnitNode>* outFlat,
                                    QString* outRoot) {
    if (outFlat == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_tree_m_c) {
        return false;
    }
    const tree_report& tr = response->api_response_response_tree_m.response_tree_Tree;
    outFlat->clear();
    QHash<QString, DecodedUnitNode> byId;
    for (size_t i = 0; i < tr.tree_report_nodes_unit_node_m_count; ++i) {
        DecodedUnitNode n = decodeUnitNodeStruct(tr.tree_report_nodes_unit_node_m[i]);
        byId.insert(n.id, n);
    }
    const QString root = tr.tree_report_root_choice == tree_report::tree_report_root_unit_id_m_c
                             ? fromZcbor(tr.tree_report_root_unit_id_m)
                             : QString();
    if (outRoot != nullptr) {
        *outRoot = root;
    }
    if (root.isEmpty()) {
        return true; // an empty tree (no root) is a valid, common (fresh-daemon) result
    }
    // Pre-order DFS from the root, assigning depth + parent; a visited set guards against cycles.
    struct Frame {
        QString id;
        QString parent;
        int depth;
    };
    QSet<QString> seen;
    QList<Frame> stack; // processed LIFO so children render in their listed order
    stack.append({root, QString(), 0});
    while (!stack.isEmpty()) {
        const Frame f = stack.takeLast();
        if (seen.contains(f.id) || !byId.contains(f.id)) {
            continue;
        }
        seen.insert(f.id);
        DecodedUnitNode node = byId.value(f.id);
        node.depth = f.depth;
        node.parentId = f.parent;
        outFlat->append(node);
        // Push children in reverse so they pop (LIFO) in their listed order.
        for (qsizetype k = node.children.size() - 1; k >= 0; --k) {
            stack.append({node.children.at(k), f.id, f.depth + 1});
        }
    }
    return true;
}

bool NodeApiCodec::decodeUnit(const QByteArray& responseCbor, DecodedUnitNode* out, bool* found) {
    if (out == nullptr || found == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_unit_m_c) {
        return false;
    }
    const response_unit& u = response->api_response_response_unit_m;
    *found = u.response_unit_Unit_choice == response_unit::response_unit_Unit_unit_node_m_c;
    if (*found) {
        *out = decodeUnitNodeStruct(u.response_unit_Unit_unit_node_m);
    }
    return true;
}

bool NodeApiCodec::decodeFleetReport(const QByteArray& responseCbor, DecodedFleetReport* out) {
    if (out == nullptr) {
        return false;
    }
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != api_response_r::api_response_response_fleet_m_c) {
        return false;
    }
    const fleet_report& fr = response->api_response_response_fleet_m.response_fleet_Fleet;
    out->children.clear();
    for (size_t i = 0; i < fr.fleet_report_children_unit_id_m_count; ++i) {
        out->children << fromZcbor(fr.fleet_report_children_unit_id_m[i]);
    }
    return true;
}

} // namespace daemonapp::daemon
