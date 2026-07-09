// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "models/mock_provider_catalog.h"

#include "models/imodel_catalog.h"
#include "models/provider_model_compose.h"
#include "uimodels/variant_list_model.h"

namespace models {
namespace {

QVariantMap providerRow(const QString& id, const QString& name, const QString& kind,
                        const QString& wireSelector, bool requiresKey, const QString& defaultBase) {
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("name")] = name;
    m[QStringLiteral("kind")] = kind;
    m[QStringLiteral("wireSelector")] = wireSelector;
    m[QStringLiteral("requiresKey")] = requiresKey;
    m[QStringLiteral("supportsModelDiscovery")] = true;
    m[QStringLiteral("cloud")] = kind != QStringLiteral("local");
    m[QStringLiteral("defaultBaseUrl")] = defaultBase;
    return m;
}

QVariantMap modelRow(const QString& id, const QString& provider) {
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("name")] = id;
    m[QStringLiteral("kind")] = QStringLiteral("model");
    m[QStringLiteral("local")] = false;
    m[QStringLiteral("provider")] = provider;
    return m;
}

} // namespace

MockProviderCatalog::MockProviderCatalog(IModelCatalog* catalog, QObject* parent)
    : IProviderCatalog(parent), m_catalog(catalog) {
    if (m_catalog != nullptr) {
        // A finished local download makes a new model selectable: re-emit for the local providers.
        connect(m_catalog, &IModelCatalog::downloadFinished, this, [this](const QString&) {
            emit offeredModelsChanged(QStringLiteral("llama_cpp"));
            emit offeredModelsChanged(QStringLiteral("mistral_rs"));
        });
    }
}

QVariantList MockProviderCatalog::providers() const {
    // Daemon Cloud first (the new-profile default), then a couple of genai cloud vendors, then the
    // local engines. Mock is deliberately absent.
    QVariantList out = {
        // Daemon Cloud lists models keyless (see refreshModels), but running turns needs a Bearer
        // key, so requiresKey is true to match the corrected node semantics.
        providerRow(QStringLiteral("daemon_cloud"), QStringLiteral("Daemon Cloud"),
                    QStringLiteral("daemon_cloud"), QStringLiteral("daemon_api"), true,
                    QStringLiteral("https://api.daemon.ai/api/v1")),
        providerRow(QStringLiteral("anthropic"), QStringLiteral("Anthropic"),
                    QStringLiteral("cloud"), QStringLiteral("genai"), true, QString()),
        providerRow(QStringLiteral("openai"), QStringLiteral("OpenAI"), QStringLiteral("cloud"),
                    QStringLiteral("genai"), true, QString()),
        providerRow(QStringLiteral("llama_cpp"), QStringLiteral("Llama.cpp"),
                    QStringLiteral("local"), QStringLiteral("llama_cpp"), false, QString()),
        providerRow(QStringLiteral("mistral_rs"), QStringLiteral("Mistral.rs"),
                    QStringLiteral("local"), QStringLiteral("mistral_rs"), false, QString()),
    };
    // Persisted custom providers overlay onto the picker (a Daemon-Cloud-kind OpenAI gateway each).
    for (const QVariantMap& c : m_customProviders) {
        const QString name = c.value(QStringLiteral("displayName")).toString();
        out.append(providerRow(c.value(QStringLiteral("id")).toString(),
                               name.isEmpty() ? c.value(QStringLiteral("id")).toString() : name,
                               QStringLiteral("daemon_cloud"), QStringLiteral("daemon_api"),
                               c.value(QStringLiteral("requiresKey")).toBool(),
                               c.value(QStringLiteral("baseUrl")).toString()));
    }
    return out;
}

QVariantList MockProviderCatalog::customProviders() const {
    QVariantList out;
    for (const QVariantMap& c : m_customProviders) {
        out.append(c);
    }
    return out;
}

void MockProviderCatalog::createCustom(const QVariantMap& fields) {
    const QString id = fields.value(QStringLiteral("id")).toString().trimmed();
    if (id.isEmpty()) {
        return;
    }
    QVariantMap row;
    row[QStringLiteral("id")] = id;
    row[QStringLiteral("displayName")] = fields.value(QStringLiteral("displayName")).toString();
    row[QStringLiteral("baseUrl")] = fields.value(QStringLiteral("baseUrl")).toString().trimmed();
    row[QStringLiteral("requiresKey")] = fields.value(QStringLiteral("requiresKey")).toBool();
    row[QStringLiteral("credentialRef")] =
        fields.value(QStringLiteral("credentialRef")).toString().trimmed();
    row[QStringLiteral("source")] = QStringLiteral("user");
    // Upsert keyed by id.
    for (QVariantMap& existing : m_customProviders) {
        if (existing.value(QStringLiteral("id")).toString() == id) {
            existing = row;
            emit customProvidersChanged();
            emit providersChanged();
            return;
        }
    }
    m_customProviders.append(row);
    emit customProvidersChanged();
    emit providersChanged();
}

void MockProviderCatalog::updateCustom(const QVariantMap& fields) {
    createCustom(fields);
}

void MockProviderCatalog::removeCustom(const QString& id) {
    const auto before = m_customProviders.size();
    m_customProviders.removeIf(
        [&](const QVariantMap& c) { return c.value(QStringLiteral("id")).toString() == id; });
    if (m_customProviders.size() != before) {
        emit customProvidersChanged();
        emit providersChanged();
    }
}

QVariantMap MockProviderCatalog::descriptorFor(const QString& providerId) const {
    for (const QVariant& v : providers()) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toString() == providerId) {
            return m;
        }
    }
    return {};
}

bool MockProviderCatalog::isLocal(const QString& providerId) const {
    return descriptorFor(providerId).value(QStringLiteral("kind")).toString() ==
           QStringLiteral("local");
}

QString MockProviderCatalog::wireSelectorFor(const QString& providerId) const {
    return descriptorFor(providerId).value(QStringLiteral("wireSelector")).toString();
}

void MockProviderCatalog::refreshModels(const QString& providerId, const QString& credentialRef,
                                        const QString& transientKey) {
    Q_UNUSED(credentialRef)
    Q_UNUSED(transientKey)
    if (providerId.isEmpty()) {
        return;
    }
    if (!isLocal(providerId)) {
        // Seed a small static cloud model set the first time a provider is queried.
        if (!m_cloudModels.contains(providerId)) {
            QVariantList models;
            if (providerId == QStringLiteral("daemon_cloud")) {
                models.append(modelRow(QStringLiteral("anthropic/claude-3.5-sonnet"), providerId));
                models.append(modelRow(QStringLiteral("openai/gpt-4o"), providerId));
            } else if (providerId == QStringLiteral("anthropic")) {
                models.append(modelRow(QStringLiteral("claude-3-5-sonnet-latest"), providerId));
            } else if (providerId == QStringLiteral("openai")) {
                models.append(modelRow(QStringLiteral("gpt-4o"), providerId));
            }
            m_cloudModels.insert(providerId, models);
        }
    }
    emit offeredModelsChanged(providerId);
}

QVariantList MockProviderCatalog::offeredModels(const QString& providerId) const {
    if (isLocal(providerId)) {
        QList<QVariantMap> installedRows;
        if (m_catalog != nullptr) {
            if (auto* model = qobject_cast<uimodels::VariantListModel*>(m_catalog->installed())) {
                installedRows = model->rows();
            }
        }
        return composeLocalOfferedModels(installedRows, providerId, wireSelectorFor(providerId));
    }
    return m_cloudModels.value(providerId);
}

} // namespace models
