// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace models {

// Provider/model discovery seam backing the node-driven "pick a provider, then pick a model it
// offers" flow (GUI ProfileEditor + first-run, TUI parity). The node OWNS enumeration: the daemon
// impl projects the `ProviderCatalog` op (providers) and the `ProviderModels` op (a provider's
// models) into rows; a mock impl serves the same shape for the mock/standalone path + offscreen
// tests.
//
// Selection-only: the model list is populated from the node, never free-text. Cloud providers list
// after a key (transient during first-run, or the profile's stored credential); local providers
// compose `[installed models] + "Discover More Models"` -> the existing model-download flow.
//
// Mock (the ProviderSelector) is NEVER offered in providers(); a Mock-valued profile still renders
// read-only elsewhere.
class IProviderCatalog : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IProviderCatalog() override = default;

    // The synthetic model-row id for the local "Discover More Models" affordance. QML/TUI treat a
    // row with this id as "open the download flow" rather than a selectable model.
    static constexpr const char* kDiscoverMoreId = "__discover_more__";

    // The providers the node offers, each a row:
    //   { id, name, kind ("local"|"cloud"|"daemon_cloud"), wireSelector (ProviderSelector),
    //     requiresKey (bool), supportsModelDiscovery (bool), cloud (bool, = kind != "local"),
    //     defaultBaseUrl (string, "" = none) }
    // `id` is the discovery key sent as ProviderModels.provider; `wireSelector` becomes the
    // persisted profile's ProviderSelector. Mock is excluded.
    [[nodiscard]] Q_INVOKABLE virtual QVariantList providers() const = 0;

    // The provider descriptor row for `providerId`, or an empty map when unknown.
    [[nodiscard]] Q_INVOKABLE virtual QVariantMap
    descriptorFor(const QString& providerId) const = 0;

    // Kick a model-list fetch for `providerId`. Cloud: pass the entered key as `transientKey`
    // (listing before a credential is stored, e.g. first-run) or the profile id as `credentialRef`
    // (an existing profile's stored key); Daemon Cloud lists keyless. Local: resolves synchronously
    // from the installed catalog + a "Discover More Models" row. offeredModelsChanged fires when
    // the list is ready.
    Q_INVOKABLE virtual void refreshModels(const QString& providerId,
                                           const QString& credentialRef = QString(),
                                           const QString& transientKey = QString()) = 0;

    // The selection-only model rows for `providerId`:
    //   { id, name, kind ("model"|"discover"), local (bool), provider (=providerId) }
    // Empty until refreshModels resolves for a cloud provider; a local provider always includes the
    // trailing "Discover More Models" row.
    [[nodiscard]] Q_INVOKABLE virtual QVariantList
    offeredModels(const QString& providerId) const = 0;

signals:
    // The provider list changed (a ProviderCatalog landed / the mock seeded).
    void providersChanged();
    // The offered-model list for `providerId` changed (a ProviderModels landed, a local download
    // finished, or a local compose ran).
    void offeredModelsChanged(const QString& providerId);
};

} // namespace models
