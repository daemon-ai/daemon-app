// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "fleet/iapprovals_inbox.h"

#include <QHash>
#include <QSet>
#include <QString>

namespace uimodels {
class VariantListModel;
}

namespace daemonapp::daemon {
class ApprovalRepository;

// PRO-11: daemon-backed approvals inbox. Projects the ApprovalRepository's decoded ApprovalInfo
// list into the VariantListModel rows the Approvals surface renders, and resolves approve(id) /
// deny(id) by sending ApprovalDecide for the row's (session, request_id). The row id is the
// ApprovalInfo string request_id (the inbox key, distinct from the in-stream uint gate the turn
// engine resolves with Respond).
class DaemonApprovalsInbox : public fleet::IApprovalsInbox {
    Q_OBJECT

public:
    explicit DaemonApprovalsInbox(ApprovalRepository* repository, QObject* parent = nullptr);

    [[nodiscard]] QObject* pending() const override;
    [[nodiscard]] int count() const override;

    void approve(const QString& id, bool allowPermanent = false) override;
    void deny(const QString& id) override;

private:
    void rebuild();

    ApprovalRepository* m_repository = nullptr;
    uimodels::VariantListModel* m_pending = nullptr;
    QHash<QString, QString> m_sessionByRequest; // request_id -> session (for ApprovalDecide)
    // request_ids the node can remember permanently (ApprovalInfo carried a fingerprint). An
    // "allow permanently" is only forwarded for these — belt-and-suspenders with the node, which
    // degrades a fingerprint-less permanent decision to a single allow.
    QSet<QString> m_permanentOfferable;
};

} // namespace daemonapp::daemon
