// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "session/icheckpoint_timeline.h"

#include <QString>

namespace uimodels {
class VariantListModel;
}

namespace daemonapp::daemon {
class CheckpointRepository;

// Daemon-backed checkpoint timeline (E4/TOOL-9): projects the CheckpointRepository's
// CheckpointList rows into the ICheckpointTimeline VariantListModel the popover + timeline strip
// bind ({ id, label, time, tokens, current, tool }). setSessionId() re-scopes + re-fetches;
// restore() issues CheckpointRewind (confirmation is the UI's job — the strip/popover confirm
// before calling). createCheckpoint() is unsupported: checkpoints are node-created on tool events
// (no wire op), so it returns "" and the daemon-mode UI hides the affordance.
class DaemonCheckpointTimeline : public session::ICheckpointTimeline {
    Q_OBJECT

public:
    explicit DaemonCheckpointTimeline(CheckpointRepository* repo, QObject* parent = nullptr);

    [[nodiscard]] QObject* checkpoints() const override;
    [[nodiscard]] int count() const override;
    [[nodiscard]] QString sessionId() const override { return m_sessionId; }
    void setSessionId(const QString& id) override;
    [[nodiscard]] bool canCreate() const override { return false; } // node-created only (v28)

    void restore(const QString& id) override;
    QString createCheckpoint(const QString& label) override;

private:
    void onCheckpointsRefreshed(const QString& sessionId);

    CheckpointRepository* m_repo = nullptr;
    uimodels::VariantListModel* m_checkpoints = nullptr;
    QString m_sessionId;
};

} // namespace daemonapp::daemon
