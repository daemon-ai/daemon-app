#pragma once

#include "settings/isettings_store.h"

#include <QSettings>

namespace settings {

// QSettings-backed ISettingsStore. Uses the same ("daemon-app","daemon-app")
// organisation/application the TUI already reads ui/theme from, so the new shared
// keys live alongside the existing appearance keys and survive restarts.
class QtSettingsStore : public ISettingsStore {
    Q_OBJECT

public:
    explicit QtSettingsStore(QObject* parent = nullptr);

    [[nodiscard]] QVariant value(const QString& key, const QVariant& fallback = {}) const override;
    void setValue(const QString& key, const QVariant& value) override;

private:
    mutable QSettings m_settings;
};

} // namespace settings
