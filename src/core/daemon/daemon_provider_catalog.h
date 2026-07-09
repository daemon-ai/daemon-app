// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "models/iprovider_catalog.h"

#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace models {
class IModelCatalog;
}

namespace daemonapp::daemon {

class ProviderRepository;

// Daemon-backed provider/model discovery: projects the node's ProviderCatalog (providers) and
// ProviderModels (a provider's models) into the IProviderCatalog rows the GUI/TUI bind. Cloud
// providers list over the wire (keyless for Daemon Cloud, or with a transient/stored key); local
// providers compose `[installed] + Discover More` from the shared IModelCatalog and a finished
// download re-offers the new model. Mock is never surfaced.
class DaemonProviderCatalog : public models::IProviderCatalog {
    Q_OBJECT

public:
    DaemonProviderCatalog(ProviderRepository* providers, models::IModelCatalog* catalog,
                          QObject* parent = nullptr);

    [[nodiscard]] QVariantList providers() const override;
    [[nodiscard]] QVariantMap descriptorFor(const QString& providerId) const override;
    void refreshModels(const QString& providerId, const QString& credentialRef = QString(),
                       const QString& transientKey = QString()) override;
    [[nodiscard]] QVariantList offeredModels(const QString& providerId) const override;

    [[nodiscard]] QVariantList customProviders() const override;
    void createCustom(const QVariantMap& fields) override;
    void updateCustom(const QVariantMap& fields) override;
    void removeCustom(const QString& id) override;

private:
    [[nodiscard]] bool isLocal(const QString& providerId) const;
    [[nodiscard]] QString wireSelectorFor(const QString& providerId) const;

    ProviderRepository* m_providers = nullptr;
    models::IModelCatalog* m_catalog = nullptr;
};

} // namespace daemonapp::daemon
