// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "domain/origin.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QVariantMap>

namespace uimodels {
class VariantListModel;
}
namespace daemonnet {
class IRoutingActions;
}

// The routing-manager surface controller (routing-manager-design.md), M3: ported onto the mirror
// STORE PROJECTIONS. It reads the store's `route_pins` / `rooms` / `transport_accounts` /
// `sessions` tables (no legacy network seam, no `RoutingRepository` cache), re-derives its panels
// on every
// `Store::committed`, and declares routing visibility so the ingestor tops the pin table up. Pin /
// unpin mutations go through the node-authoritative `IRoutingActions` seam (RoutingBindChat /
// RoutingUnbindChat direct verbs, §7); the node-acked result re-fetches into the mirror. The GUI
// `RoutingPage`/`RouteDialog` and the TUI routing hub bind the SAME controller (one shared C++ VM).
//
// Mode/degradation notes (thin client): the mirror is null in mock mode at M2 (A5 precedent) so the
// manager renders EMPTY there (A8 seeds the mock mirror). resolve()/explain() answer Pin (a pinned
// origin) else Default — the node owns the lower precedence rungs (no client re-derivation). The
// delivery/handover panel needs a per-session SessionGet read (no mirror entity yet) — it stays
// empty and handoverTo()/bindAccount() are inert (A7 sessions territory; documented in LEDGER-a6).
class RoutingManagerController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    // The mirror composition root (`Mirror` context property; null in mock mode at M2). Opaque
    // QObject* for QML; qobject_cast to mirror::MirrorService inside.
    Q_PROPERTY(QObject* mirror READ mirror WRITE setMirror NOTIFY mirrorChanged)
    // The routing mutation seam (`RoutingActions` context property; null in mock). dynamic_cast to
    // daemonnet::IRoutingActions inside.
    Q_PROPERTY(QObject* actions READ actions WRITE setActions NOTIFY actionsChanged)
    // Pins (origin -> session [+agent]), each row carrying its decidedBy rung ("pin").
    Q_PROPERTY(QObject* routes READ routes CONSTANT)
    // account -> agent baselines (transport_accounts.bound_profile).
    Q_PROPERTY(QObject* accounts READ accounts CONSTANT)
    // Config-time binding rules (read-only; no wire read op — empty).
    Q_PROPERTY(QObject* rules READ rules CONSTANT)
    // Bindable origins (rooms/chats + known pin origins) for the add-pin / explainer pickers.
    Q_PROPERTY(QObject* bindable READ bindable CONSTANT)
    // Sessions a pin can target.
    Q_PROPERTY(QObject* sessions READ sessions CONSTANT)
    // The selected session's delivery targets (empty until the SessionGet delivery seam lands).
    Q_PROPERTY(QObject* delivery READ delivery CONSTANT)
    Q_PROPERTY(QString selectedSession READ selectedSession WRITE setSelectedSession NOTIFY
                   selectedSessionChanged)

public:
    explicit RoutingManagerController(QObject* parent = nullptr);
    ~RoutingManagerController() override;

    [[nodiscard]] QObject* mirror() const;
    void setMirror(QObject* mirror);
    [[nodiscard]] QObject* actions() const;
    void setActions(QObject* actions);

    [[nodiscard]] QObject* routes() const;
    [[nodiscard]] QObject* accounts() const;
    [[nodiscard]] QObject* rules() const;
    [[nodiscard]] QObject* bindable() const;
    [[nodiscard]] QObject* sessions() const;
    [[nodiscard]] QObject* delivery() const;

    [[nodiscard]] QString selectedSession() const { return m_selectedSession; }
    void setSelectedSession(const QString& session);

    // Pin a known bindable origin (by its key) to a session, with an optional agent override.
    Q_INVOKABLE void pin(const QString& originKey, const QString& session, const QString& profile);
    // Remove a pin by its origin key.
    Q_INVOKABLE void unbind(const QString& originKey);
    // Re-point the selected session's Primary delivery — inert (no SessionGet delivery seam yet).
    Q_INVOKABLE void handoverTo(const QString& session, const QString& transport,
                                const QString& route);
    // Bind a transport account to an agent baseline — inert (profile_update owns bound_accounts).
    Q_INVOKABLE void bindAccount(const QString& transport, const QString& profile);
    // Explain an origin's resolution: { found, session, profile, decidedBy }. Pin or Default only.
    [[nodiscard]] Q_INVOKABLE QVariantMap explain(const QString& originKey) const;

signals:
    void mirrorChanged();
    void actionsChanged();
    void selectedSessionChanged();

private:
    void rebuild();
    void rebuildDelivery();
    // The bound mirror Store (mirror::Store*), or nullptr when no MirrorService is bound / the
    // substrate is not built. Kept as void* in the header so it stays substrate-agnostic.
    void bindStoreSignals();

    QObject* m_mirrorObject = nullptr;
    QObject* m_actionsObject = nullptr;
    daemonnet::IRoutingActions* m_actions = nullptr;
    uimodels::VariantListModel* m_routes = nullptr;
    uimodels::VariantListModel* m_accounts = nullptr;
    uimodels::VariantListModel* m_rules = nullptr;
    uimodels::VariantListModel* m_bindable = nullptr;
    uimodels::VariantListModel* m_sessions = nullptr;
    uimodels::VariantListModel* m_delivery = nullptr;
    QString m_selectedSession;
    // origin key -> the typed Origin, so QML mutations can pass a stable string id.
    QHash<QString, domain::Origin> m_originByKey;
};
