#pragma once

#include "daemonnet/idaemonnet.h"

#include "domain/session.h"
#include "domain/tag.h"
#include "domain/unit_node.h"

#include <QList>
#include <QSet>
#include <QVariantMap>

namespace uimodels {
class VariantListModel;
}

namespace daemonnet {

// Inert mock DaemonNet: one unified, typed seed is the single source for the whole mock UI. It builds
// the fleet-of-fleets supervision units, the sessions (each with content + tags + owning unit + its
// authoritative `sessionId`), the tags, and the transport combos (matrix DM / internal room / channel
// / peers / spectator). The session store copies `seed()`; the `fleet()`/`sessions()` projections feed
// the Fleet/Sessions pages; `channels()`/`byPeer()` + `nodes()`/`edges()` feed the future
// lenses/patch-bay. The Admin is the implicit viewer (not a node).
class MockDaemonNet : public IDaemonNet {
    Q_OBJECT

public:
    explicit MockDaemonNet(QObject* parent = nullptr);

    [[nodiscard]] QObject* fleet() const override;
    [[nodiscard]] QObject* sessions() const override;
    [[nodiscard]] QObject* channels() const override;
    [[nodiscard]] QObject* byPeer() const override;

    [[nodiscard]] QVariantList nodes() const override;
    [[nodiscard]] QVariantList edges() const override;

    [[nodiscard]] SeedBundle seed() const override;

    [[nodiscard]] QList<domain::UnitNode>
    unitChildren(const domain::UnitId& parent) const override;
    [[nodiscard]] domain::UnitNode unit(const domain::UnitId& id) const override;
    [[nodiscard]] QList<domain::Tag> tags() const override;
    [[nodiscard]] QList<domain::Session>
    sessionsInScope(const domain::ListScope& scope) const override;
    [[nodiscard]] domain::Session sessionDetail(const domain::SessionId& id) const override;
    [[nodiscard]] QString content(const domain::SessionId& id) const override;
    [[nodiscard]] QList<TransportTreeRow> transportsTree() const override;

private:
    // True when `unitId` is `rootId` or any descendant of it (parent-chain walk); the Unit-scope fold.
    [[nodiscard]] bool isInSubtree(const domain::UnitId& unitId, const domain::UnitId& rootId) const;
    // The set of session ids bound to `lensKey` by an edge of `edgeKind` (Over / Participant).
    [[nodiscard]] QSet<QString> sessionsBoundBy(const QString& edgeKind, const QString& lensKey) const;
    void buildSeed();
    void buildTransportsTree();
    void computeProjections();

    // Typed seed (the single source; copied by the session store).
    QList<domain::UnitNode> m_units;
    QList<domain::Session> m_sessions;
    QList<domain::Tag> m_tags;

    // Raw graph for the future patch-bay + the channels/byPeer projections (transport combos).
    QList<QVariantMap> m_nodes;
    QList<QVariantMap> m_edges;

    // The capability-driven Transports tree (events-IO axis), pre-order flattened.
    QList<TransportTreeRow> m_transports;

    // QML-boundary projections derived from the typed seed + transport graph.
    uimodels::VariantListModel* m_fleetModel = nullptr;
    uimodels::VariantListModel* m_sessionsModel = nullptr;
    uimodels::VariantListModel* m_channelsModel = nullptr;
    uimodels::VariantListModel* m_byPeerModel = nullptr;
};

} // namespace daemonnet
