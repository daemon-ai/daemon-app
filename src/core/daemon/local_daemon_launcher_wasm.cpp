// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Browser (wasm) stand-in for local_daemon_launcher.cpp: Qt-for-wasm has no `process` (QProcess)
// or `localserver` (QLocalSocket) feature and a browser cannot spawn or probe a local daemon, so
// the managed-local flow is impossible there. These stubs keep DaemonConnectionService linking
// unchanged; every entry point reports the capability gap (ensureRunning/replaceStaleDaemon) or
// is inert. The connection service additionally refuses non-`remote-ws` modes on wasm, so in
// practice nothing reaches this class in a browser.
//
// The private poll machinery (startPoll/pollTick/...) is declared in the header but never
// referenced from these bodies, so it needs no wasm definitions.

#include "daemon/local_daemon_launcher.h"

namespace daemonapp::daemon {

LocalDaemonLauncher::LocalDaemonLauncher(settings::ISettingsStore* settings, QObject* parent)
    : QObject(parent), m_settings(settings) {}

QString LocalDaemonLauncher::discoverDaemonBinary(const QString& override) {
    Q_UNUSED(override)
    return {};
}

bool LocalDaemonLauncher::isDaemonListening(const QString& socketPath, int timeoutMs) {
    Q_UNUSED(socketPath)
    Q_UNUSED(timeoutMs)
    return false;
}

QString LocalDaemonLauncher::pidFilePath(const QString& socketPath) {
    Q_UNUSED(socketPath)
    return {};
}

bool LocalDaemonLauncher::isProcessAlive(qint64 pid) {
    Q_UNUSED(pid)
    return false;
}

qint64 LocalDaemonLauncher::readManagedPid(const QString& socketPath) {
    Q_UNUSED(socketPath)
    return 0;
}

bool LocalDaemonLauncher::evaluateRestartBudget(QList<qint64>& history, qint64 nowMs,
                                                qint64 windowMs, int maxRestarts) {
    Q_UNUSED(history)
    Q_UNUSED(nowMs)
    Q_UNUSED(windowMs)
    Q_UNUSED(maxRestarts)
    return false;
}

void LocalDaemonLauncher::ensureRunning(const QString& socketPath) {
    m_socketPath = socketPath;
    m_managed = false;
    emit failed(tr("A browser cannot start a local daemon. Connect to a WebSocket node instead."));
}

void LocalDaemonLauncher::replaceStaleDaemon(const QString& socketPath) {
    m_socketPath = socketPath;
    m_managed = false;
    emit failed(tr("A browser cannot manage a local daemon. Connect to a WebSocket node instead."));
}

void LocalDaemonLauncher::shutdownIfManaged() {}

} // namespace daemonapp::daemon
