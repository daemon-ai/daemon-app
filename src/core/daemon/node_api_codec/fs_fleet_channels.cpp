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

// --- Channels / Events-IO read surface (story 04: EIO-1/3/8/9) -----------------------------------

namespace {

QString connectionStateName(const connection_state_r& c) {
    switch (c.connection_state_choice) {
    case connection_state_r::connection_state_Connecting_tstr_c:
        return QStringLiteral("connecting");
    case connection_state_r::connection_state_Connected_tstr_c:
        return QStringLiteral("connected");
    case connection_state_r::connection_state_Error_tstr_c:
        return QStringLiteral("error");
    case connection_state_r::connection_state_Offline_tstr_c:
    default:
        return QStringLiteral("offline");
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

} // namespace

QByteArray NodeApiCodec::encodeTransportAdaptersRequest() {
    return encodeSimple(api_request_r::api_request_request_transport_adapters_m_c);
}

QByteArray NodeApiCodec::encodeTransportInstancesRequest() {
    return encodeSimple(api_request_r::api_request_request_transport_instances_m_c);
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
        const conversation_info& cv = page.conv_page_items_conversation_info_m[i];
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
        out->append(d);
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

} // namespace daemonapp::daemon
