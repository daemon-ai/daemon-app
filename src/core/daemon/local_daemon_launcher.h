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

    // Terminate the spawned daemon iff this launcher started it AND the user opted into
    // shutdown-on-exit. A daemon the client only attached to is never touched. No-op otherwise.
    void shutdownIfManaged();

    [[nodiscard]] bool managed() const { return m_managed; }

signals:
    void ready(bool managed);
    void failed(const QString& message);

private:
    void pollUntilReady();
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
    int m_pollElapsedMs = 0;
    QList<qint64> m_spawnTimesMs; // epoch-ms of recent spawns (pruned to the window)
};

} // namespace daemonapp::daemon
