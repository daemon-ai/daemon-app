// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "models/iprovider_catalog.h"

#include <QHash>
#include <QVariantList>
#include <QVariantMap>

namespace models {

class IModelCatalog;

// In-memory provider/model discovery for the mock/standalone service mode + offscreen tests. Serves
// a representative provider list (Daemon Cloud + a couple of genai cloud vendors + the two local
// engines; never Mock) and a small static model set per cloud provider. Local providers compose
// `[installed] + Discover More` from the injected IModelCatalog (when present), matching the daemon
// seam's shape so the GUI/TUI + tests exercise the same flow with no live node.
class MockProviderCatalog : public IProviderCatalog {
    Q_OBJECT

public:
    explicit MockProviderCatalog(IModelCatalog* catalog = nullptr, QObject* parent = nullptr);

    [[nodiscard]] QVariantList providers() const override;
    [[nodiscard]] QVariantMap descriptorFor(const QString& providerId) const override;
    void refreshModels(const QString& providerId, const QString& credentialRef = QString(),
                       const QString& transientKey = QString()) override;
    [[nodiscard]] QVariantList offeredModels(const QString& providerId) const override;

private:
    [[nodiscard]] bool isLocal(const QString& providerId) const;
    [[nodiscard]] QString wireSelectorFor(const QString& providerId) const;

    IModelCatalog* m_catalog = nullptr;
    QHash<QString, QVariantList> m_cloudModels; // provider id -> static cloud model rows
};

} // namespace models
