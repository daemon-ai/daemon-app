// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "routing_manager_controller.h"

#include "daemonnet/irouting_actions.h"
#include "mirror/mirror_service.h"
#include "mirror/store.h"
#include "uimodels/variant_list_model.h"

#include <QSet>

using domain::Origin;
using domain::OriginScope;
using domain::OriginScopeKind;
using domain::ProfileRef;
using domain::SessionId;
using domain::TransportId;

namespace {

// Parse a canonical origin key (mirrors daemonnet::originKey / the RoutePin.origin_key derivation)
// back into a typed Origin, so the pin/unbind mutations can rebuild the wire origin from the row
// id. Formats: `t|dm|user`, `t|group|chat|thread`, `t|api|key`, `t|internal`.
Origin parseOriginKey(const QString& key) {
    Origin o;
    const QStringList parts = key.split(QLatin1Char('|'));
    if (parts.isEmpty()) {
        return o;
    }
    o.transport = TransportId(parts.value(0));
    const QString kind = parts.value(1);
    if (kind == QStringLiteral("dm")) {
        o.scope.kind = OriginScopeKind::Dm;
        o.scope.user = parts.value(2);
    } else if (kind == QStringLiteral("group")) {
        o.scope.kind = OriginScopeKind::Group;
        o.scope.chat = parts.value(2);
        o.scope.thread = parts.value(3);
    } else if (kind == QStringLiteral("api")) {
        o.scope.kind = OriginScopeKind::Api;
        o.scope.apiKey = parts.value(2);
    } else {
        o.scope.kind = OriginScopeKind::Internal;
    }
    return o;
}

QString scopeLabel(const OriginScope& s) {
    switch (s.kind) {
    case OriginScopeKind::Dm:
        return s.user;
    case OriginScopeKind::Group:
        return s.chat;
    case OriginScopeKind::Api:
        return QStringLiteral("key:") + s.apiKey;
    case OriginScopeKind::Internal:
        return QStringLiteral("internal");
    }
    return {};
}

// The canonical origin key for a Group room origin (transport + chat), mirroring the RoutePin
// derivation so a room's key matches a pin's key for the same origin (dedup in the picker).
QString groupOriginKey(const QString& transport, const QString& chat) {
    return transport + QStringLiteral("|group|") + chat + QLatin1Char('|');
}

} // namespace

namespace {
mirror::Store* storeOf(QObject* mirrorObject) {
    auto* svc = qobject_cast<mirror::MirrorService*>(mirrorObject);
    return svc != nullptr ? &svc->store() : nullptr;
}
} // namespace

RoutingManagerController::RoutingManagerController(QObject* parent)
    : QObject(parent), m_routes(new uimodels::VariantListModel(this)),
      m_accounts(new uimodels::VariantListModel(this)),
      m_rules(new uimodels::VariantListModel(this)),
      m_bindable(new uimodels::VariantListModel(this)),
      m_sessions(new uimodels::VariantListModel(this)),
      m_delivery(new uimodels::VariantListModel(this)) {}

RoutingManagerController::~RoutingManagerController() = default;

QObject* RoutingManagerController::mirror() const {
    return m_mirrorObject;
}

void RoutingManagerController::setMirror(QObject* mirror) {
    if (m_mirrorObject == mirror) {
        return;
    }
    m_mirrorObject = mirror;
    bindStoreSignals();
    emit mirrorChanged();
    rebuild();
}

QObject* RoutingManagerController::actions() const {
    return m_actionsObject;
}

void RoutingManagerController::setActions(QObject* actions) {
    if (m_actionsObject == actions) {
        return;
    }
    m_actionsObject = actions;
    m_actions = dynamic_cast<daemonnet::IRoutingActions*>(actions);
    emit actionsChanged();
}

