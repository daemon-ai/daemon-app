// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace models {

// Filter installed-model rows to those served by `provider` (a ProviderSelector wire string:
// "genai" | "daemon_api" | "llama_cpp" | "mistral_rs"). Installed rows tag their provider
// inconsistently - local models carry the engine ("llama" | "mistral_rs"), remote models carry the
// selector string - so the local engine name is normalized to its selector before matching. Returns
// the matching ids in row order; an empty `provider` yields an empty list (the caller decides the
// fallback, e.g. "show everything"). This is a pure helper so it is unit-testable without a live
// catalog.
[[nodiscard]] inline QStringList filterInstalledIdsByProvider(const QList<QVariantMap>& rows,
                                                              const QString& provider) {
    QStringList out;
    if (provider.isEmpty()) {
        return out;
    }
    for (const QVariantMap& row : rows) {
        QString kind = row.value(QStringLiteral("provider")).toString();
        if (kind == QStringLiteral("llama")) {
            kind = QStringLiteral("llama_cpp"); // local llama.cpp engine -> its selector
        }
        if (kind == provider) {
            out << row.value(QStringLiteral("id")).toString();
        }
    }
    return out;
}

} // namespace models
