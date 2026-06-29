// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace automation {

// Routing seam backing the routing matrix: intent/condition rules mapped to a
// target model (with a fallback), each toggleable. Rows (VariantListModel): id,
// intent, target, fallback, enabled.
class IRoutingStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* rules READ rules CONSTANT)

public:
    using QObject::QObject;
    ~IRoutingStore() override = default;

    [[nodiscard]] virtual QObject* rules() const = 0;

    // The target models a rule can route to.
    [[nodiscard]] Q_INVOKABLE virtual QStringList targets() const = 0;

    Q_INVOKABLE virtual void setIntent(const QString& ruleId, const QString& intent) = 0;
    Q_INVOKABLE virtual void setTarget(const QString& ruleId, const QString& target) = 0;
    Q_INVOKABLE virtual void setFallback(const QString& ruleId, const QString& fallback) = 0;
    Q_INVOKABLE virtual void setEnabled(const QString& ruleId, bool enabled) = 0;
    Q_INVOKABLE virtual QString addRule(const QString& intent) = 0;
    Q_INVOKABLE virtual void remove(const QString& ruleId) = 0;

signals:
    void changed();
};

} // namespace automation
