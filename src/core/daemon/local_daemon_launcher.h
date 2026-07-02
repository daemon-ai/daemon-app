// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QList>
#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace settings {
class ISettingsStore;
}

namespace daemonapp::daemon {

// Ensures a local daemon is reachable for "Local" connect (user story CON-1b).
//
// ensureRunning() probes the target Unix socket; if something is already listening it reports
// "attached" (the client did not start it). Otherwise it discovers the `daemon` binary, spawns it
// detached with DAEMON_API_SOCKET pointed at the target, and polls until the socket accepts a
// connection (or a timeout elapses). A daemon THIS launcher spawned is "managed": it is left
// running on client exit unless the user opts into shutdown-on-exit, so background wake/cron
// survive the GUI/TUI closing. A daemon the client only attached to is never terminated by the
// client.
//
// Every spawn records the daemon's pid in a pidfile next to the managed socket
// (`<socket dir>/daemon.pid`, e.g. `$XDG_RUNTIME_DIR/daemon/daemon.pid`), so ANY app instance can
// manage a daemon a previous instance spawned: shutdownIfManaged() and the version-gate respawn
// (replaceStaleDaemon()) key off the liveness-checked pidfile, never off the in-process m_pid
// alone (which dies with the process) and never off a process name/pattern.
class LocalDaemonLauncher : public QObject {
    Q_OBJECT

public:
    explicit LocalDaemonLauncher(settings::ISettingsStore* settings, QObject* parent = nullptr);

    // Resolve the `daemon` binary path using, in order: an explicit override (the daemonBinaryPath
    // setting), the DAEMON_BIN env, a binary co-located with the app executable (bundled installs),
    // then PATH. Returns an empty string if none resolve.
    [[nodiscard]] static QString discoverDaemonBinary(const QString& override = QString());

    // Cheap liveness probe: true iff a server currently accepts a connection on socketPath.
    [[nodiscard]] static bool isDaemonListening(const QString& socketPath, int timeoutMs = 200);

    // The pidfile recording the pid of an app-managed daemon serving socketPath. Lives in the
    // socket's own directory (the managed default: $XDG_RUNTIME_DIR/daemon/daemon.pid beside
    // daemon.sock) so it shares the socket's lifetime domain (wiped with the runtime dir).
    [[nodiscard]] static QString pidFilePath(const QString& socketPath);

    // kill(pid, 0)-equivalent liveness probe. False for pid <= 1 and on non-Unix (the managed
    // local-daemon flow is Unix-only).
    [[nodiscard]] static bool isProcessAlive(qint64 pid);

    // The liveness-checked pid recorded for socketPath. 0 when no pidfile exists or the recorded
    // process is gone; a stale pidfile is removed on the way.
    [[nodiscard]] static qint64 readManagedPid(const QString& socketPath);

    // Sliding-window restart budget (pure, so it is unit-testable without spawning): prunes spawns
    // older than windowMs from `history`, returns false if `maxRestarts` remain inside the window,
    // otherwise records nowMs and returns true. The instance method below applies it to live
    // spawns.
    [[nodiscard]] static bool evaluateRestartBudget(QList<qint64>& history, qint64 nowMs,
                                                    qint64 windowMs, int maxRestarts);

    // Async. Probe socketPath; if no daemon answers, discover + spawn one and poll until it is
    // ready. Emits ready(managed) on success or failed(message) on discovery/spawn/timeout failure.
    // `managed` is true iff this call spawned the daemon.
    void ensureRunning(const QString& socketPath);

    // Async. The version-gate respawn: SIGTERM the pidfile-recorded daemon serving socketPath
    // (possibly spawned by a previous app instance), wait for the process to exit, then run the
    // ensureRunning flow so the CURRENT binary is spawned. Emits failed() when no live pidfile pid
    // exists (a pre-pidfile / foreign daemon is never killed blindly - the user is told which
    // socket is stale) or when the old daemon does not exit in time.
    void replaceStaleDaemon(const QString& socketPath);

    // Terminate the managed daemon recorded for the current socket iff the user opted into
    // shutdown-on-exit. Keyed off the pidfile so a daemon spawned by a PREVIOUS app instance is
    // covered too; a daemon the app only ever attached to has no pidfile and is never touched.
    void shutdownIfManaged();

    [[nodiscard]] bool managed() const { return m_managed; }

signals:
    void ready(bool managed);
    void failed(const QString& message);

private:
    // What the shared poll timer is currently waiting for.
    enum class Phase {
        Idle,
        WaitReady, // spawn path: the socket starts accepting connections
        WaitGone,  // replace path: the SIGTERMed pid exits (then ensureRunning re-spawns)
    };

    void startPoll(Phase phase);
    void pollTick();
    void pollUntilReady();
    void pollUntilGone();
    static void writeManagedPid(const QString& socketPath, qint64 pid);
    // Spawn budget: a crash-looping daemon (dies, gets re-spawned by the reconnect path, dies
    // again) must not spawn-storm. Returns false (and prunes stale entries) once kMaxRestarts
    // spawns have happened inside kRestartWindowMs; records the spawn time on success.
    [[nodiscard]] bool restartBudgetAvailable();

    static constexpr int kMaxRestarts = 3;
    static constexpr qint64 kRestartWindowMs = 60000;

    settings::ISettingsStore* m_settings = nullptr;
    QString m_socketPath;
    qint64 m_pid = 0;
    bool m_managed = false;
    QTimer* m_pollTimer = nullptr;
    Phase m_phase = Phase::Idle;
    int m_pollElapsedMs = 0;
    qint64 m_terminatedPid = 0;   // the pid WaitGone watches
    QList<qint64> m_spawnTimesMs; // epoch-ms of recent spawns (pruned to the window)
};

} // namespace daemonapp::daemon
