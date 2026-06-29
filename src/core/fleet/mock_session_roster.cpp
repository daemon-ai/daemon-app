// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "fleet/mock_session_roster.h"

#include "daemonnet/idaemonnet.h"

namespace fleet {

MockSessionRoster::MockSessionRoster(daemonnet::IDaemonNet* net, QObject* parent)
    : ISessionRoster(parent), m_sessions(new uimodels::VariantListModel(this)) {
    // Single source: copy the DaemonNet's flat session projection at construction;
    // suspend/resume/close then mutate this local copy.
    if (net != nullptr) {
        if (auto* src = qobject_cast<uimodels::VariantListModel*>(net->sessions())) {
            m_sessions->setRows(src->rows());
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
