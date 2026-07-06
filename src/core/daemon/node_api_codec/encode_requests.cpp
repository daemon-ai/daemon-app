// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// NodeApiCodec request encoders: the ApiRequests the client issues (chat/session, model, profile,
// credentials, HITL, commands). The trivial + data-carrying shapes route through the shared
// encodeSimple / encodeWithFill helpers; the irreducible profile-spec fill stays in mappers.cpp.

#include "daemon/node_api_codec/internal.h"

namespace daemonapp::daemon {

using namespace codec_detail;
QByteArray NodeApiCodec::encodeHealthRequest() {
    return encodeSimple(api_request_r::api_request_request_health_m_c);
}

QByteArray NodeApiCodec::encodeSessionsQueryRequest(bool hasSinceRev, quint64 sinceRev,
                                                    const QString& byProfile, const QString& after,
                                                    bool archivedScope) {
    // `byProfile`/`after` must outlive the encode: zcbor holds pointers into their UTF-8 bytes.
    const QByteArray profileUtf8 = byProfile.toUtf8();
    const QByteArray afterUtf8 = after.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_sessions_query_m_c;
    // Leave limit absent: an empty session-query map asks the daemon for its default (TopLevel)
    // scope, first page. `since_rev` (when set) makes it an L4 delta read. A non-empty `byProfile`
    // sets scope = ByProfile(id) — the per-agent view; `archivedScope` sets scope = Archived
    // (archived primaries, F6) — both encoder-only (the arms are already in the CDDL session-scope
    // union). `after` (when set) resumes past the previous page's next_cursor (the wire v24 roster
    // page loop).
    request.api_request_request_sessions_query_m = request_sessions_query{};
    session_query& q = request.api_request_request_sessions_query_m.SessionsQuery_query;
    q.session_query_scope_present = !byProfile.isEmpty() || archivedScope;
    if (!byProfile.isEmpty()) {
        session_scope_r& scope = q.session_query_scope.session_query_scope;
        scope.session_scope_choice = session_scope_r::session_scope_by_profile_m_c;
        scope.session_scope_by_profile_m.session_scope_by_profile_ByProfile.value =
            reinterpret_cast<const uint8_t*>(profileUtf8.constData());
        scope.session_scope_by_profile_m.session_scope_by_profile_ByProfile.len =
            static_cast<size_t>(profileUtf8.size());
    } else if (archivedScope) {
        q.session_query_scope.session_query_scope.session_scope_choice =
            session_scope_r::session_scope_Archived_tstr_c;
    }
    q.session_query_after_present = !after.isEmpty();
    if (!after.isEmpty()) {
        q.session_query_after.session_query_after_choice =
            session_query_after_r::session_query_after_session_id_m_c;
        q.session_query_after.session_query_after_session_id_m.value =
            reinterpret_cast<const uint8_t*>(afterUtf8.constData());
        q.session_query_after.session_query_after_session_id_m.len =
            static_cast<size_t>(afterUtf8.size());
    }
    q.session_query_since_rev_present = hasSinceRev;
    if (hasSinceRev) {
        q.session_query_since_rev.session_query_since_rev_choice =
            session_query_since_rev_r::session_query_since_rev_uint64_m_c;
        q.session_query_since_rev.session_query_since_rev_uint64_m =
            static_cast<uint64_t>(sinceRev);
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

QByteArray NodeApiCodec::encodeSessionCreateRequest(const QString& sessionId,
                                                    const QString& profile) {
    // Both fields outlive the encode: zcbor holds pointers into their UTF-8 bytes.
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    const QByteArray profileUtf8 = profile.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_session_create_m_c, [&](api_request_r& request) {
            request_session_create& create = request.api_request_request_session_create_m;
            // Absent `session` => the node MINTS the id (node-authority; nothing client-minted).
            if (sessionId.isEmpty()) {
                create.SessionCreate_session_present = false;
            } else {
                create.SessionCreate_session_present = true;
                create.SessionCreate_session.SessionCreate_session_choice =
                    SessionCreate_session_r::SessionCreate_session_session_id_m_c;
                setZcbor(create.SessionCreate_session.SessionCreate_session_session_id_m,
                         sessionUtf8);
            }
            // Absent `profile` => the node binds its active default.
            if (profile.isEmpty()) {
                create.SessionCreate_profile_present = false;
            } else {
                create.SessionCreate_profile_present = true;
                create.SessionCreate_profile.SessionCreate_profile_choice =
                    SessionCreate_profile_r::SessionCreate_profile_profile_ref_m_c;
                setZcbor(create.SessionCreate_profile.SessionCreate_profile_profile_ref_m,
                         profileUtf8);
            }
        });
}

QByteArray NodeApiCodec::encodeSessionUpdateMetaRequest(const QString& sessionId,
                                                        std::optional<bool> pinned,
                                                        std::optional<bool> archived,
                                                        std::optional<QString> title) {
    // Buffers outlive the encode: zcbor borrows into their bytes across encodeWithFill.
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    const QByteArray titleUtf8 = title.has_value() ? title->toUtf8() : QByteArray();
    return encodeWithFill(
        api_request_r::api_request_request_session_update_meta_m_c, [&](api_request_r& request) {
            request_session_update_meta& meta = request.api_request_request_session_update_meta_m;
            setZcbor(meta.SessionUpdateMeta_session, sessionUtf8);
            session_meta_patch& patch = meta.SessionUpdateMeta_patch;
            // Each optional-and-nullable field: present == the key is in the map; we only ever
            // send the value arm (never the wire `null` clear arm).
            patch.session_meta_patch_title_present = title.has_value();
            if (title.has_value()) {
                patch.session_meta_patch_title.session_meta_patch_title_choice =
                    session_meta_patch_title_r::session_meta_patch_title_tstr_c;
                setZcbor(patch.session_meta_patch_title.session_meta_patch_title_tstr, titleUtf8);
            }
            patch.session_meta_patch_pinned_present = pinned.has_value();
            if (pinned.has_value()) {
                patch.session_meta_patch_pinned.session_meta_patch_pinned_choice =
                    session_meta_patch_pinned_r::session_meta_patch_pinned_bool_c;
                patch.session_meta_patch_pinned.session_meta_patch_pinned_bool = *pinned;
            }
            patch.session_meta_patch_archived_present = archived.has_value();
            if (archived.has_value()) {
                patch.session_meta_patch_archived.session_meta_patch_archived_choice =
                    session_meta_patch_archived_r::session_meta_patch_archived_bool_c;
                patch.session_meta_patch_archived.session_meta_patch_archived_bool = *archived;
            }
        });
}

QByteArray NodeApiCodec::encodeCredentialSetRequest(const QString& profile, const QString& secret) {
    const QByteArray profileUtf8 = profile.toUtf8();
    const QByteArray secretUtf8 = secret.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_credential_set_m_c, [&](api_request_r& request) {
            request_credential_set& set = request.api_request_request_credential_set_m;
            setZcbor(set.CredentialSet_profile, profileUtf8);
            setZcbor(set.CredentialSet_secret, secretUtf8);
        });
}

QByteArray NodeApiCodec::encodeCredentialListRequest() {
    return encodeSimple(api_request_r::api_request_request_credential_list_m_c);
}

QByteArray NodeApiCodec::encodeCredentialRemoveRequest(const QString& profile) {
    const QByteArray profileUtf8 = profile.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_credential_remove_m_c, [&](api_request_r& request) {
            setZcbor(request.api_request_request_credential_remove_m.CredentialRemove_profile,
                     profileUtf8);
        });
}

