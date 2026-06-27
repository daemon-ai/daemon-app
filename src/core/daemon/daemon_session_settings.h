#pragma once

#include "session/mock_session_settings.h"

namespace daemonapp::daemon {
class NodeApiClient;

// Daemon-backed per-session settings (CHA-4). Profile/effort/fast/verbose stay in-memory (they are
// composer UI overrides not yet mapped to a wire op), but setApprovalMode additionally sends
// SetSessionMode so the node's SessionOverlay.approval_mode tracks the popover. Subclassing the
// mock keeps the in-memory bookkeeping in one place.
class DaemonSessionSettings : public session::MockSessionSettings {
    Q_OBJECT

public:
    explicit DaemonSessionSettings(NodeApiClient* client, QObject* parent = nullptr);

    void setApprovalMode(const QString& mode) override;

private:
    NodeApiClient* m_client = nullptr;
};

} // namespace daemonapp::daemon