void RoutingManagerController::bindStoreSignals() {
    if (auto* svc = qobject_cast<mirror::MirrorService*>(m_mirrorObject)) {
        // Re-derive the panels on every committed batch (§8.1 — the store is the read path).
        connect(
            &svc->store(), &mirror::Store::committed, this, [this](quint64, quint64) { rebuild(); },
            Qt::UniqueConnection);
        // Visibility (§5.8): declare interest in the pin table so the ingestor tops it up.
        svc->ingestor().beginObserving(QStringLiteral("routing"));
    }
}

QObject* RoutingManagerController::routes() const {
    return m_routes;
}
QObject* RoutingManagerController::accounts() const {
    return m_accounts;
}
QObject* RoutingManagerController::rules() const {
    return m_rules;
}
QObject* RoutingManagerController::bindable() const {
    return m_bindable;
}
QObject* RoutingManagerController::sessions() const {
    return m_sessions;
}
QObject* RoutingManagerController::delivery() const {
    return m_delivery;
}

void RoutingManagerController::setSelectedSession(const QString& session) {
    if (m_selectedSession == session) {
        return;
    }
    m_selectedSession = session;
    emit selectedSessionChanged();
    rebuildDelivery();
}

QString RoutingManagerController::pinnedSessionFor(const QString& transport,
                                                   const QString& conv) const {
    mirror::Store* store = storeOf(m_mirrorObject);
    if (store == nullptr) {
        return {};
    }
    for (const mirror::RoutePin& p : store->snapshot().route_pins) {
        if (p.transport != transport) {
            continue;
        }
        const Origin o = parseOriginKey(p.origin_key);
        const bool matches = (o.scope.kind == OriginScopeKind::Group && o.scope.chat == conv) ||
                             (o.scope.kind == OriginScopeKind::Dm && o.scope.user == conv);
        if (matches) {
            return p.session;
        }
    }
    return {};
}

void RoutingManagerController::rebuild() {
    m_originByKey.clear();
    mirror::Store* store = storeOf(m_mirrorObject);
    if (store == nullptr) {
        m_routes->setRows({});
        m_accounts->setRows({});
        m_rules->setRows({});
        m_bindable->setRows({});
        m_sessions->setRows({});
        rebuildDelivery();
        emit pinsChanged();
        return;
    }
    const mirror::MirrorModel& snap = store->snapshot();

    // Pins (route_pins). Each pin resolves as decidedBy=pin (the node owns the lower rungs).
    QList<QVariantMap> routeRows;
    for (const mirror::RoutePin& p : snap.route_pins) {
        const Origin o = parseOriginKey(p.origin_key);
        m_originByKey.insert(p.origin_key, o);
        QVariantMap row;
        row[QStringLiteral("id")] = p.origin_key;
        row[QStringLiteral("transport")] = p.transport;
        row[QStringLiteral("scope")] = scopeLabel(o.scope);
        row[QStringLiteral("session")] = p.session;
        row[QStringLiteral("profile")] = p.profile;
        row[QStringLiteral("decidedBy")] = QStringLiteral("pin");
        routeRows.append(row);
    }
    m_routes->setRows(routeRows);

    // Account -> agent baselines (transport_accounts.bound_profile).
    QList<QVariantMap> accountRows;
    for (const mirror::TransportAccount& a : snap.transport_accounts) {
        if (a.bound_profile.isEmpty()) {
            continue;
        }
        QVariantMap row;
        row[QStringLiteral("id")] = a.transport;
        row[QStringLiteral("transport")] = a.transport;
        row[QStringLiteral("profile")] = a.bound_profile;
        accountRows.append(row);
    }
    m_accounts->setRows(accountRows);

    // Config-time rules: no wire read op — render none (honest thin client).
    m_rules->setRows({});

    // Sessions a pin can target (non-archived).
    QList<QVariantMap> sessionRows;
    for (const mirror::Session& s : snap.sessions) {
        if (s.archived) {
            continue;
        }
        QVariantMap row;
        row[QStringLiteral("id")] = s.session;
        row[QStringLiteral("title")] = s.title;
        sessionRows.append(row);
    }
    m_sessions->setRows(sessionRows);

    // Bindable origins: the known pin origins + each transport's bindable rooms.
    QList<QVariantMap> bindableRows;
    QSet<QString> seen;
    const auto addBindable = [&](const QString& originKey, const Origin& o, const QString& label,
                                 const QString& pinnedSession) {
        if (seen.contains(originKey)) {
            return;
        }
        seen.insert(originKey);
        m_originByKey.insert(originKey, o);
        QVariantMap row;
        row[QStringLiteral("id")] = originKey;
        row[QStringLiteral("transport")] = o.transport.toString();
        row[QStringLiteral("label")] = label;
        row[QStringLiteral("pinnedSession")] = pinnedSession;
        bindableRows.append(row);
    };
    for (const mirror::RoutePin& p : snap.route_pins) {
        const Origin o = parseOriginKey(p.origin_key);
        addBindable(p.origin_key, o,
                    o.transport.toString() + QStringLiteral(" \u00b7 ") + scopeLabel(o.scope),
                    p.session);
    }
    for (const mirror::Room& r : snap.rooms) {
        Origin o;
        o.transport = TransportId(r.transport);
        o.scope.kind = OriginScopeKind::Group;
        o.scope.chat = r.room;
        addBindable(groupOriginKey(r.transport, r.room), o,
                    r.transport + QStringLiteral(" \u00b7 ") + (r.name.isEmpty() ? r.room : r.name),
                    r.session);
    }
    m_bindable->setRows(bindableRows);

    rebuildDelivery();
    emit pinsChanged();
}

