// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/subscription_manager.h"

#include "daemon/client_cache_schema.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"

#include <QDateTime>

namespace daemonapp::daemon {

SubscriptionManager::SubscriptionManager(NodeApiClient* nodeApi, SessionRepository* sessions,
                                         ApprovalRepository* approvals, ModelRepository* models,
                                         DaemonCacheStore* cache, ProfileRepository* profiles,
                                         QObject* parent)
    : QObject(parent), m_nodeApi(nodeApi), m_sessions(sessions), m_approvals(approvals),
      m_models(models), m_profiles(profiles), m_cache(cache) {
    if (m_nodeApi != nullptr) {
        connect(m_nodeApi, &NodeApiClient::streamItem, this, &SubscriptionManager::onStreamItem);
        connect(m_nodeApi, &NodeApiClient::streamEnded, this, &SubscriptionManager::onStreamEnded);
    }
}

void SubscriptionManager::start() {
    if (m_nodeApi == nullptr || m_feedStreamId != 0) {
        return;
    }
    // Resume from the durably-persisted feed cursor across restarts (cold start = 0). A still-warm
    // m_feedCursor (reconnect within one run) wins over the on-disk value.
    if (m_feedCursor == 0 && m_cache != nullptr) {
        const QString stored = m_cache->cursor(QString::fromUtf8(cache::kEventsSinceScope));
        if (!stored.isEmpty()) {
            m_feedCursor = stored.toULongLong();
        }
    }
    // One node-wide feed from the tracked cursor (0 on a cold start; the resume point after a
    // drop).
    m_feedStreamId = m_nodeApi->openStream(NodeApiCodec::encodeEventsSinceRequest(m_feedCursor));
}

void SubscriptionManager::stop() {
    if (m_nodeApi != nullptr && m_feedStreamId != 0) {
        m_nodeApi->cancelStream(m_feedStreamId);
    }
    m_feedStreamId = 0;
}

void SubscriptionManager::registerFocus(const QString& sessionId, QObject* engine) {
    if (!sessionId.isEmpty()) {
        m_focus.insert(sessionId, QPointer<QObject>(engine));
    }
}

void SubscriptionManager::unregisterFocus(const QString& sessionId, QObject* engine) {
    if (engine != nullptr && m_focus.value(sessionId).data() != engine) {
        return; // a different engine owns this session's slot now; leave it intact
    }
    m_focus.remove(sessionId);
}

void SubscriptionManager::onStreamItem(quint64 id, const QByteArray& responseCbor) {
    if (id != m_feedStreamId) {
        return;
    }
    DecodedEventsPage page;
    if (!NodeApiCodec::decodeEventsPage(responseCbor, &page)) {
        return;
    }
    for (const DecodedNodeEvent& event : page.events) {
        applyEvent(event);
    }
    // Advance the resume cursor (keepalive pages carry next_cursor == the current head, so the
    // watermark tracks liveness too). Never moves backward. Persist it so the feed resumes across
    // app restarts (avoids re-reading the whole retained ring on next launch).
    if (page.nextCursor > m_feedCursor) {
        m_feedCursor = page.nextCursor;
        if (m_cache != nullptr) {
            m_cache->setCursor(QString::fromUtf8(cache::kEventsSinceScope),
                               QString::number(m_feedCursor), QDateTime::currentMSecsSinceEpoch());
        }
    }
}

void SubscriptionManager::onStreamEnded(quint64 id, bool /*error*/, const QString& /*message*/) {
    if (id != m_feedStreamId) {
        return;
    }
    // The feed closed (Cancel, a transport drop, or daemon teardown). Drop the id; the connection
    // service's reconnect path calls start() again, which resumes from m_feedCursor.
    m_feedStreamId = 0;
}

void SubscriptionManager::applyEvent(const DecodedNodeEvent& event) {
    switch (event.kind) {
    case DecodedNodeEvent::Kind::SessionMetaChanged:
        // A session's metadata changed. Refresh its hydrated SessionDetail so live per-session
        // projections re-read (Phase E: the composer's foreign model selector / backend). Scoped to
        // a session we ALREADY hydrated, so a burst of meta changes doesn't fetch detail for every
        // roster row. The ROSTER refetch is the mirror ingestor's policy arm (AD — the legacy
        // debounced cache refetch died with the cache feed).
        if (m_sessions != nullptr && m_sessions->cachedDetail(event.session, nullptr)) {
            m_sessions->getSessionDetail(event.session);
        }
        break;
    case DecodedNodeEvent::Kind::RosterChanged:
        // Mirror-served (the ingestor's RosterChanged arm refetches the `sessions` table).
        break;
    case DecodedNodeEvent::Kind::ApprovalPending:
        // Badge the inbox without a connect-ready storm; the repository fetches the detail.
        if (m_approvals != nullptr) {
            m_approvals->refreshPending();
        }
        break;
    case DecodedNodeEvent::Kind::SessionAdvanced:
        // Nudge the focused engine for this session so an idle focused tab catches up. Out-of-focus
        // sessions need no action (the roster activity is covered by SessionMetaChanged).
        if (auto it = m_focus.constFind(event.session); it != m_focus.constEnd()) {
            if (QObject* engine = it.value()) {
                QMetaObject::invokeMethod(engine, "nudge", Qt::DirectConnection);
            }
        }
        break;
    case DecodedNodeEvent::Kind::DownloadProgress:
        // Patch the download row in place from the feed payload (replaces the retired 600ms poll).
        // v26: the event carries real byte counters, so the row renders exact progress.
        if (m_models != nullptr) {
            m_models->applyDownloadProgress(event.downloadId, event.state, event.downloadedBytes,
                                            event.totalBytes);
        }
        break;
    case DecodedNodeEvent::Kind::CatalogChanged:
        // The installed-model registry changed (a finished download was cataloged / a model was
        // deleted): refetch the catalog. The refresh cascades through the model facade
        // (rebuildInstalled -> downloadFinished on growth) into the provider picker's offered
        // models, so a completed download becomes selectable without any client polling.
        if (m_models != nullptr) {
            m_models->refreshCatalog();
        }
        break;
    case DecodedNodeEvent::Kind::ResyncNeeded: {
        // Branch on the resync `scope` (the node stamps "all" today; "profiles" is the profile-only
        // re-baseline). "all"/"profiles" also re-fetch the reflected ProfileList, so profiles ride
        // the general resync mechanism (no dedicated ProfilesChanged event). "all" keeps the broad
        // roster/fleet/approvals re-baseline.
        const bool all = event.scope.isEmpty() || event.scope == QStringLiteral("all");
        const bool profiles = all || event.scope == QStringLiteral("profiles");
        if (all) {
            // The roster/fleet re-baseline is mirror-served (the ingestor's resync handling);
            // only the non-migrated repo domains re-baseline here.
            if (m_approvals != nullptr) {
                m_approvals->refreshPending();
            }
            if (m_models != nullptr) {
                // The gap may have swallowed DownloadProgress / CatalogChanged events: re-baseline
                // the model state those events would have patched.
                m_models->refreshCatalog();
                m_models->refreshDownloads();
            }
        }
        if (profiles && m_profiles != nullptr) {
            m_profiles->refreshProfiles();
        }
        emit resyncNeeded();
        break;
    }
    // [wave2:app-channels-liveness] F5: a fleet supervision change that need not move the roster
    // (e.g. a subagent state transition with no new session row). The TREE refetch is the mirror
    // ingestor's FleetChanged policy arm (AD).
    case DecodedNodeEvent::Kind::FleetChanged:
        // Route the fleet change to every focused turn engine so a live turn re-reads its
        // structured subagent events (UnitEvents) and the subagent strip reflects spawn/finish. The
        // feed carries no session key, so notify all focused engines; each no-ops when idle.
        for (auto it = m_focus.constBegin(); it != m_focus.constEnd(); ++it) {
            if (QObject* engine = it.value()) {
                QMetaObject::invokeMethod(engine, "fleetChanged", Qt::DirectConnection);
            }
        }
        break;
    // [wave2:app-channels-liveness] B5: live per-account transport presence. Apply the carried
    // connection/presence in place (the mirror account rows carry the status dots since AD 1a).
    // On a connect transition, refresh that account's ConvList so auto-accepted/joined rooms
    // surface promptly (B2) rather than only on manual expand.
    case DecodedNodeEvent::Kind::TransportChanged:
        if (m_transports != nullptr) {
            // [waveB:app-v30] D1: forward the disconnect provenance (reason/message/fatal) too.
            m_transports->applyTransportChanged(event.transport, event.connection, event.presence,
                                                event.hasPresence, event.reason, event.hasReason,
                                                event.message, event.hasMessage, event.fatal);
            if (event.connection == QStringLiteral("connected")) {
                m_transports->refreshConversations(event.transport);
            }
        }
        break;
    // [waveB:app-v30] D2: the conversation set for a transport changed (a room was added/
    // removed) or a room's membership changed (a join/leave can add or drop a visible room).
    // Either way, refetch that transport's ConvList — invalidation pointer only; the node owns
    // membership. On a self-removal the node already reconciled the routing pins; the pin-table
    // + roster refetches are the mirror ingestor's MembershipChanged arm (AD).
    case DecodedNodeEvent::Kind::ConversationsChanged:
    case DecodedNodeEvent::Kind::MembershipChanged:
        if (m_transports != nullptr) {
            m_transports->refreshConversations(event.transport);
        }
        break;
    // [acct-mgmt] A transport's contact roster changed (wire v34). Refetch that transport's
    // RosterList — invalidation pointer only; the node owns the roster. Mirrors
    // ConversationsChanged for rooms. (This is the transport contacts feed, NOT the session-inbox
    // RosterChanged above.)
    case DecodedNodeEvent::Kind::ContactsChanged:
        if (m_contacts != nullptr) {
            m_contacts->refreshContacts(event.transport);
        }
        break;
    // Profile roster mutation (wire v31, phase H): a create/edit/delete/select (operator or
    // agent-authored via profile_manage) bumped the profile revision. Invalidation pointer only —
    // refetch ProfileList, which re-runs DaemonProfileStore::rebuild() so the provenance/filter
    // surfaces re-render. Direct (not debounced): the node coalesces ProfilesChanged, and a profile
    // CRUD burst is small. Complements the "profiles"-scoped ResyncNeeded re-baseline above.
    case DecodedNodeEvent::Kind::ProfilesChanged:
        if (m_profiles != nullptr) {
            m_profiles->refreshProfiles();
        }
        break;
    // [integrations wire v38 → AD 1a] PersonsChanged: the mirror ingestor owns the PersonList
    // refetch (persons + endpoints land in the mirror tables the tree reads) — no repo arm here.
    // [integrations wire v38] A conversation's transcript grew: re-fetch ConvHistory for the
    // affected (transport, conv) so the chat tab's transcript lands the new message(s).
    // Invalidation pointer only — the client refetches from its cursor; it derives no message facts
    // locally.
    case DecodedNodeEvent::Kind::MessagesChanged:
        if (m_chat != nullptr) {
            m_chat->applyMessagesChanged(event.transport, event.conv);
        }
        break;
    case DecodedNodeEvent::Kind::Unknown:
        break;
    }
}

} // namespace daemonapp::daemon
