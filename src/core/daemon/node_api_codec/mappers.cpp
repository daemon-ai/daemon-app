// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Shared codec helpers: zcbor_string <-> Qt, enum/struct projection mappers, and the encode/decode
// core (encodeRequest / encodeSimple / encodeWithFill, decodeResponse / decodeChecked,
// fillProfileSpec / encodeProfileMutation). Declarations live in node_api_codec/internal.h; the
// encode_*/decode_*/wire_l0 TUs consume them via `using namespace codec_detail`.

#include "daemon/node_api_codec/internal.h"

namespace daemonapp::daemon::codec_detail {
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

// Point a generated zcbor_string at an existing UTF-8/byte buffer (zcbor borrows, never copies, so
// `b` must outlive the encode call). Folds the value/len assignment pair repeated across encoders.
void setZcbor(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
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

// [wave2:app-delegation] F3: wire delegation-lifetime enum -> facade string.
QString delegationLifetimeName(int choice) {
    switch (choice) {
    case delegation_lifetime_r::delegation_lifetime_Persistent_tstr_c:
        return QStringLiteral("Persistent");
    case delegation_lifetime_r::delegation_lifetime_Ephemeral_tstr_c:
        return QStringLiteral("Ephemeral");
    default:
        return {};
    }
}

// [wave2:app-delegation] F3: decode the v29 engine-selector faithfully (Core | Foreign{agent}).
// Standalone from the profile-spec engine decode so the app-engines EngineIdentity rename does not
// collide; `kind` is "Core"/"Foreign", `agent` is the foreign agent name (empty for Core).
void decodeEngineSelector(const engine_selector_r& sel, QString* kind, QString* agent) {
    if (sel.engine_selector_choice == engine_selector_r::engine_selector_engine_foreign_m_c) {
        *kind = QStringLiteral("Foreign");
        *agent = fromZcbor(
            sel.engine_selector_engine_foreign_m.engine_foreign_Foreign.engine_foreign_agent_agent);
    } else {
        *kind = QStringLiteral("Core");
        agent->clear();
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
    // [wave2:app-delegation] F3: decode the v29 per-child lifetime + engine enrichment. Both are
    // optional-null on the wire; absent/null leaves the facade fields empty (chip hidden).
    if (n.unit_node_lifetime_present &&
        n.unit_node_lifetime.unit_node_lifetime_choice ==
            unit_node_lifetime_r::unit_node_lifetime_delegation_lifetime_m_c) {
        out.lifetime =
            delegationLifetimeName(n.unit_node_lifetime.unit_node_lifetime_delegation_lifetime_m
                                       .delegation_lifetime_choice);
    }
    if (n.unit_node_engine_present &&
        n.unit_node_engine.unit_node_engine_choice ==
            unit_node_engine_r::unit_node_engine_engine_selector_m_c) {
        decodeEngineSelector(n.unit_node_engine.unit_node_engine_engine_selector_m, &out.engineKind,
                             &out.engineAgent);
    }
    return out;
}

CachedSessionRow sessionRowFromInfo(const session_info& info) {
    CachedSessionRow row;
    row.sessionId = fromZcbor(info.session_info_session);
    row.state = sessionStateName(info.session_info_state.session_state_choice);
    if (info.session_info_bound_profile_present &&
        info.session_info_bound_profile.session_info_bound_profile_choice ==
            session_info_bound_profile_r::session_info_bound_profile_profile_ref_m_c) {
        row.profileRef =
            fromZcbor(info.session_info_bound_profile.session_info_bound_profile_profile_ref_m);
    }
    if (info.session_info_title_present && info.session_info_title.session_info_title_choice ==
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
        row.parentSessionId = fromZcbor(info.session_info_parent.session_info_parent_session_id_m);
    }
    if (info.session_info_pinned_present) {
        row.pinned = info.session_info_pinned.session_info_pinned;
    }
    if (info.session_info_archived_present) {
        row.archived = info.session_info_archived.session_info_archived;
    }
    return row;
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

void cborAppendTextLen(QByteArray& b, const QByteArray& s) {
    b.append(static_cast<char>(0x60)); // placeholder major-3 head; rewritten below
    // Emit the major-3 (text) head with the right argument width, then the bytes.
    b.chop(1);
    const auto len = static_cast<quint64>(s.size());
    if (len < 24) {
        b.append(static_cast<char>(0x60 | static_cast<char>(len)));
    } else if (len <= 0xFF) {
        b.append(static_cast<char>(0x78));
        b.append(static_cast<char>(len));
    } else if (len <= 0xFFFF) {
        b.append(static_cast<char>(0x79));
        b.append(static_cast<char>((len >> 8) & 0xFF));
        b.append(static_cast<char>(len & 0xFF));
    } else {
        b.append(static_cast<char>(0x7A));
        for (int sft = 24; sft >= 0; sft -= 8) {
            b.append(static_cast<char>((len >> sft) & 0xFF));
        }
    }
    b.append(s);
}

void cborAppendBytesAsUintArray(QByteArray& b, const QByteArray& bytes) {
    const auto len = static_cast<quint64>(bytes.size());
    // Array (major 4) head.
    if (len < 24) {
        b.append(static_cast<char>(0x80 | static_cast<char>(len)));
    } else if (len <= 0xFF) {
        b.append(static_cast<char>(0x98));
        b.append(static_cast<char>(len));
    } else if (len <= 0xFFFF) {
        b.append(static_cast<char>(0x99));
        b.append(static_cast<char>((len >> 8) & 0xFF));
        b.append(static_cast<char>(len & 0xFF));
    } else {
        b.append(static_cast<char>(0x9A));
        for (int sft = 24; sft >= 0; sft -= 8) {
            b.append(static_cast<char>((len >> sft) & 0xFF));
        }
    }
    for (const char c : bytes) {
        cborAppendUint(b, static_cast<quint8>(c));
    }
}

bool cborReadBytesFromUintArray(const uchar* p, qsizetype n, qsizetype& i, QByteArray* out) {
    quint8 major = 0;
    quint64 arg = 0;
    if (!cborReadHead(p, n, i, major, arg) || major != 4) {
        return false; // expected a CBOR array
    }
    QByteArray bytes;
    bytes.reserve(static_cast<qsizetype>(arg));
    for (quint64 k = 0; k < arg; ++k) {
        quint8 emaj = 0;
        quint64 earg = 0;
        if (!cborReadHead(p, n, i, emaj, earg) || emaj != 0 || earg > 0xFF) {
            return false; // each element must be a uint 0..255
        }
        bytes.append(static_cast<char>(static_cast<quint8>(earg)));
    }
    if (out != nullptr) {
        *out = bytes;
    }
    return true;
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
    case provider_selector_r::provider_selector_daemon_api_tstr_c:
        return QStringLiteral("daemon_api");
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
    if (provider == QStringLiteral("daemon_api")) {
        return provider_selector_r::provider_selector_daemon_api_tstr_c;
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
    if (m.model_descriptor_display_name_present &&
        m.model_descriptor_display_name.model_descriptor_display_name_choice ==
            model_descriptor_display_name_r::model_descriptor_display_name_tstr_c) {
        out->hasDisplayName = true;
        out->displayName =
            fromZcbor(m.model_descriptor_display_name.model_descriptor_display_name_tstr);
    }
    if (m.model_descriptor_context_length_choice ==
        model_descriptor::model_descriptor_context_length_uint_c) {
        out->hasContextLength = true;
        out->contextLength = m.model_descriptor_context_length_uint;
    }
    if (m.model_descriptor_input_price_micros_per_mtok_choice ==
        model_descriptor::model_descriptor_input_price_micros_per_mtok_uint64_m_c) {
        out->hasInputPrice = true;
        out->inputPriceMicrosPerMtok = m.model_descriptor_input_price_micros_per_mtok_uint64_m;
    }
    if (m.model_descriptor_output_price_micros_per_mtok_choice ==
        model_descriptor::model_descriptor_output_price_micros_per_mtok_uint64_m_c) {
        out->hasOutputPrice = true;
        out->outputPriceMicrosPerMtok = m.model_descriptor_output_price_micros_per_mtok_uint64_m;
    }
    out->local = m.model_descriptor_local;
}

// ProviderKind wire string (serde rename_all = snake_case): local | cloud | daemon_cloud.
QString providerKindName(int choice) {
    switch (choice) {
    case provider_kind_wire_r::provider_kind_wire_cloud_tstr_c:
        return QStringLiteral("cloud");
    case provider_kind_wire_r::provider_kind_wire_daemon_cloud_tstr_c:
        return QStringLiteral("daemon_cloud");
    case provider_kind_wire_r::provider_kind_wire_local_tstr_c:
    default:
        return QStringLiteral("local");
    }
}

void fillProviderDescriptor(const provider_descriptor& p, DecodedProviderDescriptor* out) {
    out->id = fromZcbor(p.provider_descriptor_id);
    out->displayName = fromZcbor(p.provider_descriptor_display_name);
    out->kind = providerKindName(p.provider_descriptor_kind.provider_kind_wire_choice);
    out->wireSelector = providerName(p.provider_descriptor_wire_selector.provider_selector_choice);
    out->requiresKey = p.provider_descriptor_requires_key;
    out->supportsModelDiscovery = p.provider_descriptor_supports_model_discovery;
    if (p.provider_descriptor_default_base_url_choice ==
        provider_descriptor::provider_descriptor_default_base_url_tstr_c) {
        out->hasDefaultBaseUrl = true;
        out->defaultBaseUrl = fromZcbor(p.provider_descriptor_default_base_url_tstr);
    }
    // [waveB:app-v30] CON-15: optional generic sign-in (family + node label). Everything off the
    // wire — no vendor strings client-side.
    if (p.provider_descriptor_sign_in_present &&
        p.provider_descriptor_sign_in.provider_descriptor_sign_in_choice ==
            provider_descriptor_sign_in_r::provider_descriptor_sign_in_provider_sign_in_m_c) {
        const provider_sign_in& si =
            p.provider_descriptor_sign_in.provider_descriptor_sign_in_provider_sign_in_m;
        out->hasSignIn = true;
        out->signInFamily = fromZcbor(si.provider_sign_in_family);
        out->signInLabel = fromZcbor(si.provider_sign_in_label);
    }
}

void fillCustomProvider(const custom_provider& p, DecodedCustomProvider* out) {
    out->id = fromZcbor(p.custom_provider_id);
    out->displayName = fromZcbor(p.custom_provider_display_name);
    out->baseUrl = fromZcbor(p.custom_provider_base_url);
    out->wireSelector = providerName(p.custom_provider_wire_selector.provider_selector_choice);
    out->requiresKey = p.custom_provider_requires_key;
    if (p.custom_provider_credential_ref_present &&
        p.custom_provider_credential_ref.custom_provider_credential_ref_choice ==
            custom_provider_credential_ref_r::custom_provider_credential_ref_tstr_c) {
        out->hasCredentialRef = true;
        out->credentialRef =
            fromZcbor(p.custom_provider_credential_ref.custom_provider_credential_ref_tstr);
    }
    out->source = p.custom_provider_source.custom_provider_source_t_choice ==
                          custom_provider_source_t_r::custom_provider_source_t_config_tstr_c
                      ? QStringLiteral("config")
                      : QStringLiteral("user");
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

QString authorToString(const author_r& a, bool* isAgent) {
    const bool agent = a.author_choice == author_r::author_agent_m_c;
    if (isAgent != nullptr) {
        *isAgent = agent;
    }
    return agent ? fromZcbor(a.author_agent_m.author_agent_agent) : QStringLiteral("operator");
}

AgentVerification agentVerificationFromChoice(int choice) {
    switch (choice) {
    case agent_verification_r::agent_verification_Verified_tstr_c:
        return AgentVerification::Verified;
    case agent_verification_r::agent_verification_Unverified_tstr_c:
        return AgentVerification::Unverified;
    default:
        return AgentVerification::NotInstalled;
    }
}

// Decode a foreign-backend union (wire v30): externally tagged AgentNative vs NodeProvider.
DecodedForeignBackend decodeForeignBackend(const foreign_backend_r& fb) {
    DecodedForeignBackend out;
    if (fb.foreign_backend_choice == foreign_backend_r::foreign_backend_node_provider_m_c) {
        const foreign_backend_node_provider& np = fb.foreign_backend_node_provider_m;
        out.kind = QStringLiteral("NodeProvider");
        out.nodeProvider = providerName(np.NodeProvider_provider.provider_selector_choice);
        out.nodeModel = fromZcbor(np.NodeProvider_model);
        if (np.NodeProvider_credential_ref_choice ==
            foreign_backend_node_provider::NodeProvider_credential_ref_tstr_c) {
            out.hasNodeCredentialRef = true;
            out.nodeCredentialRef = fromZcbor(np.NodeProvider_credential_ref_tstr);
        }
    } else {
        const foreign_backend_agent_native& an = fb.foreign_backend_agent_native_m;
        out.kind = QStringLiteral("AgentNative");
        if (an.AgentNative_model_choice == foreign_backend_agent_native::AgentNative_model_tstr_c) {
            out.hasAgentNativeModel = true;
            out.agentNativeModel = fromZcbor(an.AgentNative_model_tstr);
        }
    }
    return out;
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
    if (ps.profile_spec_tool_allowlist_choice == profile_spec::tool_allowlist_tstr_l_c) {
        out.hasToolAllowlist = true;
        for (size_t i = 0; i < ps.tool_allowlist_tstr_l_tstr_count; ++i) {
            out.toolAllowlist << fromZcbor(ps.tool_allowlist_tstr_l_tstr[i]);
        }
    }
    if (ps.profile_spec_budget.budget_tokens_choice == budget::budget_tokens_uint64_m_c) {
        out.hasBudgetTokens = true;
        out.budgetTokens = ps.profile_spec_budget.budget_tokens_uint64_m;
    }
    if (ps.profile_spec_budget.budget_wall_ms_choice == budget::budget_wall_ms_uint64_m_c) {
        out.hasBudgetWallMs = true;
        out.budgetWallMs = ps.profile_spec_budget.budget_wall_ms_uint64_m;
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
        const bound_account& account = ps.profile_spec_bound_accounts_bound_account_m[i];
        out.boundAccounts.append(
            DecodedBoundAccount{fromZcbor(account.bound_account_transport_instance),
                                fromZcbor(account.bound_account_credential_ref)});
    }
    // The engine selector (wire v29): absent on pre-engine encodings -> the "Core" default the
    // struct already carries, so a legacy spec round-trips unchanged. "Foreign" carries the catalog
    // agent name BY NAME; the agent's protocol (ACP vs stream-json) is a catalog-entry property.
    if (ps.profile_spec_engine_present) {
        const engine_selector_r& engine = ps.profile_spec_engine.profile_spec_engine;
        if (engine.engine_selector_choice ==
            engine_selector_r::engine_selector_engine_foreign_m_c) {
            out.engineKind = QStringLiteral("Foreign");
            out.engineForeignAgent =
                fromZcbor(engine.engine_selector_engine_foreign_m.engine_foreign_Foreign
                              .engine_foreign_agent_agent);
        }
    }
    // The Foreign-engine model backend (wire v30): only meaningful for a Foreign engine; absent on
    // a pre-v30 encoding (the node defaults it to AgentNative{model:null}).
    if (ps.profile_spec_foreign_backend_present) {
        out.hasForeignBackend = true;
        out.foreignBackend =
            decodeForeignBackend(ps.profile_spec_foreign_backend.profile_spec_foreign_backend);
    }
    // Provenance (wire v31): created_by (author / null) + owner (tstr / null).
    if (ps.profile_spec_created_by_present &&
        ps.profile_spec_created_by.profile_spec_created_by_choice ==
            profile_spec_created_by_r::profile_spec_created_by_author_m_c) {
        out.hasCreatedBy = true;
        out.createdBy = authorToString(ps.profile_spec_created_by.profile_spec_created_by_author_m,
                                       &out.createdByIsAgent);
    }
    if (ps.profile_spec_owner_present && ps.profile_spec_owner.profile_spec_owner_choice ==
                                             profile_spec_owner_r::profile_spec_owner_tstr_c) {
        out.hasOwner = true;
        out.owner = fromZcbor(ps.profile_spec_owner.profile_spec_owner_tstr);
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
        if (call.tool_call_view_detail_present &&
            call.tool_call_view_detail.tool_call_view_detail_choice ==
                tool_call_view_detail_r::tool_call_view_detail_tool_detail_m_c) {
            const tool_detail& d = call.tool_call_view_detail.tool_call_view_detail_tool_detail_m;
            out.detailKind = fromZcbor(d.tool_detail_kind);
            out.detailBody = bytesFromZcbor(d.tool_detail_body);
        }
        break;
    }
    case agent_event_r::agent_event_tool_finished_m_c: {
        out.kind = AgentEventKind::ToolFinished;
        out.seq = ev.agent_event_tool_finished_m.ToolFinished_seq;
        const tool_result_view& result = ev.agent_event_tool_finished_m.ToolFinished_result;
        out.callId = fromZcbor(result.tool_result_view_call_id);
        out.toolOk = result.tool_result_view_ok;
        out.toolSummary = fromZcbor(result.tool_result_view_summary);
        if (result.tool_result_view_detail_present &&
            result.tool_result_view_detail.tool_result_view_detail_choice ==
                tool_result_view_detail_r::tool_result_view_detail_tool_detail_m_c) {
            const tool_detail& d =
                result.tool_result_view_detail.tool_result_view_detail_tool_detail_m;
            out.detailKind = fromZcbor(d.tool_detail_kind);
            out.detailBody = bytesFromZcbor(d.tool_detail_body);
        }
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
            context_status::context_status_max_tokens_uint64_m_c) {
            out.hasContextMax = true;
            out.contextMax = ctx.context_status_max_tokens_uint64_m;
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
        // [waveB:app-v30] C6: a foreign-agent failure (optional). Decode the stage + optional agent
        // so the turn engine can compose stage-specific error copy.
        if (summary.turn_summary_failure_present &&
            summary.turn_summary_failure.turn_summary_failure_choice ==
                turn_summary_failure_r::turn_summary_failure_foreign_failure_m_c) {
            const foreign_failure& ff =
                summary.turn_summary_failure.turn_summary_failure_foreign_failure_m;
            out.hasFailure = true;
            switch (ff.foreign_failure_stage.foreign_stage_choice) {
            case foreign_stage_r::foreign_stage_Spawn_tstr_c:
                out.failureStage = QStringLiteral("Spawn");
                break;
            case foreign_stage_r::foreign_stage_Handshake_tstr_c:
                out.failureStage = QStringLiteral("Handshake");
                break;
            case foreign_stage_r::foreign_stage_Turn_tstr_c:
                out.failureStage = QStringLiteral("Turn");
                break;
            case foreign_stage_r::foreign_stage_Unknown_tstr_c:
            default:
                out.failureStage = QStringLiteral("Unknown");
                break;
            }
            if (ff.foreign_failure_agent_present &&
                ff.foreign_failure_agent.foreign_failure_agent_choice ==
                    foreign_failure_agent_r::foreign_failure_agent_tstr_c) {
                out.failureAgent = fromZcbor(ff.foreign_failure_agent.foreign_failure_agent_tstr);
            }
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
    case host_request_kind_t_r::host_request_kind_t_host_request_kind_approval_m_c: {
        out->hostKind = QStringLiteral("Approval");
        const host_request_kind_approval& approval =
            kind.host_request_kind_t_host_request_kind_approval_m;
        out->hostPrompt = fromZcbor(approval.Approval_prompt);
        out->hostAllowPermanentOffered =
            approval.Approval_allow_permanent_offered_present &&
            approval.Approval_allow_permanent_offered.Approval_allow_permanent_offered;
        break;
    }
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
        // D1: the durable journal retains the tool's typed detail (same shape as the live
        // ToolFinished view) - carry it so a re-baseline replay renders the same rich card.
        // (The ToolCall block's detail is just the args echo; deliberately not decoded.)
        if (t.ToolResult_detail_present &&
            t.ToolResult_detail.ToolResult_detail_choice ==
                ToolResult_detail_r::ToolResult_detail_tool_detail_m_c) {
            const tool_detail& d = t.ToolResult_detail.ToolResult_detail_tool_detail_m;
            out.detailKind = fromZcbor(d.tool_detail_kind);
            out.detailBody = bytesFromZcbor(d.tool_detail_body);
        }
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

// Encode a payload-free ApiRequest (the ~12 no-arg requests): just select the choice arm.
QByteArray encodeSimple(decltype(api_request_r::api_request_choice) choice) {
    api_request_r request{};
    request.api_request_choice = choice;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

// Encode a data-carrying ApiRequest: select the choice arm, let `fill` populate the body, encode.
// `fill` runs synchronously before encodeRequest, so the caller's local UTF-8 buffers (captured by
// reference and borrowed by the populated zcbor_strings) stay alive across the encode.
QByteArray encodeWithFill(decltype(api_request_r::api_request_choice) choice,
                          const std::function<void(api_request_r&)>& fill) {
    api_request_r request{};
    request.api_request_choice = choice;
    fill(request);
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}
// Populate a generated profile_spec from a DecodedProfileSpec (shared by ProfileCreate/Update and
// by ProfileImport's embedded Distribution.profile).
void fillProfileSpec(profile_spec& ps, const DecodedProfileSpec& s, ProfileSpecScratch& sc) {
    const auto setZ = [](zcbor_string& z, const QByteArray& b) {
        z.value = reinterpret_cast<const uint8_t*>(b.constData());
        z.len = static_cast<size_t>(b.size());
    };
    sc.id = s.id.toUtf8();
    sc.model = s.model.toUtf8();
    sc.baseUrl = s.baseUrl.toUtf8();
    sc.credRef = s.credentialRef.toUtf8();
    sc.fbCredRef = s.fallbackCredentialRef.toUtf8();
    sc.toolBufs.clear();
    for (const QString& t : s.toolAllowlist) {
        sc.toolBufs.append(t.toUtf8());
    }
    sc.boundTransports.clear();
    sc.boundCredRefs.clear();
    for (const DecodedBoundAccount& b : s.boundAccounts) {
        sc.boundTransports.append(b.transportInstance.toUtf8());
        sc.boundCredRefs.append(b.credentialRef.toUtf8());
    }
    const QByteArray& id = sc.id;
    const QByteArray& model = sc.model;
    const QByteArray& baseUrl = sc.baseUrl;
    const QByteArray& credRef = sc.credRef;
    const QByteArray& fbCredRef = sc.fbCredRef;
    const QList<QByteArray>& toolBufs = sc.toolBufs;
    const QList<QByteArray>& boundTransports = sc.boundTransports;
    const QList<QByteArray>& boundCredRefs = sc.boundCredRefs;

    setZ(ps.profile_spec_id, id);
    // Single source of truth for the wire mapping (incl. daemon_api); unknown strings fall through
    // to the mock choice inside providerChoice().
    ps.profile_spec_provider.provider_selector_choice =
        static_cast<decltype(ps.profile_spec_provider.provider_selector_choice)>(
            providerChoice(s.provider));
    setZ(ps.profile_spec_model, model);
    if (s.hasBaseUrl) {
        ps.profile_spec_base_url_choice = profile_spec::profile_spec_base_url_tstr_c;
        setZ(ps.profile_spec_base_url_tstr, baseUrl);
    } else {
        ps.profile_spec_base_url_choice = profile_spec::profile_spec_base_url_null_m_c;
    }
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
        ps.profile_spec_budget.budget_tokens_choice = budget::budget_tokens_uint64_m_c;
        ps.profile_spec_budget.budget_tokens_uint64_m = s.budgetTokens;
    } else {
        ps.profile_spec_budget.budget_tokens_choice = budget::budget_tokens_null_m_c;
    }
    if (s.hasBudgetWallMs) {
        ps.profile_spec_budget.budget_wall_ms_choice = budget::budget_wall_ms_uint64_m_c;
        ps.profile_spec_budget.budget_wall_ms_uint64_m = s.budgetWallMs;
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
    const size_t bn = qMin<size_t>(static_cast<size_t>(boundTransports.size()), 16);
    ps.profile_spec_bound_accounts_bound_account_m_count = bn;
    for (size_t i = 0; i < bn; ++i) {
        setZ(ps.profile_spec_bound_accounts_bound_account_m[i].bound_account_transport_instance,
             boundTransports[static_cast<int>(i)]);
        setZ(ps.profile_spec_bound_accounts_bound_account_m[i].bound_account_credential_ref,
             boundCredRefs[static_cast<int>(i)]);
    }
    // The engine selector (wire v29): always emitted explicitly (matching the Rust side, which
    // has no skip_serializing_if), "Core" unless the spec binds a foreign agent BY NAME.
    sc.engineAgent = s.engineForeignAgent.toUtf8();
    ps.profile_spec_engine_present = true;
    engine_selector_r& engine = ps.profile_spec_engine.profile_spec_engine;
    if (s.engineKind == QStringLiteral("Foreign")) {
        engine.engine_selector_choice = engine_selector_r::engine_selector_engine_foreign_m_c;
        setZ(engine.engine_selector_engine_foreign_m.engine_foreign_Foreign
                 .engine_foreign_agent_agent,
             sc.engineAgent);
    } else {
        engine.engine_selector_choice = engine_selector_r::engine_selector_Core_tstr_c;
    }
    // The Foreign-engine model backend (wire v30, optional): emitted only when the spec carries one
    // (a Core profile or a pre-v30 edit leaves it absent, matching the node's serde skip).
    ps.profile_spec_foreign_backend_present = s.hasForeignBackend;
    if (s.hasForeignBackend) {
        sc.fbAgentModel = s.foreignBackend.agentNativeModel.toUtf8();
        sc.fbNodeModel = s.foreignBackend.nodeModel.toUtf8();
        sc.fbNodeCredRef = s.foreignBackend.nodeCredentialRef.toUtf8();
        foreign_backend_r& fb = ps.profile_spec_foreign_backend.profile_spec_foreign_backend;
        if (s.foreignBackend.kind == QStringLiteral("NodeProvider")) {
            fb.foreign_backend_choice = foreign_backend_r::foreign_backend_node_provider_m_c;
            foreign_backend_node_provider& np = fb.foreign_backend_node_provider_m;
            np.NodeProvider_provider.provider_selector_choice =
                static_cast<decltype(np.NodeProvider_provider.provider_selector_choice)>(
                    providerChoice(s.foreignBackend.nodeProvider));
            setZ(np.NodeProvider_model, sc.fbNodeModel);
            if (s.foreignBackend.hasNodeCredentialRef) {
                np.NodeProvider_credential_ref_choice =
                    foreign_backend_node_provider::NodeProvider_credential_ref_tstr_c;
                setZ(np.NodeProvider_credential_ref_tstr, sc.fbNodeCredRef);
            } else {
                np.NodeProvider_credential_ref_choice =
                    foreign_backend_node_provider::NodeProvider_credential_ref_null_m_c;
            }
        } else {
            fb.foreign_backend_choice = foreign_backend_r::foreign_backend_agent_native_m_c;
            foreign_backend_agent_native& an = fb.foreign_backend_agent_native_m;
            if (s.foreignBackend.hasAgentNativeModel) {
                an.AgentNative_model_choice =
                    foreign_backend_agent_native::AgentNative_model_tstr_c;
                setZ(an.AgentNative_model_tstr, sc.fbAgentModel);
            } else {
                an.AgentNative_model_choice =
                    foreign_backend_agent_native::AgentNative_model_null_m_c;
            }
        }
    }
    // Provenance (wire v31, optional): created_by (author / null) + owner (tstr / null). Emitted
    // only when the spec carries them, so an operator-authored round-trip stays byte-identical.
    ps.profile_spec_created_by_present = s.hasCreatedBy;
    if (s.hasCreatedBy) {
        profile_spec_created_by_r& cb = ps.profile_spec_created_by;
        cb.profile_spec_created_by_choice =
            profile_spec_created_by_r::profile_spec_created_by_author_m_c;
        author_r& author = cb.profile_spec_created_by_author_m;
        if (s.createdByIsAgent) {
            sc.createdByAgent = s.createdBy.toUtf8();
            author.author_choice = author_r::author_agent_m_c;
            setZ(author.author_agent_m.author_agent_agent, sc.createdByAgent);
        } else {
            author.author_choice = author_r::author_operator_tstr_c;
        }
    }
    ps.profile_spec_owner_present = s.hasOwner;
    if (s.hasOwner) {
        sc.owner = s.owner.toUtf8();
        ps.profile_spec_owner.profile_spec_owner_choice =
            profile_spec_owner_r::profile_spec_owner_tstr_c;
        setZ(ps.profile_spec_owner.profile_spec_owner_tstr, sc.owner);
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

// Decode a full ApiResponse and require it to be the `expected` arm. Returns the heap-owned
// response (the generated struct is large - keep it off the stack) or nullptr on a decode miss /
// arm mismatch. Collapses the ~39 identical decoder preambles.
std::unique_ptr<api_response_r>
decodeChecked(const QByteArray& responseCbor,
              decltype(api_response_r::api_response_choice) expected) {
    auto response = std::make_unique<api_response_r>();
    if (!decodeResponse(responseCbor, response.get()) ||
        response->api_response_choice != expected) {
        return nullptr;
    }
    return response;
}

} // namespace daemonapp::daemon::codec_detail
