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
    explicit MockCheckpointTimeline(QObject* parent = nullptr);

    [[nodiscard]] QObject* checkpoints() const override;
    [[nodiscard]] int count() const override;
    [[nodiscard]] int sessionId() const override { return m_sessionId; }
    void setSessionId(int id) override;

    void restore(const QString& id) override;
    QString createCheckpoint(const QString& label) override;

private:
    // The canonical demo timeline a freshly-seen session starts with.
    [[nodiscard]] static QList<QVariantMap> seedRows();
    // Make sure `id` has a timeline (seeding the demo rows the first time).
    void ensureSession(int id);

    uimodels::VariantListModel* m_checkpoints = nullptr; // the active session's view
    int m_sessionId = -1;
    int m_nextId = 5;
    // Per-session timelines + id counters, swapped in/out of m_checkpoints.
    QHash<int, QList<QVariantMap>> m_rowsBySession;
    QHash<int, int> m_nextIdBySession;
};

} // namespace session
