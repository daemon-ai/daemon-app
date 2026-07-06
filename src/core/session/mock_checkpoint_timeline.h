// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "session/icheckpoint_timeline.h"
#include "uimodels/variant_list_model.h"

#include <QHash>
#include <QList>
#include <QVariantMap>

namespace session {

class MockCheckpointTimeline : public ICheckpointTimeline {
    Q_OBJECT

public:
    // `seedDemo` seeds the illustrative demo checkpoints per session (the mock-mode default).
    // Daemon mode passes false: checkpoint timelines have NO node wire op yet (CheckpointRepository
    // is a stub), so each session renders empty rather than fabricated rewind points (render
    // honesty).
    explicit MockCheckpointTimeline(QObject* parent = nullptr, bool seedDemo = true);

    [[nodiscard]] QObject* checkpoints() const override;
    [[nodiscard]] int count() const override;
    [[nodiscard]] QString sessionId() const override { return m_sessionId; }
    void setSessionId(const QString& id) override;

    void restore(const QString& id) override;
    QString createCheckpoint(const QString& label) override;

private:
    // The canonical demo timeline a freshly-seen session starts with.
    [[nodiscard]] static QList<QVariantMap> seedRows();
    // Make sure `id` has a timeline (seeding the demo rows the first time).
    void ensureSession(const QString& id);

    uimodels::VariantListModel* m_checkpoints = nullptr; // the active session's view
    QString m_sessionId;
    bool m_seedDemo = true; // false in daemon mode: sessions start with an empty timeline
    int m_nextId = 5;
    // Per-session timelines + id counters, swapped in/out of m_checkpoints.
    QHash<QString, QList<QVariantMap>> m_rowsBySession;
    QHash<QString, int> m_nextIdBySession;
};

} // namespace session
