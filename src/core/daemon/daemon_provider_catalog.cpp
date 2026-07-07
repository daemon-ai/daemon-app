// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_provider_catalog.h"

#include "daemon/repositories.h"
#include "models/imodel_catalog.h"
#include "models/provider_model_compose.h"
#include "uimodels/variant_list_model.h"

namespace daemonapp::daemon {

namespace {

QVariantMap providerRow(const DecodedProviderDescriptor& d) {
    QVariantMap m;
    m[QStringLiteral("id")] = d.id;
    m[QStringLiteral("name")] = d.displayName.isEmpty() ? d.id : d.displayName;
    m[QStringLiteral("kind")] = d.kind;
    m[QStringLiteral("wireSelector")] = d.wireSelector;
    m[QStringLiteral("requiresKey")] = d.requiresKey;
    m[QStringLiteral("supportsModelDiscovery")] = d.supportsModelDiscovery;
    m[QStringLiteral("cloud")] = d.kind != QStringLiteral("local");
    m[QStringLiteral("defaultBaseUrl")] = d.hasDefaultBaseUrl ? d.defaultBaseUrl : QString();
    // [waveB:app-v30] CON-15: node-supplied sign-in family + label (empty when none). The picker
    // shows a sign-in button iff signInFamily is non-empty — zero vendor knowledge client-side.
    m[QStringLiteral("signInFamily")] = d.hasSignIn ? d.signInFamily : QString();
    m[QStringLiteral("signInLabel")] = d.hasSignIn ? d.signInLabel : QString();
    return m;
}

QVariantMap cloudModelRow(const DecodedModelDescriptor& d, const QString& providerId) {
    QVariantMap m;
    m[QStringLiteral("id")] = d.id;
    m[QStringLiteral("name")] = d.hasDisplayName && !d.displayName.isEmpty() ? d.displayName : d.id;
    m[QStringLiteral("kind")] = QStringLiteral("model");
    m[QStringLiteral("local")] = d.local;
    m[QStringLiteral("provider")] = providerId;
    return m;
}

} // namespace

DaemonProviderCatalog::DaemonProviderCatalog(ProviderRepository* providers,
                                             models::IModelCatalog* catalog, QObject* parent)
    : models::IProviderCatalog(parent), m_providers(providers), m_catalog(catalog) {
    if (m_providers != nullptr) {
        connect(m_providers, &ProviderRepository::providersRefreshed, this,
                &models::IProviderCatalog::providersChanged);
        connect(m_providers, &ProviderRepository::providerModelsRefreshed, this,
                &models::IProviderCatalog::offeredModelsChanged);
        m_providers->refreshProviders();
    }
    if (m_catalog != nullptr) {
        // A finished local download makes a new model selectable: re-offer the local providers.
        connect(m_catalog, &models::IModelCatalog::downloadFinished, this, [this](const QString&) {
            emit offeredModelsChanged(QStringLiteral("llama_cpp"));
            emit offeredModelsChanged(QStringLiteral("mistral_rs"));
        });
    }
}

QVariantList DaemonProviderCatalog::providers() const {
    QVariantList out;
    if (m_providers == nullptr) {
        return out;
    }
    for (const DecodedProviderDescriptor& d : m_providers->providers()) {
        // Never offer Mock in the picker (a Mock-valued profile still renders read-only elsewhere).
        if (d.id == QStringLiteral("mock") || d.wireSelector == QStringLiteral("mock")) {
            continue;
        }
        out.append(providerRow(d));
    }
    return out;
}

QVariantMap DaemonProviderCatalog::descriptorFor(const QString& providerId) const {
    if (m_providers == nullptr) {
        return {};
    }
    for (const DecodedProviderDescriptor& d : m_providers->providers()) {
        if (d.id == providerId) {
            return providerRow(d);
        }
    }
    return {};
}

bool DaemonProviderCatalog::isLocal(const QString& providerId) const {
    return descriptorFor(providerId).value(QStringLiteral("kind")).toString() ==
           QStringLiteral("local");
}

QString DaemonProviderCatalog::wireSelectorFor(const QString& providerId) const {
    return descriptorFor(providerId).value(QStringLiteral("wireSelector")).toString();
}

void DaemonProviderCatalog::refreshModels(const QString& providerId, const QString& credentialRef,
                                          const QString& transientKey) {
    if (providerId.isEmpty()) {
        return;
    }
    if (isLocal(providerId)) {
        // Local models come from the installed catalog, not the wire; the compose is synchronous.
        emit offeredModelsChanged(providerId);
        return;
    }
    if (m_providers != nullptr) {
        m_providers->refreshProviderModels(providerId, credentialRef, transientKey);
    }
}

QVariantList DaemonProviderCatalog::offeredModels(const QString& providerId) const {
    if (isLocal(providerId)) {
        QList<QVariantMap> installedRows;
        if (m_catalog != nullptr) {
            if (auto* model = qobject_cast<uimodels::VariantListModel*>(m_catalog->installed())) {
                installedRows = model->rows();
            }
        }
        return models::composeLocalOfferedModels(installedRows, providerId,
                                                 wireSelectorFor(providerId));
    }
    QVariantList out;
    if (m_providers != nullptr) {
        for (const DecodedModelDescriptor& d : m_providers->modelsFor(providerId)) {
            out.append(cloudModelRow(d, providerId));
        }
    }
    return out;
}

} // namespace daemonapp::daemon
