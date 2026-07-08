// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "connection/iconnection_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/local_daemon_launcher.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h" // DecodedHealth retained as the single health cache

#include <memory>
#include <QList>
#include <QTimer>
#include <QUrl>

namespace settings {
class ISettingsStore;
}

namespace daemonapp::daemon {

// Real connection seam for daemon-backed mode.
//
// Owns connection config/liveness, the persistent daemon transport, and the NodeApiClient that
// repositories share. Three modes map onto the transport's carriers: "local" (Unix socket,
// optionally app-managed via LocalDaemonLauncher), "remote" (host:port, TLS TCP), and "remote-ws"
// (a full ws:// / wss:// URL - the only mode a browser build can use; never invokes the
// launcher). Liveness is driven by a real `Health` request: connectTo() sends Health and the
// liveness state machine resolves to "ready" only once a Health response decodes, or "offline"
// if the probe fails. The transport/client are exposed so the service graph can hand the same
// client to repositories rather than building a parallel one.
class DaemonConnectionService : public connection::IConnectionService {
    Q_OBJECT

public:
    // `settings` (optional) enables managed local-daemon auto-spawn (CON-1b): when set and
    // `managedLocalDaemon` is on, a local connect to an unreachable socket discovers + spawns a
    // daemon before attaching. Without it the service is attach-only (its prior behavior). `parent`
    // stays first so existing `new DaemonConnectionService(owner)` call sites keep working.
    explicit DaemonConnectionService(QObject* parent = nullptr,
                                     settings::ISettingsStore* settings = nullptr);

    void connectTo(const QString& mode, const QString& target,
                   const QString& token = QString()) override;
    void disconnect() override;
    void testConnection(const QString& mode, const QString& target,
                        const QString& token = QString()) override;
    void login(const QString& username, const QString& password) override;
    void logout() override;

    [[nodiscard]] DaemonTransport* transport() const { return m_transport.get(); }
    [[nodiscard]] NodeApiClient* client() const { return m_client.get(); }

    // The single retained health cache (Phase F): the last decoded Health report, kept so the
    // gateway/health surfaces read the per-service entries (`gateway`, `local-inference`, ...)
    // without a second health poll. Populated on every Health response (healthy or degraded);
    // healthChanged() fires when the report changes.
    [[nodiscard]] const DecodedHealth& lastHealth() const { return m_lastHealth; }
    [[nodiscard]] QList<DecodedServiceHealth> healthServices() const {
        return m_lastHealth.services;
    }

    // Terminate a daemon this service spawned, honoring the shutdown-on-exit preference. Called
    // from the front ends' teardown. No-op when nothing was spawned or the user keeps it
    // persistent.
    void shutdownManagedDaemon();

signals:
    // Emitted alongside authenticated() on every AuthOk, reporting HOW the connection authed:
    // `resumed` = true when a persisted resume token drove the handshake (AuthResume fast-path, no
    // SCRAM challenge), false for a fresh interactive login. Drives the reload-survival auth
    // sentinel (DAEMON_APP_AUTH resumed|scram); not shown in the UI.
    void authOutcome(bool resumed);

    // The retained Health report changed (Phase F): a new Health response decoded, so lastHealth()
    // / healthServices() are fresh. Distinct from stateChanged() (connection liveness) — this fires
    // for the per-service detail even when liveness is unchanged.
    void healthChanged();

private:
    static constexpr auto kHealthCorrelation = "connection/health";
    // Steady liveness probe while ready; backoff bounds + a hard episode budget while reconnecting.
    static constexpr int kHeartbeatMs = 12000;
    static constexpr int kReprobeMinMs = 500;
    static constexpr int kReprobeMaxMs = 10000;
    static constexpr int kReconnectBudgetMs = 120000; // give up -> offline after 2 min

    void sendHealthProbe();
    // The version gate, run on every completed Hello handshake: compare the daemon's advertised
    // "api/<N>" contract version against NodeApiCodec::kDaemonApiVersion. On mismatch (a missing
    // advertisement counts) never serve the connection: an app-managed daemon is terminated via
    // its pidfile and respawned as the current binary (once per connect), an attached daemon
    // surfaces an explicit "incompatible daemon" error.
    void enforceApiVersionGate();
    // The connection became live: ready state, clear status, reset backoff, (re)start the
    // heartbeat.
    void markReady();
    // A drop/timeout was observed: enter the transient reconnecting episode (state "connecting" +
    // "Reconnecting..." status) and start retrying. Idempotent while an episode is already running.
    void enterReconnecting();
    // One attach-only reprobe tick: re-send Health and re-arm with growing backoff.
    void onReprobeTick();
    // True iff a reconnect episode is currently in progress.
    [[nodiscard]] bool reconnecting() const { return m_reconnectDeadline.isActive(); }
    // Configure the transport for the current config.mode/target and load the AuthResume token; set
    // the client credentials before the first send. Returns false if the target is unusable.
    bool configureTransport();
    // Parse a "host:port" remote target. Returns false on a malformed target.
    static bool parseHostPort(const QString& target, QString* host, quint16* port);
    // Parse/validate a "remote-ws" target: a full `ws://host[:port][/path]` or
    // `wss://host[:port][/path]` URL. Returns an invalid QUrl on anything else (other scheme,
    // missing host, unparsable). Public-ish via tests through testConnection().
    [[nodiscard]] static QUrl parseWsUrl(const QString& target);
    // Build the TLS policy from the conn/tls/* settings keys (fail-closed defaults).
    [[nodiscard]] TlsConfig tlsConfigFromSettings() const;

    std::unique_ptr<DaemonTransport> m_transport;
    std::unique_ptr<NodeApiClient> m_client;
    // The single health cache (Phase F): the last decoded Health report, retained across probes so
    // the gateway/health surfaces project the per-service entries without polling Health twice.
    DecodedHealth m_lastHealth;
    settings::ISettingsStore* m_settings = nullptr;
    LocalDaemonLauncher* m_launcher = nullptr;
    QTimer m_heartbeat;         // periodic Health while ready
    QTimer m_reprobe;           // attach-only backoff Health reprobe while reconnecting
    QTimer m_reconnectDeadline; // single-shot episode budget -> offline
    int m_backoffMs = kReprobeMinMs;

    // Browser (wasm) network-wakeup: on the browser `online` event, collapse the reconnect backoff
    // and probe immediately instead of waiting out the current reprobe interval. No-op off wasm.
    void onNetworkOnline();

    // Auth state. m_authBlocked suppresses the auto-reconnect loop while a node is waiting on
    // interactive credentials (otherwise the failed Health probe would spin reconnecting). The
    // session token (server-issued on AuthOk) drives the AuthResume reconnect fast-path.
    // m_resumeAttempt records whether the CURRENT handshake presented a resume token, so the
    // authOutcome signal can classify AuthOk as resumed (token) vs scram (fresh login).
    bool m_authBlocked = false;
    bool m_resumeAttempt = false;
    QString m_sessionToken;

    // Version-gate state. m_versionHold suppresses the auto-reconnect paths after the gate
    // dropped a connection to an incompatible daemon (reconnecting would just re-attach to it);
    // it clears on the next user connect/login or once the launcher reports the replacement
    // daemon ready. m_versionRespawnAttempted bounds the managed terminate-and-respawn to ONCE
    // per connectTo (a respawned daemon that still mismatches is a real error, not a loop).
    bool m_versionHold = false;
    bool m_versionRespawnAttempted = false;
};

} // namespace daemonapp::daemon
