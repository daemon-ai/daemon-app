// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>

namespace fleet {

// Approvals inbox seam backing the Approvals surface. Pending rows
// (VariantListModel): id, session, tool, command, risk ("low"/"medium"/"high"),
// requested (human string). approve/deny resolve a request.
class IApprovalsInbox : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* pending READ pending CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY changed)

public:
    using QObject::QObject;
    ~IApprovalsInbox() override = default;

    [[nodiscard]] virtual QObject* pending() const = 0;
    [[nodiscard]] virtual int count() const = 0;

    Q_INVOKABLE virtual void approve(const QString& id) = 0;
    Q_INVOKABLE virtual void deny(const QString& id) = 0;

signals:
    void changed();
};

} // namespace fleet
