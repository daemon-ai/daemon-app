// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// NodeApiCodec filesystem (Phase 4), fleet/subagent-tree (Phase 5b), and channels/Events-IO (story
// 04) surfaces: their request encoders + response decoders. The tree flatten DFS + the
// connection/presence/conversation enum naming stay explicit here.

#include "daemon/node_api_codec/internal.h"

#include <QHash>
#include <QSet>

namespace daemonapp::daemon {

using namespace codec_detail;
QByteArray NodeApiCodec::encodeFsRootsRequest() {
    return encodeSimple(api_request_r::api_request_request_fs_roots_m_c);
}

QByteArray NodeApiCodec::encodeFsListRequest(const QString& rootId, const QString& dir,
                                             bool showIgnored, const QString& after) {
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
    // The resume cursor (wire v24): the previous page's `next`; omitted on the first page.
    const QByteArray afterU = after.toUtf8();
    l.FsList_after_present = !after.isEmpty();
    if (!after.isEmpty()) {
        l.FsList_after.FsList_after_choice = FsList_after_r::FsList_after_tstr_c;
        l.FsList_after.FsList_after_tstr.value =
            reinterpret_cast<const uint8_t*>(afterU.constData());
        l.FsList_after.FsList_after_tstr.len = static_cast<size_t>(afterU.size());
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
    fs_write_args& w = request.api_request_request_fs_write_m.request_fs_write_FsWrite;
    QByteArray rootScratch;
    setFsRootId(rootId, w.fs_write_args_root, rootScratch);
    const QByteArray pathU = path.toUtf8();
    w.fs_write_args_path.value = reinterpret_cast<const uint8_t*>(pathU.constData());
    w.fs_write_args_path.len = static_cast<size_t>(pathU.size());
    // Content is a CBOR bstr now (no array cap), so arbitrary file sizes round-trip.
    w.fs_write_args_bytes.value = reinterpret_cast<const uint8_t*>(bytes.constData());
    w.fs_write_args_bytes.len = static_cast<size_t>(bytes.size());
    // Optional optimistic-concurrency precondition: parse the opaque "mtime:size" etag.
    const QStringList rev = baseRevision.split(QLatin1Char(':'));
    w.fs_write_args_base_revision_present = rev.size() == 2;
    if (rev.size() == 2) {
        w.fs_write_args_base_revision.fs_write_args_base_revision_choice =
            fs_write_args_base_revision_r::fs_write_args_base_revision_fs_revision_m_c;
        // mtime_ms/size are u64 (ms epoch / file length); parse as 64-bit so a real etag is not
        // truncated.
        w.fs_write_args_base_revision.fs_write_args_base_revision_fs_revision_m
            .fs_revision_mtime_ms = rev[0].toULongLong();
        w.fs_write_args_base_revision.fs_write_args_base_revision_fs_revision_m.fs_revision_size =
            rev[1].toULongLong();
    }
    w.fs_write_args_force_present = force;
    if (force) {
        w.fs_write_args_force.fs_write_args_force = true;
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeFsWatchPollRequest(const QString& rootId, const QString& dir,
                                                  quint64 afterSeq, quint32 max) {
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_fs_watch_poll_m_c;
    fs_watch_after_args& w =
        request.api_request_request_fs_watch_poll_m.request_fs_watch_poll_FsWatchPoll;
    QByteArray rootScratch;
    setFsRootId(rootId, w.fs_watch_after_args_root, rootScratch);
    const QByteArray dirU = dir.toUtf8();
    w.fs_watch_after_args_dir.value = reinterpret_cast<const uint8_t*>(dirU.constData());
    w.fs_watch_after_args_dir.len = static_cast<size_t>(dirU.size());
    // after_seq is a u64 cursor; the regenerated member is uint64_t, so assign without truncating.
    w.fs_watch_after_args_after_seq = afterSeq;
    w.fs_watch_after_args_max = max;
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
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_fs_roots_m_c);
    if (!response) {
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

bool NodeApiCodec::decodeFsList(const QByteArray& responseCbor, QList<DecodedFsEntry>* out,
                                QString* next) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_fs_list_m_c);
    if (!response) {
        return false;
    }
    const fs_list_page& page = response->api_response_response_fs_list_m.response_fs_list_FsList;
    out->clear();
    for (size_t i = 0; i < page.fs_list_page_items_fs_entry_m_count; ++i) {
        const fs_entry& e = page.fs_list_page_items_fs_entry_m[i];
        DecodedFsEntry row;
        row.name = fromZcbor(e.fs_entry_name);
        row.path = fromZcbor(e.fs_entry_path);
        row.kind = fsEntryKindName(e.fs_entry_kind.fs_entry_kind_t_choice);
        row.size = e.fs_entry_size;
        row.mtimeMs = e.fs_entry_mtime_ms;
        row.ignored = e.fs_entry_ignored_present && e.fs_entry_ignored.fs_entry_ignored;
        out->append(row);
    }
    if (next != nullptr) {
        // The resume cursor: present + non-null => more pages remain; empty => last page.
        const bool hasNext =
            page.fs_list_page_next_present && page.fs_list_page_next.fs_list_page_next_choice ==
                                                  fs_list_page_next_r::fs_list_page_next_tstr_c;
        *next = hasNext ? fromZcbor(page.fs_list_page_next.fs_list_page_next_tstr) : QString();
    }
    return true;
}

bool NodeApiCodec::decodeFsRead(const QByteArray& responseCbor, DecodedFsContent* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_fs_read_m_c);
    if (!response) {
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
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_fs_write_m_c);
    if (!response) {
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
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_fs_watch_m_c);
    if (!response) {
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
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_fs_search_m_c);
    if (!response) {
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

QByteArray NodeApiCodec::encodeTreeRequest(const QString& after) {
    const QByteArray afterUtf8 = after.toUtf8();
    return encodeWithFill(api_request_r::api_request_request_tree_m_c, [&](api_request_r& request) {
        request_tree& t = request.api_request_request_tree_m;
        t.Tree_after_present = !after.isEmpty();
        if (!after.isEmpty()) {
            t.Tree_after.Tree_after_choice = Tree_after_r::Tree_after_tstr_c;
            setZcbor(t.Tree_after.Tree_after_tstr, afterUtf8);
        }
    });
}

QByteArray NodeApiCodec::encodeFleetRequest() {
    return encodeSimple(api_request_r::api_request_request_fleet_m_c);
}

QByteArray NodeApiCodec::encodeUnitRequest(const QString& unitId) {
    const QByteArray u = unitId.toUtf8();
    return encodeWithFill(api_request_r::api_request_request_unit_m_c, [&](api_request_r& request) {
        setZcbor(request.api_request_request_unit_m.Unit_unit, u);
    });
}

QByteArray NodeApiCodec::encodeUnitEventsRequest(const QString& unitId, quint32 max) {
    const QByteArray u = unitId.toUtf8();
    return encodeWithFill(api_request_r::api_request_request_unit_events_m_c,
                          [&](api_request_r& request) {
                              auto& ev = request.api_request_request_unit_events_m;
                              setZcbor(ev.UnitEvents_unit, u);
                              ev.UnitEvents_max = max;
                          });
}

QByteArray NodeApiCodec::encodePauseRequest(const QString& unitId) {
    const QByteArray u = unitId.toUtf8();
    return encodeWithFill(api_request_r::api_request_request_pause_m_c,
                          [&](api_request_r& request) {
                              setZcbor(request.api_request_request_pause_m.Pause_unit, u);
                          });
}

QByteArray NodeApiCodec::encodeResumeRequest(const QString& unitId) {
    const QByteArray u = unitId.toUtf8();
    return encodeWithFill(api_request_r::api_request_request_resume_m_c,
                          [&](api_request_r& request) {
                              setZcbor(request.api_request_request_resume_m.Resume_unit, u);
                          });
}

QByteArray NodeApiCodec::encodeScaleRequest(const QString& unitId, quint32 n) {
    const QByteArray u = unitId.toUtf8();
    return encodeWithFill(api_request_r::api_request_request_scale_m_c,
                          [&](api_request_r& request) {
                              setZcbor(request.api_request_request_scale_m.Scale_unit, u);
                              request.api_request_request_scale_m.Scale_n = n;
                          });
}

bool NodeApiCodec::decodeTreeReport(const QByteArray& responseCbor, QList<DecodedUnitNode>* outFlat,
                                    QString* outRoot, QString* next) {
    if (outFlat == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_tree_m_c);
    if (!response) {
        return false;
    }
    const tree_report& tr = response->api_response_response_tree_m.response_tree_Tree;
    outFlat->clear();
    for (size_t i = 0; i < tr.tree_report_nodes_unit_node_m_count; ++i) {
        outFlat->append(decodeUnitNodeStruct(tr.tree_report_nodes_unit_node_m[i]));
    }
    if (outRoot != nullptr) {
        *outRoot = tr.tree_report_root_choice == tree_report::tree_report_root_unit_id_m_c
                       ? fromZcbor(tr.tree_report_root_unit_id_m)
                       : QString();
    }
    if (next != nullptr) {
        // The resume cursor (wire v25): present + non-null => more node pages remain.
        const bool hasNext =
            tr.tree_report_next_present && tr.tree_report_next.tree_report_next_choice ==
                                               tree_report_next_r::tree_report_next_tstr_c;
        *next = hasNext ? fromZcbor(tr.tree_report_next.tree_report_next_tstr) : QString();
    }
    return true;
}

QList<DecodedUnitNode> NodeApiCodec::flattenTreeNodes(const QList<DecodedUnitNode>& nodes,
                                                      const QString& root) {
    QList<DecodedUnitNode> flat;
    if (root.isEmpty()) {
        return flat; // an empty tree (no root) is a valid, common (fresh-daemon) result
    }
    QHash<QString, DecodedUnitNode> byId;
    for (const DecodedUnitNode& n : nodes) {
        byId.insert(n.id, n);
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
        flat.append(node);
        // Push children in reverse so they pop (LIFO) in their listed order.
        for (qsizetype k = node.children.size() - 1; k >= 0; --k) {
            stack.append({node.children.at(k), f.id, f.depth + 1});
        }
    }
    return flat;
}

bool NodeApiCodec::decodeUnit(const QByteArray& responseCbor, DecodedUnitNode* out, bool* found) {
    if (out == nullptr || found == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_unit_m_c);
    if (!response) {
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
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_fleet_m_c);
    if (!response) {
        return false;
    }
    const fleet_report& fr = response->api_response_response_fleet_m.response_fleet_Fleet;
    out->children.clear();
    for (size_t i = 0; i < fr.fleet_report_children_unit_id_m_count; ++i) {
        out->children << fromZcbor(fr.fleet_report_children_unit_id_m[i]);
    }
    return true;
}

bool NodeApiCodec::decodeUnitEvents(const QByteArray& responseCbor,
                                    QList<DecodedManageEvent>* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_unit_events_m_c);
    if (!response) {
        return false;
    }
    const response_unit_events& resp = response->api_response_response_unit_events_m;
    out->clear();
    for (size_t i = 0; i < resp.response_unit_events_UnitEvents_manage_event_view_m_count; ++i) {
        const manage_event_view_r& v = resp.response_unit_events_UnitEvents_manage_event_view_m[i];
        DecodedManageEvent ev;
        switch (v.manage_event_view_choice) {
        case manage_event_view_r::manage_event_view_started_m_c:
            ev.kind = DecodedManageEvent::Kind::Started;
            ev.seq = v.manage_event_view_started_m.Started_seq;
            break;
        case manage_event_view_r::manage_event_view_progress_m_c:
            ev.kind = DecodedManageEvent::Kind::Progress;
            ev.seq = v.manage_event_view_progress_m.Progress_seq;
            break;
        case manage_event_view_r::manage_event_view_usage_m_c:
            ev.kind = DecodedManageEvent::Kind::Usage;
            ev.seq = v.manage_event_view_usage_m.Usage_seq;
            break;
        case manage_event_view_r::manage_event_view_finished_m_c:
            ev.kind = DecodedManageEvent::Kind::Finished;
            ev.seq = v.manage_event_view_finished_m.Finished_seq;
            break;
        case manage_event_view_r::manage_event_view_error_m_c:
            ev.kind = DecodedManageEvent::Kind::Error;
            ev.seq = v.manage_event_view_error_m.Error_seq;
            break;
        case manage_event_view_r::manage_event_view_subagent_m_c: {
            const manage_event_view_subagent& s = v.manage_event_view_subagent_m;
            ev.kind = DecodedManageEvent::Kind::Subagent;
            ev.seq = s.Subagent_seq;
            ev.child = fromZcbor(s.Subagent_child);
            ev.role = sessionRoleName(s.Subagent_role.session_role_choice);
            ev.phase = s.Subagent_phase.subagent_phase_choice ==
                               subagent_phase_r::subagent_phase_Finished_tstr_c
                           ? QStringLiteral("Finished")
                           : QStringLiteral("Spawned");
            ev.activeChildren = s.Subagent_active_children;
            break;
        }
        default:
            break;
        }
        out->append(ev);
    }
    return true;
}

// [wave2:app-delegation] F7/DEL-7: the node's delegation guardrail ceilings (read-only policy).
QByteArray NodeApiCodec::encodeCapsRequest() {
    return encodeSimple(api_request_r::api_request_request_caps_m_c);
}

bool NodeApiCodec::decodeCaps(const QByteArray& responseCbor, DecodedCapsReport* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_caps_m_c);
    if (!response) {
        return false;
    }
    const caps_report& c = response->api_response_response_caps_m.response_caps_Caps;
    out->orchestrateMaxDepth = c.caps_report_orchestrate_max_depth;
    out->orchestrateMaxFanout = c.caps_report_orchestrate_max_fanout;
    // The agent-created-agents guardrail ceilings (wire v31, optional): absent on a pre-v31
    // encoding, decoding as 0.
    if (c.caps_report_max_composed_profiles_present) {
        out->maxComposedProfiles =
            c.caps_report_max_composed_profiles.caps_report_max_composed_profiles;
    }
    if (c.caps_report_max_ephemeral_per_session_present) {
        out->maxEphemeralPerSession =
            c.caps_report_max_ephemeral_per_session.caps_report_max_ephemeral_per_session;
    }
    return true;
}

// --- Channels / Events-IO read surface (story 04: EIO-1/3/8/9) -----------------------------------

// [wave2:app-channels-liveness] Visibility-only: these two mappers moved out of the file-local
// anonymous namespace into codec_detail (declared in internal.h) so the TransportChanged
// node-event decode (decode_responses.cpp, B5) reuses the exact same lowercase names as the
// TransportInstances decode below. Bodies/signatures unchanged.
namespace codec_detail {

QString connectionStateName(const connection_state_r& c) {
    switch (c.connection_state_choice) {
    case connection_state_r::connection_state_Connecting_tstr_c:
        return QStringLiteral("connecting");
    case connection_state_r::connection_state_Connected_tstr_c:
        return QStringLiteral("connected");
    // [waveB:app-v30] D1: the node's transient teardown state (wire v30). Rendered verbatim; the
    // app never predicts it client-side.
    case connection_state_r::connection_state_Disconnecting_tstr_c:
        return QStringLiteral("disconnecting");
    case connection_state_r::connection_state_Error_tstr_c:
        return QStringLiteral("error");
    case connection_state_r::connection_state_Offline_tstr_c:
    default:
        return QStringLiteral("offline");
    }
}

// [waveB:app-v30] D1: DisconnectReason (wire v30, 7 arms) -> a stable coarse lowercase token. The
// client maps the token to friendly copy but NEVER keys behavior off it beyond that; the node's
// `message` is the authoritative human string and `fatal` is the only behavioral gate.
QString disconnectReasonName(const disconnect_reason_r& r) {
    switch (r.disconnect_reason_choice) {
    case disconnect_reason_r::disconnect_reason_UserRequested_tstr_c:
        return QStringLiteral("user_requested");
    case disconnect_reason_r::disconnect_reason_NetworkError_tstr_c:
        return QStringLiteral("network_error");
    case disconnect_reason_r::disconnect_reason_AuthenticationFailed_tstr_c:
        return QStringLiteral("authentication_failed");
    case disconnect_reason_r::disconnect_reason_ReplacedByOtherClient_tstr_c:
        return QStringLiteral("replaced_by_other_client");
    case disconnect_reason_r::disconnect_reason_InvalidSettings_tstr_c:
        return QStringLiteral("invalid_settings");
    case disconnect_reason_r::disconnect_reason_CertificateError_tstr_c:
        return QStringLiteral("certificate_error");
    case disconnect_reason_r::disconnect_reason_Other_tstr_c:
    default:
        return QStringLiteral("other");
    }
}

QString presenceStateName(const presence_state_r& p) {
    switch (p.presence_state_choice) {
    case presence_state_r::presence_state_Offline_tstr_c:
        return QStringLiteral("offline");
    case presence_state_r::presence_state_Available_tstr_c:
        return QStringLiteral("available");
    case presence_state_r::presence_state_Idle_tstr_c:
        return QStringLiteral("idle");
    case presence_state_r::presence_state_Away_tstr_c:
        return QStringLiteral("away");
    case presence_state_r::presence_state_Busy_tstr_c:
        return QStringLiteral("busy");
    case presence_state_r::presence_state_Unknown_tstr_c:
    default:
        return QStringLiteral("unknown");
    }
}

} // namespace codec_detail

namespace {

QString conversationTypeName(const conversation_type_r& t) {
    switch (t.conversation_type_choice) {
    case conversation_type_r::conversation_type_Dm_tstr_c:
        return QStringLiteral("dm");
    case conversation_type_r::conversation_type_GroupDm_tstr_c:
        return QStringLiteral("groupdm");
    case conversation_type_r::conversation_type_Channel_tstr_c:
        return QStringLiteral("channel");
    case conversation_type_r::conversation_type_Thread_tstr_c:
        return QStringLiteral("thread");
    case conversation_type_r::conversation_type_Unset_tstr_c:
    default:
        return QStringLiteral("unset");
    }
}

// [acct-mgmt] MemberRole <-> wire string. The GUI Kit.Dropdown + the TUI role palette use the exact
// case ("None"/"Voice"/"HalfOp"/"Op"/"Founder"), so roundtrip is symmetric.
QString memberRoleName(const member_role_r& r) {
    switch (r.member_role_choice) {
    case member_role_r::member_role_Voice_tstr_c:
        return QStringLiteral("Voice");
    case member_role_r::member_role_HalfOp_tstr_c:
        return QStringLiteral("HalfOp");
    case member_role_r::member_role_Op_tstr_c:
        return QStringLiteral("Op");
    case member_role_r::member_role_Founder_tstr_c:
        return QStringLiteral("Founder");
    case member_role_r::member_role_None_tstr_c:
    default:
        return QStringLiteral("None");
    }
}

int memberRoleChoice(const QString& role) {
    if (role == QLatin1String("Voice")) {
        return member_role_r::member_role_Voice_tstr_c;
    }
    if (role == QLatin1String("HalfOp")) {
        return member_role_r::member_role_HalfOp_tstr_c;
    }
    if (role == QLatin1String("Op")) {
        return member_role_r::member_role_Op_tstr_c;
    }
    if (role == QLatin1String("Founder")) {
        return member_role_r::member_role_Founder_tstr_c;
    }
    return member_role_r::member_role_None_tstr_c;
}

// [acct-mgmt] Contact presence primitive -> lowercase token (distinct from the transport
// presence_state enum — contacts carry the richer presence-primitive set).
QString contactPresenceName(const presence_primitive_t_r& p) {
    switch (p.presence_primitive_t_choice) {
    case presence_primitive_t_r::presence_primitive_t_Available_tstr_c:
        return QStringLiteral("available");
    case presence_primitive_t_r::presence_primitive_t_Idle_tstr_c:
        return QStringLiteral("idle");
    case presence_primitive_t_r::presence_primitive_t_Invisible_tstr_c:
        return QStringLiteral("invisible");
    case presence_primitive_t_r::presence_primitive_t_Away_tstr_c:
        return QStringLiteral("away");
    case presence_primitive_t_r::presence_primitive_t_DoNotDisturb_tstr_c:
        return QStringLiteral("dnd");
    case presence_primitive_t_r::presence_primitive_t_Streaming_tstr_c:
        return QStringLiteral("streaming");
    case presence_primitive_t_r::presence_primitive_t_OutOfOffice_tstr_c:
        return QStringLiteral("out_of_office");
    case presence_primitive_t_r::presence_primitive_t_Offline_tstr_c:
    default:
        return QStringLiteral("offline");
    }
}

QString contactPermissionName(const contact_permission_r& p) {
    switch (p.contact_permission_choice) {
    case contact_permission_r::contact_permission_Allow_tstr_c:
        return QStringLiteral("allow");
    case contact_permission_r::contact_permission_Deny_tstr_c:
        return QStringLiteral("deny");
    case contact_permission_r::contact_permission_Unset_tstr_c:
    default:
        return QStringLiteral("unset");
    }
}

QString typingStateName(const typing_state_r& t) {
    switch (t.typing_state_choice) {
    case typing_state_r::typing_state_Typing_tstr_c:
        return QStringLiteral("typing");
    case typing_state_r::typing_state_Paused_tstr_c:
        return QStringLiteral("paused");
    case typing_state_r::typing_state_None_tstr_c:
    default:
        return QStringLiteral("none");
    }
}

// [acct-mgmt] Project one AccountSettingsSchema into the flattened field list.
QList<DecodedSettingsField> decodeSettingsSchema(const account_settings_schema& s) {
    QList<DecodedSettingsField> out;
    if (!s.account_settings_schema_fields_present) {
        return out;
    }
    const account_settings_schema_fields_r& f = s.account_settings_schema_fields;
    for (size_t i = 0; i < f.account_settings_schema_fields_auth_param_field_m_count; ++i) {
        const auth_param_field& fld = f.account_settings_schema_fields_auth_param_field_m[i];
        DecodedSettingsField d;
        d.key = fromZcbor(fld.auth_param_field_key);
        d.label = fromZcbor(fld.auth_param_field_label);
        d.required = fld.auth_param_field_required;
        out.append(d);
    }
    return out;
}

// [acct-mgmt] Project one ConversationMember into the display struct.
DecodedConversationMember decodeMemberStruct(const conversation_member& m) {
    DecodedConversationMember d;
    const contact_info& c = m.conversation_member_contact;
    d.contactId = fromZcbor(c.contact_info_id);
    if (c.contact_info_display_name_present &&
        c.contact_info_display_name.contact_info_display_name_choice ==
            contact_info_display_name_r::contact_info_display_name_tstr_c) {
        d.displayName = fromZcbor(c.contact_info_display_name.contact_info_display_name_tstr);
    }
    if (c.contact_info_presence_present) {
        d.presence =
            contactPresenceName(c.contact_info_presence.contact_info_presence.presence_primitive);
    }
    if (c.contact_info_permission_present) {
        d.permission = contactPermissionName(c.contact_info_permission.contact_info_permission);
    }
    if (m.conversation_member_alias_present &&
        m.conversation_member_alias.conversation_member_alias_choice ==
            conversation_member_alias_r::conversation_member_alias_tstr_c) {
        d.alias = fromZcbor(m.conversation_member_alias.conversation_member_alias_tstr);
    }
    if (m.conversation_member_nickname_present &&
        m.conversation_member_nickname.conversation_member_nickname_choice ==
            conversation_member_nickname_r::conversation_member_nickname_tstr_c) {
        d.nickname = fromZcbor(m.conversation_member_nickname.conversation_member_nickname_tstr);
    }
    if (m.conversation_member_typing_present) {
        d.typing = typingStateName(m.conversation_member_typing.conversation_member_typing);
    }
    if (m.conversation_member_role_present) {
        d.role = memberRoleName(m.conversation_member_role.conversation_member_role);
    }
    if (m.conversation_member_session_present &&
        m.conversation_member_session.conversation_member_session_choice ==
            conversation_member_session_r::conversation_member_session_session_id_m_c) {
        d.session =
            fromZcbor(m.conversation_member_session.conversation_member_session_session_id_m);
    }
    return d;
}

// [acct-mgmt] Project one ConversationInfo into DecodedConversation (title/topic/description +
// optional members). Shared by decodeConversations (list) and decodeConversation (ConvGet).
DecodedConversation decodeConversationInfoStruct(const conversation_info& cv) {
    DecodedConversation d;
    d.transport = fromZcbor(cv.conversation_info_transport);
    d.id = fromZcbor(cv.conversation_info_id);
    d.kind = conversationTypeName(cv.conversation_info_kind);
    if (cv.conversation_info_title_present &&
        cv.conversation_info_title.conversation_info_title_choice ==
            conversation_info_title_r::conversation_info_title_tstr_c) {
        d.title = fromZcbor(cv.conversation_info_title.conversation_info_title_tstr);
    }
    if (cv.conversation_info_topic_present &&
        cv.conversation_info_topic.conversation_info_topic_choice ==
            conversation_info_topic_r::conversation_info_topic_tstr_c) {
        d.topic = fromZcbor(cv.conversation_info_topic.conversation_info_topic_tstr);
    }
    if (cv.conversation_info_description_present &&
        cv.conversation_info_description.conversation_info_description_choice ==
            conversation_info_description_r::conversation_info_description_tstr_c) {
        d.description =
            fromZcbor(cv.conversation_info_description.conversation_info_description_tstr);
    }
    if (cv.conversation_info_members_present) {
        d.hasMembers = true;
        const conversation_info_members_r& mm = cv.conversation_info_members;
        for (size_t i = 0; i < mm.conversation_info_members_conversation_member_m_count; ++i) {
            d.members.append(
                decodeMemberStruct(mm.conversation_info_members_conversation_member_m[i]));
        }
    }
    return d;
}

// [acct-mgmt] Fill a generated participant_r as a contact (participant_contact) carrying only the
// id. Scratch outlives the encode (zcbor borrows the id bytes).
void fillContactParticipant(participant_r& who, const QByteArray& idScratch) {
    who.participant_choice = participant_r::participant_contact_m_c;
    contact_info& c = who.participant_contact_m.participant_contact_Contact;
    setZcbor(c.contact_info_id, idScratch);
    c.contact_info_display_name_present = false;
    c.contact_info_presence_present = false;
    c.contact_info_permission_present = false;
}

// [acct-mgmt] Fill a generated account_settings_values from a key->value map. The zcbor_strings
// borrow into `scratch` (a stable list the caller keeps alive across the encode).
void fillSettingsValues(account_settings_values& out, const QMap<QString, QString>& values,
                        QList<QByteArray>& scratch) {
    if (values.isEmpty()) {
        out.account_settings_values_values_present = false;
        return;
    }
    out.account_settings_values_values_present = true;
    account_settings_values_values_r& vr = out.account_settings_values_values;
    size_t n = 0;
    for (auto it = values.constBegin(); it != values.constEnd() && n < 64; ++it, ++n) {
        scratch.append(it.key().toUtf8());
        scratch.append(it.value().toUtf8());
        values_tstrtstr& kv = vr.values_tstrtstr[n];
        setZcbor(kv.account_settings_values_values_tstrtstr_key, scratch.at(scratch.size() - 2));
        setZcbor(kv.values_tstrtstr, scratch.at(scratch.size() - 1));
    }
    vr.values_tstrtstr_count = n;
}

} // namespace

QByteArray NodeApiCodec::encodeTransportAdaptersRequest() {
    return encodeSimple(api_request_r::api_request_request_transport_adapters_m_c);
}

QByteArray NodeApiCodec::encodeTransportInstancesRequest() {
    return encodeSimple(api_request_r::api_request_request_transport_instances_m_c);
}

// [waveB:app-v30] D1: single-string transport-teardown intents (wire v30). The node owns the full
// teardown sequence; the client sends one intent and re-reads the reported state.
QByteArray NodeApiCodec::encodeTransportDisconnectRequest(const QString& transport) {
    const QByteArray t = transport.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_transport_disconnect_m_c, [&](api_request_r& request) {
            setZcbor(
                request.api_request_request_transport_disconnect_m.TransportDisconnect_transport,
                t);
        });
}

QByteArray NodeApiCodec::encodeTransportRemoveRequest(const QString& transport) {
    const QByteArray t = transport.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_transport_remove_m_c, [&](api_request_r& request) {
            setZcbor(request.api_request_request_transport_remove_m.TransportRemove_transport, t);
        });
}

