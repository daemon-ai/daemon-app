// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QByteArray>
#include <QLatin1Char>
#include <QString>
#include <QtGlobal>

namespace daemonapp::daemon {

// Windows named-pipe name component derived from a connection socket path.
//
// This MIRRORS daemon-node's `daemon_host::windows_pipe_component`
// (daemon-node/crates/substrate/daemon-host/src/socket.rs) — the RUST SIDE IS THE AUTHORITY; keep
// this byte-for-byte identical. The daemon binds `\\.\pipe\<component>` from its `socket_path`
// config (DAEMON_SOCKET_PATH); QLocalSocket on Windows prepends `\\.\pipe\` to the name we pass, so
// here we return the bare component and let Qt add the prefix. Pipe names share one machine-global
// namespace, so the full path is folded in to keep distinct sockets on distinct pipes.
//
// Rule: the fixed prefix `daemon-api-`, then each UTF-8 byte of `socketPath` that is an ASCII
// alphanumeric or one of `.`/`_`/`-` is kept verbatim; every other byte maps to `_`.
[[nodiscard]] inline QString windowsPipeComponent(const QString& socketPath) {
    const QByteArray utf8 = socketPath.toUtf8();
    QString name = QStringLiteral("daemon-api-");
    name.reserve(name.size() + utf8.size());
    for (const char ch : utf8) {
        const unsigned char b = static_cast<unsigned char>(ch);
        const bool keep = (b >= '0' && b <= '9') || (b >= 'a' && b <= 'z') ||
                          (b >= 'A' && b <= 'Z') || b == '.' || b == '_' || b == '-';
        name.append(QLatin1Char(keep ? static_cast<char>(b) : '_'));
    }
    return name;
}

// The QLocalSocket server name for a connection socket path: on Windows the derived pipe-name
// component (Qt prepends `\\.\pipe\`), matching the daemon's bound pipe; elsewhere the raw
// filesystem path of the AF_UNIX socket.
[[nodiscard]] inline QString localServerName(const QString& socketPath) {
#ifdef Q_OS_WIN
    return windowsPipeComponent(socketPath);
#else
    return socketPath;
#endif
}

} // namespace daemonapp::daemon
