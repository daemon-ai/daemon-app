// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QCryptographicHash>
#include <QDir>
#include <QObject>
#include <QStandardPaths>
#include <QString>
#include <QtGlobal>
#include <QVariant>

namespace settings {

// Shared client-local preference store seam. App preferences (theme target,
// setupComplete, last connection) persist locally and work offline; both front
// ends bind one instance. A QSettings-backed implementation is used now; the
// daemon never owns these (it owns node-config, read over the wire instead).
//
// The store is intentionally a thin typed key/value bag plus a few named
// convenience accessors for the first-run / connection keys the gate needs, so a
// new section can persist a value without growing the interface.
class ISettingsStore : public QObject {
    Q_OBJECT
    // The first-run gate flips this once setup finishes; the shell mounts
    // straight to the main view on subsequent launches.
    Q_PROPERTY(bool setupComplete READ setupComplete WRITE setSetupComplete NOTIFY changedAny)
    // Managed local daemon (CON-1b) preferences, two-way bound by the settings UI.
    Q_PROPERTY(bool managedLocalDaemon READ managedLocalDaemon WRITE setManagedLocalDaemon NOTIFY
                   changedAny)
    Q_PROPERTY(bool managedDaemonShutdownOnExit READ managedDaemonShutdownOnExit WRITE
                   setManagedDaemonShutdownOnExit NOTIFY changedAny)
    Q_PROPERTY(
        QString daemonBinaryPath READ daemonBinaryPath WRITE setDaemonBinaryPath NOTIFY changedAny)

public:
    using QObject::QObject;
    ~ISettingsStore() override = default;

    [[nodiscard]] Q_INVOKABLE virtual QVariant value(const QString& key,
                                                     const QVariant& fallback = {}) const = 0;
    Q_INVOKABLE virtual void setValue(const QString& key, const QVariant& value) = 0;

    // Named convenience over the raw keys (so QML/TUI never hardcode key strings).
    [[nodiscard]] bool setupComplete() const {
        return value(QStringLiteral("app/setupComplete"), false).toBool();
    }
    void setSetupComplete(bool on) { setValue(QStringLiteral("app/setupComplete"), on); }

    [[nodiscard]] Q_INVOKABLE QString lastConnectionMode() const {
#ifdef Q_OS_WASM
        return value(QStringLiteral("conn/mode"), QStringLiteral("remote")).toString();
#else
        return value(QStringLiteral("conn/mode"), QStringLiteral("local")).toString();
#endif
    }
    [[nodiscard]] Q_INVOKABLE QString lastConnectionTarget() const {
        return value(QStringLiteral("conn/target"), QString()).toString();
    }
    Q_INVOKABLE void setLastConnection(const QString& mode, const QString& target) {
        setValue(QStringLiteral("conn/mode"), mode);
        setValue(QStringLiteral("conn/target"), target);
    }

    // Managed local daemon (CON-1b): when no daemon answers on the local target, the client may
    // discover + spawn one itself. On by default so first-run "Local" works without a terminal.
    [[nodiscard]] bool managedLocalDaemon() const {
#ifdef Q_OS_WASM
        return false;
#else
        return value(QStringLiteral("conn/managedLocalDaemon"), true).toBool();
#endif
    }
    void setManagedLocalDaemon(bool on) { setValue(QStringLiteral("conn/managedLocalDaemon"), on); }

    // Whether a daemon THIS client spawned is terminated when the client exits. Off by default: a
    // managed daemon is persistent (background wake/cron outlive the GUI/TUI). A daemon the client
    // only attached to is never affected by this preference.
    [[nodiscard]] bool managedDaemonShutdownOnExit() const {
        return value(QStringLiteral("conn/managedDaemonShutdownOnExit"), false).toBool();
    }
    void setManagedDaemonShutdownOnExit(bool on) {
        setValue(QStringLiteral("conn/managedDaemonShutdownOnExit"), on);
    }

    // Optional explicit path to the `daemon` binary; takes precedence over all other discovery
    // (env, co-located, PATH). Empty means "discover automatically".
    [[nodiscard]] QString daemonBinaryPath() const {
        return value(QStringLiteral("conn/daemonBinaryPath"), QString()).toString();
    }
    void setDaemonBinaryPath(const QString& path) {
        setValue(QStringLiteral("conn/daemonBinaryPath"), path);
    }

