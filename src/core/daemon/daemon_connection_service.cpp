#include "daemon/daemon_connection_service.h"

#include "daemon/node_api_codec.h"
#include "settings/isettings_store.h"

namespace daemonapp::daemon {

DaemonConnectionService::DaemonConnectionService(QObject* parent,
                                                 settings::ISettingsStore* settings)
    : connection::IConnectionService(parent), m_transport(std::make_unique<DaemonTransport>()),
      m_client(std::make_unique<NodeApiClient>(m_transport.get())), m_settings(settings) {
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
                    setStatusMessage(QString());
                    setState(QStringLiteral("ready"));
                } else {
                    setStatusMessage(tr("The daemon reported it is not healthy."));
                    setState(QStringLiteral("offline"));
                }
            });
    connect(m_client.get(), &NodeApiClient::failed, this,
            [this](const QString& correlationId, const QString&) {
                if (correlationId == QLatin1String(kHealthCorrelation)) {
                    setState(QStringLiteral("offline"));
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
