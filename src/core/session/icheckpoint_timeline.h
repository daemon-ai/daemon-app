// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>

namespace session {

// Checkpoint/rewind timeline seam backing the composer's checkpoints panel. Rows
// (VariantListModel): id, label, time, tokens (-1 = unknown, hide the badge), current.
// restore() rewinds the session to a checkpoint (the mock flips `current` and drops later
// entries; the daemon seam issues CheckpointRewind).
class ICheckpointTimeline : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* checkpoints READ checkpoints CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY changed)
    // The session whose timeline is shown. Setting it swaps the live
    // checkpoints to that session's own timeline (per-session rewind),
    // emitting changed() so the bound panel refreshes.
    Q_PROPERTY(QString sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged)
    // Whether this seam supports client-initiated checkpoints ("Save here"). The daemon seam
    // returns false: checkpoints are node-created on tool events (no wire op at v28), so the UI
    // hides the affordance rather than offering a silent no-op.
    Q_PROPERTY(bool canCreate READ canCreate CONSTANT)

public:
    using QObject::QObject;
    ~ICheckpointTimeline() override = default;

    [[nodiscard]] virtual QObject* checkpoints() const = 0;
    [[nodiscard]] virtual int count() const = 0;
    [[nodiscard]] virtual QString sessionId() const = 0;
    virtual void setSessionId(const QString& id) = 0;
    [[nodiscard]] virtual bool canCreate() const { return true; }

    Q_INVOKABLE virtual void restore(const QString& id) = 0;
    Q_INVOKABLE virtual QString createCheckpoint(const QString& label) = 0;

signals:
    void changed();
    void sessionIdChanged();
};

} // namespace session
