#include "daemon/daemon_connection_service.h"

#include "daemon/node_api_codec.h"
#include "settings/isettings_store.h"

#include <algorithm>

namespace daemonapp::daemon {

DaemonConnectionService::DaemonConnectionService(QObject* parent,
                                                 settings::ISettingsStore* settings)
    : connection::IConnectionService(parent), m_transport(std::make_unique<DaemonTransport>()),
      m_client(std::make_unique<NodeApiClient>(m_transport.get())), m_settings(settings) {
    m_heartbeat.setInterval(kHeartbeatMs);
    connect(&m_heartbeat, &QTimer::timeout, this, [this] { sendHealthProbe(); });
    m_reprobe.setSingleShot(true);
    connect(&m_reprobe, &QTimer::timeout, this, &DaemonConnectionService::onReprobeTick);
    m_reconnectDeadline.setSingleShot(true);
    m_reconnectDeadline.setInterval(kReconnectBudgetMs);
    connect(&m_reconnectDeadline, &QTimer::timeout, this, [this] {
        m_reprobe.stop();
        setStatusMessage(tr("Could not reach the daemon. Check it is running and try again."));
        setState(QStringLiteral("offline"));
    });

    connect(m_client.get(), &NodeApiClient::responseReady, this,
            [this](const QString& correlationId, const QByteArray& responseCbor) {
                if (correlationId != QLatin1String(kHealthCorrelation)) {
                    return;
                }
                DecodedHealth health;
                // Liveness requires a decodable Health that reports all services OK; a degraded or
                // undecodable response is treated as not-ready rather than silently "connected".
                const bool ok = NodeApiCodec::decodeHealth(responseCbor, &health) && health.allOk;
                if (ok) {
                    markReady();
                } else {
                    // The daemon answered but is unhealthy/degraded - it is up, not dropped, so
                    // this is offline (a real condition), not a transient reconnect.
                    m_heartbeat.stop();
                    m_reprobe.stop();
                    m_reconnectDeadline.stop();
                    setStatusMessage(tr("The daemon reported it is not healthy."));
                    setState(QStringLiteral("offline"));
                }
            });
    connect(m_client.get(), &NodeApiClient::failed, this,
            [this](const QString& correlationId, const QString&) {
                // The Health request itself failed (transport drop / timeout): the connection is
                // suspect, so kick the reconnect episode (initial-connect failures retry too).
                if (correlationId == QLatin1String(kHealthCorrelation)) {
                    enterReconnecting();
                }
            });
    // A transport-level drop (peer disconnect / socket error) while live is the primary liveness
    // signal: self-correct out of stale "ready" immediately rather than waiting for the next
    // request.
    connect(m_transport.get(), &DaemonTransport::disconnected, this, [this] {
        if (ready() || state() == QStringLiteral("connecting")) {
            enterReconnecting();
        }
    });
    connect(m_transport.get(), &DaemonTransport::failed, this, [this](const QString&) {
        if (ready() || state() == QStringLiteral("connecting")) {
            enterReconnecting();
        }
    });

    if (m_settings != nullptr) {
        m_launcher = new LocalDaemonLauncher(m_settings, this);
        connect(m_launcher, &LocalDaemonLauncher::ready, this, [this](bool /*managed*/) {
            // The daemon is reachable (spawned or already running); confirm liveness over the wire.
            sendHealthProbe();
        });
        connect(m_launcher, &LocalDaemonLauncher::failed, this, [this](const QString& message) {
            setStatusMessage(message);
            setState(QStringLiteral("offline"));
        });
    }
}

void DaemonConnectionService::sendHealthProbe() {
    m_client->sendRequest(NodeApiCodec::encodeHealthRequest(), QLatin1String(kHealthCorrelation));
}

void DaemonConnectionService::markReady() {
    m_reprobe.stop();
    m_reconnectDeadline.stop();
    m_backoffMs = kReprobeMinMs;
    setStatusMessage(QString());
    setState(QStringLiteral("ready"));
    m_heartbeat.start(); // periodic liveness so a silent drop is noticed without user traffic
}

void DaemonConnectionService::enterReconnecting() {
    if (reconnecting()) {
        // An episode is already running (the reprobe / launcher loop is retrying); don't restart
        // it.
        return;
    }
    m_heartbeat.stop();
    setStatusMessage(tr("Reconnecting..."));
    // Reuse the "connecting" state string so StatusBarModel.gatewayState renders it degraded with
    // no StatusBar change; the "Reconnecting..." statusMessage distinguishes it from a first
    // connect.
    setState(QStringLiteral("connecting"));
    m_reconnectDeadline.start(); // hard episode budget -> offline if recovery never lands

    if (m_launcher != nullptr && m_settings != nullptr && m_settings->managedLocalDaemon()) {
        // Managed: delegate retry to the launcher (probe-first re-spawn within its restart budget +
        // its own readiness poll); its ready -> sendHealthProbe wiring closes the loop.
        m_launcher->ensureRunning(m_config.target);
        return;
    }
    // Attach-only / remote: drive a backoff Health reprobe ourselves (the transport reconnects
    // lazily under the send).
    m_backoffMs = kReprobeMinMs;
    m_reprobe.start(m_backoffMs);
}

void DaemonConnectionService::onReprobeTick() {
    if (!reconnecting()) {
        return; // episode resolved (ready) or gave up (offline) between scheduling and firing
    }
    sendHealthProbe();
    m_backoffMs = std::min(m_backoffMs * 2, kReprobeMaxMs);
    m_reprobe.start(m_backoffMs);
}

void DaemonConnectionService::connectTo(const QString& mode, const QString& target,
                                        const QString& token) {
    Q_UNUSED(token)
    m_config.mode = mode;
    m_config.target = target;
    emit configChanged();

    if (mode != QStringLiteral("local")) {
        setState(QStringLiteral("needs setup"));
        return;
    }
    if (target.isEmpty()) {
        setState(QStringLiteral("needs setup"));
        return;
    }

    m_transport->setSocketPath(target);
    setStatusMessage(QString());
    setState(QStringLiteral("connecting"));

    // Managed local daemon (CON-1b): ensure a daemon is running before attaching. ensureRunning is
    // probe-first, so an already-running daemon is reused rather than duplicated; on success it
    // signals back and we send the Health probe. Liveness is resolved by the Health response
    // handler wired in the constructor.
    if (m_launcher != nullptr && m_settings != nullptr && m_settings->managedLocalDaemon()) {
        m_launcher->ensureRunning(target);
        return;
    }

    sendHealthProbe();
}

void DaemonConnectionService::disconnect() {
    // User-initiated drop: stop all liveness machinery so we don't fight the explicit offline.
    m_heartbeat.stop();
    m_reprobe.stop();
    m_reconnectDeadline.stop();
    m_transport->close();
    setState(QStringLiteral("offline"));
}

void DaemonConnectionService::testConnection(const QString& mode, const QString& target,
                                             const QString& token) {
    Q_UNUSED(token)
    setTesting(true);
    const bool ok = mode == QStringLiteral("local") && !target.isEmpty();
    emit testResult(ok, ok ? QStringLiteral("Unix socket target accepted")
                           : QStringLiteral("Only local Unix-socket targets are supported"));
    setTesting(false);
}

void DaemonConnectionService::shutdownManagedDaemon() {
    if (m_launcher != nullptr) {
        m_launcher->shutdownIfManaged();
    }
}

} // namespace daemonapp::daemon