QByteArray NodeApiCodec::encodeModelsRequest(const QString& after) {
    const QByteArray afterUtf8 = after.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_models_m_c, [&](api_request_r& request) {
            request_models& m = request.api_request_request_models_m;
            m.Models_after_present = !after.isEmpty();
            if (!after.isEmpty()) {
                m.Models_after.Models_after_choice = Models_after_r::Models_after_tstr_c;
                setZcbor(m.Models_after.Models_after_tstr, afterUtf8);
            }
        });
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

QByteArray NodeApiCodec::encodeProviderCatalogRequest() {
    return encodeSimple(api_request_r::api_request_request_provider_catalog_m_c);
}

QByteArray NodeApiCodec::encodeAcpCatalogRequest() {
    return encodeSimple(api_request_r::api_request_request_acp_catalog_m_c);
}

QByteArray NodeApiCodec::encodeProviderModelsRequest(const QString& provider,
                                                     const QString& credentialRef,
                                                     const QString& transientKey,
                                                     const QString& after) {
    const QByteArray providerUtf8 = provider.toUtf8();
    const QByteArray credRefUtf8 = credentialRef.toUtf8();
    const QByteArray keyUtf8 = transientKey.toUtf8();
    const QByteArray afterUtf8 = after.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_provider_models_m_c, [&](api_request_r& request) {
            request_provider_models& pm = request.api_request_request_provider_models_m;
            setZcbor(pm.ProviderModels_provider, providerUtf8);
            if (credentialRef.isEmpty()) {
                pm.ProviderModels_credential_ref_present = false;
            } else {
                pm.ProviderModels_credential_ref_present = true;
                pm.ProviderModels_credential_ref.ProviderModels_credential_ref_choice =
                    ProviderModels_credential_ref_r::ProviderModels_credential_ref_tstr_c;
                setZcbor(pm.ProviderModels_credential_ref.ProviderModels_credential_ref_tstr,
                         credRefUtf8);
            }
            if (transientKey.isEmpty()) {
                pm.ProviderModels_transient_key_present = false;
            } else {
                pm.ProviderModels_transient_key_present = true;
                pm.ProviderModels_transient_key.ProviderModels_transient_key_choice =
                    ProviderModels_transient_key_r::ProviderModels_transient_key_tstr_c;
                setZcbor(pm.ProviderModels_transient_key.ProviderModels_transient_key_tstr,
                         keyUtf8);
            }
            pm.ProviderModels_after_present = !after.isEmpty();
            if (!after.isEmpty()) {
                pm.ProviderModels_after.ProviderModels_after_choice =
                    ProviderModels_after_r::ProviderModels_after_tstr_c;
                setZcbor(pm.ProviderModels_after.ProviderModels_after_tstr, afterUtf8);
            }
        });
}

