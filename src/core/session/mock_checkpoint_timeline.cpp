#include "session/mock_checkpoint_timeline.h"

namespace session {
namespace {

QVariantMap mk(const QString& id, const QString& label, const QString& time, int tokens,
               bool current)
{
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
    : ICheckpointTimeline(parent)
    , m_checkpoints(new uimodels::VariantListModel(this))
{
    // Seed the initial (default) conversation so the panel has content before any
    // conversation id is bound.
    ensureConversation(m_conversationId);
    m_checkpoints->setRows(m_rowsByConv.value(m_conversationId));
    m_nextId = m_nextIdByConv.value(m_conversationId);
    connect(m_checkpoints, &uimodels::VariantListModel::countChanged, this,
            &ICheckpointTimeline::changed);
}

QList<QVariantMap> MockCheckpointTimeline::seedRows()
{
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

void MockCheckpointTimeline::ensureConversation(int id)
{
    if (!m_rowsByConv.contains(id)) {
        m_rowsByConv.insert(id, seedRows());
        m_nextIdByConv.insert(id, 5);
    }
}

void MockCheckpointTimeline::setConversationId(int id)
{
    if (m_conversationId == id) {
        return;
    }
    // Stash the outgoing conversation's live timeline, then swap in the target's.
    m_rowsByConv[m_conversationId] = m_checkpoints->rows();
    m_nextIdByConv[m_conversationId] = m_nextId;

    m_conversationId = id;
    ensureConversation(id);
    m_checkpoints->setRows(m_rowsByConv.value(id));
    m_nextId = m_nextIdByConv.value(id);

    emit conversationIdChanged();
    emit changed();
}

QObject* MockCheckpointTimeline::checkpoints() const { return m_checkpoints; }
int MockCheckpointTimeline::count() const { return m_checkpoints->count(); }

void MockCheckpointTimeline::restore(const QString& id)
{
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

QString MockCheckpointTimeline::createCheckpoint(const QString& label)
{
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
