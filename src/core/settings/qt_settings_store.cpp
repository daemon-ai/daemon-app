// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "settings/qt_settings_store.h"

#include <algorithm>
#include <QCryptographicHash>
#include <QDebug>
#include <QStandardPaths>
#include <QStringList>
#include <utility>

#ifdef DAEMON_APP_HAVE_KEYCHAIN
#include <QEventLoop>
#include <qt6keychain/keychain.h>
#endif

namespace settings {

namespace {
// The keychain "service"/collection these tokens live under.
QString keychainService() {
    return QStringLiteral("daemon-app");
}

// A stable, filesystem/keyring-safe key for a target (mirrors ISettingsStore::tokenKey).
QString keychainKey(const QString& target) {
    const QByteArray h =
        QCryptographicHash::hash(target.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QStringLiteral("conn-token-") + QString::fromLatin1(h.left(16));
}
} // namespace

bool QtSettingsStore::keychainEnabled() {
#ifdef DAEMON_APP_HAVE_KEYCHAIN
    // Skip the keychain in test mode / when explicitly opted out, so headless CI uses the
    // deterministic QSettings fallback rather than a (possibly absent) Secret Service.
    return !(QStandardPaths::isTestModeEnabled() ||
             qEnvironmentVariableIsSet("DAEMON_APP_NO_KEYCHAIN"));
#else
    return false;
#endif
}

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
    version = std::max(version, 1);

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

void QtSettingsStore::setValues(const QVariantMap& values) {
    QStringList changedKeys;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        if (m_settings.value(it.key()) == it.value()) {
            continue;
        }
        m_settings.setValue(it.key(), it.value());
        changedKeys << it.key();
    }
    if (changedKeys.isEmpty()) {
        return;
    }
    m_settings.sync(); // one flush for the whole batch
    for (const QString& key : std::as_const(changedKeys)) {
        emit changed(key);
    }
    emit changedAny();
}

QString QtSettingsStore::connectionToken(const QString& target) const {
    if (target.isEmpty()) {
        return {};
    }
#ifdef DAEMON_APP_HAVE_KEYCHAIN
    if (keychainEnabled()) {
        QKeychain::ReadPasswordJob job(keychainService());
        job.setAutoDelete(false);
        job.setKey(keychainKey(target));
        QEventLoop loop;
        QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
        job.start();
        loop.exec();
        if (job.error() == QKeychain::NoError) {
            return job.textData();
        }
        if (job.error() == QKeychain::EntryNotFound) {
            return {}; // no token stored for this target
        }
        // Backend unavailable / other error: fall through to the QSettings fallback below.
        qWarning() << "settings: keychain read failed, using QSettings fallback:"
                   << job.errorString();
    }
#endif
    return ISettingsStore::connectionToken(target);
}

void QtSettingsStore::setConnectionToken(const QString& target, const QString& token) {
    if (target.isEmpty()) {
        return;
    }
#ifdef DAEMON_APP_HAVE_KEYCHAIN
    if (keychainEnabled()) {
        QKeychain::WritePasswordJob job(keychainService());
        job.setAutoDelete(false);
        job.setKey(keychainKey(target));
        job.setTextData(token);
        QEventLoop loop;
        QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
        job.start();
        loop.exec();
        if (job.error() == QKeychain::NoError) {
            // Don't also keep it in QSettings (the keychain is authoritative when present).
            ISettingsStore::clearConnectionToken(target);
            return;
        }
        qWarning() << "settings: keychain write failed, using QSettings fallback:"
                   << job.errorString();
    }
#endif
    ISettingsStore::setConnectionToken(target, token);
}

void QtSettingsStore::clearConnectionToken(const QString& target) {
    if (target.isEmpty()) {
        return;
    }
#ifdef DAEMON_APP_HAVE_KEYCHAIN
    if (keychainEnabled()) {
        QKeychain::DeletePasswordJob job(keychainService());
        job.setAutoDelete(false);
        job.setKey(keychainKey(target));
        QEventLoop loop;
        QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
        job.start();
        loop.exec();
        // Best-effort; ignore errors (also clear the QSettings fallback below).
    }
#endif
    ISettingsStore::clearConnectionToken(target);
}

} // namespace settings
