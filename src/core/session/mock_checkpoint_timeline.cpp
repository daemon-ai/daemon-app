#include "session/mock_checkpoint_timeline.h"

namespace session {
namespace {

QVariantMap mk(const QString& id, const QString& label, const QString& time, int tokens,
               bool current) {
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("label")] = label;
    m[QStringLiteral("time")] = time;
    m[QStringLiteral("tokens")] = tokens;
    m[QStringLiteral("current")] = current;
    return m;
}

} // namespace

MockCheckpointTimeline::MockCheckpointTimeline(QObject* parent)
    : ICheckpointTimeline(parent), m_checkpoints(new uimodels::VariantListModel(this)) {
    // Seed the initial (default) session so the panel has content before any
    // session id is bound.
    ensureSession(m_sessionId);
    m_checkpoints->setRows(m_rowsBySession.value(m_sessionId));
    m_nextId = m_nextIdBySession.value(m_sessionId);
    connect(m_checkpoints, &uimodels::VariantListModel::countChanged, this,
            &ICheckpointTimeline::changed);
}

QList<QVariantMap> MockCheckpointTimeline::seedRows() {
    return {
        mk(QStringLiteral("cp-1"), QStringLiteral("Initial prompt"), QStringLiteral("10:02"), 1280,
           false),
        mk(QStringLiteral("cp-2"), QStringLiteral("Add failing test"), QStringLiteral("10:09"),
           6420, false),
        mk(QStringLiteral("cp-3"), QStringLiteral("Implement fix"), QStringLiteral("10:21"), 18940,
           false),
        mk(QStringLiteral("cp-4"), QStringLiteral("Refine + docs"), QStringLiteral("10:35"), 24110,
           true),
    };
}

void MockCheckpointTimeline::ensureSession(const QString& id) {
    if (!m_rowsBySession.contains(id)) {
        m_rowsBySession.insert(id, seedRows());
        m_nextIdBySession.insert(id, 5);
    }
}

void MockCheckpointTimeline::setSessionId(const QString& id) {
    if (m_sessionId == id) {
        return;
    }
    // Stash the outgoing session's live timeline, then swap in the target's.
    m_rowsBySession[m_sessionId] = m_checkpoints->rows();
    m_nextIdBySession[m_sessionId] = m_nextId;

    m_sessionId = id;
    ensureSession(id);
    m_checkpoints->setRows(m_rowsBySession.value(id));
    m_nextId = m_nextIdBySession.value(id);

    emit sessionIdChanged();
    emit changed();
}

QObject* MockCheckpointTimeline::checkpoints() const {
    return m_checkpoints;
}
int MockCheckpointTimeline::count() const {
    return m_checkpoints->count();
}

void MockCheckpointTimeline::restore(const QString& id) {
    const int target = m_checkpoints->indexOfId(id);
    if (target < 0) {
        return;
    }
    // Rewinding drops every checkpoint after the target and makes it current.
    const auto rows = m_checkpoints->rows();
    for (int i = static_cast<int>(rows.size()) - 1; i > target; --i) {
        m_checkpoints->removeById(rows.at(i).value(QStringLiteral("id")));
    }
    for (const QVariantMap& row : m_checkpoints->rows()) {
        const bool isTarget = row.value(QStringLiteral("id")).toString() == id;
        if (row.value(QStringLiteral("current")).toBool() != isTarget) {
            QVariantMap updated = row;
            updated[QStringLiteral("current")] = isTarget;
            m_checkpoints->upsert(updated);
        }
    }
    emit changed();
}

QString MockCheckpointTimeline::createCheckpoint(const QString& label) {
    const QString id = QStringLiteral("cp-%1").arg(m_nextId++);
    for (const QVariantMap& row : m_checkpoints->rows()) {
        if (row.value(QStringLiteral("current")).toBool()) {
            QVariantMap updated = row;
            updated[QStringLiteral("current")] = false;
            m_checkpoints->upsert(updated);
        }
    }
    m_checkpoints->upsert(mk(id, label.isEmpty() ? QStringLiteral("Checkpoint") : label,
                             QStringLiteral("now"), 0, true));
    emit changed();
    return id;
}

} // namespace session
