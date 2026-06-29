// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "settings/qt_settings_store.h"

namespace settings {

QtSettingsStore::QtSettingsStore(QObject* parent)
    : ISettingsStore(parent),
      m_settings(QStringLiteral("daemon-app"), QStringLiteral("daemon-app")) {}

QVariant QtSettingsStore::value(const QString& key, const QVariant& fallback) const {
    return m_settings.value(key, fallback);
}

void QtSettingsStore::setValue(const QString& key, const QVariant& value) {
    if (m_settings.value(key) == value) {
        return;
    }
    m_settings.setValue(key, value);
    m_settings.sync();
    emit changed(key);
    emit changedAny();
}

} // namespace settings
