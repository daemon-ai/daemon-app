// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QLocalServer;
QT_END_NAMESPACE

namespace platform {

// Per-user single-instance guard for the GUI. With autostart shipping, a
// hidden instance is usually already running at login, so a manual launch
// must raise the existing window instead of spawning a duplicate (and macOS
// SMAppService registration bootstraps the agent immediately, launching a
// second copy the moment the toggle is flipped on).
//
// Protocol: the primary listens on a per-user QLocalServer; a secondary
// connects and sends one line - "raise" (surface the primary's window; the
// normal manual-launch case) or "ping" (just prove liveness; a --hidden
// autostart duplicate must not yank the window forward) - then exits.
// The TUI is exempt on purpose: multiple terminals are a feature.
class SingleInstanceGuard : public QObject {
    Q_OBJECT

public:
    explicit SingleInstanceGuard(QString key, QObject* parent = nullptr);

    // The per-user server name: same user + same app -> same key; distinct
    // users on one machine never collide.
    [[nodiscard]] static QString defaultKey();

    // Try to become the primary. On a leftover socket from a crashed primary
    // (unix), probes for liveness, removes the stale socket, and retries once.
    [[nodiscard]] bool tryClaim();

    // Secondary path: deliver "raise"/"ping" to the primary. False when no
    // live primary answered (the caller then continues as a normal instance).
    bool notifyPrimary(bool raise, int timeoutMs = 500);

signals:
    // The primary received a "raise": surface the main window.
    void activateRequested();

private:
    QString m_key;
    QLocalServer* m_server = nullptr;
};

} // namespace platform
