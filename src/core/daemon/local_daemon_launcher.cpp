// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/local_daemon_launcher.h"

#include "settings/isettings_store.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QLocalSocket>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTimer>

#ifdef Q_OS_UNIX
#include <csignal>
#include <sys/types.h>
#endif

namespace daemonapp::daemon {

namespace {
constexpr int kSpawnReadyTimeoutMs = 10000;
constexpr int kPollIntervalMs = 100;
constexpr auto kDaemonBinaryName = "daemon";

[[nodiscard]] bool isExecutableFile(const QString& path) {
    const QFileInfo info(path);
    return info.isFile() && info.isExecutable();
}
} // namespace

LocalDaemonLauncher::LocalDaemonLauncher(settings::ISettingsStore* settings, QObject* parent)
    : QObject(parent), m_settings(settings) {}

QString LocalDaemonLauncher::discoverDaemonBinary(const QString& override) {
    // 1. Explicit override (the daemonBinaryPath setting): if set it must resolve, with no fallback
    //    to a different binary - an operator who pinned a path expects that one or an error.
    if (!override.isEmpty()) {
        return isExecutableFile(override) ? QFileInfo(override).absoluteFilePath() : QString();
    }
    // 2. DAEMON_BIN env (dev / tests).
    const QString env = qEnvironmentVariable("DAEMON_BIN");
    if (!env.isEmpty() && isExecutableFile(env)) {
        return QFileInfo(env).absoluteFilePath();
    }
    // 3. Co-located next to the app executable (bundled installs).
    const QString colocated =
        QDir(QCoreApplication::applicationDirPath()).filePath(QLatin1String(kDaemonBinaryName));
    if (isExecutableFile(colocated)) {
        return QFileInfo(colocated).absoluteFilePath();
    }
    // 4. PATH.
    return QStandardPaths::findExecutable(QLatin1String(kDaemonBinaryName));
}

bool LocalDaemonLauncher::isDaemonListening(const QString& socketPath, int timeoutMs) {
    QLocalSocket probe;
    probe.connectToServer(socketPath);
    const bool ok = probe.waitForConnected(timeoutMs);
    probe.abort();
    return ok;
}

void LocalDaemonLauncher::ensureRunning(const QString& socketPath) {
    m_socketPath = socketPath;
    m_managed = false;

    // Probe-first: never spawn a second daemon when one already answers on the target socket.
    if (isDaemonListening(socketPath)) {
        emit ready(false);
        return;
    }

    const QString binary =
        discoverDaemonBinary(m_settings != nullptr ? m_settings->daemonBinaryPath() : QString());
    if (binary.isEmpty()) {
        emit failed(tr("Could not find a local daemon binary. Set its path in Settings, or "
                       "connect to a remote daemon instead."));
        return;
    }

    // A daemon that crash-loops would otherwise be re-spawned forever by the reconnect path; cap
    // it.
    if (!restartBudgetAvailable()) {
        emit failed(tr("The local daemon keeps crashing (restarted too many times). Check its log "
                       "and your configuration."));
        return;
    }

    // Spawn detached so the daemon outlives the client (background wake/cron). Give it the chosen
    // socket + an isolated data dir; capture its output to a log file for diagnostics.
    const QString dataDir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                                .filePath(QStringLiteral("daemon"));
    QDir().mkpath(dataDir);
    // The managed socket may live under a runtime dir whose leaf we own (e.g.
    // $XDG_RUNTIME_DIR/daemon/); create its parent so the spawned daemon can bind it. The node
    // also creates this, but doing it here keeps the client side self-sufficient.
    const QString socketDir = QFileInfo(socketPath).path();
    if (!socketDir.isEmpty()) {
        QDir().mkpath(socketDir);
    }

    QProcessEnvironment penv = QProcessEnvironment::systemEnvironment();
    // The node reads its api-socket path from DAEMON_SOCKET_PATH (figment: `DAEMON_` + the
    // `socket_path` config key). DAEMON_API_SOCKET is NOT a config key, so the node silently
    // ignored it and bound its default $TMPDIR/daemon-api.sock while we polled `socketPath` -> "did
    // not become ready". Set the name the node actually reads.
    penv.insert(QStringLiteral("DAEMON_SOCKET_PATH"), socketPath);
    penv.insert(QStringLiteral("DAEMON_DATA_DIR"), dataDir);
    // Run the managed daemon durable (SQLite backend). With DAEMON_STORE_PATH left unset the node
    // defaults the sqlite database to `<data_dir>/daemon-store.sqlite` (DAEMON_DATA_DIR, set
    // above), so it lives alongside the other durable stores rather than in $TMPDIR (which a reboot
    // / tmp reaper would wipe). Durability is required for the daemon to be coherent with its
    // "persistent by default" lifetime: it binds the revision log (profile/skill versioning) and
    // the file-backed credential/profile stores, so API keys, agents, and history survive a daemon
    // restart rather than vanishing with the in-memory default.
    if (!penv.contains(QStringLiteral("DAEMON_STORE"))) {
        penv.insert(QStringLiteral("DAEMON_STORE"), QStringLiteral("sqlite"));
    }

    auto* proc = new QProcess(this);
    proc->setProgram(binary);
    proc->setProcessEnvironment(penv);
    const QString logPath = QDir(dataDir).filePath(QStringLiteral("daemon.log"));
    proc->setStandardOutputFile(logPath, QIODevice::Append);
    proc->setStandardErrorFile(logPath, QIODevice::Append);

    // #region agent log
    {
        QFile dbg(QStringLiteral("/home/j/experiments/daemon/.cursor/debug-96b7ad.log"));
        if (dbg.open(QIODevice::Append | QIODevice::Text)) {
            dbg.write(
                QStringLiteral(
                    "{\"sessionId\":\"96b7ad\",\"runId\":\"post-fix\",\"hypothesisId\":\"H1\","
                    "\"location\":\"local_daemon_launcher.cpp:ensureRunning\","
                    "\"message\":\"spawn managed daemon\",\"data\":{\"socket\":\"%1\","
                    "\"env\":\"DAEMON_SOCKET_PATH\"},\"timestamp\":%2}\n")
                    .arg(socketPath)
                    .arg(QDateTime::currentMSecsSinceEpoch())
                    .toUtf8());
        }
    }
    // #endregion
    qint64 pid = 0;
    const bool started = proc->startDetached(&pid);
    proc->deleteLater();
    if (!started || pid <= 0) {
        emit failed(tr("Failed to start the local daemon (%1).").arg(binary));
        return;
    }
    m_pid = pid;
    m_managed = true;

    // Poll for readiness inside the event loop so the UI's "connecting" state stays responsive.
    m_pollElapsedMs = 0;
    if (m_pollTimer == nullptr) {
        m_pollTimer = new QTimer(this);
        m_pollTimer->setInterval(kPollIntervalMs);
        connect(m_pollTimer, &QTimer::timeout, this, &LocalDaemonLauncher::pollUntilReady);
    }
    m_pollTimer->start();
}

bool LocalDaemonLauncher::evaluateRestartBudget(QList<qint64>& history, qint64 nowMs,
                                                qint64 windowMs, int maxRestarts) {
    // Drop spawns that aged out of the sliding window.
    while (!history.isEmpty() && nowMs - history.first() > windowMs) {
        history.removeFirst();
    }
    if (history.size() >= maxRestarts) {
        return false;
    }
    history.append(nowMs);
    return true;
}

bool LocalDaemonLauncher::restartBudgetAvailable() {
    return evaluateRestartBudget(m_spawnTimesMs, QDateTime::currentMSecsSinceEpoch(),
                                 kRestartWindowMs, kMaxRestarts);
}

void LocalDaemonLauncher::pollUntilReady() {
    if (isDaemonListening(m_socketPath, 50)) {
        // #region agent log
        {
            QFile dbg(QStringLiteral("/home/j/experiments/daemon/.cursor/debug-96b7ad.log"));
            if (dbg.open(QIODevice::Append | QIODevice::Text)) {
                dbg.write(
                    QStringLiteral(
                        "{\"sessionId\":\"96b7ad\",\"runId\":\"post-fix\",\"hypothesisId\":\"H1\","
                        "\"location\":\"local_daemon_launcher.cpp:pollUntilReady\","
                        "\"message\":\"managed daemon READY on polled socket\","
                        "\"data\":{\"socket\":\"%1\"},\"timestamp\":%2}\n")
                        .arg(m_socketPath)
                        .arg(QDateTime::currentMSecsSinceEpoch())
                        .toUtf8());
            }
        }
        // #endregion
        m_pollTimer->stop();
        emit ready(true);
        return;
    }
    m_pollElapsedMs += kPollIntervalMs;
    if (m_pollElapsedMs >= kSpawnReadyTimeoutMs) {
        m_pollTimer->stop();
        emit failed(tr("The local daemon did not become ready in time."));
    }
}

void LocalDaemonLauncher::shutdownIfManaged() {
    if (!m_managed || m_pid <= 0) {
        return;
    }
    if (m_settings == nullptr || !m_settings->managedDaemonShutdownOnExit()) {
        return; // persistent by default: leave the managed daemon running
    }
#ifdef Q_OS_UNIX
    // SIGTERM lets the daemon shut its resident services down cleanly and remove its socket.
    ::kill(static_cast<pid_t>(m_pid), SIGTERM);
#endif
    m_managed = false;
    m_pid = 0;
}

} // namespace daemonapp::daemon