QByteArray NodeApiCodec::encodeConvListRequest(const QString& transport, const QString& after) {
    const QByteArray t = transport.toUtf8();
    const QByteArray afterUtf8 = after.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_conv_list_m_c, [&](api_request_r& request) {
            request_conv_list& l = request.api_request_request_conv_list_m;
            setZcbor(l.ConvList_transport, t);
            l.ConvList_after_present = !after.isEmpty();
            if (!after.isEmpty()) {
                l.ConvList_after.ConvList_after_choice = ConvList_after_r::ConvList_after_tstr_c;
                setZcbor(l.ConvList_after.ConvList_after_tstr, afterUtf8);
            }
        });
}

bool NodeApiCodec::decodeAdapters(const QByteArray& responseCbor, QList<DecodedAdapterInfo>* out) {
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
    for (size_t i = 0; i < ra.response_adapters_Adapters_adapter_info_m_count; ++i) {
        const adapter_info& a = ra.response_adapters_Adapters_adapter_info_m[i];
        DecodedAdapterInfo d;
        d.family = fromZcbor(a.adapter_info_family);
        d.displayName = fromZcbor(a.adapter_info_display_name);
        const adapter_capabilities& c = a.adapter_info_capabilities;
        d.capabilities[QStringLiteral("rooms")] = c.adapter_capabilities_rooms;
        d.capabilities[QStringLiteral("directMessages")] = c.adapter_capabilities_direct_messages;
        d.capabilities[QStringLiteral("presence")] = c.adapter_capabilities_presence;
        d.capabilities[QStringLiteral("roomEnumeration")] = c.adapter_capabilities_room_enumeration;
        d.capabilities[QStringLiteral("fileTransfer")] = c.adapter_capabilities_file_transfer;
        d.capabilities[QStringLiteral("interactiveAuth")] = c.adapter_capabilities_interactive_auth;
        // [waveB:app-v30] D3: node-labeled policy rows (optional). Rendered verbatim; behavior is
        // never keyed off `key`.
        if (a.adapter_info_policies_present) {
            for (size_t p = 0;
                 p < a.adapter_info_policies.adapter_info_policies_policy_entry_m_count; ++p) {
                const policy_entry& pe =
                    a.adapter_info_policies.adapter_info_policies_policy_entry_m[p];
                QVariantMap row;
                row[QStringLiteral("key")] = fromZcbor(pe.policy_entry_key);
                row[QStringLiteral("label")] = fromZcbor(pe.policy_entry_label);
                row[QStringLiteral("value")] = fromZcbor(pe.policy_entry_value);
                d.policies.append(row);
            }
        }
        // [acct-mgmt] Per-verb adapter ops (wire v33, optional-nullable). Absent (a v32 node /
        // non-messaging adapter) and the null arm (an adapter without that feature) both leave
        // has*Ops false — "no per-verb info", the UI falls back to the coarse capability. A
        // concrete ops map sets has*Ops and is authoritative. Keys follow the capabilities map's
        // camelCase convention.
        if (a.adapter_info_conversation_ops_present &&
            a.adapter_info_conversation_ops.adapter_info_conversation_ops_choice ==
                adapter_info_conversation_ops_r::
                    adapter_info_conversation_ops_conversation_ops_m_c) {
            const conversation_ops& ops =
                a.adapter_info_conversation_ops.adapter_info_conversation_ops_conversation_ops_m;
            d.hasConversationOps = true;
            d.conversationOps[QStringLiteral("create")] = ops.conversation_ops_create;
            d.conversationOps[QStringLiteral("joinChannel")] = ops.conversation_ops_join_channel;
            d.conversationOps[QStringLiteral("leave")] = ops.conversation_ops_leave;
            d.conversationOps[QStringLiteral("delete")] = ops.conversation_ops_delete;
            d.conversationOps[QStringLiteral("send")] = ops.conversation_ops_send;
            d.conversationOps[QStringLiteral("setTopic")] = ops.conversation_ops_set_topic;
            d.conversationOps[QStringLiteral("setTitle")] = ops.conversation_ops_set_title;
            d.conversationOps[QStringLiteral("setDescription")] =
                ops.conversation_ops_set_description;
        }
        if (a.adapter_info_membership_ops_present &&
            a.adapter_info_membership_ops.adapter_info_membership_ops_choice ==
                adapter_info_membership_ops_r::adapter_info_membership_ops_membership_ops_m_c) {
            const membership_ops& ops =
                a.adapter_info_membership_ops.adapter_info_membership_ops_membership_ops_m;
            d.hasMembershipOps = true;
            d.membershipOps[QStringLiteral("invite")] = ops.membership_ops_invite;
            d.membershipOps[QStringLiteral("remove")] = ops.membership_ops_remove;
            d.membershipOps[QStringLiteral("ban")] = ops.membership_ops_ban;
            d.membershipOps[QStringLiteral("setRole")] = ops.membership_ops_set_role;
        }
        if (a.adapter_info_contacts_ops_present &&
            a.adapter_info_contacts_ops.adapter_info_contacts_ops_choice ==
                adapter_info_contacts_ops_r::adapter_info_contacts_ops_contacts_ops_m_c) {
            const contacts_ops& ops =
                a.adapter_info_contacts_ops.adapter_info_contacts_ops_contacts_ops_m;
            d.hasContactsOps = true;
            d.contactsOps[QStringLiteral("getProfile")] = ops.contacts_ops_get_profile;
            d.contactsOps[QStringLiteral("actionMenu")] = ops.contacts_ops_action_menu;
            d.contactsOps[QStringLiteral("setAlias")] = ops.contacts_ops_set_alias;
        }
        if (a.adapter_info_roster_ops_present &&
            a.adapter_info_roster_ops.adapter_info_roster_ops_choice ==
                adapter_info_roster_ops_r::adapter_info_roster_ops_roster_ops_m_c) {
            const roster_ops& ops = a.adapter_info_roster_ops.adapter_info_roster_ops_roster_ops_m;
            d.hasRosterOps = true;
            d.rosterOps[QStringLiteral("add")] = ops.roster_ops_add;
            d.rosterOps[QStringLiteral("update")] = ops.roster_ops_update;
            d.rosterOps[QStringLiteral("remove")] = ops.roster_ops_remove;
        }
        if (a.adapter_info_directory_present) {
            d.hasDirectory = true;
            d.directory = a.adapter_info_directory.adapter_info_directory;
        }
        out->append(d);
    }
    return true;
}

