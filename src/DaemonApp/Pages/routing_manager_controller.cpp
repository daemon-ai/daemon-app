// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "routing_manager_controller.h"

#include "daemonnet/idaemonnet.h"
#include "daemonnet/routing_dtos.h"
#include "uimodels/variant_list_model.h"

#include <QSet>

using daemonnet::AccountAgent;
using daemonnet::BindingRule;
using daemonnet::IDaemonNet;
using daemonnet::Resolution;
using daemonnet::RoomBinding;
using daemonnet::RoutingPin;
using daemonnet::TransportTreeRow;
using domain::DeliveryTarget;
using domain::Origin;
using domain::OriginScope;
using domain::OriginScopeKind;
using domain::ProfileRef;
using domain::RouteAddr;
using domain::SessionId;
using domain::SinkKind;
using domain::TransportId;

namespace {

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

QString sinkStr(SinkKind k) {
    return k == SinkKind::Primary ? QStringLiteral("primary") : QStringLiteral("spectator");
}

} // namespace

RoutingManagerController::RoutingManagerController(QObject* parent)
    : QObject(parent), m_routes(new uimodels::VariantListModel(this)),
      m_accounts(new uimodels::VariantListModel(this)),
      m_rules(new uimodels::VariantListModel(this)),
      m_bindable(new uimodels::VariantListModel(this)),
      m_sessions(new uimodels::VariantListModel(this)),
      m_delivery(new uimodels::VariantListModel(this)) {}

QObject* RoutingManagerController::daemonNet() const {
    return m_net;
}

