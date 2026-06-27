#include "daemon/daemon_session_settings.h"

#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"

namespace daemonapp::daemon {

DaemonSessionSettings::DaemonSessionSettings(NodeApiClient* client, QObject* parent)
    : session::MockSessionSettings(parent), m_client(client) {}

void DaemonSessionSettings::setApprovalMode(const QString& mode) {
    const bool changed = mode != approvalMode();
    session::MockSessionSettings::setApprovalMode(mode);
    // Push the mode to the live session so the daemon parks (or auto-resolves) approvals per the
    // selection. A blank session id means no chat is focused yet - the next setApprovalMode after a
    // session binds will send it.
    if (changed && m_client != nullptr && !sessionId().isEmpty()) {
        m_client->sendRequest(NodeApiCodec::encodeSetSessionModeRequest(sessionId(), mode),
                              QStringLiteral("session/mode/") + sessionId());
    }
}

} // namespace daemonapp::daemon
