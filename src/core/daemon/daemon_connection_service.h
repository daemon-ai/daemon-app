#pragma once

#include "connection/iconnection_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/local_daemon_launcher.h"
#include "daemon/node_api_client.h"

#include <memory>

namespace settings {
class ISettingsStore;
}

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

    [[nodiscard]] DaemonTransport* transport() const { return m_transport.get(); }
    [[nodiscard]] NodeApiClient* client() const { return m_client.get(); }

    // Terminate a daemon this service spawned, honoring the shutdown-on-exit preference. Called
    // from the front ends' teardown. No-op when nothing was spawned or the user keeps it
    // persistent.
    void shutdownManagedDaemon();

private:
    static constexpr auto kHealthCorrelation = "connection/health";

    void sendHealthProbe();

    std::unique_ptr<DaemonTransport> m_transport;
    std::unique_ptr<NodeApiClient> m_client;
    settings::ISettingsStore* m_settings = nullptr;
    LocalDaemonLauncher* m_launcher = nullptr;
};

} // namespace daemonapp::daemon
