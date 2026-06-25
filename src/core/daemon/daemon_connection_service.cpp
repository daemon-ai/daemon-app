#include "daemon/daemon_connection_service.h"

#include "daemon/node_api_codec.h"

namespace daemonapp::daemon {

DaemonConnectionService::DaemonConnectionService(QObject* parent)
    : connection::IConnectionService(parent)
    , m_transport(std::make_unique<DaemonTransport>())
    , m_client(std::make_unique<NodeApiClient>(m_transport.get()))
{
    connect(m_client.get(), &NodeApiClient::responseReady, this,
            [this](const QString& correlationId, const QByteArray& responseCbor) {
                if (correlationId != QLatin1String(kHealthCorrelation)) {
                    return;
                }
                DecodedHealth health;
                setState(NodeApiCodec::decodeHealth(responseCbor, &health)
                             ? QStringLiteral("ready")
                             : QStringLiteral("offline"));
            });
    connect(m_client.get(), &NodeApiClient::failed, this,
            [this](const QString& correlationId, const QString&) {
                if (correlationId == QLatin1String(kHealthCorrelation)) {
                    setState(QStringLiteral("offline"));
                }
            });
}

void DaemonConnectionService::connectTo(const QString& mode, const QString& target,
                                        const QString& token)
{
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
    // Liveness is resolved by the Health response handler wired in the constructor.
    setState(QStringLiteral("connecting"));
    m_client->sendRequest(NodeApiCodec::encodeHealthRequest(),
                          QLatin1String(kHealthCorrelation));
}

void DaemonConnectionService::disconnect()
{
    m_transport->close();
    setState(QStringLiteral("offline"));
}

void DaemonConnectionService::testConnection(const QString& mode, const QString& target,
                                             const QString& token)
{
    Q_UNUSED(token)
    setTesting(true);
    const bool ok = mode == QStringLiteral("local") && !target.isEmpty();
    emit testResult(ok, ok ? QStringLiteral("Unix socket target accepted")
                           : QStringLiteral("Only local Unix-socket targets are supported"));
    setTesting(false);
}

} // namespace daemonapp::daemon
