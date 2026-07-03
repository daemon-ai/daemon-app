// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Browser (wasm) stand-in for daemon_transport_stream.cpp: Qt-for-wasm has no `localserver`
// (QLocalSocket) or `ssl` (QSslSocket) feature, so the length-framed stream carriers cannot
// exist there. These stubs keep DaemonTransport linking with its full API while failing CLOSED:
// any attempt to use the Unix / TLS-TCP modes reports an explicit error and never connects.
// The WebSocket carrier (daemon_transport.cpp) is the browser's only transport.

#include "daemon/daemon_transport.h"

namespace daemonapp::daemon {

bool DaemonTransport::streamSocketConnected() const {
    return false;
}

void DaemonTransport::ensureStreamSocket() {}

void DaemonTransport::openUnix() {
    emit failed(QStringLiteral("Unix-socket connections are not available in a browser"));
}

void DaemonTransport::openTcp() {
    emit failed(QStringLiteral("TLS TCP connections are not available in a browser"));
}

void DaemonTransport::flushStreamSocket() {}

void DaemonTransport::resetStreamSockets() {}

} // namespace daemonapp::daemon