bool NodeApiCodec::decodeTransportInstances(const QByteArray& responseCbor,
                                            QList<DecodedTransportInstance>* out) {
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
    for (size_t i = 0;
         i < ri.response_transport_instances_TransportInstances_transport_instance_info_m_count;
         ++i) {
        const transport_instance_info& t =
            ri.response_transport_instances_TransportInstances_transport_instance_info_m[i];
        DecodedTransportInstance d;
        d.transport = fromZcbor(t.transport_instance_info_transport);
        d.family = fromZcbor(t.transport_instance_info_family);
        d.displayName = fromZcbor(t.transport_instance_info_display_name);
        if (t.transport_instance_info_connection_present) {
            d.connection = connectionStateName(
                t.transport_instance_info_connection.transport_instance_info_connection);
        }
        if (t.transport_instance_info_presence_present) {
            d.presence = presenceStateName(
                t.transport_instance_info_presence.transport_instance_info_presence);
        }
        if (t.transport_instance_info_bound_profile_present &&
            t.transport_instance_info_bound_profile.transport_instance_info_bound_profile_choice ==
                transport_instance_info_bound_profile_r::
                    transport_instance_info_bound_profile_profile_ref_m_c) {
            d.boundProfile = fromZcbor(t.transport_instance_info_bound_profile
                                           .transport_instance_info_bound_profile_profile_ref_m);
        }
        // [waveB:app-v30] D1: node-reported disconnect provenance (wire v30, all optional).
        if (t.transport_instance_info_reason_present &&
            t.transport_instance_info_reason.transport_instance_info_reason_choice ==
                transport_instance_info_reason_r::
                    transport_instance_info_reason_disconnect_reason_m_c) {
            d.reason =
                disconnectReasonName(t.transport_instance_info_reason
                                         .transport_instance_info_reason_disconnect_reason_m);
        }
        if (t.transport_instance_info_message_present &&
            t.transport_instance_info_message.transport_instance_info_message_choice ==
                transport_instance_info_message_r::transport_instance_info_message_tstr_c) {
            d.message =
                fromZcbor(t.transport_instance_info_message.transport_instance_info_message_tstr);
        }
        if (t.transport_instance_info_fatal_present) {
            d.fatal = t.transport_instance_info_fatal.transport_instance_info_fatal;
        }
        out->append(d);
    }
    return true;
}

