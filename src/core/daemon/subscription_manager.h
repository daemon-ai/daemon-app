// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>

namespace daemonapp::daemon {

class NodeApiClient;
class SessionRepository;
class ApprovalRepository;
class ModelRepository;
class FleetRepository;
class ProfileRepository;
class TransportRepository; // [wave2:app-channels-liveness]
class RoutingRepository;   // [waveB:app-v30] D2
class DaemonCacheStore;
struct DecodedNodeEvent;

// The single client consumer of the node-wide event feed (L3 `EventsSince`;
// daemon-sync-protocol-spec §5 / client-sync-architecture §6-§8). It holds ONE push stream of
// payload-free `NodeEvent` pointers and routes them so the client learns what changed *out of
// focus* without polling and without the connect-ready full-refetch storm:
//
//   - RosterChanged / SessionMetaChanged -> a debounced SessionRepository::refreshSessions()
//                                           (+ FleetRepository::refreshTree(), since a fleet unit
//                                           IS a durable session: a session created lazily on first
//                                           Submit must also repopulate the projected tree - FIX 2)
//   - ApprovalPending                    -> ApprovalRepository::refreshPending() (the inbox badge)
//   - DownloadProgress                   -> ModelRepository (retires the 600ms poll; see
//   p3-downloads)
//   - SessionAdvanced                    -> nudge the focused engine registered for that session
//   - ResyncNeeded                       -> a baseline refetch (the feed cursor aged out)
//
// Per-tab focus: the orchestrator registers a focused session's turn engine (by QObject, invoked by
// name as "nudge" so this module stays decoupled from the Turn module). The lazy-focus
// subscribe-first handoff is the engine's own concern; the manager only routes the awareness nudge.
class SubscriptionManager : public QObject {
    Q_OBJECT

public:
    SubscriptionManager(NodeApiClient* nodeApi, SessionRepository* sessions,
                        ApprovalRepository* approvals, ModelRepository* models,
                        DaemonCacheStore* cache, FleetRepository* fleet = nullptr,
                        ProfileRepository* profiles = nullptr, QObject* parent = nullptr);

    // Open (or re-open) the single node-wide feed from the tracked cursor. Idempotent; call on
    // connect-ready. A re-open after a drop resumes from the last applied cursor (the retained ring
    // backfills, or surfaces ResyncNeeded if it aged out).
    void start();
    // Tear the feed stream down (call on disconnect). The cursor is retained for the next start().
    void stop();

    // Register/clear the focused turn engine for a session so SessionAdvanced nudges it. `engine`
    // is a QObject exposing an invokable `nudge()` (ITurnEngine); held as a QPointer so a destroyed
    // engine auto-clears. `unregisterFocus` with a non-null `engine` only clears the slot when that
    // engine still owns it (so one tab's teardown can't unregister another tab of the same
    // session).
    void registerFocus(const QString& sessionId, QObject* engine);
    void unregisterFocus(const QString& sessionId, QObject* engine = nullptr);

    // [wave2:app-channels-liveness] B5: the transport repository the live TransportChanged event
    // patches (connection/presence). Wired via a setter because the repo is constructed AFTER this
    // manager in the service graph. Optional; TransportChanged is ignored when unset.
    void setTransportRepository(TransportRepository* transports) { m_transports = transports; }

    // [waveB:app-v30] D2: the routing repository refetched on a self membership removal (the node
    // has already reconciled the routing pins; the client re-lists to render the node's state).
    // Wired via a setter because the repo is constructed after this manager. Optional.
    void setRoutingRepository(RoutingRepository* routing) { m_routing = routing; }

    [[nodiscard]] quint64 feedCursor() const { return m_feedCursor; }

signals:
    // A ResyncNeeded arrived (the feed cursor aged out); the manager already kicked the baseline
    // refetch — emitted so tests / higher layers can observe it.
    void resyncNeeded();

private:
    void onStreamItem(quint64 id, const QByteArray& responseCbor);
    void onStreamEnded(quint64 id, bool error, const QString& message);
    void applyEvent(const DecodedNodeEvent& event);
    void scheduleRosterRefetch();

    NodeApiClient* m_nodeApi = nullptr;
    SessionRepository* m_sessions = nullptr;
    ApprovalRepository* m_approvals = nullptr;
    ModelRepository* m_models = nullptr;
    FleetRepository* m_fleet = nullptr;
    ProfileRepository* m_profiles = nullptr;
    TransportRepository* m_transports = nullptr; // [wave2:app-channels-liveness]
    RoutingRepository* m_routing = nullptr;      // [waveB:app-v30] D2
    DaemonCacheStore* m_cache = nullptr;

    quint64 m_feedStreamId = 0; // the open EventsSince stream id (0 = none)
    quint64 m_feedCursor = 0;   // the applied feed-cursor watermark (resume point)

    QHash<QString, QPointer<QObject>> m_focus; // sessionId -> focused turn engine
    QTimer m_rosterDebounce;                   // coalesce roster refetches under event bursts
    bool m_rosterDirty = false;

    static constexpr int kRosterDebounceMs = 300;
};

} // namespace daemonapp::daemon
