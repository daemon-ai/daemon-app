#include "daemon/daemon_connection_service.h"

namespace daemonapp::daemon {

DaemonConnectionService::DaemonConnectionService(QObject* parent)
    : connection::IConnectionService(parent)
    , m_transport(std::make_unique<DaemonTransport>())
{
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

    m_transport->setSocketPath(target);
    // The first real slice will replace this optimistic state with an actual Health request.
    setState(target.isEmpty() ? QStringLiteral("needs setup") : QStringLiteral("ready"));
}

void DaemonConnectionService::disconnect()
{
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
