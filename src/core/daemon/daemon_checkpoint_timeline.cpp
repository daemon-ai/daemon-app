// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_checkpoint_timeline.h"

#include "daemon/repositories.h"
#include "uimodels/variant_list_model.h"

#include <QDateTime>

namespace daemonapp::daemon {

DaemonCheckpointTimeline::DaemonCheckpointTimeline(CheckpointRepository* repo, QObject* parent)
    : session::ICheckpointTimeline(parent), m_repo(repo),
      m_checkpoints(new uimodels::VariantListModel(this)) {
    if (m_repo != nullptr) {
        connect(m_repo, &CheckpointRepository::checkpointsRefreshed, this,
                &DaemonCheckpointTimeline::onCheckpointsRefreshed);
    }
}

QObject* DaemonCheckpointTimeline::checkpoints() const {
    return m_checkpoints;
}

int DaemonCheckpointTimeline::count() const {
    return m_checkpoints->count();
}

void DaemonCheckpointTimeline::setSessionId(const QString& id) {
    if (m_sessionId == id) {
        return;
    }
    m_sessionId = id;
    // Swap to the target session's timeline: clear the stale rows immediately (a rewind
    // affordance must never act on another session's rows), then re-fetch live.
    m_checkpoints->setRows({});
    if (m_repo != nullptr && !id.isEmpty()) {
        m_repo->refresh(id);
    }
    emit sessionIdChanged();
    emit changed();
}

void DaemonCheckpointTimeline::restore(const QString& id) {
    if (m_repo != nullptr && !m_sessionId.isEmpty() && !id.isEmpty()) {
        m_repo->rewind(m_sessionId, id);
    }
}

QString DaemonCheckpointTimeline::createCheckpoint(const QString& label) {
    // Checkpoints are node-created on tool events; there is no client-initiated create op at
    // v28. The daemon-mode UI hides the "Save here" affordance.
    Q_UNUSED(label)
    return {};
}

void DaemonCheckpointTimeline::onCheckpointsRefreshed(const QString& sessionId) {
    if (sessionId != m_sessionId || m_repo == nullptr) {
        return;
    }
    QList<QVariantMap> rows;
    const QList<DecodedCheckpointInfo>& list = m_repo->checkpoints();
    for (int i = 0; i < list.size(); ++i) {
        const DecodedCheckpointInfo& c = list.at(i);
        QVariantMap row;
        row[QStringLiteral("id")] = c.id;
        // The tool whose execution the checkpoint precedes IS the human handle for the point.
        row[QStringLiteral("label")] =
            c.tool.isEmpty() ? tr("Checkpoint") : tr("before %1").arg(c.tool);
        row[QStringLiteral("tool")] = c.tool;
        row[QStringLiteral("time")] =
            QDateTime::fromSecsSinceEpoch(static_cast<qint64>(c.createdUnix))
                .toLocalTime()
                .toString(QStringLiteral("hh:mm"));
        // The wire carries no token counter; -1 tells the panel to omit the badge.
        row[QStringLiteral("tokens")] = -1;
        row[QStringLiteral("current")] = i == list.size() - 1; // newest = the live tip
        rows.append(row);
    }
    m_checkpoints->setRows(rows);
    emit changed();
}

} // namespace daemonapp::daemon