void RoutingManagerController::setDaemonNet(QObject* net) {
    auto* dn = qobject_cast<IDaemonNet*>(net);
    if (m_net == dn) {
        return;
    }
    if (m_net) {
        m_net->disconnect(this);
    }
    m_net = dn;
    if (m_net) {
        connect(m_net, &IDaemonNet::changed, this, &RoutingManagerController::rebuild);
    }
    emit daemonNetChanged();
    rebuild();
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

void RoutingManagerController::rebuild() {
    m_originByKey.clear();
    if (!m_net) {
        m_routes->setRows({});
        m_accounts->setRows({});
        m_rules->setRows({});
        m_bindable->setRows({});
        m_sessions->setRows({});
        rebuildDelivery();
        return;
    }

    // Pins, each resolved for its decidedBy rung.
    QList<QVariantMap> routeRows;
    for (const RoutingPin& p : m_net->routes()) {
        const QString key = daemonnet::originKey(p.origin);
        m_originByKey.insert(key, p.origin);
        const Resolution r = m_net->resolve(p.origin);
        QVariantMap row;
        row[QStringLiteral("id")] = key;
        row[QStringLiteral("transport")] = p.origin.transport.toString();
        row[QStringLiteral("scope")] = scopeLabel(p.origin.scope);
        row[QStringLiteral("session")] = p.session.toString();
        row[QStringLiteral("profile")] =
            r.profile.isEmpty() ? p.profile.toString() : r.profile.toString();
        row[QStringLiteral("decidedBy")] = daemonnet::decidedByStr(r.decidedBy);
        routeRows.append(row);
    }
    m_routes->setRows(routeRows);

    // Account -> agent baselines.
    QList<QVariantMap> accountRows;
    for (const AccountAgent& a : m_net->accountsAgents()) {
        QVariantMap row;
        row[QStringLiteral("id")] = a.transport.toString();
        row[QStringLiteral("transport")] = a.transport.toString();
        row[QStringLiteral("profile")] = a.profile.toString();
        accountRows.append(row);
    }
    m_accounts->setRows(accountRows);

    // Config-time rules (read-only).
    QList<QVariantMap> ruleRows;
    int idx = 0;
    for (const BindingRule& b : m_net->bindingRules()) {
        QVariantMap row;
        row[QStringLiteral("id")] = idx++;
        row[QStringLiteral("transportPattern")] = b.transportPattern;
        row[QStringLiteral("scopeGlob")] = b.scopeGlob;
        row[QStringLiteral("isolation")] = b.isolation;
        row[QStringLiteral("profile")] = b.profile.toString();
        row[QStringLiteral("delivery")] = b.delivery;
        ruleRows.append(row);
    }
    m_rules->setRows(ruleRows);

    // Sessions a pin can target (from the unified seed).
    QList<QVariantMap> sessionRows;
    for (const domain::Session& s : m_net->seed().sessions) {
        QVariantMap row;
        row[QStringLiteral("id")] = s.sessionId.toString();
        row[QStringLiteral("title")] = s.title;
        sessionRows.append(row);
    }
    m_sessions->setRows(sessionRows);

    // Bindable origins: the known pin origins + each messaging account's rooms/chats.
    QList<QVariantMap> bindableRows;
    QSet<QString> seen;
    const auto addBindable = [&](const Origin& o, const QString& label,
                                 const QString& pinnedSession) {
        const QString key = daemonnet::originKey(o);
        if (seen.contains(key)) {
            return;
        }
        seen.insert(key);
        m_originByKey.insert(key, o);
        QVariantMap row;
        row[QStringLiteral("id")] = key;
        row[QStringLiteral("transport")] = o.transport.toString();
        row[QStringLiteral("label")] = label;
        row[QStringLiteral("pinnedSession")] = pinnedSession;
        bindableRows.append(row);
    };
    for (const RoutingPin& p : m_net->routes()) {
        addBindable(p.origin,
                    p.origin.transport.toString() + QStringLiteral(" · ") +
                        scopeLabel(p.origin.scope),
                    p.session.toString());
    }
    for (const TransportTreeRow& t : m_net->transportsTree()) {
        if (t.kind != QStringLiteral("account") || t.scopeKey.isEmpty()) {
            continue;
        }
        const TransportId transport(t.scopeKey);
        for (const RoomBinding& rb : m_net->transportRooms(transport)) {
            Origin o;
            o.transport = transport;
            o.scope.kind = OriginScopeKind::Group;
            o.scope.chat = rb.chat;
            addBindable(o, transport.toString() + QStringLiteral(" · ") + rb.label,
                        rb.pinnedSession.toString());
        }
    }
    m_bindable->setRows(bindableRows);

    rebuildDelivery();
}

void RoutingManagerController::rebuildDelivery() {
    QList<QVariantMap> rows;
    if (m_net && !m_selectedSession.isEmpty()) {
        for (const DeliveryTarget& t : m_net->deliveryTargets(SessionId(m_selectedSession))) {
            QVariantMap row;
            row[QStringLiteral("id")] =
                t.transport.toString() + QLatin1Char('/') + t.route.toString();
            row[QStringLiteral("transport")] = t.transport.toString();
            row[QStringLiteral("route")] = t.route.toString();
            row[QStringLiteral("kind")] = sinkStr(t.kind);
            rows.append(row);
        }
    }
    m_delivery->setRows(rows);
}

void RoutingManagerController::pin(const QString& originKey, const QString& session,
                                   const QString& profile) {
    if (!m_net || !m_originByKey.contains(originKey)) {
        return;
    }
    m_net->bindChat(m_originByKey.value(originKey), SessionId(session), ProfileRef(profile));
}

void RoutingManagerController::unbind(const QString& originKey) {
    if (!m_net || !m_originByKey.contains(originKey)) {
        return;
    }
    m_net->unbindChat(m_originByKey.value(originKey));
}

void RoutingManagerController::handoverTo(const QString& session, const QString& transport,
                                          const QString& route) {
    if (!m_net) {
        return;
    }
    DeliveryTarget t;
    t.transport = TransportId(transport);
    t.route = RouteAddr(route);
    t.kind = SinkKind::Primary;
    m_net->handover(SessionId(session), t);
}

void RoutingManagerController::bindAccount(const QString& transport, const QString& profile) {
    if (!m_net) {
        return;
    }
    m_net->bindAccount(TransportId(transport), ProfileRef(profile));
}

QVariantMap RoutingManagerController::explain(const QString& originKey) const {
    QVariantMap out;
    if (!m_net || !m_originByKey.contains(originKey)) {
        out[QStringLiteral("found")] = false;
        return out;
    }
    const Resolution r = m_net->resolve(m_originByKey.value(originKey));
    out[QStringLiteral("found")] = true;
    out[QStringLiteral("session")] = r.session.toString();
    out[QStringLiteral("profile")] = r.profile.toString();
    out[QStringLiteral("transport")] = r.delivery.transport.toString();
    out[QStringLiteral("route")] = r.delivery.route.toString();
    out[QStringLiteral("kind")] = sinkStr(r.delivery.kind);
    out[QStringLiteral("decidedBy")] = daemonnet::decidedByStr(r.decidedBy);
    return out;
}