bool NodeApiCodec::decodeConversations(const QByteArray& responseCbor,
                                       QList<DecodedConversation>* out, QString* next) {
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
    for (size_t i = 0; i < page.conv_page_items_conversation_info_m_count; ++i) {
        // [acct-mgmt] Use the shared projection so any members the page carries decode too.
        out->append(decodeConversationInfoStruct(page.conv_page_items_conversation_info_m[i]));
    }
    if (next != nullptr) {
        // The resume cursor (wire v25): present + non-null => more pages remain.
        const bool hasNext =
            page.conv_page_next_present &&
            page.conv_page_next.conv_page_next_choice == conv_page_next_r::conv_page_next_tstr_c;
        *next = hasNext ? fromZcbor(page.conv_page_next.conv_page_next_tstr) : QString();
    }
    return true;
}

// --- [acct-mgmt] Room lifecycle + member management (wire v32) -----------------------------------

QByteArray NodeApiCodec::encodeConvJoinDetailsRequest(const QString& transport) {
    const QByteArray t = transport.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_conv_join_details_m_c, [&](api_request_r& request) {
            setZcbor(request.api_request_request_conv_join_details_m.ConvJoinDetails_transport, t);
        });
}

QByteArray NodeApiCodec::encodeConvCreateDetailsRequest(const QString& transport) {
    const QByteArray t = transport.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_conv_create_details_m_c, [&](api_request_r& request) {
            setZcbor(request.api_request_request_conv_create_details_m.ConvCreateDetails_transport,
                     t);
        });
}

