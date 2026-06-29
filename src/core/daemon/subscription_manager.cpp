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
                                         DaemonCacheStore* cache, QObject* parent)
    : QObject(parent), m_nodeApi(nodeApi), m_sessions(sessions), m_approvals(approvals),
      m_models(models), m_cache(cache) {
    m_rosterDebounce.setSingleShot(true);
    m_rosterDebounce.setInterval(kRosterDebounceMs);
    connect(&m_rosterDebounce, &QTimer::timeout, this, [this] {
        if (m_rosterDirty && m_sessions != nullptr) {
            m_rosterDirty = false;
            m_sessions->refreshSessions();
        }
    });
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
    case DecodedNodeEvent::Kind::RosterChanged:
    case DecodedNodeEvent::Kind::SessionMetaChanged:
        // The roster set or a row's metadata changed; refetch the roster (a delta query is L4).
        // Debounced so a burst of meta changes is one refetch, not N.
        scheduleRosterRefetch();
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
        if (m_models != nullptr) {
            m_models->applyDownloadProgress(event.downloadId, event.pct, event.state);
        }
        break;
    case DecodedNodeEvent::Kind::ResyncNeeded:
        // The feed cursor aged out of the retained ring; re-baseline the awareness surfaces. The
        // page carrying this event sets next_cursor to the feed head, so the stream continues from
        // there after this refetch.
        m_rosterDirty = true;
        if (m_sessions != nullptr) {
            m_sessions->refreshSessions();
        }
        if (m_approvals != nullptr) {
            m_approvals->refreshPending();
        }
        emit resyncNeeded();
        break;
    case DecodedNodeEvent::Kind::Unknown:
        break;
    }
}

void SubscriptionManager::scheduleRosterRefetch() {
    m_rosterDirty = true;
    if (!m_rosterDebounce.isActive()) {
        m_rosterDebounce.start();
    }
}

} // namespace daemonapp::daemon
