// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "fleet/idashboard.h"

namespace uimodels {
class VariantListModel;
}
namespace connection {
class IConnectionService;
}

namespace fleet {

class ISessionRoster;
class IFleetTree;
class IApprovalsInbox;

// Daemon-backed dashboard: derives its counters live from the daemon-backed roster + fleet +
// approvals seams, and `healthy` from the connection service. The activity feed is client-derived
// (prepended when a session is closed / an approval resolved), since there is no node-side
// dashboard feed yet. `tokensToday` is degraded (no per-session token data client-side; a future
// Stats/ telemetry feed can populate it).
class DaemonDashboard : public IDashboard {
    Q_OBJECT

public:
    DaemonDashboard(ISessionRoster* roster, IFleetTree* fleet, IApprovalsInbox* approvals,
                    connection::IConnectionService* connection, QObject* parent = nullptr);

    [[nodiscard]] int activeSessions() const override;
    [[nodiscard]] int runningAgents() const override;
    [[nodiscard]] int pendingApprovals() const override;
    [[nodiscard]] QString tokensToday() const override;
    [[nodiscard]] bool healthy() const override;
    [[nodiscard]] QObject* activity() const override;

private:
    void prependActivity(const QString& text, const QString& kind);

    ISessionRoster* m_roster = nullptr;
    IFleetTree* m_fleet = nullptr;
    IApprovalsInbox* m_approvals = nullptr;
    connection::IConnectionService* m_connection = nullptr;
    uimodels::VariantListModel* m_activity = nullptr;
    int m_activitySeq = 1;
};

} // namespace fleet