QByteArray NodeApiCodec::encodeProfileListRequest() {
    return encodeSimple(api_request_r::api_request_request_profile_list_m_c);
}

QByteArray NodeApiCodec::encodeProfileSelectRequest(const QString& id) {
    const QByteArray idUtf8 = id.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_profile_select_m_c, [&](api_request_r& request) {
            setZcbor(request.api_request_request_profile_select_m.ProfileSelect_id, idUtf8);
        });
}

QByteArray NodeApiCodec::encodeProfileDeleteRequest(const QString& id) {
    const QByteArray idUtf8 = id.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_profile_delete_m_c, [&](api_request_r& request) {
            setZcbor(request.api_request_request_profile_delete_m.ProfileDelete_id, idUtf8);
        });
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
    return encodeWithFill(
        api_request_r::api_request_request_profile_clone_m_c, [&](api_request_r& request) {
            request_profile_clone& clone = request.api_request_request_profile_clone_m;
            setZcbor(clone.ProfileClone_source, sourceUtf8);
            setZcbor(clone.ProfileClone_new_id, newIdUtf8);
        });
}

QByteArray NodeApiCodec::encodeProfileGetRequest(const QString& id) {
    const QByteArray idUtf8 = id.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_profile_get_m_c, [&](api_request_r& request) {
            setZcbor(request.api_request_request_profile_get_m.ProfileGet_id, idUtf8);
        });
}

QByteArray NodeApiCodec::encodeProfileExportRequest(const QString& id) {
    const QByteArray idUtf8 = id.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_profile_export_m_c, [&](api_request_r& request) {
            setZcbor(request.api_request_request_profile_export_m.ProfileExport_id, idUtf8);
        });
}

QByteArray NodeApiCodec::encodeProfileHistoryRequest(const QString& id, const QString& after) {
    const QByteArray idUtf8 = id.toUtf8();
    const QByteArray afterUtf8 = after.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_profile_history_m_c, [&](api_request_r& request) {
            request_profile_history& h = request.api_request_request_profile_history_m;
            setZcbor(h.ProfileHistory_id, idUtf8);
            h.ProfileHistory_after_present = !after.isEmpty();
            if (!after.isEmpty()) {
                h.ProfileHistory_after.ProfileHistory_after_choice =
                    ProfileHistory_after_r::ProfileHistory_after_tstr_c;
                setZcbor(h.ProfileHistory_after.ProfileHistory_after_tstr, afterUtf8);
            }
        });
}

