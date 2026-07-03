// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

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
                // BUT not while a node is waiting on interactive credentials - reconnecting there
                // would just re-fail auth in a loop; the login UI drives the retry instead. And
                // not while the version gate holds the line - reconnecting would just re-attach
                // to the same incompatible daemon.
                if (correlationId == QLatin1String(kHealthCorrelation) && !m_authBlocked &&
                    !m_versionHold) {
                    enterReconnecting();
                }
            });
    // The version gate runs on every completed handshake, before any response can mark the
    // connection ready: a stale daemon is replaced (managed) or refused (attach), never served.
    connect(m_client.get(), &NodeApiClient::handshakeReady, this,
            [this] { enforceApiVersionGate(); });
    // SASL relays: the client owns the exchange; we map its outcome onto the connection state.
    connect(m_client.get(), &NodeApiClient::tokenIssued, this, [this](const QString& token) {
        // Persist the SERVER-issued token (never the password), keyed per target, and keep it in
        // memory so a transport drop reconnects via AuthResume.
        m_sessionToken = token;
        m_client->setAuthCredentials([this] {
            NodeApiClient::AuthCredentials c;
            c.resumeToken = m_sessionToken;
            c.tlsActive = m_transport->tlsActive();
            c.hasClientCert = m_transport->clientCertConfigured();
            return c;
        }());
        if (m_settings != nullptr && !m_config.target.isEmpty()) {
            m_settings->setConnectionToken(m_config.target, token);
        }
    });
    connect(m_client.get(), &NodeApiClient::authenticated, this, [this] { emit authenticated(); });
    connect(m_client.get(), &NodeApiClient::authFailed, this, [this](const QString& reason) {
        // The node needs (different) credentials. Stop the liveness machinery, surface the login
        // form, and stay out of the reconnect loop until the user submits new credentials.
        m_authBlocked = true;
        m_heartbeat.stop();
        m_reprobe.stop();
        m_reconnectDeadline.stop();
        const bool needCreds = reason == QStringLiteral("authentication required");
        // A stale stored token that the server rejected must not wedge future logins.
        if (!needCreds && m_settings != nullptr && !m_config.target.isEmpty()) {
            m_settings->clearConnectionToken(m_config.target);
        }
        m_sessionToken.clear();
        setStatusMessage(needCreds ? QString() : reason);
        setState(QStringLiteral("authenticating"));
        emit authRequired();
        if (!needCreds) {
            emit authFailed(reason);
        }
    });
    // A transport-level drop (peer disconnect / socket error) while live is the primary liveness
    // signal: self-correct out of stale "ready" immediately rather than waiting for the next
    // request.
    connect(m_transport.get(), &DaemonTransport::disconnected, this, [this] {
        if (!m_authBlocked && !m_versionHold &&
            (ready() || state() == QStringLiteral("connecting"))) {
            enterReconnecting();
        }
    });
    connect(m_transport.get(), &DaemonTransport::failed, this, [this](const QString&) {
        if (!m_authBlocked && !m_versionHold &&
            (ready() || state() == QStringLiteral("connecting"))) {
            enterReconnecting();
        }
    });

    if (m_settings != nullptr) {
        m_launcher = new LocalDaemonLauncher(m_settings, this);
        connect(m_launcher, &LocalDaemonLauncher::ready, this, [this](bool /*managed*/) {
            // The daemon is reachable (spawned, replaced, or already running); confirm liveness
            // over the wire. A version-gate replacement is complete here, so lift its hold - the
            // fresh handshake below re-runs the gate against the new daemon.
            m_versionHold = false;
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

void DaemonConnectionService::enforceApiVersionGate() {
    const quint32 daemonApi = m_client->daemonApiVersion();
    if (daemonApi == NodeApiCodec::kDaemonApiVersion) {
        m_versionHold = false;
        return;
    }
    // Contract mismatch (0 = the daemon predates the "api/<N>" advertisement): this daemon's
    // wire shapes cannot be trusted, so never let the connection reach "ready" (the day-old
    // stale-daemon incident). Stop the liveness machinery and hold the reconnect paths BEFORE
    // closing - the close fails the in-flight Health, which must not re-enter the loop; closing
    // also drops any not-yet-delivered Reply, so no response can leak through the gate.
    m_heartbeat.stop();
    m_reprobe.stop();
    m_reconnectDeadline.stop();
    m_versionHold = true;
    m_transport->close();

    const QString daemonLabel = daemonApi == 0 ? tr("unknown") : QString::number(daemonApi);
    const bool managed = m_config.mode == QStringLiteral("local") && m_launcher != nullptr &&
                         m_settings != nullptr && m_settings->managedLocalDaemon();
    if (managed && !m_versionRespawnAttempted) {
        // App-managed socket (the launcher spawned this daemon, or would spawn one): replace the
        // stale daemon with the current binary. Bounded to ONE attempt per connect - if the
        // replacement still mismatches, that is a real error surfaced below, not a retry loop.
        m_versionRespawnAttempted = true;
        setStatusMessage(tr("Replacing an incompatible local daemon (api %1, need %2)...")
                             .arg(daemonLabel)
                             .arg(NodeApiCodec::kDaemonApiVersion));
        setState(QStringLiteral("connecting"));
        m_launcher->replaceStaleDaemon(m_config.target);
        return;
    }
    // Attach (user-supplied socket / remote node / respawn already spent): fail the connect
    // explicitly - a silent attach to an incompatible daemon is exactly the incident this gates.
    setStatusMessage(tr("Incompatible daemon (api %1, need %2).")
                         .arg(daemonLabel)
                         .arg(NodeApiCodec::kDaemonApiVersion));
    setState(QStringLiteral("offline"));
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

QUrl DaemonConnectionService::parseWsUrl(const QString& target) {
    // Remote-ws target syntax is a FULL URL (ws://host:port or wss://host[:port][/path]) - unlike
    // "remote", where the bare host:port implies the one TLS carrier, the scheme here decides
    // plaintext vs TLS, so it must be explicit.
    QUrl url(target.trimmed(), QUrl::StrictMode);
    const bool wsScheme =
        url.scheme() == QLatin1String("ws") || url.scheme() == QLatin1String("wss");
    if (!url.isValid() || !wsScheme || url.host().isEmpty()) {
        return {};
    }
    return url;
}

bool DaemonConnectionService::parseHostPort(const QString& target, QString* host, quint16* port) {
    // Remote target syntax is "host:port" (decision 4). Tolerate an optional scheme prefix so a
    // pasted URL still parses, but the canonical form is bare host:port.
    QString t = target.trimmed();
    const qsizetype scheme = t.indexOf(QStringLiteral("://"));
    if (scheme >= 0) {
        t = t.mid(scheme + 3);
    }
    const qsizetype colon = t.lastIndexOf(QLatin1Char(':'));
    if (colon <= 0 || colon == t.size() - 1) {
        return false;
    }
    bool ok = false;
    const uint p = t.mid(colon + 1).toUInt(&ok);
    if (!ok || p == 0 || p > 0xFFFF) {
        return false;
    }
    *host = t.left(colon);
    *port = static_cast<quint16>(p);
    return true;
}

TlsConfig DaemonConnectionService::tlsConfigFromSettings() const {
    TlsConfig tls;
    if (m_settings == nullptr) {
        return tls;
    }
    tls.caFile = m_settings->value(QStringLiteral("conn/tls/caFile")).toString();
    tls.pinnedSha256 = m_settings->value(QStringLiteral("conn/tls/pinnedSha256")).toString();
    tls.clientCertFile = m_settings->value(QStringLiteral("conn/tls/clientCertFile")).toString();
    tls.clientKeyFile = m_settings->value(QStringLiteral("conn/tls/clientKeyFile")).toString();
    tls.insecureSkipVerify =
        m_settings->value(QStringLiteral("conn/tls/insecureSkipVerify"), false).toBool();
    return tls;
}

bool DaemonConnectionService::configureTransport() {
    if (m_config.target.isEmpty()) {
        return false;
    }
    if (m_config.mode == QStringLiteral("remote")) {
        QString host;
        quint16 port = 0;
        if (!parseHostPort(m_config.target, &host, &port)) {
            return false;
        }
        m_transport->setTcpTarget(host, port, tlsConfigFromSettings());
    } else if (m_config.mode == QStringLiteral("remote-ws")) {
        const QUrl url = parseWsUrl(m_config.target);
        if (!url.isValid()) {
            return false;
        }
        m_transport->setWsTarget(url);
    } else if (m_config.mode == QStringLiteral("local")) {
        m_transport->setSocketPath(m_config.target);
    } else {
        return false;
    }
    // Seed credentials: an explicit/stored token takes the AuthResume fast-path; username/password
    // arrive later via login() for interactive SCRAM.
    NodeApiClient::AuthCredentials creds;
    creds.tlsActive = m_transport->tlsActive();
    creds.hasClientCert = m_transport->clientCertConfigured();
    if (!m_sessionToken.isEmpty()) {
        creds.resumeToken = m_sessionToken;
    } else if (m_settings != nullptr) {
        creds.resumeToken = m_settings->connectionToken(m_config.target);
        m_sessionToken = creds.resumeToken;
    }
    m_client->setAuthCredentials(creds);
    return true;
}

void DaemonConnectionService::connectTo(const QString& mode, const QString& target,
                                        const QString& token) {
    m_config.mode = mode;
    m_config.target = target;
    if (!token.isEmpty()) {
        m_sessionToken = token; // an explicit token from the picker drives AuthResume
    }
    m_authBlocked = false;
    // A user-initiated connect gets a fresh version-gate budget (new target, or a deliberate
    // retry after the stale daemon was dealt with).
    m_versionHold = false;
    m_versionRespawnAttempted = false;
    emit configChanged();

#ifdef Q_OS_WASM
    // A browser build has exactly one usable transport: the WebSocket mux ("remote-ws"). Unix
    // sockets, raw TLS TCP, and managed spawn do not exist on wasm - refuse anything else up
    // front instead of failing deep inside a stubbed carrier.
    if (mode != QStringLiteral("remote-ws")) {
        setStatusMessage(tr("Only WebSocket connections (ws:// or wss://) work in a browser."));
        setState(QStringLiteral("needs setup"));
        return;
    }
#endif

    if (!configureTransport()) {
        setState(QStringLiteral("needs setup"));
        return;
    }

    setStatusMessage(QString());
    setState(QStringLiteral("connecting"));

    // Managed local daemon (CON-1b): ensure a daemon is running before attaching. ensureRunning is
    // probe-first, so an already-running daemon is reused rather than duplicated; on success it
    // signals back and we send the Health probe. Liveness is resolved by the Health response
    // handler wired in the constructor. (Remote nodes are never auto-spawned.)
    if (mode == QStringLiteral("local") && m_launcher != nullptr && m_settings != nullptr &&
        m_settings->managedLocalDaemon()) {
        m_launcher->ensureRunning(target);
        return;
    }

    sendHealthProbe();
}

void DaemonConnectionService::login(const QString& username, const QString& password) {
    // Interactive credentials for a node in the "authenticating" state. Use SCRAM (not a stale
    // token), so clear any resume token and re-handshake on a fresh connection.
    m_authBlocked = false;
    m_versionHold = false;
    m_sessionToken.clear();
    NodeApiClient::AuthCredentials creds;
    creds.username = username;
    creds.password = password;
    creds.tlsActive = m_transport->tlsActive();
    creds.hasClientCert = m_transport->clientCertConfigured();
    m_client->setAuthCredentials(creds);
    // Drop the prior (unauthenticated) connection so the next send re-handshakes from Hello.
    m_transport->close();
    setStatusMessage(QString());
    setState(QStringLiteral("connecting"));
    sendHealthProbe();
}

void DaemonConnectionService::logout() {
    if (m_settings != nullptr && !m_config.target.isEmpty()) {
        m_settings->clearConnectionToken(m_config.target);
    }
    m_sessionToken.clear();
    m_client->clearAuthCredentials();
    disconnect();
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
    bool ok = false;
    QString message;
#ifdef Q_OS_WASM
    // Mirror connectTo()'s wasm gate so Test tells the same truth Connect would.
    if (mode != QStringLiteral("remote-ws")) {
        emit testResult(false,
                        QStringLiteral("Only WebSocket connections (ws:// or wss://) work in a "
                                       "browser"));
        setTesting(false);
        return;
    }
#endif
    if (mode == QStringLiteral("local")) {
        ok = !target.isEmpty();
        message = ok ? QStringLiteral("Unix socket target accepted")
                     : QStringLiteral("Enter a local socket path");
    } else if (mode == QStringLiteral("remote")) {
        QString host;
        quint16 port = 0;
        ok = parseHostPort(target, &host, &port);
        // Shape-only: a full reachability/TLS probe needs the server (verified end-to-end later).
        message = ok ? QStringLiteral("Remote target accepted (host:port, TLS)")
                     : QStringLiteral("Use host:port for a remote TLS node");
    } else if (mode == QStringLiteral("remote-ws")) {
        ok = parseWsUrl(target).isValid();
        // Shape-only, like "remote": reachability is proven by the Health probe on Connect.
        message = ok ? QStringLiteral("WebSocket target accepted (ws:// or wss://)")
                     : QStringLiteral("Use ws://host:port or wss://host[:port][/path]");
    } else {
        message = QStringLiteral("Unsupported transport");
    }
    emit testResult(ok, message);
    setTesting(false);
}

void DaemonConnectionService::shutdownManagedDaemon() {
    if (m_launcher != nullptr) {
        m_launcher->shutdownIfManaged();
    }
}

} // namespace daemonapp::daemon