void RoutingManagerController::rebuildDelivery() {
    // Delivery targets are a per-session SessionGet read (no mirror entity yet); the panel stays
    // empty until that seam lands (A7 sessions territory — see LEDGER-a6).
    m_delivery->setRows({});
}

void RoutingManagerController::pin(const QString& originKey, const QString& session,
                                   const QString& profile) {
    if (m_actions == nullptr || !m_originByKey.contains(originKey)) {
        return;
    }
    m_actions->routingBindChat(m_originByKey.value(originKey), SessionId(session),
                               ProfileRef(profile));
}

void RoutingManagerController::unbind(const QString& originKey) {
    if (m_actions == nullptr || !m_originByKey.contains(originKey)) {
        return;
    }
    m_actions->routingUnbindChat(m_originByKey.value(originKey));
}

void RoutingManagerController::handoverTo(const QString& session, const QString& transport,
                                          const QString& route) {
    // Inert: re-pointing the Primary delivery needs the SessionGet delivery seam (A7). An inert
    // no-op beats a fake local mutation.
    Q_UNUSED(session)
    Q_UNUSED(transport)
    Q_UNUSED(route)
}

void RoutingManagerController::bindAccount(const QString& transport, const QString& profile) {
    // Inert: account->agent baselines mutate via profile_update(bound_accounts); the ProfileEditor
    // owns that flow.
    Q_UNUSED(transport)
    Q_UNUSED(profile)
}

QVariantMap RoutingManagerController::explain(const QString& originKey) const {
    QVariantMap out;
    mirror::Store* store = storeOf(m_mirrorObject);
    if (store != nullptr) {
        for (const mirror::RoutePin& p : store->snapshot().route_pins) {
            if (p.origin_key == originKey) {
                out[QStringLiteral("found")] = true;
                out[QStringLiteral("session")] = p.session;
                out[QStringLiteral("profile")] = p.profile;
                out[QStringLiteral("decidedBy")] = QStringLiteral("pin");
                return out;
            }
        }
    }
    // No pin for this origin: the node owns the lower rungs; the client honestly reports Default.
    out[QStringLiteral("found")] = true;
    out[QStringLiteral("decidedBy")] = QStringLiteral("default");
    return out;
}