QByteArray NodeApiCodec::encodeProfileAtRequest(const QString& id, quint64 seq) {
    const QByteArray idUtf8 = id.toUtf8();
    return encodeWithFill(api_request_r::api_request_request_profile_at_m_c,
                          [&](api_request_r& request) {
                              auto& at = request.api_request_request_profile_at_m;
                              setZcbor(at.ProfileAt_id, idUtf8);
                              at.ProfileAt_seq = static_cast<uint32_t>(seq);
                          });
}

QByteArray NodeApiCodec::encodeProfileRevertRequest(const QString& id, quint64 seq) {
    const QByteArray idUtf8 = id.toUtf8();
    return encodeWithFill(api_request_r::api_request_request_profile_revert_m_c,
                          [&](api_request_r& request) {
                              auto& rev = request.api_request_request_profile_revert_m;
                              setZcbor(rev.ProfileRevert_id, idUtf8);
                              rev.ProfileRevert_seq = static_cast<uint32_t>(seq);
                          });
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
                                                      bool allow, bool allowPermanent) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    api_request_r request{};
    fillRespondShell(&request, sessionUtf8, requestId);
    host_response_body_t_r& body =
        request.api_request_request_respond_m.Respond_response.host_response_body;
    body.host_response_body_t_choice =
        host_response_body_t_r::host_response_body_t_host_response_body_approved_m_c;
    host_response_body_approved& approved = body.host_response_body_t_host_response_body_approved_m;
    approved.Approved_approved = allow;
    // `allow_permanent` is an optional wire field (wire v28): emit it only for an ALLOW that
    // chose "allow permanently" on an offered gate. A deny is never persisted, and a plain approve
    // stays byte-identical to the pre-v28 shape (absent == not permanent).
    const bool permanent = allow && allowPermanent;
    approved.Approved_allow_permanent_present = permanent;
    approved.Approved_allow_permanent.Approved_allow_permanent = permanent;
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
                                                 const QString& revision, const QString& after) {
    const QByteArray repoUtf8 = repo.toUtf8();
    const QByteArray revUtf8 = revision.toUtf8();
    const QByteArray afterUtf8 = after.toUtf8();
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
    f.ModelFiles_after_present = !after.isEmpty();
    if (!after.isEmpty()) {
        f.ModelFiles_after.ModelFiles_after_choice = ModelFiles_after_r::ModelFiles_after_tstr_c;
        setZcbor(f.ModelFiles_after.ModelFiles_after_tstr, afterUtf8);
    }
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
    model_recommend_args& r =
        request.api_request_request_model_recommend_m.request_model_recommend_ModelRecommend;
    r.model_recommend_args_repo.value = reinterpret_cast<const uint8_t*>(repoUtf8.constData());
    r.model_recommend_args_repo.len = static_cast<size_t>(repoUtf8.size());
    if (revision.isEmpty()) {
        r.model_recommend_args_revision_choice =
            model_recommend_args::model_recommend_args_revision_null_m_c;
    } else {
        r.model_recommend_args_revision_choice =
            model_recommend_args::model_recommend_args_revision_tstr_c;
        r.model_recommend_args_revision_tstr.value =
            reinterpret_cast<const uint8_t*>(revUtf8.constData());
        r.model_recommend_args_revision_tstr.len = static_cast<size_t>(revUtf8.size());
    }
    r.model_recommend_args_engine.model_engine_choice =
        static_cast<decltype(r.model_recommend_args_engine.model_engine_choice)>(
            modelEngineChoice(engine));
    if (hasBudget) {
        r.model_recommend_args_budget_bytes_choice =
            model_recommend_args::model_recommend_args_budget_bytes_uint64_m_c;
        r.model_recommend_args_budget_bytes_uint64_m = static_cast<uint64_t>(budgetBytes);
    } else {
        r.model_recommend_args_budget_bytes_choice =
            model_recommend_args::model_recommend_args_budget_bytes_null_m_c;
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeApprovalDecideRequest(const QString& sessionId,
                                                     const QString& requestId, bool allow,
                                                     bool allowPermanent) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    const QByteArray requestIdUtf8 = requestId.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_approval_decide_m_c, [&](api_request_r& request) {
            request_approval_decide& decide = request.api_request_request_approval_decide_m;
            setZcbor(decide.ApprovalDecide_session, sessionUtf8);
            setZcbor(decide.ApprovalDecide_request_id, requestIdUtf8);
            decide.ApprovalDecide_allow = allow;
            // Optional wire v28 field: emit `allow_permanent` only for an ALLOW that chose "allow
            // permanently". A deny is never persisted, and a plain approve stays byte-identical to
            // the pre-v28 shape (absent == not permanent).
            const bool permanent = allow && allowPermanent;
            decide.ApprovalDecide_allow_permanent_present = permanent;
            decide.ApprovalDecide_allow_permanent.ApprovalDecide_allow_permanent = permanent;
        });
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

QByteArray NodeApiCodec::encodeApprovalsPendingRequest(const QString& sessionId,
                                                       const QString& after) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    const QByteArray afterUtf8 = after.toUtf8();
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
    pending.ApprovalsPending_after_present = !after.isEmpty();
    if (!after.isEmpty()) {
        pending.ApprovalsPending_after.ApprovalsPending_after_choice =
            ApprovalsPending_after_r::ApprovalsPending_after_tstr_c;
        setZcbor(pending.ApprovalsPending_after.ApprovalsPending_after_tstr, afterUtf8);
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
    return encodeSimple(api_request_r::api_request_request_model_downloads_m_c);
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
    return encodeSimple(api_request_r::api_request_request_model_catalog_m_c);
}

QByteArray NodeApiCodec::encodeCommandListRequest() {
    return encodeSimple(api_request_r::api_request_request_command_list_m_c);
}

QByteArray NodeApiCodec::encodeModelDeleteRequest(const QString& id) {
    const QByteArray idUtf8 = id.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_model_delete_m_c, [&](api_request_r& request) {
            setZcbor(request.api_request_request_model_delete_m.ModelDelete_id, idUtf8);
        });
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
    // The contract download-id is uint64; never narrow it (a >4G job id must round-trip).
    request.api_request_request_model_cancel_m.ModelCancel_id = id;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelPauseRequest(quint64 id) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_pause_m_c;
    request.api_request_request_model_pause_m.ModelPause_id = id;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeModelResumeRequest(quint64 id) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_model_resume_m_c;
    request.api_request_request_model_resume_m.ModelResume_id = id;
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeSessionSearchRequest(const QString& query, quint32 limit) {
    const QByteArray queryUtf8 = query.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_session_search_m_c, [&](api_request_r& request) {
            request_session_search& search = request.api_request_request_session_search_m;
            setZcbor(search.SessionSearch_query, queryUtf8);
            search.SessionSearch_limit = limit;
        });
}

