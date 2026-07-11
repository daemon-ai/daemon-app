// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "fleet/mock_session_roster.h"

#include "daemonnet/mock_fleet_source.h"

namespace fleet {

MockSessionRoster::MockSessionRoster(daemonnet::MockFleetSource* src, QObject* parent)
    : ISessionRoster(parent), m_sessions(new uimodels::VariantListModel(this)) {
    // Single source: copy the mock seed's flat session projection at construction;
    // suspend/resume/close then mutate this local copy.
    if (src != nullptr) {
        if (auto* rows = qobject_cast<uimodels::VariantListModel*>(src->sessions())) {
            m_sessions->setRows(rows->rows());
        }
    }
    connect(m_sessions, &uimodels::VariantListModel::countChanged, this, &ISessionRoster::changed);
}

QObject* MockSessionRoster::sessions() const {
    return m_sessions;
}
int MockSessionRoster::count() const {
    return m_sessions->count();
}

void MockSessionRoster::setState(const QString& id, const QString& state) {
    const int row = m_sessions->indexOfId(id);
    if (row < 0) {
        return;
    }
    QVariantMap s = m_sessions->at(row);
    s[QStringLiteral("state")] = state;
    m_sessions->upsert(s);
    emit changed();
}

void MockSessionRoster::suspend(const QString& id) {
    setState(id, QStringLiteral("suspended"));
}
void MockSessionRoster::resume(const QString& id) {
    setState(id, QStringLiteral("active"));
}

void MockSessionRoster::close(const QString& id) {
    m_sessions->removeById(id);
    emit changed();
}

} // namespace fleet
