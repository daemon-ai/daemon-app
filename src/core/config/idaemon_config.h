// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace config {

// Daemon-authoritative configuration seam (distinct from the client-local
// ISettingsStore). These are node-side settings - default model, chat behaviour,
// safety/approval policy, memory/context, workspace, voice, advanced - that the
// daemon owns and the UI reads/writes over the wire. The mock holds an in-memory
// map seeded with believable defaults; a daemon adapter later reads/writes the
// same keys via IConnectionService.request without any UI change.
class IDaemonConfig : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IDaemonConfig() override = default;

    [[nodiscard]] Q_INVOKABLE virtual QVariant value(const QString& key,
                                                     const QVariant& fallback = {}) const = 0;
    Q_INVOKABLE virtual void setValue(const QString& key, const QVariant& value) = 0;
    // Enumerated choices for a key (e.g. safety/approvalPolicy), for combo rows.
    [[nodiscard]] Q_INVOKABLE virtual QStringList options(const QString& key) const = 0;

signals:
    void changed(const QString& key);
};

} // namespace config