QByteArray NodeApiCodec::encodeConvJoinRequest(const QString& transport, const ConvJoinForm& form) {
    const QByteArray t = transport.toUtf8();
    const QByteArray name = form.name.toUtf8();
    const QByteArray nickname = form.nickname.toUtf8();
    const QByteArray password = form.password.toUtf8();
    QList<QByteArray> extrasScratch;
    return encodeWithFill(
        api_request_r::api_request_request_conv_join_m_c, [&](api_request_r& request) {
            request_conv_join& j = request.api_request_request_conv_join_m;
            setZcbor(j.ConvJoin_transport, t);
            channel_join_details& d = j.ConvJoin_details;
            d.channel_join_details_name_present = true;
            d.channel_join_details_name.channel_join_details_name_choice =
                channel_join_details_name_r::channel_join_details_name_tstr_c;
            setZcbor(d.channel_join_details_name.channel_join_details_name_tstr, name);
            d.channel_join_details_name_max_length_present = false;
            d.channel_join_details_nickname_present = form.hasNickname;
            if (form.hasNickname) {
                d.channel_join_details_nickname.channel_join_details_nickname_choice =
                    channel_join_details_nickname_r::channel_join_details_nickname_tstr_c;
                setZcbor(d.channel_join_details_nickname.channel_join_details_nickname_tstr,
                         nickname);
            }
            d.channel_join_details_nickname_supported_present = false;
            d.channel_join_details_nickname_max_length_present = false;
            d.channel_join_details_password_present = form.hasPassword;
            if (form.hasPassword) {
                d.channel_join_details_password.channel_join_details_password_choice =
                    channel_join_details_password_r::channel_join_details_password_tstr_c;
                setZcbor(d.channel_join_details_password.channel_join_details_password_tstr,
                         password);
            }
            d.channel_join_details_password_supported_present = false;
            d.channel_join_details_password_max_length_present = false;
            d.channel_join_details_extras_schema_present = false;
            d.channel_join_details_extras_present = !form.extras.isEmpty();
            if (!form.extras.isEmpty()) {
                fillSettingsValues(d.channel_join_details_extras.channel_join_details_extras,
                                   form.extras, extrasScratch);
            }
        });
}

