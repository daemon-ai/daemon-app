#include "daemon/daemon_approvals_inbox.h"

#include "daemon/repositories.h"
#include "uimodels/variant_list_model.h"

namespace daemonapp::daemon {

DaemonApprovalsInbox::DaemonApprovalsInbox(ApprovalRepository* repository, QObject* parent)
    : fleet::IApprovalsInbox(parent), m_repository(repository),
      m_pending(new uimodels::VariantListModel(this)) {
    connect(m_pending, &uimodels::VariantListModel::countChanged, this,
            &fleet::IApprovalsInbox::changed);
    if (m_repository != nullptr) {
        connect(m_repository, &ApprovalRepository::pendingRefreshed, this,
                &DaemonApprovalsInbox::rebuild);
        rebuild();
    }
}

QObject* DaemonApprovalsInbox::pending() const {
    return m_pending;
}

int DaemonApprovalsInbox::count() const {
    return m_pending->count();
}

void DaemonApprovalsInbox::rebuild() {
    m_sessionByRequest.clear();
    QList<QVariantMap> rows;
    rows.reserve(m_repository->pending().size());
    for (const DecodedApprovalInfo& info : m_repository->pending()) {
        m_sessionByRequest.insert(info.requestId, info.session);
        QVariantMap row;
        row[QStringLiteral("id")] = info.requestId;
        row[QStringLiteral("session")] = info.session;
        row[QStringLiteral("tool")] = QStringLiteral("approval");
        row[QStringLiteral("command")] = info.hasPath ? info.path : info.prompt;
        // The wire carries no risk classification; surface the request reason and a neutral tier.
        row[QStringLiteral("risk")] = QStringLiteral("medium");
        row[QStringLiteral("requested")] = info.prompt;
        rows.append(row);
    }
    m_pending->setRows(rows);
}

void DaemonApprovalsInbox::approve(const QString& id) {
    if (m_repository != nullptr) {
        m_repository->decide(m_sessionByRequest.value(id), id, /*allow=*/true);
    }
}

void DaemonApprovalsInbox::deny(const QString& id) {
    if (m_repository != nullptr) {
        m_repository->decide(m_sessionByRequest.value(id), id, /*allow=*/false);
    }
}

} // namespace daemonapp::daemon
