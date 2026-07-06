// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "automation/mock_routing_store.h"

namespace automation {
namespace {

QVariantMap mk(const QString& id, const QString& intent, const QString& target,
               const QString& fallback, bool enabled) {
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("intent")] = intent;
    m[QStringLiteral("target")] = target;
    m[QStringLiteral("fallback")] = fallback;
    m[QStringLiteral("enabled")] = enabled;
    return m;
}

} // namespace

MockRoutingStore::MockRoutingStore(QObject* parent, bool seedDemo)
    : IRoutingStore(parent), m_rules(new uimodels::VariantListModel(this)) {
    if (!seedDemo) {
        // Daemon mode: no routing wire op exists, so render empty rather than fabricated rules.
        return;
    }
    m_rules->upsert(mk(QStringLiteral("r-1"), QStringLiteral("Code & engineering"),
                       QStringLiteral("qwen2.5-coder-32b"), QStringLiteral("llama-3.1-8b-instruct"),
                       true));
    m_rules->upsert(mk(QStringLiteral("r-2"), QStringLiteral("Research & analysis"),
                       QStringLiteral("mixtral-8x7b"), QStringLiteral("llama-3.1-8b-instruct"),
                       true));
    m_rules->upsert(mk(QStringLiteral("r-3"), QStringLiteral("Quick chat / low latency"),
                       QStringLiteral("mistral-7b-instruct"), QStringLiteral("phi-3.5-mini"),
                       true));
    m_rules->upsert(mk(QStringLiteral("r-4"), QStringLiteral("Long-context summaries"),
                       QStringLiteral("llama-3.1-70b-instruct"), QStringLiteral("mixtral-8x7b"),
                       false));
    m_nextId = 5;
}

QObject* MockRoutingStore::rules() const {
    return m_rules;
}

QStringList MockRoutingStore::targets() const {
    return {QStringLiteral("llama-3.1-8b-instruct"), QStringLiteral("llama-3.1-70b-instruct"),
            QStringLiteral("mistral-7b-instruct"),   QStringLiteral("mixtral-8x7b"),
            QStringLiteral("qwen2.5-7b-instruct"),   QStringLiteral("qwen2.5-coder-32b"),
            QStringLiteral("gemma-2-9b-it"),         QStringLiteral("phi-3.5-mini")};
}

void MockRoutingStore::patch(const QString& ruleId, const QString& key, const QVariant& value) {
    const int row = m_rules->indexOfId(ruleId);
    if (row < 0) {
        return;
    }
    QVariantMap r = m_rules->at(row);
    r[key] = value;
    m_rules->upsert(r);
    emit changed();
}

void MockRoutingStore::setIntent(const QString& ruleId, const QString& intent) {
    patch(ruleId, QStringLiteral("intent"), intent);
}

void MockRoutingStore::setTarget(const QString& ruleId, const QString& target) {
    patch(ruleId, QStringLiteral("target"), target);
}

void MockRoutingStore::setFallback(const QString& ruleId, const QString& fallback) {
    patch(ruleId, QStringLiteral("fallback"), fallback);
}

void MockRoutingStore::setEnabled(const QString& ruleId, bool enabled) {
    patch(ruleId, QStringLiteral("enabled"), enabled);
}

QString MockRoutingStore::addRule(const QString& intent) {
    const QString id = QStringLiteral("r-%1").arg(m_nextId++);
    m_rules->upsert(mk(id, intent.isEmpty() ? QStringLiteral("New rule") : intent,
                       QStringLiteral("llama-3.1-8b-instruct"),
                       QStringLiteral("mistral-7b-instruct"), true));
    emit changed();
    return id;
}

void MockRoutingStore::remove(const QString& ruleId) {
    m_rules->removeById(ruleId);
    emit changed();
}

} // namespace automation
