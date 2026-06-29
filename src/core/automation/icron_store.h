// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace automation {

// Cron seam backing the scheduled-jobs manager. Rows (VariantListModel): id,
// name, schedule (cron expr), profile, prompt, nextRun, lastRun, enabled.
class ICronStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* jobs READ jobs CONSTANT)

public:
    using QObject::QObject;
    ~ICronStore() override = default;

    [[nodiscard]] virtual QObject* jobs() const = 0;

    [[nodiscard]] Q_INVOKABLE virtual QVariantMap job(const QString& id) const = 0;

    Q_INVOKABLE virtual QString createJob(const QString& name, const QString& schedule,
                                          const QString& profile) = 0;
    Q_INVOKABLE virtual void updateJob(const QString& id, const QVariantMap& fields) = 0;
    Q_INVOKABLE virtual void setEnabled(const QString& id, bool enabled) = 0;
    Q_INVOKABLE virtual void runNow(const QString& id) = 0;
    Q_INVOKABLE virtual void remove(const QString& id) = 0;

signals:
    void changed();
};

} // namespace automation
