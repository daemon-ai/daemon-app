#pragma once

#include <QObject>
#include <QString>

namespace fleet {

// Dashboard seam backing the overview surface. Exposes summary counters (derived
// from the roster + approvals) plus a recent-activity feed (VariantListModel rows:
// time, text, kind).
class IDashboard : public QObject {
    Q_OBJECT
    Q_PROPERTY(int activeSessions READ activeSessions NOTIFY changed)
    Q_PROPERTY(int runningAgents READ runningAgents NOTIFY changed)
    Q_PROPERTY(int pendingApprovals READ pendingApprovals NOTIFY changed)
    Q_PROPERTY(QString tokensToday READ tokensToday NOTIFY changed)
    Q_PROPERTY(bool healthy READ healthy NOTIFY changed)
    Q_PROPERTY(QObject* activity READ activity CONSTANT)

public:
    using QObject::QObject;
    ~IDashboard() override = default;

    [[nodiscard]] virtual int activeSessions() const = 0;
    [[nodiscard]] virtual int runningAgents() const = 0;
    [[nodiscard]] virtual int pendingApprovals() const = 0;
    [[nodiscard]] virtual QString tokensToday() const = 0;
    [[nodiscard]] virtual bool healthy() const = 0;
    [[nodiscard]] virtual QObject* activity() const = 0;

signals:
    void changed();
};

} // namespace fleet
