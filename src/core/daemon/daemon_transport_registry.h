#pragma once

#include "transports/itransport_registry.h"

namespace daemonapp::daemon {
class TransportRepository;
}

namespace transports {

// Daemon-backed, offline-first transport-adapter registry (Phase 6a, story 04). Projects the
// TransportRepository's node data into the ITransportRegistry rows the Channels surface renders:
// availableAdapters() from the live TransportAdapters list (the "Add channel" picker -
// connect-only, empty offline), instances() from the offline-first daemon_transport_instances cache
// (configured accounts, render with no connection). Rebuilds/re-emits on the repository's refresh
// signals.
//
// Lives under src/core/daemon (not transports/) because it depends on the daemon
// TransportRepository, which links the transports interface - keeping it here avoids a library
// cycle (cf. DaemonFleetTree).
class DaemonTransportRegistry : public ITransportRegistry {
    Q_OBJECT

public:
    explicit DaemonTransportRegistry(daemonapp::daemon::TransportRepository* repo,
                                     QObject* parent = nullptr);

    [[nodiscard]] QVariantList availableAdapters() const override;
    [[nodiscard]] QVariantList instances() const override;
    [[nodiscard]] QVariantList conversations(const QString& transport) const override;
    void refreshConversations(const QString& transport) override;

private:
    daemonapp::daemon::TransportRepository* m_repo = nullptr;
};

} // namespace transports