QByteArray NodeApiCodec::encodeConvCreateRequest(const QString& transport,
                                                 const ConvCreateForm& form) {
    const QByteArray t = transport.toUtf8();
    QList<QByteArray> participantScratch;
    QList<QByteArray> extrasScratch;
    return encodeWithFill(
        api_request_r::api_request_request_conv_create_m_c, [&](api_request_r& request) {
            request_conv_create& c = request.api_request_request_conv_create_m;
            setZcbor(c.ConvCreate_transport, t);
            create_conversation_details& d = c.ConvCreate_details;
            d.create_conversation_details_max_participants_present = form.hasMaxParticipants;
            if (form.hasMaxParticipants) {
                d.create_conversation_details_max_participants
                    .create_conversation_details_max_participants = form.maxParticipants;
            }
            d.create_conversation_details_participants_present = !form.participants.isEmpty();
            if (!form.participants.isEmpty()) {
                create_conversation_details_participants_r& pr =
                    d.create_conversation_details_participants;
                size_t n = 0;
                for (const QString& pid : form.participants) {
                    if (n >= 64) {
                        break;
                    }
                    participantScratch.append(pid.toUtf8());
                    contact_info& ci =
                        pr.create_conversation_details_participants_contact_info_m[n];
                    setZcbor(ci.contact_info_id, participantScratch.last());
                    ci.contact_info_display_name_present = false;
                    ci.contact_info_presence_present = false;
                    ci.contact_info_permission_present = false;
                    ++n;
                }
                pr.create_conversation_details_participants_contact_info_m_count = n;
            }
            d.create_conversation_details_extras_schema_present = false;
            d.create_conversation_details_extras_present = !form.extras.isEmpty();
            if (!form.extras.isEmpty()) {
                fillSettingsValues(
                    d.create_conversation_details_extras.create_conversation_details_extras,
                    form.extras, extrasScratch);
            }
        });
}

QByteArray NodeApiCodec::encodeConvLeaveRequest(const QString& transport, const QString& conv) {
    const QByteArray t = transport.toUtf8();
    const QByteArray c = conv.toUtf8();
    return encodeWithFill(api_request_r::api_request_request_conv_leave_m_c,
                          [&](api_request_r& request) {
                              request_conv_leave& l = request.api_request_request_conv_leave_m;
                              setZcbor(l.ConvLeave_transport, t);
                              setZcbor(l.ConvLeave_conv, c);
                          });
}

QByteArray NodeApiCodec::encodeConvDeleteRequest(const QString& transport, const QString& conv) {
    const QByteArray t = transport.toUtf8();
    const QByteArray c = conv.toUtf8();
    return encodeWithFill(api_request_r::api_request_request_conv_delete_m_c,
                          [&](api_request_r& request) {
                              request_conv_delete& del = request.api_request_request_conv_delete_m;
                              setZcbor(del.ConvDelete_transport, t);
                              setZcbor(del.ConvDelete_conv, c);
                          });
}

QByteArray NodeApiCodec::encodeConvGetRequest(const QString& transport, const QString& conv) {
    const QByteArray t = transport.toUtf8();
    const QByteArray c = conv.toUtf8();
    return encodeWithFill(api_request_r::api_request_request_conv_get_m_c,
                          [&](api_request_r& request) {
                              request_conv_get& g = request.api_request_request_conv_get_m;
                              setZcbor(g.ConvGet_transport, t);
                              setZcbor(g.ConvGet_conv, c);
                          });
}

QByteArray NodeApiCodec::encodeMemberInviteRequest(const QString& transport, const QString& conv,
                                                   const QString& contactId) {
    const QByteArray t = transport.toUtf8();
    const QByteArray c = conv.toUtf8();
    const QByteArray who = contactId.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_member_invite_m_c, [&](api_request_r& request) {
            member_invite_args& a =
                request.api_request_request_member_invite_m.request_member_invite_MemberInvite;
            setZcbor(a.member_invite_args_transport, t);
            setZcbor(a.member_invite_args_conv, c);
            fillContactParticipant(a.member_invite_args_who, who);
            a.member_invite_args_message_present = false;
        });
}

QByteArray NodeApiCodec::encodeMemberRemoveRequest(const QString& transport, const QString& conv,
                                                   const QString& contactId,
                                                   const QString& reason) {
    const QByteArray t = transport.toUtf8();
    const QByteArray c = conv.toUtf8();
    const QByteArray who = contactId.toUtf8();
    const QByteArray reasonU = reason.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_member_remove_m_c, [&](api_request_r& request) {
            member_remove_args& a =
                request.api_request_request_member_remove_m.request_member_remove_MemberRemove;
            setZcbor(a.member_remove_args_transport, t);
            setZcbor(a.member_remove_args_conv, c);
            fillContactParticipant(a.member_remove_args_who, who);
            a.member_remove_args_reason_present = !reason.isEmpty();
            if (!reason.isEmpty()) {
                a.member_remove_args_reason.member_remove_args_reason_choice =
                    member_remove_args_reason_r::member_remove_args_reason_tstr_c;
                setZcbor(a.member_remove_args_reason.member_remove_args_reason_tstr, reasonU);
            }
        });
}

QByteArray NodeApiCodec::encodeMemberBanRequest(const QString& transport, const QString& conv,
                                                const QString& contactId, const QString& reason) {
    const QByteArray t = transport.toUtf8();
    const QByteArray c = conv.toUtf8();
    const QByteArray who = contactId.toUtf8();
    const QByteArray reasonU = reason.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_member_ban_m_c, [&](api_request_r& request) {
            member_ban_args& a =
                request.api_request_request_member_ban_m.request_member_ban_MemberBan;
            setZcbor(a.member_ban_args_transport, t);
            setZcbor(a.member_ban_args_conv, c);
            fillContactParticipant(a.member_ban_args_who, who);
            a.member_ban_args_reason_present = !reason.isEmpty();
            if (!reason.isEmpty()) {
                a.member_ban_args_reason.member_ban_args_reason_choice =
                    member_ban_args_reason_r::member_ban_args_reason_tstr_c;
                setZcbor(a.member_ban_args_reason.member_ban_args_reason_tstr, reasonU);
            }
        });
}

