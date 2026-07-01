// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "models/iprovider_catalog.h"
#include "models/provider_filter.h"

#include <QList>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace models {

// Compose the selection-only offered-model rows for a LOCAL provider from the installed-catalog
// rows: the installed models served by `wireSelector` (engine-normalized), followed by the
// synthetic "Discover More Models" row that opens the download flow. Pure so both the daemon + mock
// provider catalogs share it (no IModelCatalog coupling) and it is unit-testable. Empty installed
// set -> just the Discover row (a fresh machine).
[[nodiscard]] inline QVariantList composeLocalOfferedModels(const QList<QVariantMap>& installedRows,
                                                            const QString& providerId,
                                                            const QString& wireSelector) {
    QVariantList out;
    for (const QVariantMap& row : installedRows) {
        QString kind = row.value(QStringLiteral("provider")).toString();
        if (kind == QStringLiteral("llama")) {
            kind = QStringLiteral("llama_cpp"); // local llama.cpp engine -> its selector
        }
        if (kind != wireSelector) {
            continue;
        }
        const QString id = row.value(QStringLiteral("id")).toString();
        QVariantMap m;
        m[QStringLiteral("id")] = id;
        const QString name = row.value(QStringLiteral("name")).toString();
        m[QStringLiteral("name")] = name.isEmpty() ? id : name;
        m[QStringLiteral("kind")] = QStringLiteral("model");
        m[QStringLiteral("local")] = true;
        m[QStringLiteral("provider")] = providerId;
        out.append(m);
    }
    QVariantMap discover;
    discover[QStringLiteral("id")] = QString::fromLatin1(IProviderCatalog::kDiscoverMoreId);
    discover[QStringLiteral("name")] = QStringLiteral("Discover More Models");
    discover[QStringLiteral("kind")] = QStringLiteral("discover");
    discover[QStringLiteral("local")] = true;
    discover[QStringLiteral("provider")] = providerId;
    out.append(discover);
    return out;
}

} // namespace models
