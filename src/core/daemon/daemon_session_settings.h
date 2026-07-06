// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "session/mock_session_settings.h"

namespace uimodels {
class VariantListModel;
}

namespace daemonapp::daemon {
class NodeApiClient;
class FingerprintRepository;

// Daemon-backed per-session settings (CHA-4). Profile/effort/fast/verbose stay in-memory (they are
// composer UI overrides not yet mapped to a wire op), but setApprovalMode additionally sends
// SetSessionMode so the node's SessionOverlay.approval_mode tracks the popover. Subclassing the
// mock keeps the in-memory bookkeeping in one place.
//
// [wave2:app-approvals-safety] D4: also backs the active session's remembered-fingerprint list via
// a FingerprintRepository (FingerprintList/FingerprintRevoke). Switching the active session
// re-queries the allow-list; revoke re-syncs from the node.
class DaemonSessionSettings : public session::MockSessionSettings {
    Q_OBJECT

public:
    DaemonSessionSettings(NodeApiClient* client, FingerprintRepository* fingerprints,
                          QObject* parent = nullptr);

    void setApprovalMode(const QString& mode) override;
    void setSessionId(const QString& id) override;

    [[nodiscard]] QObject* rememberedFingerprints() const override;
    void refreshFingerprints() override;
    void revokeFingerprint(const QString& fingerprint) override;

private:
    void rebuildFingerprints();

    NodeApiClient* m_client = nullptr;
    FingerprintRepository* m_fingerprints = nullptr;
    uimodels::VariantListModel* m_fingerprintRows = nullptr;
};

} // namespace daemonapp::daemon
