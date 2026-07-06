// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "automation/irouting_store.h"
#include "uimodels/variant_list_model.h"

namespace automation {

class MockRoutingStore : public IRoutingStore {
    Q_OBJECT

public:
    // `seedDemo` seeds the illustrative demo rules (the mock-mode default). Daemon mode passes
    // false: routing has NO node wire op yet, so it must render empty rather than fabricated rules
    // that would imply node-backed routing (render honesty).
    explicit MockRoutingStore(QObject* parent = nullptr, bool seedDemo = true);

    [[nodiscard]] QObject* rules() const override;
    [[nodiscard]] QStringList targets() const override;

    void setIntent(const QString& ruleId, const QString& intent) override;
    void setTarget(const QString& ruleId, const QString& target) override;
    void setFallback(const QString& ruleId, const QString& fallback) override;
    void setEnabled(const QString& ruleId, bool enabled) override;
    QString addRule(const QString& intent) override;
    void remove(const QString& ruleId) override;

private:
    void patch(const QString& ruleId, const QString& key, const QVariant& value);
    uimodels::VariantListModel* m_rules = nullptr;
    int m_nextId = 1;
};

} // namespace automation
