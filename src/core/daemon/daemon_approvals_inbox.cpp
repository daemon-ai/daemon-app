// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

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
    m_permanentOfferable.clear();
    QList<QVariantMap> rows;
    rows.reserve(m_repository->pending().size());
    for (const DecodedApprovalInfo& info : m_repository->pending()) {
        m_sessionByRequest.insert(info.requestId, info.session);
        if (info.hasFingerprint) {
            m_permanentOfferable.insert(info.requestId);
        }
        QVariantMap row;
        row[QStringLiteral("id")] = info.requestId;
        row[QStringLiteral("session")] = info.session;
        row[QStringLiteral("tool")] = QStringLiteral("approval");
        row[QStringLiteral("command")] = info.hasPath ? info.path : info.prompt;
        // The wire carries no risk classification; surface the request reason and a neutral tier.
        row[QStringLiteral("risk")] = QStringLiteral("medium");
        row[QStringLiteral("requested")] = info.prompt;
        // Wire v28: offer "allow permanently" only when the node attached a fingerprint it can
        // remember (see DecodedApprovalInfo::hasFingerprint).
        row[QStringLiteral("canAllowPermanent")] = info.hasFingerprint;
        rows.append(row);
    }
    m_pending->setRows(rows);
}

void DaemonApprovalsInbox::approve(const QString& id, bool allowPermanent) {
    if (m_repository != nullptr) {
        // Forward "allow permanently" (wire v28) only for an approval the node offered to remember
        // (carried a fingerprint); otherwise it stays absent = one-shot allow. The node also
        // degrades a fingerprint-less permanent decision to a single allow, so this is defensive.
        const bool permanent = allowPermanent && m_permanentOfferable.contains(id);
        m_repository->decide(m_sessionByRequest.value(id), id, /*allow=*/true, permanent);
    }
}

void DaemonApprovalsInbox::deny(const QString& id) {
    if (m_repository != nullptr) {
        m_repository->decide(m_sessionByRequest.value(id), id, /*allow=*/false);
    }
}

} // namespace daemonapp::daemon
