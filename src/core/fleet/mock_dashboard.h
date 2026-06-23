#pragma once

#include "fleet/idashboard.h"

namespace uimodels {
class VariantListModel;
}

namespace fleet {

class ISessionRoster;
class IFleetTree;
class IApprovalsInbox;

// Dashboard mock that derives its counters live from the roster, fleet, and
// approvals seams (so approving a request or suspending a session updates the
// overview), plus a seeded recent-activity feed.
class MockDashboard : public IDashboard {
    Q_OBJECT

public:
    MockDashboard(ISessionRoster* roster, IFleetTree* fleet, IApprovalsInbox* approvals,
                  QObject* parent = nullptr);

    [[nodiscard]] int activeSessions() const override;
    [[nodiscard]] int runningAgents() const override;
    [[nodiscard]] int pendingApprovals() const override;
    [[nodiscard]] QString tokensToday() const override;
    [[nodiscard]] bool healthy() const override { return true; }
    [[nodiscard]] QObject* activity() const override;

private:
    // Prepend a recent-activity entry (newest-first) and notify.
    void prependActivity(const QString& text, const QString& kind);

    ISessionRoster* m_roster = nullptr;
    IFleetTree* m_fleet = nullptr;
    IApprovalsInbox* m_approvals = nullptr;
    uimodels::VariantListModel* m_activity = nullptr;
    int m_activitySeq = 1;
};

} // namespace fleet