QByteArray NodeApiCodec::encodeMemberSetRoleRequest(const QString& transport, const QString& conv,
                                                    const QString& contactId, const QString& role) {
    const QByteArray t = transport.toUtf8();
    const QByteArray c = conv.toUtf8();
    const QByteArray who = contactId.toUtf8();
    return encodeWithFill(
        api_request_r::api_request_request_member_set_role_m_c, [&](api_request_r& request) {
            member_set_role_args& a =
                request.api_request_request_member_set_role_m.request_member_set_role_MemberSetRole;
            setZcbor(a.member_set_role_args_transport, t);
            setZcbor(a.member_set_role_args_conv, c);
            fillContactParticipant(a.member_set_role_args_who, who);
            a.member_set_role_args_role.member_role_choice =
                static_cast<decltype(a.member_set_role_args_role.member_role_choice)>(
                    memberRoleChoice(role));
        });
}

bool NodeApiCodec::decodeConvJoinDetails(const QByteArray& responseCbor,
                                         DecodedChannelJoinDetails* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_conv_join_details_m_c);
    if (!response) {
        return false;
    }
    const channel_join_details& d = response->api_response_response_conv_join_details_m
                                        .response_conv_join_details_ConvJoinDetails;
    *out = DecodedChannelJoinDetails{};
    if (d.channel_join_details_name_present &&
        d.channel_join_details_name.channel_join_details_name_choice ==
            channel_join_details_name_r::channel_join_details_name_tstr_c) {
        out->hasName = true;
        out->name = fromZcbor(d.channel_join_details_name.channel_join_details_name_tstr);
    }
    if (d.channel_join_details_name_max_length_present) {
        out->hasNameMaxLength = true;
        out->nameMaxLength =
            d.channel_join_details_name_max_length.channel_join_details_name_max_length;
    }
    if (d.channel_join_details_nickname_supported_present) {
        out->nicknameSupported =
            d.channel_join_details_nickname_supported.channel_join_details_nickname_supported;
    }
    if (d.channel_join_details_nickname_max_length_present) {
        out->hasNicknameMaxLength = true;
        out->nicknameMaxLength =
            d.channel_join_details_nickname_max_length.channel_join_details_nickname_max_length;
    }
    if (d.channel_join_details_password_supported_present) {
        out->passwordSupported =
            d.channel_join_details_password_supported.channel_join_details_password_supported;
    }
    if (d.channel_join_details_password_max_length_present) {
        out->hasPasswordMaxLength = true;
        out->passwordMaxLength =
            d.channel_join_details_password_max_length.channel_join_details_password_max_length;
    }
    if (d.channel_join_details_extras_schema_present) {
        out->extrasSchema = decodeSettingsSchema(
            d.channel_join_details_extras_schema.channel_join_details_extras_schema);
    }
    return true;
}

bool NodeApiCodec::decodeConvCreateDetails(const QByteArray& responseCbor,
                                           DecodedCreateConversationDetails* out) {
    if (out == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_conv_create_details_m_c);
    if (!response) {
        return false;
    }
    const create_conversation_details& d = response->api_response_response_conv_create_details_m
                                               .response_conv_create_details_ConvCreateDetails;
    *out = DecodedCreateConversationDetails{};
    if (d.create_conversation_details_max_participants_present) {
        out->hasMaxParticipants = true;
        out->maxParticipants = d.create_conversation_details_max_participants
                                   .create_conversation_details_max_participants;
    }
    if (d.create_conversation_details_extras_schema_present) {
        out->extrasSchema = decodeSettingsSchema(
            d.create_conversation_details_extras_schema.create_conversation_details_extras_schema);
    }
    return true;
}

bool NodeApiCodec::decodeConversation(const QByteArray& responseCbor, DecodedConversation* out,
                                      bool* found) {
    if (out == nullptr || found == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_conversation_m_c);
    if (!response) {
        return false;
    }
    const response_conversation& r = response->api_response_response_conversation_m;
    if (r.response_conversation_Conversation_choice ==
        response_conversation::response_conversation_Conversation_conversation_info_m_c) {
        *out =
            decodeConversationInfoStruct(r.response_conversation_Conversation_conversation_info_m);
        *found = true;
    } else {
        *found = false;
    }
    return true;
}

// --- Routing (B6/ROU: origin->session pins; wire v28) --------------------------------------------

namespace codec_detail {

void fillOrigin(origin& out, const DecodedOrigin& o, OriginScratch& sc) {
    sc.transport = o.transport.toUtf8();
    setZcbor(out.origin_transport, sc.transport);
    origin_scope_t_r& scope = out.origin_scope;
    if (o.scopeKind == QStringLiteral("dm")) {
        scope.origin_scope_t_choice = origin_scope_t_r::origin_scope_t_origin_scope_dm_m_c;
        sc.user = o.user.toUtf8();
        setZcbor(scope.origin_scope_t_origin_scope_dm_m.Dm_user, sc.user);
    } else if (o.scopeKind == QStringLiteral("group")) {
        scope.origin_scope_t_choice = origin_scope_t_r::origin_scope_t_origin_scope_group_m_c;
        origin_scope_group& g = scope.origin_scope_t_origin_scope_group_m;
        sc.chat = o.chat.toUtf8();
        setZcbor(g.Group_chat, sc.chat);
        if (o.hasThread) {
            g.Group_thread_choice = origin_scope_group::Group_thread_tstr_c;
            sc.thread = o.thread.toUtf8();
            setZcbor(g.Group_thread_tstr, sc.thread);
        } else {
            g.Group_thread_choice = origin_scope_group::Group_thread_null_m_c;
        }
    } else if (o.scopeKind == QStringLiteral("api")) {
        scope.origin_scope_t_choice = origin_scope_t_r::origin_scope_t_origin_scope_api_m_c;
        sc.apiKey = o.apiKey.toUtf8();
        setZcbor(scope.origin_scope_t_origin_scope_api_m.Api_key, sc.apiKey);
    } else {
        scope.origin_scope_t_choice = origin_scope_t_r::origin_scope_t_Internal_tstr_c;
    }
    out.origin_sender_present = false; // pins key on transport+scope; no sender attribution
}

DecodedOrigin decodeOriginStruct(const origin& o) {
    DecodedOrigin d;
    d.transport = fromZcbor(o.origin_transport);
    switch (o.origin_scope.origin_scope_t_choice) {
    case origin_scope_t_r::origin_scope_t_origin_scope_dm_m_c:
        d.scopeKind = QStringLiteral("dm");
        d.user = fromZcbor(o.origin_scope.origin_scope_t_origin_scope_dm_m.Dm_user);
        break;
    case origin_scope_t_r::origin_scope_t_origin_scope_group_m_c: {
        d.scopeKind = QStringLiteral("group");
        const origin_scope_group& g = o.origin_scope.origin_scope_t_origin_scope_group_m;
        d.chat = fromZcbor(g.Group_chat);
        if (g.Group_thread_choice == origin_scope_group::Group_thread_tstr_c) {
            d.hasThread = true;
            d.thread = fromZcbor(g.Group_thread_tstr);
        }
        break;
    }
    case origin_scope_t_r::origin_scope_t_origin_scope_api_m_c:
        d.scopeKind = QStringLiteral("api");
        d.apiKey = fromZcbor(o.origin_scope.origin_scope_t_origin_scope_api_m.Api_key);
        break;
    default:
        d.scopeKind = QStringLiteral("internal");
        break;
    }
    return d;
}

DecodedChatRoute decodeChatRouteStruct(const chat_route& r) {
    DecodedChatRoute d;
    d.origin = decodeOriginStruct(r.chat_route_origin);
    d.session = fromZcbor(r.chat_route_session);
    if (r.chat_route_profile_present &&
        r.chat_route_profile.chat_route_profile_choice ==
            chat_route_profile_r::chat_route_profile_profile_ref_m_c) {
        d.profile = fromZcbor(r.chat_route_profile.chat_route_profile_profile_ref_m);
    }
    if (r.chat_route_isolation_present) {
        switch (r.chat_route_isolation.chat_route_isolation.isolation_policy_choice) {
        case isolation_policy_r::isolation_policy_PerUser_tstr_c:
            d.isolation = QStringLiteral("PerUser");
            break;
        case isolation_policy_r::isolation_policy_PerChat_tstr_c:
            d.isolation = QStringLiteral("PerChat");
            break;
        case isolation_policy_r::isolation_policy_PerThread_tstr_c:
            d.isolation = QStringLiteral("PerThread");
            break;
        case isolation_policy_r::isolation_policy_Shared_tstr_c:
            d.isolation = QStringLiteral("Shared");
            break;
        }
    }
    return d;
}

} // namespace codec_detail