    // Per-target server-issued session token (AuthResume reconnect fast-path). Keyed by a hash of
    // the connection target so multiple nodes don't share a token. This stores the SERVER token
    // (never the password). The default impl persists to QSettings; a keychain-backed store may
    // override the raw value() path. Empty target / token are treated as "no token".
    // virtual so a keychain-backed store (QtSettingsStore) can prefer the OS keychain and fall back
    // to these QSettings implementations.
    [[nodiscard]] Q_INVOKABLE virtual QString connectionToken(const QString& target) const {
        if (target.isEmpty()) {
            return {};
        }
        return value(tokenKey(target)).toString();
    }
    Q_INVOKABLE virtual void setConnectionToken(const QString& target, const QString& token) {
        if (target.isEmpty()) {
            return;
        }
        setValue(tokenKey(target), token);
    }
    Q_INVOKABLE virtual void clearConnectionToken(const QString& target) {
        if (!target.isEmpty()) {
            setValue(tokenKey(target), QString());
        }
    }

    // The default user-writable socket for an app-managed local daemon. Cross-platform via
    // QStandardPaths: RuntimeLocation (Linux -> $XDG_RUNTIME_DIR; macOS -> the per-user temp dir),
    // falling back to TempLocation then AppDataLocation. Kept short (leaf `daemon/daemon.sock`) to
    // stay under the Unix `sun_path` limit (~104 macOS / ~108 Linux). This is a Unix-only flow
    // (QLocalSocket is named pipes on Windows and the daemon binds a Unix socket), so a filesystem
    // path is the right shape here.
    [[nodiscard]] Q_INVOKABLE QString defaultManagedSocketPath() const {
        QString base = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
        if (base.isEmpty()) {
            base = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        }
        if (base.isEmpty()) {
            base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        }
        if (base.isEmpty()) {
            base = QStringLiteral("/tmp");
        }
        return QDir(base).filePath(QStringLiteral("daemon/daemon.sock"));
    }

    // The local socket path to auto-open, resolving the test/CI override first.
    // `DAEMON_APP_SOCKET` lets the end-to-end harness point a headless GUI/TUI at an isolated
    // per-test socket without seeding QSettings; it wins over the persisted `conn/target`, which in
    // turn wins over the app-managed default (a user-writable path, NOT the root-owned
    // `/run/daemon.sock` a non-root managed daemon could never bind).
    [[nodiscard]] Q_INVOKABLE QString
    resolvedConnectionTarget(const QString& fallback = QString()) const {
#ifdef Q_OS_WASM
        const QString remoteOverride = qEnvironmentVariable("DAEMON_APP_REMOTE_URL");
        if (!remoteOverride.isEmpty()) {
            return remoteOverride;
        }
        const QString savedRemote = lastConnectionTarget();
        if (!savedRemote.isEmpty()) {
            return savedRemote;
        }
        return fallback.isEmpty() ? QStringLiteral("ws://localhost:8787/api/ws") : fallback;
#else
        const QString override = qEnvironmentVariable("DAEMON_APP_SOCKET");
        if (!override.isEmpty()) {
            return override;
        }
        const QString saved = lastConnectionTarget();
        if (!saved.isEmpty()) {
            return saved;
        }
        return fallback.isEmpty() ? defaultManagedSocketPath() : fallback;
#endif
    }

signals:
    // Emitted on every write (carries the key), plus a no-arg overload so QML
    // property bindings (setupComplete) refresh without naming the key.
    void changed(const QString& key);
    void changedAny();

private:
    // Stable per-target token key (hashed so arbitrary socket paths / host:port are filesystem- and
    // ini-safe). Not security: tokens are stored verbatim; the hash only namespaces the key.
    [[nodiscard]] static QString tokenKey(const QString& target) {
        const QByteArray h =
            QCryptographicHash::hash(target.toUtf8(), QCryptographicHash::Sha256).toHex();
        return QStringLiteral("conn/tokens/") + QString::fromLatin1(h.left(16));
    }
};

} // namespace settings
