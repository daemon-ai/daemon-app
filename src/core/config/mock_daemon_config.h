#pragma once

#include "config/idaemon_config.h"

#include <QHash>

namespace config {

// In-memory IDaemonConfig seeded with believable node defaults, so every settings
// section is fully interactive without a daemon. Not persisted (the real daemon
// owns persistence); enumerated options for the combo-backed keys are baked in.
class MockDaemonConfig : public IDaemonConfig {
    Q_OBJECT

public:
    explicit MockDaemonConfig(QObject* parent = nullptr);

    [[nodiscard]] QVariant value(const QString& key, const QVariant& fallback = {}) const override;
    void setValue(const QString& key, const QVariant& value) override;
    [[nodiscard]] QStringList options(const QString& key) const override;

private:
    // Persist the full key/value map to the last-known on-disk cache.
    void save() const;

    QHash<QString, QVariant> m_values;
    QHash<QString, QStringList> m_options;
};

} // namespace config
