#pragma once

#include "connection/iconnection_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"

#include <memory>

namespace daemonapp::daemon {

// Real connection seam for daemon-backed mode.
//
// Owns connection config/liveness, the persistent Unix-socket transport, and the NodeApiClient that
// repositories share. Liveness is driven by a real `Health` request: connectTo() sends Health and
// the liveness state machine resolves to "ready" only once a Health response decodes, or "offline"
// if the probe fails. The transport/client are exposed so the service graph can hand the same
// client to repositories rather than building a parallel one.
class DaemonConnectionService : public connection::IConnectionService {
    Q_OBJECT

public:
    explicit DaemonConnectionService(QObject* parent = nullptr);

    void connectTo(const QString& mode, const QString& target,
                   const QString& token = QString()) override;
    void disconnect() override;
    void testConnection(const QString& mode, const QString& target,
                        const QString& token = QString()) override;

    [[nodiscard]] DaemonTransport* transport() const { return m_transport.get(); }
    [[nodiscard]] NodeApiClient* client() const { return m_client.get(); }

private:
    static constexpr auto kHealthCorrelation = "connection/health";

    std::unique_ptr<DaemonTransport> m_transport;
    std::unique_ptr<NodeApiClient> m_client;
};

} // namespace daemonapp::daemon
