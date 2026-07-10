// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// The daemon bridge (spec 09 §5.1, 07§10 codec boundary): translates the vendored codec's
// DecodedNodeEvent (the app's GENERATED wire NodeEvent surface) into the ingestor's decoupled
// mirror::NodeEvent. This `translateNodeEvent` switch is the SECOND compile-time exhaustiveness
// layer (spec §12): it has NO default, so a new wire arm added to DecodedNodeEvent::Kind fails
// compilation here until it is mapped — which in turn requires a mirror::NodeEventKind arm, which
// requires a §5.2 policy row (policy_table.h's own no-default switch). The wire enum, the policy
// table, and the arm census cannot silently drift apart.
//
// Live wiring of the ingestor beside/instead of SubscriptionManager in app_service_graph (the
// FetchExecutor over NodeApiClient + repointing readers + deleting the connect-ready storm) is
// A5's consumer-migration step under dual-write discipline; A4 provides this pure, tested
// translation + the ingestor it feeds.

#include "daemon/node_api_codec.h"
#include "mirror/node_event.h"

namespace daemonapp::daemon {

[[nodiscard]] inline mirror::NodeEvent translateNodeEvent(const DecodedNodeEvent& in) {
    mirror::NodeEvent out;
    using WK = DecodedNodeEvent::Kind;
    using MK = mirror::NodeEventKind;
    switch (in.kind) {
    case WK::Unknown:
        out.kind = MK::Unknown;
        break;
    case WK::SessionAdvanced:
        out.kind = MK::SessionAdvanced;
        out.session = in.session;
        out.epoch = in.epoch;
        out.headSeq = in.headSeq;
        break;
    case WK::SessionMetaChanged:
        out.kind = MK::SessionMetaChanged;
        out.session = in.session;
        out.rev = in.rev;
        out.hasRev = (in.rev != 0);
        break;
    case WK::RosterChanged:
        out.kind = MK::RosterChanged;
        out.rev = in.rev;
        out.hasRev = (in.rev != 0);
        break;
    case WK::ApprovalPending:
        out.kind = MK::ApprovalPending;
        out.session = in.session;
        out.requestId = in.requestId;
        break;
    case WK::DownloadProgress:
        out.kind = MK::DownloadProgress;
        out.downloadId = in.downloadId;
        out.pct = in.pct;
        out.downloadState = in.state;
        out.downloadedBytes = in.downloadedBytes;
        out.totalBytes = in.totalBytes;
        break;
    case WK::CatalogChanged:
        out.kind = MK::CatalogChanged;
        out.rev = in.rev;
        out.hasRev = (in.rev != 0);
        break;
    case WK::ResyncNeeded:
        out.kind = MK::ResyncNeeded;
        out.scope = in.scope;
        break;
    case WK::FleetChanged:
        out.kind = MK::FleetChanged;
        out.rev = in.rev;
        out.hasRev = (in.rev != 0);
        break;
    case WK::TransportChanged:
        out.kind = MK::TransportChanged;
        out.transport = in.transport;
        out.connection = in.connection;
        out.presence = in.presence;
        out.hasPresence = in.hasPresence;
        out.reason = in.reason;
        out.hasReason = in.hasReason;
        out.message = in.message;
        out.hasMessage = in.hasMessage;
        out.fatal = in.fatal;
        break;
    case WK::ConversationsChanged:
        out.kind = MK::ConversationsChanged;
        out.transport = in.transport;
        out.conv = in.conv;
        out.convChange = in.convChange;
        out.rev = in.rev;
        out.hasRev = (in.rev != 0);
        break;
    case WK::MembershipChanged:
        out.kind = MK::MembershipChanged;
        out.transport = in.transport;
        out.conv = in.conv;
        out.member = in.member;
        out.membershipChange = in.membershipChange;
        out.isSelf = in.isSelf;
        break;
    case WK::ContactsChanged:
        out.kind = MK::ContactsChanged;
        out.transport = in.transport;
        break;
    case WK::ProfilesChanged:
        out.kind = MK::ProfilesChanged;
        out.rev = in.rev;
        out.hasRev = (in.rev != 0);
        break;
    case WK::PersonsChanged:
        out.kind = MK::PersonsChanged;
        out.rev = in.rev;
        out.hasRev = (in.rev != 0);
        break;
    case WK::MessagesChanged:
        out.kind = MK::MessagesChanged;
        out.transport = in.transport;
        out.conv = in.conv;
        break;
    }
    return out;
}

} // namespace daemonapp::daemon
