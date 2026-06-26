#pragma once

#include "domain/origin.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace daemonnet {
class IDaemonNet;
}
namespace uimodels {
class VariantListModel;
}

// The routing-manager surface controller (routing-manager-design.md). Bridges the DaemonNet routing
// projections + mutations to QML: it exposes VariantListModels the control panels bind to and
// Q_INVOKABLE mutations that call `IDaemonNet` (mock today; the wire `routing_*`/`delivery_*`/
// `handover` ops later). Bound from QML to the shared `DaemonNet` context property.
class RoutingManagerController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* daemonNet READ daemonNet WRITE setDaemonNet NOTIFY daemonNetChanged)
    // Pins (origin -> session [+agent]), each row resolved with its decidedBy rung.
    Q_PROPERTY(QObject* routes READ routes CONSTANT)
    // account -> agent baselines (instance_profiles).
    Q_PROPERTY(QObject* accounts READ accounts CONSTANT)
    // Config-time binding rules (read-only).
    Q_PROPERTY(QObject* rules READ rules CONSTANT)
    // Bindable origins (rooms/chats + known pin origins) for the add-pin / explainer pickers.
    Q_PROPERTY(QObject* bindable READ bindable CONSTANT)
    // Sessions a pin can target.
    Q_PROPERTY(QObject* sessions READ sessions CONSTANT)
    // The selected session's delivery targets (Primary + Spectators).
    Q_PROPERTY(QObject* delivery READ delivery CONSTANT)
    Q_PROPERTY(QString selectedSession READ selectedSession WRITE setSelectedSession NOTIFY
                   selectedSessionChanged)

public:
    explicit RoutingManagerController(QObject* parent = nullptr);

    [[nodiscard]] QObject* daemonNet() const;
    void setDaemonNet(QObject* net);

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
    // Re-point the selected session's Primary delivery to (transport, route).
    Q_INVOKABLE void handoverTo(const QString& session, const QString& transport,
                                const QString& route);
    // Bind a transport account to an agent baseline.
    Q_INVOKABLE void bindAccount(const QString& transport, const QString& profile);
    // Explain an origin's resolution: { session, profile, transport, route, kind, decidedBy }.
    [[nodiscard]] Q_INVOKABLE QVariantMap explain(const QString& originKey) const;

signals:
    void daemonNetChanged();
    void selectedSessionChanged();

private:
    void rebuild();
    void rebuildDelivery();

    daemonnet::IDaemonNet* m_net = nullptr;
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
