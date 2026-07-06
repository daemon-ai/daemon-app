// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_session_settings.h"

#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "uimodels/variant_list_model.h"

namespace daemonapp::daemon {

namespace {
// A compact, copy-friendly display of a fingerprint hex: first 12 chars + ellipsis. The full value
// stays available for the tooltip/revoke call.
QString shortFingerprint(const QString& fp) {
    constexpr int kHead = 12;
    return fp.size() > kHead ? fp.left(kHead) + QStringLiteral("…") : fp;
}
} // namespace

DaemonSessionSettings::DaemonSessionSettings(NodeApiClient* client,
                                             FingerprintRepository* fingerprints, QObject* parent)
    : session::MockSessionSettings(parent), m_client(client), m_fingerprints(fingerprints),
      m_fingerprintRows(new uimodels::VariantListModel(this)) {
    if (m_fingerprints != nullptr) {
        connect(m_fingerprints, &FingerprintRepository::fingerprintsRefreshed, this,
                &DaemonSessionSettings::rebuildFingerprints);
    }
}

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

void DaemonSessionSettings::setSessionId(const QString& id) {
    const bool changed = id != sessionId();
    session::MockSessionSettings::setSessionId(id);
    // [wave2:app-approvals-safety] D4: the remembered-approval list is per session — re-query the
    // new session's allow-list so the popover reflects the focused chat.
    if (changed) {
        refreshFingerprints();
    }
}

QObject* DaemonSessionSettings::rememberedFingerprints() const {
    return m_fingerprintRows;
}

void DaemonSessionSettings::refreshFingerprints() {
    if (m_fingerprints != nullptr) {
        m_fingerprints->refreshFingerprints(sessionId());
    }
}

void DaemonSessionSettings::revokeFingerprint(const QString& fingerprint) {
    if (m_fingerprints != nullptr && !sessionId().isEmpty()) {
        m_fingerprints->revoke(sessionId(), fingerprint);
    }
}

void DaemonSessionSettings::rebuildFingerprints() {
    if (m_fingerprints == nullptr) {
        return;
    }
    QList<QVariantMap> rows;
    // Only render the list that belongs to the active session (a stale response from a prior
    // session must not leak into the popover after a fast tab switch).
    if (m_fingerprints->session() == sessionId()) {
        rows.reserve(m_fingerprints->fingerprints().size());
        for (const DecodedRememberedFingerprint& fp : m_fingerprints->fingerprints()) {
            QVariantMap row;
            row[QStringLiteral("fingerprint")] = fp.fingerprint;
            row[QStringLiteral("shortFingerprint")] = shortFingerprint(fp.fingerprint);
            // The node stores only the hash today (label always empty); surface it when present so
            // a future provenance capture needs no client change.
            row[QStringLiteral("label")] = fp.label;
            rows.append(row);
        }
    }
    m_fingerprintRows->setRows(rows);
}

} // namespace daemonapp::daemon
