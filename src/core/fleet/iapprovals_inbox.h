// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>

namespace fleet {

// Approvals inbox seam backing the Approvals surface. Pending rows
// (VariantListModel): id, session, tool, command, risk ("low"/"medium"/"high"),
// requested (human string), canAllowPermanent (bool; the node can remember this approval
// permanently). approve/deny resolve a request; approve's allowPermanent (wire v28) asks the node
// to remember the approved command's fingerprint for the session and is only meaningful when the
// row's canAllowPermanent is true.
class IApprovalsInbox : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* pending READ pending CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY changed)

public:
    using QObject::QObject;
    ~IApprovalsInbox() override = default;

    [[nodiscard]] virtual QObject* pending() const = 0;
    [[nodiscard]] virtual int count() const = 0;

    Q_INVOKABLE virtual void approve(const QString& id, bool allowPermanent = false) = 0;
    // [wave2:app-approvals-safety] D3: `reason` (wire v29) rides ApprovalDecide.reason — the
    // operator deny explanation the node threads to the model. Empty = a plain deny (absent).
    Q_INVOKABLE virtual void deny(const QString& id, const QString& reason = QString()) = 0;

signals:
    void changed();
};

} // namespace fleet
