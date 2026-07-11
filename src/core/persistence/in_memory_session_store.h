// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "persistence/isession_store.h"

namespace daemonnet {
class MockFleetSource;
}

namespace persistence {

// In-memory ISessionStore. Two seed sources: the canonical demo `seedSampleData()` (test fixtures),
// and `seedFromSource()` which copies the mock fleet/session seed (MockFleetSource). Not persisted
// across runs. The demo data is a deliberate fleet-of-fleets that exercises arbitrary depth
// (orchestrators nested inside orchestrators) and a lone-unit root.
class InMemorySessionStore : public ISessionStore {
    Q_OBJECT

public:
    explicit InMemorySessionStore(QObject* parent = nullptr);
    // Seed the store from the mock fleet seed: copies its units/sessions/tags, assigning each
    // session a stable int handle while carrying its authoritative string `sessionId`.
    explicit InMemorySessionStore(daemonnet::MockFleetSource* src, QObject* parent = nullptr);

    [[nodiscard]] QList<domain::UnitNode>
    unitChildren(const domain::UnitId& parentId) const override;
    [[nodiscard]] domain::UnitNode unit(const domain::UnitId& id) const override;
    [[nodiscard]] QList<domain::Tag> tags() const override;
    [[nodiscard]] QList<domain::Participant> participants() const override;
    [[nodiscard]] QList<domain::Session> sessions(const domain::ListScope& scope) const override;
    [[nodiscard]] int sessionCount(const domain::ListScope& scope) const override;

    // SessionId-keyed canonical API.
    [[nodiscard]] QString content(const domain::SessionId& id) const override;
    [[nodiscard]] QString title(const domain::SessionId& id) const override;
    [[nodiscard]] bool isPinned(const domain::SessionId& id) const override;
    domain::SessionId newSession(const domain::UnitId& unitId) override;
    void setContent(const domain::SessionId& id, const QString& markdown) override;
    void setArchived(const domain::SessionId& id, bool archived) override;
    void renameSession(const domain::SessionId& id, const QString& title) override;
    void deleteSession(const domain::SessionId& id) override;
    void setPinned(const domain::SessionId& id, bool pinned) override;
    void moveSession(const domain::SessionId& id, int delta) override;

    domain::UnitId createUnit(const domain::UnitId& parentId, domain::UnitKind kind) override;
    int createTag(const QString& name, const QString& color) override;

protected:
    // Subclass entry point: build the base without the sample seed (a durable
    // store loads its rows from disk instead and seeds only when empty).
    InMemorySessionStore(QObject* parent, bool seed);

    // Populate the in-memory tree/tags/sessions with the canonical demo data.
    // Protected so a durable subclass can seed a fresh (empty) database.
    void seedSampleData();

    // Replace the in-memory tree/tags/sessions with the mock fleet seed bundle.
    void seedFromSource(daemonnet::MockFleetSource* src);

    // Derive a unit's daemon-parity metadata (profile / session / role) from its
    // id + kind. These fields are NOT persisted (the SQLite schema predates them),
    // so this is applied uniformly after both seeding and loading so every unit
    // carries an agent identity regardless of where it came from. The mapping is
    // deterministic: `session == id`, roots are Primary (children ManagedChild,
    // with one EphemeralSubagent for variety), and a small id->profile table binds
    // engine/orchestrator units to a seeded profile (Hosts get no profile).
    static void applyUnitMeta(domain::UnitNode& n);

    // In-memory state. Protected so a durable subclass (e.g. the SQLite store) can
    // load these from disk and persist them write-through. The query and mutation
    // logic above operates on exactly these members.
    QList<domain::UnitNode> m_units;
    QList<domain::Tag> m_tags;
    QList<domain::Participant> m_participants;
    QList<domain::Session> m_sessions;
    int m_nextId = 1;      // session ids
    int m_nextUnitSeq = 1; // suffix for generated unit ids
    int m_nextTagId = 1;   // tag ids

private:
    [[nodiscard]] bool matchesScope(const domain::Session& c, const domain::ListScope& scope) const;
    // True when `unitId` is `rootId` or any descendant of it (subtree fold).
    // Walks the parent chain - a single recursive rule for every depth.
    [[nodiscard]] bool isInSubtree(const domain::UnitId& unitId,
                                   const domain::UnitId& rootId) const;
    // Mint a fresh authoritative SessionId for a locally-created session.
    [[nodiscard]] static domain::SessionId mintSessionId();
};

} // namespace persistence
