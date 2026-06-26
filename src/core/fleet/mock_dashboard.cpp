#include "fleet/mock_dashboard.h"

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

QVariantMap mkActivity(const QString& time, const QString& text, const QString& kind) {
    QVariantMap m;
    m[QStringLiteral("id")] = time + text; // stable enough for a static feed
    m[QStringLiteral("time")] = time;
    m[QStringLiteral("text")] = text;
    m[QStringLiteral("kind")] = kind; // "info" | "approval" | "error"
    return m;
}

} // namespace

MockDashboard::MockDashboard(ISessionRoster* roster, IFleetTree* fleet, IApprovalsInbox* approvals,
                             QObject* parent)
    : IDashboard(parent), m_roster(roster), m_fleet(fleet), m_approvals(approvals),
      m_activity(new uimodels::VariantListModel(this)) {
    m_activity->upsert(mkActivity(QStringLiteral("just now"),
                                  QStringLiteral("Coder requested shell approval"),
                                  QStringLiteral("approval")));
    m_activity->upsert(mkActivity(QStringLiteral("2m"),
                                  QStringLiteral("Researcher finished web crawl"),
                                  QStringLiteral("info")));
    m_activity->upsert(mkActivity(QStringLiteral("8m"),
                                  QStringLiteral("Session 'Daily standup' suspended"),
                                  QStringLiteral("info")));
    m_activity->upsert(mkActivity(QStringLiteral("15m"),
                                  QStringLiteral("Model llama-3.1-8b activated"),
                                  QStringLiteral("info")));

    if (m_roster != nullptr) {
        connect(m_roster, &ISessionRoster::changed, this, &IDashboard::changed);
    }
    if (m_fleet != nullptr) {
        connect(m_fleet, &IFleetTree::changed, this, &IDashboard::changed);
    }
    if (m_approvals != nullptr) {
        connect(m_approvals, &IApprovalsInbox::changed, this, &IDashboard::changed);
    }

    // Append real activity on real user actions: closing a session and resolving
    // an approval are structural removals on the underlying models, so the feed
    // reflects what the user just did instead of a frozen seed.
    if (auto* sessions = qobject_cast<uimodels::VariantListModel*>(
            m_roster != nullptr ? m_roster->sessions() : nullptr)) {
        connect(sessions, &QAbstractItemModel::rowsRemoved, this, [this] {
            prependActivity(QStringLiteral("Session closed"), QStringLiteral("info"));
        });
    }
    if (auto* approvals = qobject_cast<uimodels::VariantListModel*>(
            m_approvals != nullptr ? m_approvals->pending() : nullptr)) {
        connect(approvals, &QAbstractItemModel::rowsRemoved, this, [this] {
            prependActivity(QStringLiteral("Approval resolved"), QStringLiteral("info"));
        });
    }
}

void MockDashboard::prependActivity(const QString& text, const QString& kind) {
    QList<QVariantMap> rows = m_activity->rows();
    QVariantMap row = mkActivity(QStringLiteral("now"), text, kind);
    // A unique id so successive same-text events stay distinct rows.
    row[QStringLiteral("id")] = QStringLiteral("act-%1").arg(m_activitySeq++);
    rows.prepend(row);
    m_activity->setRows(rows);
    emit changed();
}

int MockDashboard::activeSessions() const {
    return m_roster
               ? countWhere(m_roster->sessions(), QStringLiteral("state"), QStringLiteral("active"))
               : 0;
}

int MockDashboard::runningAgents() const {
    return m_fleet
               ? countWhere(m_fleet->nodes(), QStringLiteral("status"), QStringLiteral("running"))
               : 0;
}

int MockDashboard::pendingApprovals() const {
    return m_approvals ? m_approvals->count() : 0;
}

QString MockDashboard::tokensToday() const {
    // Derived from the live roster (sum of per-session token usage) rather than a
    // hardcoded figure, so closing/adding sessions moves the number.
    long long total = 0;
    if (auto* sessions = qobject_cast<uimodels::VariantListModel*>(
            m_roster != nullptr ? m_roster->sessions() : nullptr)) {
        for (const QVariantMap& row : sessions->rows()) {
            total += row.value(QStringLiteral("tokens")).toLongLong();
        }
    }
    if (total >= 1000000) {
        return QStringLiteral("%1M").arg(QString::number(total / 1000000.0, 'f', 2));
    }
    if (total >= 1000) {
        return QStringLiteral("%1k").arg(QString::number(total / 1000.0, 'f', 1));
    }
    return QString::number(total);
}

QObject* MockDashboard::activity() const {
    return m_activity;
}

} // namespace fleet
