#pragma once

#include "connection/iconnection_service.h"
#include "daemon/daemon_transport.h"

#include <memory>

namespace daemonapp::daemon {

// Real connection seam for daemon-backed mode.
//
// This owns connection config/liveness and the Unix-socket transport. The full health probe is wired
// once the generated NodeApi codec can encode `Health`; for now it centralizes configuration and
// exposes the transport/client graph to repositories without changing GUI/TUI code.
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

private:
    std::unique_ptr<DaemonTransport> m_transport;
};

} // namespace daemonapp::daemon
