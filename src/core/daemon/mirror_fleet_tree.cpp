// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/mirror_fleet_tree.h"

#include "daemon/repositories.h"
#include "mirror/ingestor.h"
#include "mirror/store.h"
#include "uimodels/variant_list_model.h"

#include <QList>
#include <QStringList>

namespace fleet {
namespace {

constexpr auto kConsumer = "mirror_fleet_tree";

// Map the wire UnitKind onto the FleetPage's display discriminator (orchestrator icon vs worker) —
// the exact rule the legacy DaemonFleetTree applied.
QString kindFor(const QString& wireKind) {
    return wireKind == QStringLiteral("Orchestrator") ? QStringLiteral("orchestrator")
                                                      : QStringLiteral("worker");
}

// Map the wire UnitState onto the FleetPage status (the wire has no "paused"; that overlay is
// applied client-side in rebuild()).
QString statusFor(const QString& wireState) {
    return wireState == QStringLiteral("Running") ? QStringLiteral("running")
                                                  : QStringLiteral("idle");
}

QStringList splitChildIds(const QString& joined) {
    if (joined.isEmpty()) {
        return {};
    }
    return joined.split(QChar(0x1f), Qt::SkipEmptyParts);
}

} // namespace

MirrorFleetTree::MirrorFleetTree(mirror::Store* store, mirror::Ingestor* ingestor,
                                 daemonapp::daemon::FleetRepository* control, QObject* parent)
    : IFleetTree(parent), m_mirror(store), m_ingestor(ingestor), m_control(control),
      m_nodes(new uimodels::VariantListModel(this)) {
    if (m_mirror != nullptr) {
        m_mirror->journal().registerConsumer(QLatin1String(kConsumer));
        m_watermark = m_mirror->journal().headRev();
        m_mirror->journal().advanceWatermark(QLatin1String(kConsumer), m_watermark);
        connect(m_mirror, &mirror::Store::committed, this, &MirrorFleetTree::onCommitted);
    }
    if (m_ingestor != nullptr) {
        m_ingestor->beginObserving(QStringLiteral("fleet"));
    }
    if (m_control != nullptr) {
        connect(m_control, &daemonapp::daemon::FleetRepository::controlFailed, this,
                &MirrorFleetTree::onControlRejected);
        // A control Ok re-fetches the legacy repo tree and emits treeRefreshed (the legacy tree's
        // rebuild trigger). Route it to a mirror refetch so the post-control-ack refresh reaches
        // the read model deterministically (belt-and-braces beside the FleetChanged event arm; the
        // scheduler's coalesce key dedups the overlap).
        if (m_ingestor != nullptr) {
            connect(m_control, &daemonapp::daemon::FleetRepository::treeRefreshed, this,
                    [this] { m_ingestor->refetchFleet(); });
        }
    }
    // Offline-first: render the last-known mirror tree immediately (the mirror persists across
    // reconnects), before any refresh.
    rebuild();
}

QObject* MirrorFleetTree::nodes() const {
    return m_nodes;
}

void MirrorFleetTree::onCommitted() {
    if (m_mirror == nullptr) {
        return;
    }
    const auto deltas = m_mirror->journal().since(m_watermark, mirror::EntityKind::FleetUnit);
    m_watermark = m_mirror->journal().headRev();
    m_mirror->journal().advanceWatermark(QLatin1String(kConsumer), m_watermark);
    if (deltas.empty()) {
        return;
    }
    rebuild();
}

void MirrorFleetTree::rebuild() {
    if (m_mirror == nullptr) {
        return;
    }
    // Index the mirror rows and collect the child edge so the root (a unit no child list names) and
    // per-node depth can be reconstructed (§3.5 unitsByParent, derived at projection time).
    QHash<QString, mirror::FleetUnit> byId;
    QSet<QString> namedAsChild;
    for (const mirror::FleetUnit& u : m_mirror->snapshot().fleet_units) {
        byId.insert(u.id, u);
        for (const QString& c : splitChildIds(u.child_ids)) {
            namedAsChild.insert(c);
        }
    }
    QStringList roots;
    for (auto it = byId.constBegin(); it != byId.constEnd(); ++it) {
        if (!namedAsChild.contains(it.key())) {
            roots.push_back(it.key());
        }
    }
    roots.sort(); // deterministic multi-root order (a single-root tree is the common case)

    // Pre-order DFS assigning depth; a seen-set guards a pathological cycle. Children are pushed in
    // reverse so they pop (LIFO) in their listed order — the legacy flattenTreeNodes rule.
    struct Frame {
        QString id;
        int depth;
    };
    QList<Frame> stack;
    for (qsizetype k = roots.size() - 1; k >= 0; --k) {
        stack.append({roots.at(k), 0});
    }
    QList<QVariantMap> rows;
    QSet<QString> live;
    QSet<QString> seen;
    while (!stack.isEmpty()) {
        const Frame f = stack.takeLast();
        if (seen.contains(f.id) || !byId.contains(f.id)) {
            continue;
        }
        seen.insert(f.id);
        const mirror::FleetUnit& u = byId[f.id];
        live.insert(u.id);
        QVariantMap row;
        row[QStringLiteral("id")] = u.id;
        row[QStringLiteral("depth")] = f.depth;
        row[QStringLiteral("name")] = u.title.isEmpty() ? u.id : u.title;
        row[QStringLiteral("kind")] = kindFor(u.kind);
        row[QStringLiteral("status")] =
            m_paused.contains(u.id) ? QStringLiteral("paused") : statusFor(u.state);
        // No model field on a unit; the bound profile is the closest "what is this running as".
        row[QStringLiteral("model")] = u.profile;
        // Engine identity (C3/F3): "Core" | "Foreign" + the foreign agent name, decoded straight
        // off the wire unit-node engine-selector (v29) — no client-side profile-join.
        row[QStringLiteral("engine")] = u.engine;
        row[QStringLiteral("engineAgent")] = u.engine_agent;
        // Delegation lifetime (F3): "Persistent" | "Ephemeral" | "".
        row[QStringLiteral("lifetime")] = u.lifetime;
        // F4: the unit's bound session id + wire role, so delegated-child rows can offer the
        // operator Steer/Cancel actions (a child IS a session; Submit is session-addressable).
        row[QStringLiteral("sessionId")] = u.session;
        row[QStringLiteral("role")] = u.role;
        rows.append(row);
        const QStringList kids = splitChildIds(u.child_ids);
        for (qsizetype k = kids.size() - 1; k >= 0; --k) {
            stack.append({kids.at(k), f.depth + 1});
        }
    }
    // Drop paused-overlay entries for units that left the tree (finished), so the set does not grow
    // unbounded.
    m_paused.intersect(live);
    m_nodes->setRows(rows);
    emit changed();
}

void MirrorFleetTree::pause(const QString& id) {
    // The paused overlay is CLIENT-LOCAL presentation state (§8.5) — applied in both modes so
    // the surfaces render identically (AD: the mock tree is this same class with a null control
    // seam). The wire op goes out only where the control seam exists; a daemon rejection
    // reverts the overlay via controlFailed.
    m_lastControlId = id;
    m_paused.insert(id); // optimistic; reverted on controlFailed
    rebuild();
    if (m_control != nullptr) {
        m_control->pause(id);
    }
}

void MirrorFleetTree::resume(const QString& id) {
    m_lastControlId = id;
    m_paused.remove(id);
    rebuild();
    if (m_control != nullptr) {
        m_control->resume(id);
    }
}

void MirrorFleetTree::scale(const QString& id, int n) {
    if (m_control == nullptr || n < 0) {
        return;
    }
    m_lastControlId = id;
    m_control->scale(id, static_cast<quint32>(n));
}

void MirrorFleetTree::refresh() {
    // The mirror twin of FleetRepository::refreshTree: re-fetch the Tree into the mirror (the
    // ingestor is the single writer, §5.1). The FleetUnit delta then drives rebuild via
    // onCommitted.
    if (m_ingestor != nullptr) {
        m_ingestor->refetchFleet();
    }
}

void MirrorFleetTree::onControlRejected(const QString& message) {
    // The optimistic pause overlay was wrong (e.g. Unsupported on an engine leaf); revert it.
    if (!m_lastControlId.isEmpty()) {
        m_paused.remove(m_lastControlId);
        rebuild();
    }
    emit controlRejected(message);
}

} // namespace fleet