// --- Checkpoints (E4/TOOL-9) ---------------------------------------------------------------------

QByteArray NodeApiCodec::encodeCheckpointListRequest(const QString& sessionId,
                                                     const QString& after) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    const QByteArray afterUtf8 = after.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_checkpoint_list_m_c;
    request_checkpoint_list& list = request.api_request_request_checkpoint_list_m;
    list.CheckpointList_session_present = !sessionId.isEmpty();
    if (!sessionId.isEmpty()) {
        list.CheckpointList_session.CheckpointList_session_choice =
            CheckpointList_session_r::CheckpointList_session_session_id_m_c;
        setZcbor(list.CheckpointList_session.CheckpointList_session_session_id_m, sessionUtf8);
    }
    list.CheckpointList_after_present = !after.isEmpty();
    if (!after.isEmpty()) {
        list.CheckpointList_after.CheckpointList_after_choice =
            CheckpointList_after_r::CheckpointList_after_tstr_c;
        setZcbor(list.CheckpointList_after.CheckpointList_after_tstr, afterUtf8);
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeCheckpointRewindRequest(const QString& sessionId,
                                                       const QString& checkpointId) {
    const QByteArray sessionUtf8 = sessionId.toUtf8();
    const QByteArray idUtf8 = checkpointId.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_checkpoint_rewind_m_c, [&](api_request_r& request) {
            request_checkpoint_rewind& rewind = request.api_request_request_checkpoint_rewind_m;
            setZcbor(rewind.CheckpointRewind_session, sessionUtf8);
            setZcbor(rewind.CheckpointRewind_checkpoint_id, idUtf8);
        });
}

} // namespace daemonapp::daemon
