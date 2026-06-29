// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_session_roster.h"

#include "domain/session.h"
#include "domain/sidebar_node.h"
#include "persistence/isession_store.h"
#include "uimodels/variant_list_model.h"

#include <QDateTime>

namespace fleet {
namespace {

// Lowercase the domain state/lifecycle for the roster row (the UI expects active/idle/suspended +
// live/durable). Ready/Completed/Unknown collapse to "idle" (the roster's neutral resting state).
QString stateStr(domain::SessionState s) {
    switch (s) {
    case domain::SessionState::Active:
        return QStringLiteral("active");
    case domain::SessionState::Suspended:
        return QStringLiteral("suspended");
    default:
        return QStringLiteral("idle");
    }
}

QString lifecycleStr(domain::Lifecycle l) {
    return l == domain::Lifecycle::Live ? QStringLiteral("live") : QStringLiteral("durable");
}

// A coarse relative-time label from a ms epoch (the roster's human "lastActivity").
QString relativeTime(qint64 ms) {
    if (ms <= 0) {
        return QStringLiteral("never");
    }
    const qint64 delta = QDateTime::currentMSecsSinceEpoch() - ms;
    if (delta < 60'000) {
        return QStringLiteral("just now");
    }
    if (delta < 3'600'000) {
        return QStringLiteral("%1m").arg(delta / 60'000);
    }
    if (delta < 86'400'000) {
        return QStringLiteral("%1h").arg(delta / 3'600'000);
    }
    return QStringLiteral("%1d").arg(delta / 86'400'000);
}

} // namespace

DaemonSessionRoster::DaemonSessionRoster(persistence::ISessionStore* store, QObject* parent)
    : ISessionRoster(parent), m_store(store), m_sessions(new uimodels::VariantListModel(this)) {
    connect(m_sessions, &uimodels::VariantListModel::countChanged, this, &ISessionRoster::changed);
    if (m_store != nullptr) {
        // Rebuild whenever the cache projection changes (a SessionsQuery refresh, an archive,
        // etc.).
        connect(m_store, &persistence::ISessionStore::changed, this, &DaemonSessionRoster::rebuild);
        rebuild(); // offline-first: render the last-known sessions immediately
    }
}

QObject* DaemonSessionRoster::sessions() const {
    return m_sessions;
}

int DaemonSessionRoster::count() const {
    return m_sessions->count();
}

void DaemonSessionRoster::rebuild() {
    if (m_store == nullptr) {
        return;
    }
    QList<QVariantMap> rows;
    QSet<QString> live;
    for (const domain::Session& s : m_store->sessions(domain::ListScope{})) {
        const QString id = s.sessionId.toString();
        live.insert(id);
        QVariantMap row;
        row[QStringLiteral("id")] = id;
        row[QStringLiteral("title")] = s.title;
        row[QStringLiteral("profile")] = s.boundProfile.toString();
        // The client suspend overlay wins over the wire state (no op to flip it server-side yet).
        row[QStringLiteral("state")] =
            m_suspended.contains(id) ? QStringLiteral("suspended") : stateStr(s.state);
        row[QStringLiteral("lifecycle")] = lifecycleStr(s.lifecycle);
        row[QStringLiteral("lastActivity")] = relativeTime(s.lastActivityMs);
        row[QStringLiteral("tokens")] = 0; // degraded: no per-session token data client-side
        row[QStringLiteral("rewindable")] = false; // degraded: checkpoints not wired
        rows.append(row);
    }
    m_suspended.intersect(live); // drop overlay entries for sessions that left the roster
    m_sessions->setRows(rows);
    emit changed();
}

void DaemonSessionRoster::suspend(const QString& id) {
    m_suspended.insert(id);
    rebuild();
}

void DaemonSessionRoster::resume(const QString& id) {
    m_suspended.remove(id);
    rebuild();
}

void DaemonSessionRoster::close(const QString& id) {
    m_suspended.remove(id);
    if (m_store != nullptr) {
        // Client-local archive (no wire close op): drops the session from the AllSessions scope;
        // setArchived reloads the store -> our rebuild fires via ISessionStore::changed.
        m_store->setArchived(id, true);
    }
}

} // namespace fleet
