// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "settings/qt_settings_store.h"

#include <QDebug>

namespace settings {

QtSettingsStore::QtSettingsStore(QObject* parent)
    : ISettingsStore(parent),
      m_settings(QStringLiteral("daemon-app"), QStringLiteral("daemon-app")) {
    migrate();
}

void QtSettingsStore::migrate() {
    // `sync()` first so we also see values written by another process (e.g. the TUI).
    m_settings.sync();
    const QString versionKey = QStringLiteral("_meta/settingsSchemaVersion");
    int version = m_settings.value(versionKey, 0).toInt();

    if (version > kSettingsSchema) {
        // A newer build wrote these settings. Leave them untouched (defensive reads elsewhere clamp
        // unknown values) rather than risk corrupting state we don't understand.
        qWarning() << "settings: store is at schema" << version << "but this build supports"
                   << kSettingsSchema << "- leaving as-is";
        return;
    }

    // Ordered, forward-only migration steps. Each block transforms keys for one version bump, then
    // falls through to the next. Add future steps as `if (version < N) { ...; version = N; }`.
    //
    //   if (version < 2) {
    //       // e.g. rename a key:
    //       // if (m_settings.contains("old/key")) { m_settings.setValue("new/key",
    //       //     m_settings.value("old/key")); m_settings.remove("old/key"); }
    //       version = 2;
    //   }
    //
    // v0 -> v1 is the initial baseline: no transforms, just adopt the current layout.
    if (version < 1) {
        version = 1;
    }

    if (m_settings.value(versionKey, 0).toInt() != version) {
        m_settings.setValue(versionKey, version);
        m_settings.sync();
    }
}

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
