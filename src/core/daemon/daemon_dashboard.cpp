#include "daemon/daemon_dashboard.h"

#include "connection/iconnection_service.h"
#include "fleet/iapprovals_inbox.h"
#include "fleet/ifleet_tree.h"
#include "fleet/isession_roster.h"
#include "uimodels/variant_list_model.h"

namespace fleet {
namespace {

int countWhere(QObject* model, const QString& key, const QString& value) {
    auto* vlm = qobject_cast<uimodels::VariantListModel*>(model);
    if (vlm == nullptr) {
        return 0;
    }
    int n = 0;
    for (const QVariantMap& row : vlm->rows()) {
        if (row.value(key).toString() == value) {
            ++n;
        }
    }
    return n;
}

} // namespace

DaemonDashboard::DaemonDashboard(ISessionRoster* roster, IFleetTree* fleet,
                                 IApprovalsInbox* approvals,
                                 connection::IConnectionService* connection, QObject* parent)
    : IDashboard(parent), m_roster(roster), m_fleet(fleet), m_approvals(approvals),
      m_connection(connection), m_activity(new uimodels::VariantListModel(this)) {
    // Counters recompute on any input change; healthy tracks the connection state.
    if (m_roster != nullptr) {
        connect(m_roster, &ISessionRoster::changed, this, &IDashboard::changed);
    }
    if (m_fleet != nullptr) {
        connect(m_fleet, &IFleetTree::changed, this, &IDashboard::changed);
    }
    if (m_approvals != nullptr) {
        connect(m_approvals, &IApprovalsInbox::changed, this, &IDashboard::changed);
    }
    if (m_connection != nullptr) {
        connect(m_connection, &connection::IConnectionService::stateChanged, this,
                &IDashboard::changed);
    }
    // Client-derived activity feed: a structural removal on the roster/approvals models is a
    // session close / approval resolve - reflect what the user just did (no node-side feed yet).
    if (auto* sessions = qobject_cast<uimodels::VariantListModel*>(
            m_roster != nullptr ? m_roster->sessions() : nullptr)) {
        connect(sessions, &QAbstractItemModel::rowsRemoved, this, [this] {
            prependActivity(QStringLiteral("Session closed"), QStringLiteral("info"));
        });
    }
    if (auto* approvalRows = qobject_cast<uimodels::VariantListModel*>(
            m_approvals != nullptr ? m_approvals->pending() : nullptr)) {
        connect(approvalRows, &QAbstractItemModel::rowsRemoved, this, [this] {
            prependActivity(QStringLiteral("Approval resolved"), QStringLiteral("info"));
        });
    }
}

void DaemonDashboard::prependActivity(const QString& text, const QString& kind) {
    QList<QVariantMap> rows = m_activity->rows();
    QVariantMap row;
    row[QStringLiteral("id")] = QStringLiteral("act-%1").arg(m_activitySeq++);
    row[QStringLiteral("time")] = QStringLiteral("now");
    row[QStringLiteral("text")] = text;
    row[QStringLiteral("kind")] = kind;
    rows.prepend(row);
    m_activity->setRows(rows);
    emit changed();
}

int DaemonDashboard::activeSessions() const {
    return m_roster != nullptr
               ? countWhere(m_roster->sessions(), QStringLiteral("state"), QStringLiteral("active"))
               : 0;
}

int DaemonDashboard::runningAgents() const {
    return m_fleet != nullptr
               ? countWhere(m_fleet->nodes(), QStringLiteral("status"), QStringLiteral("running"))
               : 0;
}

int DaemonDashboard::pendingApprovals() const {
    return m_approvals != nullptr ? m_approvals->count() : 0;
}

QString DaemonDashboard::tokensToday() const {
    // Degraded: the client has no per-session token usage yet (the roster reports tokens=0). A
    // future Stats/telemetry feed can populate this; until then show a neutral placeholder.
    return QStringLiteral("-");
}

bool DaemonDashboard::healthy() const {
    return m_connection != nullptr && m_connection->ready();
}

QObject* DaemonDashboard::activity() const {
    return m_activity;
}

} // namespace fleet
