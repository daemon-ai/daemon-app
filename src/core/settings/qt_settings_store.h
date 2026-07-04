// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

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
    // Batched write: apply every changed key, then sync() once (one backend flush for the whole
    // batch instead of one per key). changed()/changedAny() still fire per changed key.
    void setValues(const QVariantMap& values) override;

    // Server-token storage prefers the OS keychain (Qt Keychain) when built with it and a keyring
    // is usable at runtime; otherwise it transparently falls back to the QSettings base
    // implementation. The password is never stored here - only the SERVER-issued session token.
    [[nodiscard]] QString connectionToken(const QString& target) const override;
    void setConnectionToken(const QString& target, const QString& token) override;
    void clearConnectionToken(const QString& target) override;

private:
    // Whether to attempt the OS keychain. False when built without Qt Keychain, in QStandardPaths
    // test mode, or when DAEMON_APP_NO_KEYCHAIN is set (keeps headless tests on the QSettings
    // path).
    [[nodiscard]] static bool keychainEnabled();
    // The settings schema version: bump ONLY when a key is renamed/retyped/removed (not on every
    // app release). `migrate()` runs an ordered, forward-only ladder up to this on construction.
    // Independent of the app version (app 0.4.1 may still be on settings schema 2).
    static constexpr int kSettingsSchema = 1;

    // Bring an older on-disk settings layout forward, before any read. Stamps
    // `_meta/settingsSchemaVersion`; safe to call on a fresh store.
    void migrate();

    mutable QSettings m_settings;
};

} // namespace settings