QByteArray NodeApiCodec::encodeRoutingListChatsRequest(const QString& after) {
    const QByteArray afterUtf8 = after.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_routing_list_chats_m_c;
    request_routing_list_chats& list = request.api_request_request_routing_list_chats_m;
    list.RoutingListChats_after_present = !after.isEmpty();
    if (!after.isEmpty()) {
        list.RoutingListChats_after.RoutingListChats_after_choice =
            RoutingListChats_after_r::RoutingListChats_after_tstr_c;
        setZcbor(list.RoutingListChats_after.RoutingListChats_after_tstr, afterUtf8);
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeRoutingGetRequest(const DecodedOrigin& originArg) {
    OriginScratch sc;
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_routing_get_m_c;
    fillOrigin(request.api_request_request_routing_get_m.RoutingGet_origin, originArg, sc);
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeRoutingSetRequest(const DecodedChatRoute& route) {
    OriginScratch sc;
    const QByteArray sessionUtf8 = route.session.toUtf8();
    const QByteArray profileUtf8 = route.profile.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_routing_set_m_c;
    chat_route& r = request.api_request_request_routing_set_m.RoutingSet_route;
    fillOrigin(r.chat_route_origin, route.origin, sc);
    setZcbor(r.chat_route_session, sessionUtf8);
    r.chat_route_profile_present = !route.profile.isEmpty();
    if (!route.profile.isEmpty()) {
        r.chat_route_profile.chat_route_profile_choice =
            chat_route_profile_r::chat_route_profile_profile_ref_m_c;
        setZcbor(r.chat_route_profile.chat_route_profile_profile_ref_m, profileUtf8);
    }
    r.chat_route_isolation_present = !route.isolation.isEmpty();
    if (!route.isolation.isEmpty()) {
        auto& iso = r.chat_route_isolation.chat_route_isolation;
        if (route.isolation == QStringLiteral("PerUser")) {
            iso.isolation_policy_choice = isolation_policy_r::isolation_policy_PerUser_tstr_c;
        } else if (route.isolation == QStringLiteral("PerThread")) {
            iso.isolation_policy_choice = isolation_policy_r::isolation_policy_PerThread_tstr_c;
        } else if (route.isolation == QStringLiteral("Shared")) {
            iso.isolation_policy_choice = isolation_policy_r::isolation_policy_Shared_tstr_c;
        } else {
            iso.isolation_policy_choice = isolation_policy_r::isolation_policy_PerChat_tstr_c;
        }
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeRoutingBindChatRequest(const DecodedOrigin& originArg,
                                                      const QString& session,
                                                      const QString& profile) {
    OriginScratch sc;
    const QByteArray sessionUtf8 = session.toUtf8();
    const QByteArray profileUtf8 = profile.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_routing_bind_chat_m_c;
    request_routing_bind_chat& bind = request.api_request_request_routing_bind_chat_m;
    fillOrigin(bind.RoutingBindChat_origin, originArg, sc);
    setZcbor(bind.RoutingBindChat_session, sessionUtf8);
    bind.RoutingBindChat_profile_present = !profile.isEmpty();
    if (!profile.isEmpty()) {
        bind.RoutingBindChat_profile.RoutingBindChat_profile_choice =
            RoutingBindChat_profile_r::RoutingBindChat_profile_profile_ref_m_c;
        setZcbor(bind.RoutingBindChat_profile.RoutingBindChat_profile_profile_ref_m, profileUtf8);
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeRoutingUnbindChatRequest(const DecodedOrigin& originArg) {
    OriginScratch sc;
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_routing_unbind_chat_m_c;
    fillOrigin(request.api_request_request_routing_unbind_chat_m.RoutingUnbindChat_origin,
               originArg, sc);
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

QByteArray NodeApiCodec::encodeTransportRoomsRequest(const QString& transport,
                                                     const QString& after) {
    const QByteArray transportUtf8 = transport.toUtf8();
    const QByteArray afterUtf8 = after.toUtf8();
    api_request_r request{};
    request.api_request_choice = api_request_r::api_request_request_transport_rooms_m_c;
    request_transport_rooms& rooms = request.api_request_request_transport_rooms_m;
    setZcbor(rooms.TransportRooms_transport, transportUtf8);
    rooms.TransportRooms_after_present = !after.isEmpty();
    if (!after.isEmpty()) {
        rooms.TransportRooms_after.TransportRooms_after_choice =
            TransportRooms_after_r::TransportRooms_after_tstr_c;
        setZcbor(rooms.TransportRooms_after.TransportRooms_after_tstr, afterUtf8);
    }
    QByteArray out;
    return encodeRequest(request, &out) ? out : QByteArray{};
}

bool NodeApiCodec::decodeChatRoutes(const QByteArray& responseCbor, QList<DecodedChatRoute>* out,
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
    for (size_t i = 0; i < page.chat_route_page_items_chat_route_m_count; ++i) {
        out->append(decodeChatRouteStruct(page.chat_route_page_items_chat_route_m[i]));
    }
    if (next != nullptr) {
        const bool hasNext = page.chat_route_page_next_present &&
                             page.chat_route_page_next.chat_route_page_next_choice ==
                                 chat_route_page_next_r::chat_route_page_next_tstr_c;
        *next =
            hasNext ? fromZcbor(page.chat_route_page_next.chat_route_page_next_tstr) : QString();
    }
    return true;
}

bool NodeApiCodec::decodeChatRoute(const QByteArray& responseCbor, DecodedChatRoute* out,
                                   bool* found) {
    if (out == nullptr || found == nullptr) {
        return false;
    }
    const auto response =
        decodeChecked(responseCbor, api_response_r::api_response_response_chat_route_m_c);
    if (!response) {
        return false;
    }
    const response_chat_route& r = response->api_response_response_chat_route_m;
    if (r.response_chat_route_ChatRoute_choice ==
        response_chat_route::response_chat_route_ChatRoute_chat_route_m_c) {
        *out = decodeChatRouteStruct(r.response_chat_route_ChatRoute_chat_route_m);
        *found = true;
    } else {
        *found = false;
    }
    return true;
}

bool NodeApiCodec::decodeRooms(const QByteArray& responseCbor, QList<DecodedRoomInfo>* out,
                               QString* next) {
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
    for (size_t i = 0; i < page.room_page_items_room_info_m_count; ++i) {
        const room_info& info = page.room_page_items_room_info_m[i];
        DecodedRoomInfo d;
        d.transport = fromZcbor(info.room_info_transport);
        d.room = fromZcbor(info.room_info_room);
        if (info.room_info_name_present &&
            info.room_info_name.room_info_name_choice == room_info_name_r::room_info_name_tstr_c) {
            d.name = fromZcbor(info.room_info_name.room_info_name_tstr);
        }
        if (info.room_info_session_present &&
            info.room_info_session.room_info_session_choice ==
                room_info_session_r::room_info_session_session_id_m_c) {
            d.session = fromZcbor(info.room_info_session.room_info_session_session_id_m);
        }
        out->append(d);
    }
    if (next != nullptr) {
        const bool hasNext =
            page.room_page_next_present &&
            page.room_page_next.room_page_next_choice == room_page_next_r::room_page_next_tstr_c;
        *next = hasNext ? fromZcbor(page.room_page_next.room_page_next_tstr) : QString();
    }
    return true;
}

} // namespace daemonapp::daemon
